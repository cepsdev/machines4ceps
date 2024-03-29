/*
Copyright 2021 Tomas Prerovsky (cepsdev@hotmail.com).

Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/


#define _CRT_SECURE_NO_WARNINGS

#include "core/include/state_machine_simulation_core.hpp"
#include "core/include/base_defs.hpp"
#include <time.h>
#include "core/include/base_defs.hpp"
#include <poll.h>
#include <unistd.h>

#define USE_TIMER_FD_API 1


static bool is_second(ceps::ast::Unit_rep unit)
{
	return unit.ampere == 0 && unit.candela == 0 && unit.kelvin == 0 && unit.kg == 0 && unit.m == 0 && unit.mol == 0 && unit.s == 1;
}


std::mutex timer_threads_m;
std::vector< decltype(timer_threads)::value_type> timer_threads;


void 
main_timer_thread_fn(State_machine_simulation_core* smc)
{
	pollfd* poll_fds = new pollfd[smc->timer_table_size];
	memset(poll_fds,0,sizeof(pollfd)*smc->timer_table_size);
	int* tidxs = new int[smc->timer_table_size];
	memset(tidxs,0,sizeof(int)*smc->timer_table_size);

	for(;!smc->shutdown();){
	 size_t active_timers = 0;

	 {
	  std::lock_guard<std::mutex> lk(smc->timer_table_mtx);
      auto at = smc->timed_events_active_;
	  for(size_t i = 0; i!=smc->timer_table.size();++i){
	   if (smc->timer_table[i].kill && smc->timer_table[i].in_use){
		   smc->timer_table[i].in_use = false;
		   //smc->warn_(-1,"Closing timer fd, fd="+std::to_string(smc->timer_table[i].fd)+ " ");
		   close(smc->timer_table[i].fd);
		   smc->timer_table[i].fd = -1;
		   continue;
	   }
       if(!smc->timer_table[i].in_use || smc->timer_table[i].kill || smc->timer_table[i].fd < 0) continue;
       poll_fds[active_timers].fd = smc->timer_table[i].fd;
       poll_fds[active_timers].events = POLLIN;
       tidxs[active_timers] = i;
       ++active_timers;
	  }//for
	  smc->timed_events_active_ = active_timers;
	 }
	 if (active_timers == 0){
		  std::this_thread::sleep_for(std::chrono::milliseconds(1));
		  continue;
	 }
	 auto r = poll(poll_fds, active_timers, 10);
	 if (r == 0) continue;
	 if (r < 0) smc->fatal_(-1," main_timer_thread_fn: poll failed. "+std::to_string(errno));

	 for(size_t j = 0;j!=active_timers;++j){
	  if ((poll_fds[j].revents & POLLIN) == 0) {
		  continue;
	  }
	  uint64_t exp;
	  auto r = read(poll_fds[j].fd,&exp,sizeof(uint64_t));auto rr = errno;
	  if (r < 0 && rr == EAGAIN) continue;
	  if (r!=sizeof(uint64_t)) {
		  smc->fatal_(-1," main_timer_thread_fn: read failed. errno="+std::to_string(errno)+" ev="+smc->timer_table[tidxs[j]].event.id_+" fd="+std::to_string(poll_fds[j].fd));
		  //std::this_thread::sleep_for(std::chrono::milliseconds(100));
		  continue;
	  }
	  {
		  std::lock_guard<std::mutex> lk(smc->timer_table_mtx);


		  if(!smc->timer_table[tidxs[j]].kill){
			  if (smc->timer_table[tidxs[j]].siggen){

				  auto g = smc->timer_table[tidxs[j]].siggen;
				  if (g->values().size()){
					  double v = g->values()[smc->timer_table[tidxs[j]].loc_storage++ % g->values().size()];
					  auto& ev = smc->timer_table[tidxs[j]].event;
					  ev.payload_native_[1] = sm4ceps_plugin_int::Variant{v};
				  }
			  }
			  smc->enqueue_event(smc->timer_table[tidxs[j]].event,false);
		  }
		  if (!smc->timer_table[tidxs[j]].periodic)  smc->timer_table[tidxs[j]].kill = true;
	  }
	 }

	}//for
}


bool 
State_machine_simulation_core::exec_action_timer(
	double t,
	sm4ceps_plugin_int::ev ev_,
	sm4ceps_plugin_int::id id_,
	bool periodic_timer,
	sm4ceps_plugin_int::Variant (*fp)(),
	sm4ceps::datasources::Signalgenerator* siggen
	)
{

	if (t < 0) return true;
	std::string ev_id = ev_.name_;
	if (fp != nullptr){
			ev_id = "@@queued_action";
	}
	if (siggen != nullptr){
		ev_id = "@@set_state";
	}
	std::string timer_id;

	if (id_.name_.length())
	{
		timer_id = id_.name_;
                kill_named_timer_main_timer_table(timer_id);
	}

	double delta = t;
	ceps::ast::Unit_rep u;

	//log() << "[QUEUEING TIMED EVENT][Delta = "<< std::setprecision(8)<< delta << " sec.][Event = " << ev_id <<"]" << "\n";

	event_t ev_to_send(ev_id);
	ev_to_send.unique_ = this->unique_events().find(ev_id) != this->unique_events().end();
	ev_to_send.already_sent_to_out_queues_ = false;
	ev_to_send.glob_func_ = fp;
	bool create_thread = false;
	if (ev_.args_.size())		ev_to_send.payload_native_ = ev_.args_;
	if (siggen != nullptr){
		ev_to_send.payload_native_.push_back(sm4ceps_plugin_int::Variant(siggen->target_state()));
		ev_to_send.payload_native_.push_back(sm4ceps_plugin_int::Variant(0.0));
	}


	{
		std::lock_guard<std::mutex> lk(timer_table_mtx);
		if (timer_table.size() == 0){
			timer_table.resize(timer_table_size);
			create_thread = true;
			timed_events_active_ = 0;
		}
		bool entry_found = false;
		for(auto & e : timer_table){
			if (e.in_use) continue;
			entry_found = true;
			e.in_use =true;
			e.fresh = true;
			e.kill = false;
			e.name = timer_id;
			e.time_remaining_in_ms = e.period_in_ms = (long)(t * 1000.0);
			e.event = ev_to_send;
			e.fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
			//std::cout << "Creating fd="<< std::to_string(e.fd)<< std::endl;
			e.periodic = periodic_timer;
			e.loc_storage = 0;
			e.siggen = siggen;
			if (e.fd < 0) fatal_(-1,"timerfd_create failed.");
			itimerspec tspec;
			tspec.it_interval.tv_nsec = 0;
			tspec.it_interval.tv_sec = 0;
			tspec.it_value.tv_nsec = 0;
			tspec.it_value.tv_sec = 0;
			if (periodic_timer){
			 tspec.it_interval.tv_sec = (long) delta;
			 tspec.it_interval.tv_nsec = (delta - floor(delta)) * 1000000000.0;
			}
			tspec.it_value.tv_sec = (long) delta;
			tspec.it_value.tv_nsec = (delta - floor(delta)) * 1000000000.0;
			auto r = timerfd_settime(e.fd, 0, &tspec, nullptr);
			auto err = errno;
			if (r < 0) fatal_(-1,"timerfd_settime failed: errno="+std::to_string(err)+" fd="+std::to_string(e.fd));
			++timed_events_active_;
			break;
		}
		if (!entry_found) fatal_(-1,"Out of resources: no available timer slot left.");
	}

	if (create_thread) new std::thread{main_timer_thread_fn,this};
	return true;
}




void 
State_machine_simulation_core::exec_action_timer(
	std::vector<ceps::ast::Nodebase_ptr> const & args,
	bool periodic_timer,
	sm4ceps::datasources::Signalgenerator* siggen
	)
{
	using namespace ceps::ast;
 
	if (args.size() >= 2 && (       
                                ( args[1]->kind() == ceps::ast::Ast_node_kind::symbol && kind(ceps::ast::as_symbol_ref(args[1])) == "Event")
		                        || args[1]->kind() == ceps::ast::Ast_node_kind::func_call  
                                || args[1]->kind() == ceps::ast::Ast_node_kind::identifier)
	)
	{
		std::string ev_id;
		std::vector<ceps::ast::Nodebase_ptr> fargs;
		auto timed_expr = args[1];

		if (is<Ast_node_kind::symbol>(timed_expr)) 
		 ev_id = name(ceps::ast::as_symbol_ref(timed_expr));
		else if (is<Ast_node_kind::identifier>(timed_expr))
		{
			ev_id = "@@queued_action";
			fargs.push_back(args[1]);
		}
		else if (is<Ast_node_kind::func_call>(timed_expr)) {
			std::string  ev_name;
			auto& timed_call = as_func_call_ref(timed_expr);
			auto time_call_params = children(timed_call)[1]; 
			auto timed_call_target =  func_call_target(timed_call);

			fargs = ceps::interpreter::get_args(as_call_params_ref(time_call_params));


			if (is<Ast_node_kind::symbol>(timed_call_target) && kind(as_symbol_ref(timed_call_target))=="Event")
				ev_id = name(as_symbol_ref(timed_call_target));
			else fatal_(-1,"start_timer/start_periodic_timer: second argument has to be an event.");
		}
		std::string timer_id;
		if (args.size() > 2)
		{
			if (args[2]->kind() != ceps::ast::Ast_node_kind::identifier)
			fatal_(-1,"start_timer: third argument (the timer id) has to be an unbound identifier.\n");
			timer_id = ceps::ast::name(ceps::ast::as_id_ref(args[2]));
			kill_named_timer(timer_id);
		}
		double delta;
		ceps::ast::Unit_rep u;
		if (args[0]->kind() == ceps::ast::Ast_node_kind::int_literal)
		{
			auto t = ceps::ast::as_int_ref(args[0]);
			delta = value(t);
			if(delta < 0) return;
			u = ceps::ast::unit(t);
		}
		else if (args[0]->kind() == ceps::ast::Ast_node_kind::float_literal)
		{
			auto t = ceps::ast::as_double_ref(args[0]);
			delta = value(t);
			if(delta < 0) return;
			u = ceps::ast::unit(t);
		}
        else fatal_(-1,"Timer expectes first argument to be a numerical value.");

		if ( ! is_second(u) )
			fatal_(-1,"Timer function expects first argument to be a duration (SI unit seconds).");

		event_t ev_to_send(ev_id);
		ev_to_send.unique_ = this->unique_events().find(ev_id) != this->unique_events().end();
		ev_to_send.already_sent_to_out_queues_ = false;

		if (fargs.size())
				ev_to_send.payload_ = fargs;

		if (ev_id != "@@queued_action"){
		 if(periodic_timer){
			if (timer_id.length() == 0) start_periodic_timer(delta,sm4ceps_plugin_int::ev{ev_id});
			else start_periodic_timer(delta,sm4ceps_plugin_int::ev{ev_id},sm4ceps_plugin_int::id{timer_id});
			return;
		 }
		}
	{
		std::lock_guard<std::mutex> lk(timer_table_mtx);
		if (timer_table.size() == 0){
			timer_table.resize(timer_table_size);
			timed_events_active_ = 0;
		}
		bool entry_found = false;
		for(auto & e : timer_table){
			if (e.in_use) continue;
			entry_found = true;
			e.in_use =true;
			e.fresh = true;
			e.kill = false;
			e.name = timer_id;
			e.time_remaining_in_ms = e.period_in_ms = (long)(delta * 1000.0);
			e.event = ev_to_send;
			e.fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
			if (e.fd < 0) {
                auto err = errno;
                char buffer [1024];
                auto errstr = strerror_r(err,buffer,1024);
                std::string error_text{errstr};

                fatal_(-1,"tried to start a timer, then timerfd_create() failed ('"+error_text+"').");
            }
			e.periodic = periodic_timer;
			e.loc_storage = 0;
			e.siggen = siggen;
			
			itimerspec tspec;
			tspec.it_interval.tv_nsec = 0;
			tspec.it_interval.tv_sec = 0;
			tspec.it_value.tv_nsec = 0;
			tspec.it_value.tv_sec = 0;
			if (periodic_timer){
			 tspec.it_interval.tv_sec = (long) delta;
			 tspec.it_interval.tv_nsec = (delta - floor(delta)) * 1000000000.0;
			}
			tspec.it_value.tv_sec = (long) delta;
			tspec.it_value.tv_nsec = (delta - floor(delta)) * 1000000000.0;
			auto r = timerfd_settime(e.fd, 0, &tspec, nullptr);
			auto err = errno;
			if (r < 0) fatal_(-1,"timerfd_settime failed: errno="+std::to_string(err)+" fd="+std::to_string(e.fd));
			++timed_events_active_;
			break;
		}
		if (!entry_found) fatal_(-1,"Out of resources: no available timer slot left.");
        if (!fdtimer_api_thread_running){
            std::thread trd{main_timer_thread_fn,this};
            trd.detach();
            fdtimer_api_thread_running = true;
            
        }
	}
	}
}


bool 
State_machine_simulation_core::kill_named_timer(std::string const & timer_id){
	stop_timer(sm4ceps_plugin_int::id{timer_id});return true;
}


bool 
State_machine_simulation_core::kill_named_timer_main_timer_table(std::string const & timer_id){
	std::lock_guard<std::mutex> lk(timer_table_mtx);
	auto t = 0;
	for(auto& e : timer_table){
		if (timer_id.length() == 0 || timer_id == e.name){
			e.kill = true;
		} else if (e.in_use && !e.kill) ++t;
	}
	timed_events_active_ = t;
	return false;
}