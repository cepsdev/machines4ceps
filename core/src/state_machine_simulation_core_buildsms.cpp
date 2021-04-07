/*
Copyright 2014,2015,2016,2017,2018,2019,2020,2021 Tomas Prerovsky (cepsdev@hotmail.com).

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
#include "core/include/sm_comm_naive_msg_prot.hpp"
#include "core/include/sm_ev_comm_layer.hpp"
#include "core/include/serialization.hpp"
#include "core/include/sm_raw_frame.hpp"
#include "core/include/sm_xml_frame.hpp"
#include "pugixml.hpp"
#include "utils/can_layer_docgen.hpp"
#ifdef __gnu_linux__

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <dlfcn.h>

#else

#endif
#include <sys/types.h>
#include <limits>
#include <cstring>

#include "core/include/base_defs.hpp"
#include <cassert>
#include <algorithm>
#include <unordered_set>
#include <list>

static std::string escape_string(std::string const & s){
    bool transform_necessary = false;
    for(std::size_t i = 0; i!=s.length();++i){
        auto ch = s[i];
        if (ch == '\n' || ch == '\t'|| ch == '\r' || ch == '"' || ch == '\\'){
            transform_necessary = true; break;
        }
    }
    if (!transform_necessary) return s;

    std::stringstream ss;
    for(std::size_t i = 0; i!=s.length();++i){
        char buffer[2] = {0};
        char ch = buffer[0] = s[i];
        if (ch == '\n') ss << "\\n";
        else if (ch == '\t') ss << "\\t";
        else if (ch == '\r' ) ss << "\\r";
        else if (ch == '"') ss << "\\\"";
        else if (ch == '\\') ss << "\\\\";
        else ss << buffer;
    }
    return ss.str();
}

const auto SM4CEPS_VER_MAJ = 0;
const auto SM4CEPS_VER_MINOR = .600;

int get_sm4ceps_ver_maj() { return SM4CEPS_VER_MAJ; }
double get_sm4ceps_ver_minor() { return SM4CEPS_VER_MINOR; }
int odd(int i) { return i % 2; }
int even(int i) { return !odd(i); }
int truncate(double i) { return (int)i; }
int truncate(int i) { return i; }
double mymin(double a, double b) { return std::min(a,b); }
double mymin(double a, int b) { return std::min(a, (double)b); }
double mymin(int a, double b) { return std::min((double)a, b); }
int mymin(int a, int b) {  return std::min(a, b); }

extern std::vector<std::thread*> comm_threads;


bool read_func_call_values(State_machine_simulation_core* smc,	ceps::ast::Nodebase_ptr root_node,
							std::string & func_name,
							std::vector<ceps::ast::Nodebase_ptr>& args);
void comm_sender_generic_tcp_out_thread(State_machine_simulation_core::frame_queue_t* frames,
			     State_machine_simulation_core* smc,
			     std::string ip,
			     std::string port,
			     std::string som,
			     std::string eof,
			     std::string sock_name,
			     bool reuse_socket, bool reg_socket);

void comm_generic_tcp_in_dispatcher_thread(int id,
				 Rawframe_generator* gen,
				 std::string ev_id,
				 std::vector<std::string> params,
			     State_machine_simulation_core* smc,
			     std::string ip,
			     std::string port,std::string som,std::string eof,std::string sock_name,bool reg_sock,bool reuse_sock,
			     void (*handler_fn) (int,Rawframe_generator*,std::string,std::vector<std::string> ,State_machine_simulation_core* , sockaddr_storage,int,std::string,std::string,std::string,bool,bool));

void comm_generic_tcp_in_thread_fn(int id,
		 Rawframe_generator* gen,
		 std::string ev_id,
		 std::vector<std::string> params,
		 State_machine_simulation_core* smc,
		 sockaddr_storage claddr,int sck,std::string som,std::string eof,std::string sock_name,bool reg_sock,bool reuse_sock);

extern void print_qualified_id(std::ostream& os,std::vector<std::string> const & q_id);
extern std::string qualified_id_to_str(std::vector<std::string> const & q_id);

template<typename F, typename T> void traverse_sm(std::unordered_set<State_machine*>& m,State_machine* sm, T const & sms, F f){
	f(sm);
	
	for(auto state: sm->states()){
		if (!state->is_sm() || state->smp() == nullptr) continue;
		if (m.find(state->smp()) != m.end()) continue;
		m.insert(state->smp());
		traverse_sm(m,state->smp(),sms,f);
	}
	
	for(auto subsm: sm->children()){
		//assert(m.find(subsm) != m.end());			
		if (m.find(subsm) != m.end()) continue;
		m.insert(subsm);
		traverse_sm(m,subsm,sms,f);
	}
}

template<typename F, typename T> void traverse_sms(T const & sms, F f){
	std::unordered_set<State_machine*> m;
	for(auto sm: sms){
	 if (m.find(sm) != m.end()) continue;
	 traverse_sm(m,sm,sms,f);
	 m.insert(sm);
	}	
}

static int get_state_idx(state_rep_t  s){
	if (s.is_sm_) return s.smp_->idx_;
	for(auto ss : s.smp_->states()){
		if (ss->id_ != s.sid_) continue;
		return ss->idx_;
	}
	return -1;
}

int compute_state_and_event_ids(State_machine_simulation_core* smp,
		                        std::map<std::string,State_machine*> sms,
								std::map<std::string,int>& map_fullqualified_sm_id_to_computed_idx){
	using namespace ceps::ast;
	using namespace std;
	DEBUG_FUNC_PROLOGUE;
	int ev_ctr = 1;

	auto & ev_to_id = smp->executionloop_context().ev_to_id;

	auto& ctx = smp->executionloop_context();

	for(auto const &s : smp->ceps_env_current().get_global_symboltable().scopes)
	{
		for(auto se : s->name_to_symbol)
		{
			if(se.second.category != ceps::parser_env::Symbol::SYMBOL) continue;
			if(((ceps::parser_env::Symbol*)se.second.payload)->name != "Event") continue;
			ev_to_id[se.first] = ev_ctr++;
		}
	}

	smp->executionloop_context().transitions.clear();
	smp->executionloop_context().state_to_first_transition.clear();


	std::vector<State_machine*> smsv;
	for(auto sm : sms) smsv.push_back(sm.second);
    int ctr = 1;
    bool non_cover_sm = true;
	auto compute_state_ids_fn = [&ctr,&ev_to_id,&non_cover_sm](State_machine* cur_sm){
		if (cur_sm->cover() && non_cover_sm) return;
		if (!cur_sm->cover() && !non_cover_sm) return;

		cur_sm->idx_ = ctr++;
        std::vector<State_machine::State*> stts{cur_sm->states().begin(),cur_sm->states().end()};
        std::sort(stts.begin(),stts.end(),[](State_machine::State* a, State_machine::State* b){return a->order_ < b->order_;});
        for(auto it = stts.begin(); it != stts.end(); ++it){
		 auto state = *it;
		 state->idx_ = ctr++;
		 if (!state->is_sm_) continue;
		 state->smp_->idx_ = state->idx_;
		}	
	 };
	traverse_sms(smsv,compute_state_ids_fn);
	non_cover_sm = false;
	auto old_state_ctr = ctr;
	traverse_sms(smsv,compute_state_ids_fn);
    if (old_state_ctr != ctr) smp->executionloop_context().start_of_covering_states = old_state_ctr;

	smp->executionloop_context().number_of_states = ctr-1;

	//Associated sms

	traverse_sms(smsv,[&ctx,&ctr,&ev_to_id](State_machine* cur_sm){
		ctx.set_assoc_sm(cur_sm->idx_,cur_sm);
		for(auto it = cur_sm->states().begin(); it != cur_sm->states().end(); ++it) {
		 auto state = *it;
		 ctx.set_assoc_sm(state->idx_,cur_sm);
		}
	 }
	);


	traverse_sms(smsv,[&ctr,&ev_ctr,&ev_to_id](State_machine* cur_sm){
	  for(auto & t : cur_sm->transitions()){
		for(auto e : t.events()){
		 if(ev_to_id.find(e.id_) != ev_to_id.end()) continue;
		 ev_to_id[e.id_] = ev_ctr;
		 ++ev_ctr;
		}
	  }
	 } 
	);

	traverse_sms(smsv,[&ctr,&ev_ctr,&ev_to_id](State_machine* cur_sm){
		for(auto it = cur_sm->states().begin(); it != cur_sm->states().end(); ++it) {
		 auto state = *it;
		 for(auto & t : cur_sm->transitions()){
			if (t.from_.is_sm_) {t.from_.idx_ = t.from_.smp_->idx_;assert(t.from_.idx_>0);}
			else if (t.from_.id_ == state->id_) {t.from_.idx_ = state->idx_;assert(t.from_.idx_>0);}
			if (t.orig_parent_ != nullptr && t.orig_parent_ != cur_sm){
				if (t.to_.is_sm_) {t.to_.idx_ = t.to_.smp_->idx_;assert(t.to_.idx_>0);}
				else {
					for(auto it = t.orig_parent_->states().begin(); it != t.orig_parent_->states().end(); ++it){
						auto state = *it;
						if (t.to_.id_ == state->id_) {t.to_.idx_ = state->idx_;}
					}
					assert(t.to_.idx_>0);
				}
			} else{
			 if (t.to_.is_sm_) {t.to_.idx_ = t.to_.smp_->idx_;assert(t.to_.idx_>0);}
			 else if (t.to_.id_ == state->id_) {t.to_.idx_ = state->idx_;assert(t.to_.idx_>0);}
			}
		 }//for
		}
	 }
	);

    //Build a map which maps any sm A to the sm's A(i) s.t. there exists a transition A(i) -> A

    std::map<State_machine*, std::set<State_machine*> > mmm;
    traverse_sms(smsv,[&ctx,&mmm](State_machine* sm){
        for(auto & t : sm->transitions()){
            if (!t.from_.is_sm_) continue;
            if (t.from_.smp_ == sm) continue;
            mmm[t.from_.smp_].insert(sm);
        }
    });

	//Build loop execution context
    auto build_transitions_table = [&ctr,&ev_ctr,&ev_to_id, &ctx,&smsv,smp,&non_cover_sm,&mmm](State_machine* cur_sm){

		if (cur_sm->cover() && non_cover_sm) return;
		if (!cur_sm->cover() && !non_cover_sm) return;

		executionloop_context_t::transition_t t;
		t.smp = cur_sm->idx_;
		assert(t.smp > 0);
		ctx.transitions.push_back(t);
		ctx.state_to_first_transition[t.smp] = ctx.transitions.size()-1;
		auto insert_transitions = [&ctx,&ev_to_id,smp](State_machine* sm_from, State_machine* sm_to){
			 for(auto & t : sm_from->transitions()){
			  if (sm_from != sm_to){
               if (!t.from_.is_sm_) continue;
               if (t.from_.smp_ != sm_to) continue;
			  } else {
			   if (t.from_.is_sm_) continue;
			   if (t.from_.parent_!= nullptr && t.from_.parent_ != sm_from) continue;
			  }
			  executionloop_context_t::transition_t tt;

			  tt.props |= (t.abstract ? executionloop_context_t::TRANS_PROP_ABSTRACT : 0);
			  tt.smp = sm_to->idx_;
			  tt.from = t.from_.idx_;
			  assert(tt.from > 0);
			  tt.to = t.to_.idx_;
			  assert(tt.to > 0);
			  for(auto  const &  ev : t.events())
			  {
				tt.ev = ev_to_id[ev.id_];
				assert(tt.ev > 0);
				break;
			  }
			  {
			   auto it =  smp->get_user_supplied_guards().find(t.guard_);
			   if (it !=  smp->get_user_supplied_guards().end())
				  tt.guard = it->second;
			  }
			  if (tt.guard == nullptr && t.has_guard()){
				  tt.script_guard = t.guard();
			  }
              if (t.action_.size() && t.action_[0].native_func_ == nullptr){
               //Case: no native implementation available
               tt.native = false;
               if (t.action_.size() >= 1) tt.a1_script = t.action_[0].body();
               if (t.action_.size() >= 2) tt.a2_script = t.action_[1].body();
               if (t.action_.size() >= 3) tt.a3_script = t.action_[2].body();
              } else{
               if (t.action_.size() >= 1) tt.a1 = t.action_[0].native_func_;
               if (t.action_.size() >= 2) tt.a2 = t.action_[1].native_func_;
               if (t.action_.size() >= 3) tt.a3 = t.action_[2].native_func_;
              }
              //assert(t.action_.size() < 4);
              t.id_ = ctx.transitions.size();
			  ctx.transitions.push_back(tt);
			  if(ctx.state_to_first_transition.find(tt.from) != ctx.state_to_first_transition.end()) continue;
			  ctx.state_to_first_transition[tt.from] = ctx.transitions.size()-1;
			 }
		};

        //Fetch foreign transitions i.e. transitions with from != cur_sm

        for(auto sm : mmm[cur_sm]) {
            insert_transitions(sm,cur_sm);
        }
        /*traverse_sms(smsv,[&ctx,&cur_sm,&ev_to_id,insert_transitions](State_machine* sm){
            if (sm != cur_sm) insert_transitions(sm,cur_sm);
        });*/
        insert_transitions(cur_sm,cur_sm);
    };
	non_cover_sm = true;
	traverse_sms(smsv,build_transitions_table);
    auto count_transitions = ctx.transitions.size();

	non_cover_sm = false;
	traverse_sms(smsv,build_transitions_table);
	if (count_transitions != ctx.transitions.size()) ctx.start_of_covering_transitions = count_transitions;

	//Set parent information
	traverse_sms(smsv,[&ctr,&ev_ctr,&ev_to_id, &ctx](State_machine* cur_sm){
        ctx.set_inf(cur_sm->idx_,executionloop_context_t::LOG_ENTER_TIME,cur_sm->log_enter_state());
        ctx.set_inf(cur_sm->idx_,executionloop_context_t::LOG_EXIT_TIME,cur_sm->log_exit_state());
        if (cur_sm->label().length()){
            ctx.set_inf(cur_sm->idx_,executionloop_context_t::HAS_LABEL,cur_sm->label().length());
            ctx.state_labels[cur_sm->idx_] = cur_sm->label();
        }
		bool inside_thread = false;
		ctx.set_inf(cur_sm->idx_,executionloop_context_t::SM,true);
		ctx.set_inf(cur_sm->idx_,executionloop_context_t::THREAD,inside_thread = cur_sm->is_thread_);
		ctx.set_inf(cur_sm->idx_,executionloop_context_t::JOIN,cur_sm->join_);
        if (cur_sm->dont_cover_loops()) ctx.set_inf(cur_sm->idx_,executionloop_context_t::DONT_COVER_LOOPS,true);
        if (cur_sm->hidden()) ctx.set_inf(cur_sm->idx_,executionloop_context_t::HIDDEN,true);

		if (cur_sm->join_) {
			for(auto s : cur_sm->states()){
				if(s->id_ !=cur_sm->join_state_.id_) continue;
				ctx.set_join_state(cur_sm->idx_,s->idx_);
				break;
			}

		}
		for(auto s : cur_sm->states()){
			ctx.set_parent(s->idx_,cur_sm->idx_);
			ctx.set_inf(s->idx_,executionloop_context_t::INIT,s->is_initial());
            ctx.set_inf(s->idx_,executionloop_context_t::DONT_COVER,s->dont_cover());
			ctx.set_inf(s->idx_,executionloop_context_t::FINAL,s->is_final());
			ctx.set_inf(s->idx_,executionloop_context_t::IN_THREAD,inside_thread);
            ctx.set_inf(s->idx_,executionloop_context_t::LOG_ENTER_TIME,cur_sm->log_enter_state());
            ctx.set_inf(s->idx_,executionloop_context_t::LOG_EXIT_TIME,cur_sm->log_exit_state());
            if (s->label().length()){
                ctx.set_inf(s->idx_,executionloop_context_t::HAS_LABEL,s->label().length());
                ctx.state_labels[s->idx_] = s->label();
            }

			if (s->is_initial()) ctx.set_initial_state(cur_sm->idx_,s->idx_);
			if (s->is_final()) ctx.set_final_state(cur_sm->idx_,s->idx_);
		}
		for(auto s : cur_sm->children()){
			ctx.set_parent(s->idx_,cur_sm->idx_);
			if (s->is_thread_)
				ctx.set_inf(cur_sm->idx_,executionloop_context_t::REGION,true);
            ctx.set_inf(s->idx_,executionloop_context_t::LOG_ENTER_TIME,cur_sm->log_enter_state());
            ctx.set_inf(s->idx_,executionloop_context_t::LOG_EXIT_TIME,cur_sm->log_exit_state());
            if (s->label().length()){
                ctx.set_inf(s->idx_,executionloop_context_t::HAS_LABEL,s->label().length());
                ctx.state_labels[s->idx_] = s->label();
            }
		}
	});

	//Set on_enter / on_exit information
	traverse_sms(smsv,[&ctr,&ev_ctr,&ev_to_id, &ctx](State_machine* cur_sm){
		for(auto a : cur_sm->actions()){
			if (a.id_ == "on_enter") {
			 ctx.set_on_enter(cur_sm->idx_,a.native_func_);
			 ctx.set_inf(cur_sm->idx_,executionloop_context_t::ENTER,true);
			}
			else if (a.id_ == "on_exit") {
			 ctx.set_on_exit(cur_sm->idx_,a.native_func_);
			 ctx.set_inf(cur_sm->idx_,executionloop_context_t::EXIT,true);
			}
		}
	});

	//Set children information
	ctx.init_state_to_children();
	traverse_sms(smsv,[&ctr,&ev_ctr,&ev_to_id, &ctx](State_machine* cur_sm){
		ctx.state_to_children[cur_sm->idx_] = ctx.children.size();
		ctx.children.push_back(cur_sm->idx_);
		for(auto s : cur_sm->states())ctx.children.push_back(s->idx_);
		for(auto s : cur_sm->children_)ctx.children.push_back(s->idx_);
		ctx.children.push_back(0);
	});


	traverse_sms(smsv,[&map_fullqualified_sm_id_to_computed_idx,&smp](State_machine* cur_sm){
		state_rep_t srep(true,true,cur_sm,cur_sm->id(),cur_sm->idx_);
		//std::string t1;
		map_fullqualified_sm_id_to_computed_idx[ smp->get_fullqualified_id(srep)]=cur_sm->idx_;
		
		for(auto it = cur_sm->states().begin(); it != cur_sm->states().end(); ++it) {
		 auto state = *it;
		 map_fullqualified_sm_id_to_computed_idx[smp->get_full_qualified_id(*state)]=state->idx_;
		 //std::cout << t1 <<" : " << state->idx_ << " : " << state->id()<< std::endl;
		}	
	 } 
	);
	
	//compute root sms for each transition
	for(auto & t : smp->executionloop_context().transitions)
	{
		auto p = t.smp;
		if ( ((ssize_t)smp->executionloop_context().parent_vec.size()) > p)
			for(;smp->executionloop_context().parent_vec[p];p = smp->executionloop_context().parent_vec[p]);
		t.root_sms = p;
	}

	//Compute Shadow States
	smp->executionloop_context().shadow_state.resize(ctr);
	traverse_sms(smsv,[&smp](State_machine* cur_sm){
	  if (cur_sm->shadow.valid()){
		  smp->executionloop_context().shadow_state[cur_sm->idx_] = get_state_idx(cur_sm->shadow);
	  } else smp->executionloop_context().shadow_state[cur_sm->idx_] = -1;

	  for(auto it = cur_sm->states().begin(); it != cur_sm->states().end(); ++it) {
	   auto state = *it;
	   if(state->is_sm_) continue;
	   if (!state->shadow.valid()) smp->executionloop_context().shadow_state[state->idx_] = -1;
	   else smp->executionloop_context().shadow_state[state->idx_] = get_state_idx(state->shadow);
	  }
	 }
	);
	return ctr;
}
bool State_machine_simulation_core::register_action_impl(std::string state_machine_id,
		                                                 std::string action,
														 void(*fn)(),State_machine* parent){
	//std::cerr << state_machine_id  << std::endl;
	if (state_machine_id.length() == 0)
	{
		if (parent == nullptr) return false;
		//std::cerr << sm_to_id_[parent] <<"."<< action << std::endl;
		bool r = false;
		for(auto & a : parent->actions()){
			//std::cerr << a.id_ << std::endl;
			if (a.id_ != action) continue;
			a.native_func() = fn;
			r = true;
		}
		for(auto & t: parent->transitions()){
		  if(t.orig_parent() != nullptr && parent != t.orig_parent())//transition was moved
		    continue;
			for (auto & a : t.actions()){
				if (a.id_ != action) continue;
				a.native_func() = fn;
				r = true;
			}
		}
		for(auto foreign_smp: parent->smps_containing_moved_transitions()){
		 for(auto & t: foreign_smp->transitions()){
		  if(t.orig_parent() !=  parent )//transition was moved
		    continue;
			for (auto & a : t.actions()){
				if (a.id_ != action) continue;
				a.native_func() = fn;
				r = true;
			}
		 }
		}
		//std::cerr << r << std::endl;
		return r;
	}

	auto i = state_machine_id.find_first_of('.');
	std::string base;
	std::string rest;

	if (i == std::string::npos) base = state_machine_id;
	else {base = state_machine_id.substr(0,i);rest =  state_machine_id.substr(i+1);}
	if (parent == nullptr){
		auto it = State_machine::statemachines.find(base);
		if (it == State_machine::statemachines.end()) {
			return false;
		}
		return register_action_impl(rest,action,fn,it->second);
	}

	for(auto smp: parent->children()){
		if (smp->id() == base)
			return register_action_impl(rest,action,fn,smp);
	}

	for(auto s: parent->states()){
		if(!s->is_sm_) continue;
		if (s->id() == base)
			return register_action_impl(rest,action,fn,s->smp_);
	}
	return false;
}

bool State_machine_simulation_core::register_action(std::string state_machine_id,std::string action, void(*fn)()){
	auto r = register_action_impl(state_machine_id,action,fn,nullptr);
	if (!r)
			this->fatal_(-1,"Failed to register native implementation for '"+state_machine_id+"."+action+"'");
	return true;
}


void State_machine_simulation_core::register_frame_ctxt(sm4ceps_plugin_int::Framecontext* ctxt, std::string receiver_id){
	if (ctxt == nullptr) return;
	auto thrd_ctxt = this->get_dispatcher_thread_ctxt(receiver_id);
	if(thrd_ctxt ==nullptr) this->fatal_(-1,"State_machine_simulation_core::register_frame_ctxt: unknown receiver id '"+receiver_id+"'");
	thrd_ctxt->get_native_handler().push_back(ctxt);

}

static void run_simulations(State_machine_simulation_core* smc,
		                Result_process_cmd_line const& result_cmd_line,
						ceps::Ceps_Environment& ceps_env,
						ceps::ast::Nodeset& universe){
 using namespace ceps::ast;
 if (result_cmd_line.ignore_simulations) return;

 auto simulations = universe[all{"Simulation"}];
 if (!simulations.size()) return;

 for (auto simulation_ : simulations){
	auto simulation = simulation_["Simulation"];
	smc->process_simulation(simulation,ceps_env,universe);
 }//for
 smc->print_report(result_cmd_line, ceps_env, universe);
}

static void flatten_args(ceps::ast::Nodebase_ptr r, std::vector<ceps::ast::Nodebase_ptr>& v, char op_val = ',')
{
	using namespace ceps::ast;
	if (r == nullptr) return;
	if (r->kind() == ceps::ast::Ast_node_kind::binary_operator && op(as_binop_ref(r)) ==  op_val)
	{
		auto& t = as_binop_ref(r);
		flatten_args(t.left(),v,op_val);
		flatten_args(t.right(),v,op_val);
		return;
	}
	v.push_back(r);
}

static bool is_func_call(ceps::ast::Nodebase_ptr p,std::string& fid,std::vector<ceps::ast::Nodebase_ptr>& args){
 if (p->kind() != ceps::ast::Ast_node_kind::func_call) return false;
 ceps::ast::Func_call& func_call = *dynamic_cast<ceps::ast::Func_call*>(p);
 ceps::ast::Identifier& id = *dynamic_cast<ceps::ast::Identifier*>(func_call.children()[0]);
 ceps::ast::Call_parameters& params = *dynamic_cast<ceps::ast::Call_parameters*>(func_call.children()[1]);
 if (params.children().size()) flatten_args(params.children()[0], args);
 fid = ceps::ast::name(id);
 return true;
}

static bool is_func_call(ceps::ast::Nodebase_ptr p,std::string& fid,std::vector<ceps::ast::Nodebase_ptr>& args, std::size_t number_of_arguments){
	auto r = is_func_call(p,fid,args);
	if (!r) return r;
	return number_of_arguments == args.size();
}

static bool is_id_or_symbol(ceps::ast::Nodebase_ptr p, std::string& n, std::string& k){
	using namespace ceps::ast;
	if (p->kind() == Ast_node_kind::identifier) {n = name(as_id_ref(p));k = ""; return true;}
	if (p->kind() == Ast_node_kind::symbol) {n = name(as_symbol_ref(p));k = kind(as_symbol_ref(p)); return true;}
	return false;
}

static bool is_id(ceps::ast::Nodebase_ptr p, std::string & result, std::string& base_kind){
	using namespace ceps::ast;
	std::string k,l;
	if (p->kind() == Ast_node_kind::binary_operator && op(as_binop_ref(p)) == '.'){
	 if (!is_id_or_symbol(as_binop_ref(p).right(),k,l)) return false;

	 if (!is_id(as_binop_ref(p).left(),result,base_kind)) return false;
	 result = result + "." + k;
	 return true;
	} else if (is_id_or_symbol(p,k,l)){ base_kind = l; result = k; return true; }
	return false;
}

static ceps::ast::Struct_ptr as_struct(ceps::ast::Nodebase_ptr p){
	if (p->kind() == ceps::ast::Ast_node_kind::structdef) return ceps::ast::as_struct_ptr(p);
	return nullptr;
}

static std::tuple<bool,std::string,std::string,std::vector<ceps::ast::Nodebase_ptr>,State_machine*> is_sm_instantiation(
		State_machine_simulation_core* smc,ceps::ast::Nodebase_ptr p){
 if (!smc->is_assignment_op(p)) return {};
 ceps::ast::Binary_operator& opr = ceps::ast::as_binop_ref(p);
 auto left = opr.left();
 auto right = opr.right();
 std::string lhs_id;
 std::string base_kind;
 auto rid = is_id(left, lhs_id,  base_kind);
 if (!rid) return {};

 std::vector<ceps::ast::Nodebase_ptr> args;
 std::string rhs_id;
 auto rf = is_func_call(right,rhs_id,args);
 if(!rf){
	 auto rid = is_id(right, rhs_id,  base_kind);
	 if(!rid) return {};
 }
 auto r = State_machine::statemachines.find(rhs_id);
 if (r==State_machine::statemachines.end()) return {};
 return std::make_tuple(true,lhs_id,rhs_id,args,r->second);
}

static State_machine* handle_instantiation(State_machine_simulation_core* smc,
						  State_machine* base_sm,
						  std::string const & rhs_id,std::string const & sm_id,
						  std::vector<ceps::ast::Nodebase_ptr> const & args){
 State_machine* sm = new State_machine(0,sm_id,nullptr,0);
 sm->clone_from(base_sm,State_machine_simulation_core::SM_COUNTER,rhs_id,nullptr,smc);
 for(auto p:args){
	 auto r = is_sm_instantiation(smc,p);
	 if (!std::get<0>(r)) continue;
	 State_machine* base_sm2 = std::get<4>(r);
	 auto sub_sm = handle_instantiation(smc,base_sm2,std::get<2>(r),std::get<1>(r),std::get<3>(r) );
	 if (sub_sm == nullptr) smc->fatal_(-1,"Failed to instantiate "+std::get<1>(r));
	 sub_sm->parent_ = sm;
	 State_machine::statemachines[sm_id+"."+std::get<1>(r)] = sub_sm;
	 for(auto it = sm->states().begin(); it != sm->states().end();++it){
		if ( (*it)->id() != std::get<1>(r)) continue;
		sm->states().erase(it);break;
	 }
	 sm->add_child(sub_sm);

	 for(auto it = sm->transitions().begin();it!=sm->transitions().end();++it){
		 if (it->from_.id_ == std::get<1>(r)){it->from_.is_sm_=true;it->from_.smp_=sub_sm;}
		 if (it->to_.id_ == std::get<1>(r)){it->to_.is_sm_=true;it->to_.smp_=sub_sm;}
	 }
 }
 return sm;
}

void handle_instantiations(ceps::ast::Nodeset& ns,State_machine_simulation_core* smc){
 //std::cout << ns << std::endl;
 for(auto p : ns.nodes()){
	 auto r = is_sm_instantiation(smc,p);
	 if (!std::get<0>(r)) continue;
	 //std::cout << std::get<1>(r) <<"="<<std::get<2>(r)<< std::endl;
	 State_machine* base_sm = std::get<4>(r);

	 auto sm = handle_instantiation(smc,base_sm,std::get<2>(r),std::get<1>(r),std::get<3>(r) );
	 if (sm == nullptr) smc->fatal_(-1,"Failed to instantiate "+std::get<1>(r));
	 State_machine::statemachines[std::get<1>(r)] = sm;
 }
}

static bool is_a_lazy_ceps_fun(std::string const & id){
	return id == "start_signal" || id == "changed" || id == "breakup_byte_sequence";
}



static int establish_inet_stream_connect(std::string remote, std::string port) {
    int cfd = -1;
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(remote.c_str(), port.c_str(), &hints, &result) != 0) return -1;
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        cfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (cfd == -1) continue;
        if (connect(cfd, rp->ai_addr, rp->ai_addrlen) != -1) break;

        if (rp->ai_next == NULL) {
                close(cfd); cfd = -1;
            if (result != nullptr) freeaddrinfo(result);
            return -1;
        }
        close(cfd); cfd = -1;
    }
    return cfd;
}

static std::pair<bool,std::string> get_virtual_can_attribute_content(std::string attr, std::vector<std::pair<std::string,std::string>> const & http_header){
 for(auto const & v : http_header){
     if (v.first == attr)
         return {true,v.second};
 }
 return {false,{}};
}


static std::tuple<bool,std::string,std::vector<std::pair<std::string,std::string>>> read_virtual_can_msg(int sck,std::string& unconsumed_data){
 using header_t = std::vector<std::pair<std::string,std::string>>;
 std::tuple<bool,std::string,header_t> r;

 constexpr auto buf_size = 4096;
 char buf[buf_size];
 std::string buffer = unconsumed_data;
 std::string eom = "\r\n\r\n";
 std::size_t eom_pos = 0;

 unconsumed_data.clear();
 bool req_complete = false;
 ssize_t readbytes = 0;
 ssize_t buf_pos = 0;

 for(; (readbytes=recv(sck,buf,buf_size-1,0)) > 0;){
  buf[readbytes] = 0;
  for(buf_pos = 0; buf_pos < readbytes; ++buf_pos){
   if (buf[buf_pos] == eom[eom_pos])++eom_pos;else eom_pos = 0;
   if (eom_pos == eom.length()){
    req_complete = true;
    if (buf_pos+1 < readbytes) unconsumed_data = buf+buf_pos+1;
    buf[buf_pos+1] = 0;
    break;
   }
  }
  buffer.append(buf);
  if(req_complete) break;
 }

 if (req_complete) {
  header_t header;
  std::string first_line;
  size_t line_start = 0;
  for(size_t i = 0; i < buffer.length();++i){
    if (i+1 < buffer.length() && buffer[i] == '\r' && buffer[i+1] == '\n' ){
        if (line_start == 0) first_line = buffer.substr(line_start,i);
        else if (line_start != i){
         std::string attribute;
         std::string content;
         std::size_t j = line_start;
         for(;j < i && buffer[j]==' ';++j);
         auto attr_start = j;
         for(;j < i && buffer[j]!=':';++j);
         attribute = buffer.substr(attr_start,j-attr_start);
         ++j;
         for(;j < i && buffer[j]==' ' ;++j);
         auto cont_start = j;
         auto cont_end = i - 1;
         for(;buffer[cont_end] == ' ';--cont_end);
         content = buffer.substr(cont_start, cont_end - cont_start + 1);
         header.push_back(std::make_pair(attribute,content));
        }
        line_start = i + 2;++i;
    }
  }
  return std::make_tuple(true,first_line,header);
 }

 return std::make_tuple(false,std::string{},header_t{});
}

static std::tuple<bool, std::string, std::vector<std::pair<std::string, std::string>>> send_cmd(int sock,
                                                                                                std::string command,
                                                                                                std::vector<std::pair<std::string,std::string>> parameters) {
    std::stringstream cmd;
    cmd << "HTTP/1.1 100\r\n";
    cmd << "cmd: " + command + "\r\n";
    for(auto e : parameters)
     cmd << e.first << ": "<<e.second<<"\r\n";
    cmd << "\r\n";
    ssize_t r = send(sock, cmd.str().c_str(), cmd.str().length(), 0);
    if (r != cmd.str().length()) return std::make_tuple(false,std::string{},std::vector<std::pair<std::string, std::string>>{});
    std::string unconsumed_data;
    return read_virtual_can_msg(sock, unconsumed_data);
}





struct fmt_out_ctx{
	bool bold = false;
	bool italic = false;
	std::string foreground_color;
	std::string suffix;
	std::string eol="\n";
	std::vector<std::string> info; 
    std::string indent_str = "  ";
	int indent = 0;
};

static void fmt_out_layout_outer_strct(bool is_schema, fmt_out_ctx& ctx){
	if (is_schema) {
		ctx.foreground_color = "214";
		ctx.suffix = ":";
		ctx.info.push_back("schema");
	}
}

static void fmt_out_layout_inner_strct(fmt_out_ctx& ctx){
	ctx.foreground_color = "25";
	ctx.suffix = ":";
	//ctx.info.push_back("schema");
}


static void fmt_out(std::ostream& os, std::string s, fmt_out_ctx ctx){
 for(int i = 0; i < ctx.indent; ++ i)
  os << ctx.indent_str;
 os << "\033[0m"; //reset
 if (ctx.foreground_color.size()) os << "\033[38;5;"<< ctx.foreground_color << "m";
 os << s;
 if (ctx.info.size()){
    os << "\033[0m"; //reset
	os << "\033[2m";
	os << " (";
	for(size_t i = 0; i + 1 < ctx.info.size(); ++i)
	 os << ctx.info[i] << ",";
	os << ctx.info[ctx.info.size()-1];
	os << ")";
	os << "\033[0m"; //reset
    if (ctx.foreground_color.size()) os << "\033[38;5;"<< ctx.foreground_color << "m";
 }
 os << ctx.suffix;
 os << ctx.eol;
 os << "\033[0m"; //reset
}

static std::vector<ceps::ast::Symbol*> fetch_symbols_standing_alone(std::vector<ceps::ast::Nodebase_ptr> const & nodes){
	std::vector<ceps::ast::Symbol*> r;
	for (auto n : nodes)
	 if (ceps::ast::is_a_symbol(n))
	 	r.push_back(ceps::ast::as_symbol_ptr(n));
	return r;
}

static void fmt_out_handle_inner_struct(std::ostream& os, ceps::ast::Struct& strct, fmt_out_ctx ctx){		    
	{
		auto lctx{ctx};
		fmt_out_layout_inner_strct(lctx);
		fmt_out(os,ceps::ast::name(strct),lctx);
	}
	++ctx.indent;
	for(auto n: strct.children()){
		if (is_a_struct(n)){
			fmt_out_handle_inner_struct(os,ceps::ast::as_struct_ref(n),ctx);
		}
	}
}

static void fmt_out_handle_outer_struct(std::ostream& os, ceps::ast::Struct& strct, fmt_out_ctx ctx){
	auto v = fetch_symbols_standing_alone(strct.children());
	auto is_schema = v.end() != std::find_if(v.begin(),v.end(),[](ceps::ast::Symbol* p){ return ceps::ast::kind(*p) == "Schema"; });
	{
		auto local_ctx{ctx};
		fmt_out_layout_outer_strct(is_schema,local_ctx);
		fmt_out(os,ceps::ast::name(strct),local_ctx);
	}
	++ctx.indent;
	for(auto n: strct.children()){
		if (is_a_struct(n)){
			fmt_out_handle_inner_struct(os,ceps::ast::as_struct_ref(n),ctx);
		}
	}
}

static void fmt_out(std::ostream& os, ceps::ast::Nodeset& ns){
	using namespace ceps::ast;
	fmt_out_ctx ctx;

	for(auto n : ns.nodes()){
		//os << "\033[1;34m" << "Hello!"  << "\033[0;39m" << "\n";
		//os << "\033[38;5;19m\033[3;m" << "Hello!"  << "\033[0;39m" << "\n";
		if (is_a_struct(n)){
			auto&  current_struct{as_struct_ref(n)};
			fmt_out_handle_outer_struct(os,current_struct,ctx);



			//os << "\033[1;34m" << name(as_struct_ref(n)) << "\033[0;39m" << "\n";
			//os << "\033[1;34m" << v.size() << "\033[0;39m" << "\n";

		}
	}
}

void State_machine_simulation_core::processs_content(Result_process_cmd_line const& result_cmd_line,State_machine **entry_machine)
{
	using namespace ceps::ast;
	using namespace std;
	DEBUG_FUNC_PROLOGUE;

	this->enforce_native() = result_cmd_line.enforce_native;

	regfn("sm4ceps_version_major",get_sm4ceps_ver_maj);
	regfn("sm4ceps_version_minor", get_sm4ceps_ver_minor);
	regfn("odd", odd);
	regfn("even", even);
	regfn("truncate",static_cast<int(*)(double)>(truncate));
	regfn("truncate",static_cast<int(*)(int)>(truncate));
	regfn("min", static_cast<double(*)(double,double)> (mymin));
	regfn("min", static_cast<double(*)(int, double)> (mymin));
	regfn("min", static_cast<double(*)(double, int)> (mymin));
	regfn("min", static_cast<int(*)(int, int)> (mymin));

    ceps::ast::Nodeset& ns            = current_universe();

    auto globalfunctions             = ns["global_functions"];
    auto frames                      = ns[all{"raw_frame"}];
    auto xmlframes                   = ns[all{"xml_frame"}];
    auto all_sender                  = ns[all{"sender"}];
    auto all_receiver                = ns[all{"receiver"}];
    auto unique_event_declarations   = ns["unique"];
	auto no_transitions_declarations = ns["no_transitions"];
    auto exported_events             = ns["export"];

	start_comm_threads() = !generate_cpp_code();

	//Register global functions
	reg_global_ceps_fn("start_signal",&State_machine_simulation_core::ceps_fn_start_signal_gen);
	this->ceps_env_current().interpreter_env().is_lazy_func = &is_a_lazy_ceps_fun;

	if (result_cmd_line.print_evaluated_input_tree){
		if (result_cmd_line.output_format_flags.size()){
			fmt_out(std::cout,current_universe());
		}
		else 
			std::cout << ceps::ast::Nodebase::pretty_print << this->current_universe() << std::endl;
	}


	//handle unique definitions

	if (unique_event_declarations.size()){
		for (auto const & e : unique_event_declarations.nodes()){
			if (e->kind() != ceps::ast::Ast_node_kind::symbol || ceps::ast::kind(ceps::ast::as_symbol_ref(e)) != "Event")
				fatal_(-1,"unique definition requires a list of events.");
			auto  & ev = ceps::ast::as_symbol_ref(e);
			std::string id = ceps::ast::name(ev);
			this->unique_events().insert(id);
		}
	}
	//handle no_transitions definitions

	if (no_transitions_declarations.size()){
		for (auto const & e : no_transitions_declarations.nodes()){
			if (e->kind() != ceps::ast::Ast_node_kind::symbol || ceps::ast::kind(ceps::ast::as_symbol_ref(e)) != "Event")
				fatal_(-1,"no_transitions definition requires a list of events.");
			auto  & ev = ceps::ast::as_symbol_ref(e);
			std::string id = ceps::ast::name(ev);
			this->not_transitional_events().insert(id);
		}
	}

	post_event_processing() = ns["post_event_processing"];

	auto can_frames = ns[all{"frame"}];
	frames.nodes().insert(frames.nodes().end(),can_frames.nodes().begin(),can_frames.nodes().end());

	for(auto p:globalfunctions.nodes())
	{
		if(p->kind() != ceps::ast::Ast_node_kind::structdef) continue;
		std::string id = ceps::ast::name(ceps::ast::as_struct_ref(p));
		global_funcs()[id] = p;
	}

	for(auto p : current_universe().nodes()){
		auto pp = as_struct(p);
		if (pp != nullptr && ( name(*pp) == "Statemachine" || name(*pp) == "statemachine" || name(*pp) == "sm" ) ) {
		 Nodeset n(pp->children());
		 process_statemachine(n,"",nullptr,1,0);
		} else if (pp == nullptr) {
		 Scope ac_seq;ac_seq.owns_children_ = false;ac_seq.children().push_back(p);
		 execute_action_seq(nullptr,&ac_seq);
		}
	}

	auto entry_ = ns["main"];

	if (!entry_.empty() && entry_machine != nullptr) {
		string entry_name = name(as_id_ref(entry_.nodes()[0]));
		*entry_machine = State_machine::statemachines[entry_name ];
		if (entry_machine == nullptr)
		  fatal_(-1,"No statemachine with id "+entry_name+" found: No main statemachine defined.");
	}

	//BACK PATCH IMPORTS
	for(;;)
	{

		bool imports_resolved = false;
		auto s_t = State_machine::statemachines;
		for(auto & sim_ : s_t)
		{
			DEBUG << "[RESOLVE_IMPORTS][ID =  " << sim_.second->id_ << "]\n";
			auto & sim = sim_.second;
			if (sim->definition_complete() || sim->unresolved_imports().size() == 0) continue;

			if (!resolve_imports(*sim))
				{
				 fatal_(-1,sim->id()+": Error while resolving imports");
				}
			imports_resolved = true;
		}
		if (!imports_resolved) break;
	}

	DEBUG << "[BACK PATCH JOINS]\n";

	for(auto & sim_ : State_machine::statemachines)
	{
		if (!sim_.second->join()) continue;
		if (!sim_.second->join_state().unresolved()) continue;

		DEBUG << "[BACK PATCH JOIN STATE][SM=" << sim_.first <<"]\n";
		State_machine::State s;
		resolve_q_id(sim_.second, sim_.second->join_state().q_id(), s);
		sim_.second->join_state() = s;

	}


	DEBUG << "[BACK PATCH TRANSITIONS]\n";


	//STEP#1 resolve transitions
	DEBUG << "[BACK PATCH TRANSITIONS][STEP(1):RESOLVE_TRANSITIONS]\n";
	std::set<State_machine*> machines_with_transitions_which_may_have_foreign_source;
	for(auto & sim_ : State_machine::statemachines)
	{
		for(size_t i = 0; i < sim_.second->transitions().size(); ++i)
		{
			auto & t = sim_.second->transitions()[i];
			if (t.from_.unresolved()) {
				DEBUG << "[RESOLVE FROM]\n";
				State_machine::State s;
				resolve_q_id(sim_.second, t.from_.q_id(), s);
				auto orig_q_id = t.from_.q_id();
				t.from_ = s;
				assert(t.from_.is_sm_ || t.from_.smp_ != nullptr);
				assert(t.from_.id_.size());
				assert(!t.from_.unresolved());

				if (print_debug_info_){
					state_rep_t sr,sr2;
					sr.valid_ = sr2.valid_ = true;
					sr.is_sm_ = s.is_sm_;
					sr.sid_ = s.id_;
					sr.smp_ = s.smp_;
					sr2.is_sm_ = true;
					sr2.sid_ = sim_.second->id_;
					sr2.smp_ = sim_.second;
					DEBUG << "[RESOLVE Q-ID STATE][SM=" << get_fullqualified_id(sr2) << "]"
						  <<"[" << qualified_id_to_str(orig_q_id)<< " ==> " << get_fullqualified_id(sr) << "]\n";
				}

				if (t.from_.parent() == sim_.second) t.from_.parent_ = nullptr;
				machines_with_transitions_which_may_have_foreign_source.insert(sim_.second);

			}
			if (t.to_.unresolved()){
				DEBUG << "[RESOLVE TO]\n";
				State_machine::State s;
				resolve_q_id(sim_.second, t.to_.q_id(), s);
				auto orig_q_id = t.to_.q_id();
				t.to_ = s;
				assert(t.to_.is_sm_ || t.to_.smp_ != nullptr);
				assert(t.to_.id_.size());
				assert(!t.to_.unresolved());
				if (print_debug_info_){
					state_rep_t sr,sr2;
					sr.valid_ = sr2.valid_ = true;
					sr.is_sm_ = s.is_sm_;
					sr.sid_ = s.id_;
					sr.smp_ = s.smp_;
					sr2.is_sm_ = true;
					sr2.sid_ = sim_.second->id_;
					sr2.smp_ = sim_.second;
					DEBUG << "[RESOLVE Q-ID STATE][SM=" << get_fullqualified_id(sr2) << "][" << qualified_id_to_str(orig_q_id)<< " ==> " << get_fullqualified_id(sr) << "]\n";
				}
			}
		}//for
	}//for


	DEBUG << "[BACK PATCH TRANSITIONS][STEP(1):MOVE_FOREIGN_TRANSITIONS]\n";
	for(auto sm :  machines_with_transitions_which_may_have_foreign_source)
	{
		auto it = std::stable_partition(sm->transitions().begin(),sm->transitions().end(),
				[&] (State_machine::Transition const & t) {
					return !t.from_.is_foreign(sm);
					}
				);
		if (it == sm->transitions().end()) continue;

		std::for_each(it,sm->transitions().end(),
		 [&](State_machine::Transition & t) {
			DEBUG << "[TRANSITION_MOVE(PRE)][FROM="<< get_full_qualified_id(t.from_) << "][TO=" << get_full_qualified_id(t.to_) << "]\n";
			auto parent = t.from_.parent();t.from_.parent_=nullptr;t.from_.q_id_.clear();
			if(!t.from_.is_sm_)t.from_.smp_ = parent;
			assert(sm != parent);
			t.orig_parent() = sm;
			parent->transitions().push_back(t);
			sm->smps_containing_moved_transitions().push_back(parent);

			DEBUG << "[TRANSITION_MOVE(POST)][FROM="<< get_full_qualified_id(t.from_) << "][TO=" << get_full_qualified_id(t.to_) << "]\n";
			}
		);
		sm->transitions().erase(it,sm->transitions().end());
	}
	//handle instantiations
	handle_instantiations(ns,this);

	this->statemachines() = State_machine::statemachines;



    //Check for consistency
	for (auto m: State_machine::statemachines)
	{
		for(auto& t: m.second->transitions())
		{
			assert(!t.from().unresolved());
			assert(!t.to().unresolved());
			assert(t.from().id_.size() > 0);
			assert(t.to().id_.size() > 0);
		}
	}

	DEBUG << "[PROCESSING TYPE DEFINITIONS][START]\n";

	auto typedefs = ns["typedef"];
	if (typedefs.size())
		for(auto node: typedefs.nodes())
		{
			if(node->kind() != ceps::ast::Ast_node_kind::structdef) continue;
			auto td = ceps::ast::as_struct_ptr(node);
			type_definitions()[ceps::ast::name(*td)] = td;
			DEBUG << "[PROCESSING TYPE DEFINITIONS][TYPEDEF] "<< ceps::ast::name(*td) <<"\n";
		}
	DEBUG << "[PROCESSING TYPE DEFINITIONS][END]\n";

	//Handle CALL frames

	for(auto rawframe_ : frames)
	{
		auto rawframe = rawframe_["raw_frame"];
		if(!rawframe.size()) rawframe = rawframe_["frame"];

		auto gen = new Podframe_generator;
		if (rawframe["id"].size() != 1)
			fatal_(-1,"raw_frame definition: missing id.");
		if (rawframe["id"].nodes()[0]->kind() != ceps::ast::Ast_node_kind::identifier)
			fatal_(-1,"raw_frame definition: id must be an unbound identifier.");

		std::string id = ceps::ast::name(ceps::ast::as_id_ref(rawframe["id"].nodes()[0]));
		DEBUG << "[PROCESSING RAW_FRAME_SPEC("<< id <<")][START]\n";
		gen->readfrom_spec(rawframe);
		frame_generators()[id] = gen;
		DEBUG << "[PROCESSING RAW_FRAME_SPEC][FINISHED]\n";
	}

	for(auto xmlframe_ : xmlframes)
	{
		auto xmlframe = xmlframe_["xml_frame"];
		auto gen = new Xmlframe_generator;
		if (xmlframe["id"].size() != 1)
			fatal_(-1,"xml_frame definition: missing id.");
		if (xmlframe["id"].nodes()[0]->kind() != ceps::ast::Ast_node_kind::identifier)
			fatal_(-1,"xml_frame definition: id must be an unbound identifier.");

		std::string id = ceps::ast::name(ceps::ast::as_id_ref(xmlframe["id"].nodes()[0]));
		DEBUG << "[PROCESSING XML_FRAME_SPEC("<< id <<")][START]\n";
		gen->readfrom_spec(xmlframe);
		frame_generators()[id] = gen;
		DEBUG << "[PROCESSING XML_FRAME_SPEC][FINISHED]\n";
	}

	//Handle CALL sender
	for(auto sender_ : all_sender)
	{
		DEBUG << "[PROCESSING SENDER]\n";
		auto sender = sender_["sender"];
		auto when = sender["when"];
		auto emit = sender["emit"];
		auto transport  = sender["transport"];

		if (transport["generic_tcp_out"].empty() && 0==transport["use"].size()) {
			//Handle user defined transport layers
			std::string call_name="(NULL)";
			if (transport.nodes().size() && transport.nodes()[0]->kind() == ceps::ast::Ast_node_kind::structdef)
				call_name = ceps::ast::name(ceps::ast::as_struct_ref(transport.nodes()[0]));
			auto r = handle_userdefined_sender_definition(call_name,sender);
			
			if (!r) 
				fatal_(-1, "Sender definition: '"+call_name+"' unknown CAL-identifier (Communication Abstraction Layer).\n");
			continue;
		}

		std::string sock_name;
		std::string port;
		std::string ip;
		std::string channel_id;
		bool reuse_sock = false;
		bool reg_socket = false;

		//std::cout << when << std::endl;
		bool condition_defined = true;
		bool emit_defined = true;

		if (when.size() != 1 || when.nodes()[0]->kind() != ceps::ast::Ast_node_kind::symbol || "Event" != ceps::ast::kind(ceps::ast::as_symbol_ref( when.nodes()[0])) )
			condition_defined=false;
		if ( emit.size() != 1 || emit.nodes()[0]->kind() != ceps::ast::Ast_node_kind::identifier)
			emit_defined=false;

		if (transport["use"].size() )
		{
			if (transport["use"].nodes()[0]->kind() == ceps::ast::Ast_node_kind::identifier){
				reuse_sock = true;
				sock_name = ceps::ast::name(ceps::ast::as_id_ref(transport["use"].nodes()[0]));
			}
		} else {
			if (!transport["generic_tcp_out"]["port"].empty()) port = transport["generic_tcp_out"]["port"].as_str();
			if (!transport["generic_tcp_out"]["ip"].empty()) ip = transport["generic_tcp_out"]["ip"].as_str();
		}


        if (sender["id"].size()) {
        	auto p = sender["id"].nodes()[0];
        	if (p->kind() != ceps::ast::Ast_node_kind::identifier) fatal_(-1,"Id field in sender definition should contain a single id");
        	channel_id = ceps::ast::name(ceps::ast::as_id_ref(p));
        	reg_socket = true;
        }


		std::string ev_id;
		if (condition_defined) ev_id = ceps::ast::name(ceps::ast::as_symbol_ref( when.nodes()[0]));
		std::string frame_id;
		if (emit_defined) frame_id = ceps::ast::name(ceps::ast::as_id_ref( emit.nodes()[0]));
		std::string eof="";
		std::string som="";

		if (transport["eom"].size()){
			auto  v = transport["eom"].nodes();
			//DOESN'T WORK!!!!!!: GCC BUG ??????: auto&  v = transport["generic_tcp_out"]["eof"].nodes();
			for(auto  e : v)
			{
				if (e->kind() == ceps::ast::Ast_node_kind::string_literal)
					eof.append(ceps::ast::value(ceps::ast::as_string_ref(e)));
				else if (e->kind() == ceps::ast::Ast_node_kind::int_literal && ceps::ast::value(ceps::ast::as_int_ref(e)) != 0)
					{eof.append(" "); eof[eof.length()-1] =  ceps::ast::value(ceps::ast::as_int_ref(e));}
			}
		}
		if (transport["som"].size()){
			auto  v = transport["som"].nodes();
			//DOESN'T WORK!!!!!!: GCC BUG ??????: auto&  v = transport["generic_tcp_out"]["eof"].nodes();
			for(auto  e : v)
			{
				if (e->kind() == ceps::ast::Ast_node_kind::string_literal)
					som.append(ceps::ast::value(ceps::ast::as_string_ref(e)));
				else if (e->kind() == ceps::ast::Ast_node_kind::int_literal && ceps::ast::value(ceps::ast::as_int_ref(e)) != 0)
					{som.append(" "); som[som.length()-1] =  ceps::ast::value(ceps::ast::as_int_ref(e));}
			}
		}



		if (condition_defined && emit_defined){
		 DEBUG << "[PROCESSING_EVENT_TRIGGERED_SENDER][ev_id="
			   << ev_id << "]"
			   <<"[frame_id="
			   << frame_id
			   <<"]"
			   <<"[ip="
			   << ip
			   << "]"
			   <<  "[port="
			   << port
			   <<"][eom='"<<eof<<"']"
			   <<"][som='"<<som<<"']"
			   <<"\n";

		 auto it = frame_generators().find(frame_id);
		 if (it == frame_generators().end()) fatal_(-1,"sender declaration: Unknown frame with id '"+frame_id+"'");
		 auto gen = it->second;
		 event_triggered_sender_t descr;
		 descr.event_id_ = ev_id;
		 descr.frame_gen_ = gen;
		 descr.frame_id_ = frame_id;
		 descr.frame_queue_ = new frame_queue_t;

		 if (start_comm_threads()){
		  comm_threads.push_back(new std::thread{comm_sender_generic_tcp_out_thread,descr.frame_queue_,this,ip,port,som,eof,sock_name,reuse_sock,reg_socket });
		  running_as_node() = true;
		 }

		 event_triggered_sender().push_back(descr);
		} else if (!condition_defined && !emit_defined && channel_id.length()){
		 auto channel = new frame_queue_t;
         this->set_out_channel(channel_id,channel,"INET_STREAM");
		 if (start_comm_threads()){
	     running_as_node() = true;
		 comm_threads.push_back(
					 new std::thread{comm_sender_generic_tcp_out_thread,
				                     channel,
				                     this,
				                     ip,
				                     port,
				                     som,
				                     eof,
				                     sock_name,
				                     reuse_sock,
				                     reg_socket });
			 }
		} else {
            warn_(-1,"Sender definition incomplete, will be ignored.");
		}

	}
	//Handle CALL receiver
	for(auto receiver_ : all_receiver)
	{
	 DEBUG << "[PROCESSING RECEIVER][START]\n";
	 auto receiver = receiver_["receiver"];
	 auto when = receiver["when"];
	 auto emit = receiver["emit"];
	 auto transport  = receiver["transport"];
	 std::string port;
	 std::string ip;
	 bool reuse_sock=false;
	 bool reg_sock = false;
	 bool websocket_server = false;
	 bool websocket_client = false;
	 std::string receiver_id;
	 for(auto p: transport.nodes()){
      if (p->kind() == Ast_node_kind::identifier){
	   if(ceps::ast::name(ceps::ast::as_id_ref(p))=="websocket_server") websocket_server = true;
	   if(ceps::ast::name(ceps::ast::as_id_ref(p))=="websocket_client") websocket_client = true;
      }
	 }

	 std::string sock_name;
	 bool no_when_emit = false;

	 if (transport["generic_tcp_in"].empty() && transport["use"].empty()) {
	  //Handle user defined transport layers
	  std::string call_name = "(NULL)";
	  if (transport.nodes().size() && transport.nodes()[0]->kind() == ceps::ast::Ast_node_kind::structdef)
	   call_name = ceps::ast::name(ceps::ast::as_struct_ref(transport.nodes()[0]));
	   auto r = handle_userdefined_receiver_definition(call_name, receiver);
	   if (!r)
		fatal_(-1, "Receiver definition: '" + call_name + "' unknown CAL-identifier (Communication Abstraction Layer).\n");
	   continue;
	 }


		if (when.empty() && emit.empty()) no_when_emit = true;
		else if (when.size() != 1 || when.nodes()[0]->kind() != ceps::ast::Ast_node_kind::identifier)
            {warn_(-1,"Illformed receiver definition."); continue;}

		if (transport["generic_tcp_in"].empty() && transport["use"].empty())
            {warn_(-1,"Illformed receiver definition."); continue;}

		if (!transport["use"].empty()){
			if (transport["use"].nodes()[0]->kind() == ceps::ast::Ast_node_kind::identifier){
					reuse_sock = true;
					sock_name = ceps::ast::name(ceps::ast::as_id_ref(transport["use"].nodes()[0]));
			} else fatal_(-1,"Receiver definition illformed");
		} else
		{
			if (!transport["generic_tcp_in"]["port"].empty()) port = transport["generic_tcp_in"]["port"].as_str();
			if (!transport["generic_tcp_in"]["ip"].empty()) ip = transport["generic_tcp_in"]["ip"].as_str();
		}

		std::string eof;
		std::string som;


		if (receiver["id"].size()) {
			auto p = receiver["id"].nodes()[0];
			if (p->kind() != ceps::ast::Ast_node_kind::identifier) fatal_(-1,"Id field in receiver definition should contain a single id");
			receiver_id = ceps::ast::name(ceps::ast::as_id_ref(p));
			if (!reuse_sock){ sock_name = receiver_id; reg_sock = true; }
		}


		if (transport["eom"].size()) {
			//eof = transport["generic_tcp_in"]["eof"].as_str();
			auto  v = transport["eom"].nodes();
			//DOESN'T WORK!!!!!!: GCC BUG ??????: auto&  v = transport["generic_tcp_out"]["eof"].nodes();
			for(auto  e : v)
			{
				if (e->kind() == ceps::ast::Ast_node_kind::string_literal)
					eof.append(ceps::ast::value(ceps::ast::as_string_ref(e)));
				else if (e->kind() == ceps::ast::Ast_node_kind::int_literal && ceps::ast::value(ceps::ast::as_int_ref(e)) != 0)
					{eof.append(" "); eof[eof.length()-1] =  ceps::ast::value(ceps::ast::as_int_ref(e));}
			}
		}
		if (transport["som"].size()){
			auto  v = transport["som"].nodes();
			//DOESN'T WORK!!!!!!: GCC BUG ??????: auto&  v = transport["generic_tcp_out"]["eof"].nodes();
			for(auto  e : v)
			{
				if (e->kind() == ceps::ast::Ast_node_kind::string_literal)
					som.append(ceps::ast::value(ceps::ast::as_string_ref(e)));
				else if (e->kind() == ceps::ast::Ast_node_kind::int_literal && ceps::ast::value(ceps::ast::as_int_ref(e)) != 0)
					{som.append(" "); som[som.length()-1] =  ceps::ast::value(ceps::ast::as_int_ref(e));}
			}
		}

		std::string ev_id;
		std::vector<std::string> ev_params;

		if (emit.size() == 1 && emit.nodes()[0]->kind() == ceps::ast::Ast_node_kind::symbol && "Event" == ceps::ast::kind(ceps::ast::as_symbol_ref( emit.nodes()[0])) )
			ev_id = ceps::ast::name(ceps::ast::as_symbol_ref( emit.nodes()[0]));
		else if (emit.size() == 1 && emit.nodes()[0]->kind() == ceps::ast::Ast_node_kind::func_call)
		{
			std::vector<ceps::ast::Nodebase_ptr> args;
			read_func_call_values(this,	emit.nodes()[0],ev_id,args);
			for(auto p:args){
				if (p->kind() == ceps::ast::Ast_node_kind::identifier)
				{
					ev_params.push_back(ceps::ast::name(ceps::ast::as_id_ref(p)));
				}
			}
		}

		std::string frame_id;
		if (!when.nodes().empty()) frame_id = ceps::ast::name(ceps::ast::as_id_ref( when.nodes()[0]));
		if (!no_when_emit){
			DEBUG << "[PROCESSING EVENT_TRIGGERING_RECEIVER][ev_id="<< ev_id << "]"<<"[frame_id="<< frame_id <<"]" <<"[ip="<< ip << "]" <<  "[port=" << port <<"]" <<"\n";

			auto it = frame_generators().find(frame_id);

			if (it == frame_generators().end()) fatal_(-1,"receiver declaration: Unknown frame with id '"+frame_id+"'");

			auto gen = it->second;

			if (start_comm_threads()){
			 if (websocket_server || websocket_client){
			  int dispatcher_id=-1;
			  auto ctxt = allocate_dispatcher_thread_ctxt(dispatcher_id);
			  ctxt->websocket_server() = websocket_server;
			  ctxt->websocket_client() = websocket_client;
			  ctxt->id() = receiver_id;
			  comm_threads.push_back(new std::thread{comm_generic_tcp_in_dispatcher_thread,
													   dispatcher_id,
													   gen,
													   ev_id,
													   ev_params,
													   this,
													   ip,
													   port,
													   som,
													   eof,
													   sock_name,reg_sock,reuse_sock,
													   comm_generic_tcp_in_thread_fn });
			 } else comm_threads.push_back(new std::thread{comm_generic_tcp_in_dispatcher_thread,
												   -1,
												   gen,
												   ev_id,
												   ev_params,
												   this,
												   ip,
												   port,
												   som,
												   eof,
												   sock_name,reg_sock,reuse_sock,
												   comm_generic_tcp_in_thread_fn });
			running_as_node() = true;
			}
		} else {
			DEBUG << "[PROCESSING_UNCONDITIONED_RECEIVER]" <<"[ip="<< ip << "]" <<  "[port=" << port <<"]" <<"\n";
			auto handlers = receiver[all{"on_msg"}];
			if (handlers.empty()) fatal_(-1,"on_msg required in receiver definition.");

			int dispatcher_id=-1;
			auto ctxt = allocate_dispatcher_thread_ctxt(dispatcher_id);
			DEBUG << "[PROCESSING_UNCONDITIONED_RECEIVER]" <<"[dispatcher_id="<< dispatcher_id << "]\n";

			for(auto const & handler_ : handlers){
				auto const & handler = handler_["on_msg"];
				auto frame_id_ = handler["frame_id"];
				auto handler_func_ = handler["handler"];

				if (frame_id_.size() != 1 || frame_id_.nodes()[0]->kind() != ceps::ast::Ast_node_kind::identifier)
					fatal_(-1,"Receiver definition illformed: frame_id not an identifier / wrong number of arguments.");
				if (handler_func_.size() != 1 || handler_func_.nodes()[0]->kind() != ceps::ast::Ast_node_kind::identifier)
					fatal_(-1,"Receiver definition illformed: handler not an identifier / wrong number of arguments.");

				auto frame_id = ceps::ast::name(ceps::ast::as_id_ref(frame_id_.nodes()[0]));
				auto handler_id = ceps::ast::name(ceps::ast::as_id_ref(handler_func_.nodes()[0]));
				auto it_frame = frame_generators().find(frame_id);
				if (it_frame == frame_generators().end()) fatal_(-1,"Receiver definition: on_msg : frame_id unknown.");
				auto it_func = global_funcs().find(handler_id);
				if (it_func == global_funcs().end()) fatal_(-1,"Receiver definition: on_msg : function unknown.");
				ctxt->handler.push_back(std::make_pair(it_frame->second,it_func->second));
			}
			if (start_comm_threads()){
			 comm_threads.push_back(new std::thread{comm_generic_tcp_in_dispatcher_thread,
												   dispatcher_id,
												   nullptr,
												   "",
												   ev_params,
												   this,
												   ip,
												   port,
												   som,
												   eof,
												   sock_name,reg_sock,reuse_sock,
												   comm_generic_tcp_in_thread_fn });
			 running_as_node() = true;
			}

		}
	}

	for(auto p: exported_events.nodes()){
	 if (p->kind() != ceps::ast::Ast_node_kind::symbol || ceps::ast::kind(ceps::ast::as_symbol_ref(p)) != "Event"){
	   std::stringstream ss;
	   ss << *p;
	   fatal_(-1,"export section: '"+ss.str()+"' is not an event.");
	  }
	  exported_events_.insert(ceps::ast::name(ceps::ast::as_symbol_ref(p)));
	}

	//Handle lookup tables

	auto lookups = ns[all{"lookup"}];
	for(auto e : lookups){
		auto lkt = e["lookup"];
		if (lkt.nodes().size() == 0 || lkt.nodes()[0]->kind() != ceps::ast::Ast_node_kind::identifier)
			fatal_(-1,"First entry of lookup tables must be an identifier.");
		auto & tbl = lookup_tables()[ceps::ast::name(ceps::ast::as_id_ref(lkt.nodes()[0]))];
		for(auto i = 1; i < (int)lkt.nodes().size()-1;i+=2)
			tbl.push_back(std::make_pair(lkt.nodes()[i],lkt.nodes()[i+1]));
	}
	build_signal_structures(result_cmd_line);
	{
	 std::map<std::string,int> map_fullqualified_sm_id_to_computed_idx;
	 compute_state_and_event_ids(this,State_machine::statemachines,map_fullqualified_sm_id_to_computed_idx);
	 for(auto p : map_fullqualified_sm_id_to_computed_idx)
		 map_state_id_to_full_qualified_id[p.second] = p.first;
     for(auto e: executionloop_context().ev_to_id) executionloop_context().id_to_ev[e.second] = e.first;
     executionloop_context().id_to_ev[0] = "null";
	 for(auto e : map_fullqualified_sm_id_to_computed_idx){
		 executionloop_context().idx_to_state_id[e.second] = e.first;
	 }
	 executionloop_context().state_id_to_idx = std::move(map_fullqualified_sm_id_to_computed_idx);
	 for(auto const & e: this->exported_events_) executionloop_context().exported_events.insert(executionloop_context().ev_to_id[e]);

	 executionloop_context().shadow_transitions.resize(executionloop_context().transitions.size()); for(auto &i : executionloop_context().shadow_transitions) i = -1;
	 compute_shadow_transitions();
	 executionloop_context().init_coverage_structures();

     if (result_cmd_line.live_log) {
      enable_live_logging(result_cmd_line.live_log_port);
      livelog::Livelogger::Storage* idx2fqs = new livelog::Livelogger::Storage( executionloop_context().number_of_states*128);
      live_logger()->register_storage(sm4ceps::STORAGE_IDX2FQS,idx2fqs);
      std::map<int,std::string> m;
      for(auto const & v : executionloop_context().state_id_to_idx) m[v.second] = v.first;
      sm4ceps::storage_write(*idx2fqs,m,std::get<1>(live_logger()->find_storage_by_id(sm4ceps::STORAGE_IDX2FQS)->second ));
     }

	 if (result_cmd_line.print_transition_tables){
	  std::cout << "Full qualified state id => Index :\n";
	  if (executionloop_context().start_of_covering_states_valid())
		  std::cout << " There are states to be covered, start of covering state space = " << executionloop_context().start_of_covering_states << "\n";
	  for(auto e : executionloop_context().state_id_to_idx){
       std::cout <<" \"" << e.first <<"\" => " << "" << e.second << " ";
	   if (executionloop_context().get_inf(e.second,executionloop_context_t::SM))std::cout <<" compound_state";
	   if (executionloop_context().get_inf(e.second,executionloop_context_t::INIT))std::cout <<" initial_state";
       if (executionloop_context().get_inf(e.second,executionloop_context_t::DONT_COVER))std::cout <<" don't cover";
       if (executionloop_context().get_inf(e.second,executionloop_context_t::DONT_COVER_LOOPS))std::cout <<" don't cover loops";
       if (executionloop_context().get_inf(e.second,executionloop_context_t::HIDDEN))std::cout <<" hidden";
	   if (executionloop_context().get_inf(e.second,executionloop_context_t::FINAL))std::cout <<" final_state";
	   if (executionloop_context().get_inf(e.second,executionloop_context_t::THREAD))std::cout <<" thread";
	   if (executionloop_context().get_inf(e.second,executionloop_context_t::IN_THREAD))std::cout <<" in_thread";
	   if (executionloop_context().get_inf(e.second,executionloop_context_t::REGION))std::cout <<" orthogonal_regions";
	   if (executionloop_context().get_inf(e.second,executionloop_context_t::JOIN)){
		   std::cout <<" join="<< executionloop_context().get_join_state(e.second);
	   }
	   if (executionloop_context().get_inf(e.second,executionloop_context_t::EXIT))std::cout <<" on_exit";
	   if (executionloop_context().get_inf(e.second,executionloop_context_t::ENTER))std::cout <<" on_enter";
	   if (executionloop_context().shadow_state[e.second] >= 0) std::cout << " shadow_state = "<< executionloop_context().shadow_state[e.second];
       if (executionloop_context().get_inf(e.second,executionloop_context_t::LOG_ENTER_TIME))std::cout <<" log_enter_time";
       if (executionloop_context().get_inf(e.second,executionloop_context_t::LOG_EXIT_TIME))std::cout <<" log_exit_time";
       if (executionloop_context().get_inf(e.second,executionloop_context_t::HAS_LABEL))std::cout <<" label=\"" << escape_string(executionloop_context().state_labels[e.second])<<"\"";

	   std::cout << "\n";
	  }
	  std::cout << "event id => event name :\n";
	  for(auto e:executionloop_context().ev_to_id){
	   std::cout <<" " << e.second <<";" << "\"" << e.first;
 	   std::cout << "\";\n";
	  }
	  int j = -1;
	  std::cout << "(Transition Id) Index of State Machine (parent sm) :\n Index of Start State -> Index of Destination State/Event Index Info...\n";
	  if (executionloop_context().start_of_covering_transitions_valid())
		  std::cout << " There are transitions to be covered, start of covering transition space = " << executionloop_context().start_of_covering_transitions << "\n";
	  for(auto const & t : executionloop_context().transitions){
	   ++j;if (j == 0) continue;
	   std::cout << " ("<< j << ") ";
       std::cout << t.smp;
       std::cout << " (" << executionloop_context().parent_vec[t.smp] << ") : ";
       std::cout << t.from << "->"<<t.to<<" /"<<t.ev<<" g="<<t.guard << " a1= " << ((long long)t.a1) << " a2= "<< ((long long)t.a2);
       std::cout << " (root=" <<t.root_sms<<") ";
       if (t.props & executionloop_context_t::TRANS_PROP_ABSTRACT) std::cout << "(abstract)";
       if (executionloop_context().shadow_transitions[j] > 0) std::cout << " (shadow transition is " << executionloop_context().shadow_transitions[j] <<")";
       std::cout << std::endl;
	  }
	  std::cout << "Total number of states == " << executionloop_context().number_of_states <<" (option --print_transition_tables)\n\n";
	 }//print transition tables
   }

	if (result_cmd_line.print_statemachines){
		std::vector<State_machine*> smsv;for(auto sm : State_machine::statemachines) smsv.push_back(sm.second);
		std::cout << "\nStatemachines:\n";
		traverse_sms(smsv,[&](State_machine* sm){
			std::cout << " id: "<< get_qualified_id(sm).second << "\n";
			std::cout << " parent: " << (sm->parent() == nullptr ? "null" : get_qualified_id(sm).second ) <<"\n";
			std::cout << " states:\n";
			for(auto s:sm->states()){
				std::string parent="null";
				std::string smp="null";
				if (s->parent_) parent = get_qualified_id(s->parent_).second;
				if (s->smp_) smp = get_qualified_id(s->smp_).second;

				std::cout << " " << s->id_ << "(parent:"<<parent<< ",smp:"<<smp<<",idx:"<<s->idx_<<", shadowing:"<<s->shadow.valid() << ")\n";
			}
			std::cout << " transitions:\n";
			for(auto const & t : sm->transitions()){
				std::string parent1="null";
				std::string parent2="null";
				std::string smp1="null";
				std::string smp2="null";
				if (t.from_.parent_) parent1 = get_qualified_id(t.from_.parent_).second;
				if (t.to_.parent_) parent2 = get_qualified_id(t.to_.parent_).second;
				if (t.from_.smp_) smp1 = get_qualified_id(t.from_.smp_).second;
				if (t.to_.smp_) smp2 = get_qualified_id(t.to_.smp_).second;
				std::cout <<"  "<< t.from_.id_ <<"(parent:"<< parent1 << ", smp:"<<smp1<<") -> " << t.to_.id_ <<"(parent:"<<parent2 <<",smp:"<<smp2<<")";
				if (t.abstract) std::cout << " (abstract)";
				std::cout << "\n";
			}
			std::cout << "\n";
		});
	}

   //Handle event signatures
   {
	using ceps::ast::all;
    auto ev_sigs = ns[all{"event_signature"}];
    for(auto ev_sig_ : ev_sigs){
     auto ev_sig = ev_sig_["event_signature"];
     auto cur_ev_id = -1;
     State_machine_simulation_core::event_signature cur_sig;
     for(auto e : ev_sig.nodes()){
    	 std::string n;std::string k;
    	 if (is_id_or_symbol(e,n,k)){
    		 if (k != "Event"){
    			 std::stringstream ss;ss << *e;warn_(-1,"event_signature: illformed expression "+ss.str());continue;
    		 }
    		 auto new_ev_it = executionloop_context().ev_to_id.find(n);
    		 if (new_ev_it == executionloop_context().ev_to_id.end()){
    			 std::stringstream ss;ss << *e;warn_(-1,"event_signature: unknown event (?) "+ss.str());continue;
    		 }
    		 auto new_ev = new_ev_it->second;
    		 if (cur_ev_id >= 0) event_signatures()[cur_ev_id].push_back(cur_sig);
    		 cur_sig = {};
    		 cur_ev_id = new_ev;
    		 continue;
    	 }
    	 if (cur_ev_id < 0) continue;
    	 std::string fid;
    	 std::vector<ceps::ast::Nodebase_ptr> args;

    	 if (is_func_call(e, fid, args, 1)){
    		 auto arg = args[0];
    		 if (is_id_or_symbol(arg,n,k)){
    			 if(k.length() == 0 && n == "any")
    			  cur_sig.entries.push_back({fid,ceps::ast::Ast_node_kind::undefined});
    			 else if (k.length() == 0)
       			  cur_sig.entries.push_back({fid,ceps::ast::Ast_node_kind::identifier});
    			 else
    			  cur_sig.entries.push_back({fid,ceps::ast::Ast_node_kind::symbol});
    		 } else cur_sig.entries.push_back({fid,arg->kind()});
    	 } else {
			 std::stringstream ss;ss << *e;warn_(-1,"event_signature: illformed expression (expected something like 'myparam1(int);')  got "+ss.str());continue;
		 }
     }//processing of event signatures
     if (cur_ev_id >= 0) event_signatures()[cur_ev_id].push_back(cur_sig);//tailing signature
    }

    if (result_cmd_line.print_event_signatures){
  	 std::cout << "Event signatures : (option --print_event_signatures)\n";
     for (auto const & e : event_signatures() ){
      std::cout << "Event '"<< executionloop_context().id_to_ev[e.first] << "' (id="<<e.first<<") signature count is " << e.second.size() << "\n";
      auto ctr = 0;
      for(auto const & sig : e.second){
       std::cout << "\n Signature overload #"<< ++ctr << ":\n";
       for(auto const & entry : sig.entries){
    	   std::cout << "  "<<entry.arg_name <<" : ";
    	   if (entry.kind == ceps::ast::Ast_node_kind::undefined) std::cout << " ANY\n";
    	   else std::cout << ceps::ast::ast_node_kind_to_text[(int)entry.kind] << "\n";
       }
      }
     }
    }
   }





   // Check a native implementation is present for each action in the case the flag --enforce_native set.
   if(enforce_native()){
	 std::vector<State_machine*> smsv;
	 std::vector<std::string> r;
	 auto tt = this;

	 for(auto sm :State_machine::statemachines) smsv.push_back(sm.second);

	 traverse_sms(smsv,[&r,&tt](State_machine* cur_sm){
		for(auto it = cur_sm->actions().begin(); it != cur_sm->actions().end(); ++it) {
		 auto& act = *it;
		 if(act.body() != nullptr && act.native_func() == nullptr){
		  state_rep_t st;
		  st.is_sm_ = true;st.smp_=cur_sm;st.valid_=true;st.sid_=cur_sm->id();
		  r.push_back(tt->get_fullqualified_id(st)+"."+act.id());
		  }
		}
		for(auto const & t : cur_sm->transitions()){
		  for(auto it = t.actions().begin(); it != t.actions().end(); ++it) {
		    auto& act = *it;
		    if(act.body() != nullptr && act.native_func() == nullptr){
		      state_rep_t st;
		      st.is_sm_ = true;st.smp_=cur_sm;st.valid_=true;st.sid_=cur_sm->id();
		      r.push_back(tt->get_fullqualified_id(st)+"."+act.id()+" (action referred to in transition)");
		   }
		  }
		}
	    });

	 if(r.size()){
	  std::string s = "No native implementation for following state machines registered:\n";
	  for(auto & e:r){
	   s+= "  "; s+= e; s+="\n";
	  }
	  this->fatal_(-1,s);

	 }
	}
	if(generate_cpp_code()){
		do_generate_cpp_code(ceps_env_current(),current_universe(),global_guards,result_cmd_line);
	}
	if (result_cmd_line.dump_asciidoc_can_layer){
		sm4ceps::utils::dump_asciidoc_canlayer_doc(std::cout,this);
	}
    if (result_cmd_line.dump_stddoc_canlayer){
        if(result_cmd_line.out_path.length() == 0) sm4ceps::utils::dump_stddoc_canlayer_doc(result_cmd_line,std::cout,this);
        else {
            std::ofstream os{result_cmd_line.out_path};
            if (!os) fatal_(-1,"Failed to create '"+result_cmd_line.out_path+"' (output path)");
            sm4ceps::utils::dump_stddoc_canlayer_doc(result_cmd_line,os,this);
        }
    }

    if(result_cmd_line.vcan_api_on || ns["package"]["vcan_api_port"].as_str().length()){
        auto simcore_directory  = ns["package"][ceps::ast::all{"hub_directory"}];
        auto vcan_api_port = ns["package"]["vcan_api_port"].as_str();
        if (vcan_api_port.length())
            vcan_api() = new Virtual_can_interface(this,vcan_api_port);
        else vcan_api() = new Virtual_can_interface(this,result_cmd_line.vcan_api_port);
        if (simcore_directory.empty()){
            directory_of_known_simcores::simcore_info info;
            char buffer[1025] = {0};
            if (0 != gethostname(buffer,1024)) fatal_(-1,"gethostname() failed");
            info.host_name = buffer;
            info.port = vcan_api()->port();
            if (ns["package"]["name"].as_str().length() == 0) info.name = info.host_name+"_at_"+info.port;
            else info.name = ns["package"]["name"].as_str();
            if (ns["package"]["uri"].as_str().length() == 0) info.short_name = info.name;
            else info.short_name = ns["package"]["uri"].as_str();
            if (nullptr != ws_api())
                info.ws_api_port = ws_api()->port();
            info.role = "this_core";
            vcan_api()->known_simcores().entries.push_back(info);
            vcan_api()->reset_directory_of_known_simcores() = false;
        } else {
            vcan_api()->hub_directory() = simcore_directory;
        }
        vcan_api()->start();
        running_as_node() = true;
        auto vcan_api_directory = ns["package"]["directory_master"];
        if (!vcan_api_directory.empty()) {
            auto reg_name = ns["package"]["name"].as_str();
            auto reg_short_name = ns["package"]["uri"].as_str();
            auto reg_port = ns["package"]["vcan_api_port"].as_str();
            if (reg_port.length() == 0) reg_port = result_cmd_line.vcan_api_port;
            auto reg_host_name = ns["package"]["vcan_api_host_name"].as_str();
            auto dir_master_name = ns["package"]["directory_master"]["host_name"].as_str();
            auto dir_master_port = ns["package"]["directory_master"]["port"].as_str();
            auto sock = -1;
            for(;sock == -1;){
                sock = establish_inet_stream_connect(dir_master_name, dir_master_port);
                if (sock == -1) std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            std::vector<std::pair<std::string,std::string>> p;
            p.push_back(std::make_pair("name",reg_name));
            p.push_back(std::make_pair("short_name",reg_short_name));
            p.push_back(std::make_pair("host_name",reg_host_name));
            p.push_back(std::make_pair("port",reg_port));
            if (nullptr != ws_api())
              p.push_back(std::make_pair("ws_api_port",ws_api()->port()));
            auto r = send_cmd(sock,"register_sim_core",p);
            if (!std::get<0>(r)) this->fatal_(-1,"Failed to register to directory master.");
        }
    }


    if (result_cmd_line.dot_gen){
        do_generate_dot_code(ceps_env_current(),current_universe(),global_guards,result_cmd_line);
    }

    if (result_cmd_line.sleep_before_ws_api_on){
            auto delta = std::stoi(result_cmd_line.sleep_before_ws_delta_ms);
            std::this_thread::sleep_for(std::chrono::milliseconds(delta));
    }

    if(result_cmd_line.ws_api_on){
        ws_api() = new Websocket_interface(this,result_cmd_line.ws_api_port);
        ws_api()->start();
        running_as_node() = true;
    }
    if(result_cmd_line.start_paused){
        bool start_event_triggered = false;
        for(;!start_event_triggered;){
            main_event_queue().wait_for_data();
            std::unique_lock<std::mutex> lk(main_event_queue().data_mutex());
            std::queue<event_t> q;
            while (!main_event_queue().data().empty()) {
                q.push(main_event_queue().data().front());
                main_event_queue().data().pop();
            }
            lk.unlock();
            while(!q.empty()){
                auto eev = q.front();
                q.pop();
                if (eev.exec){
                    eev.exec->run(this);
                    eev.exec = nullptr;
                    delete eev.exec;
                } else if (eev.id_ == "@@start") start_event_triggered = true;
            }
        }
        run_simulations(this,result_cmd_line,ceps_env_current(),current_universe());
    } else run_simulations(this,result_cmd_line,ceps_env_current(),current_universe());

    if(result_cmd_line.post_processing_rel_paths.size() ){
    	std::string last_file_processed;
    	auto new_seg = process_files(	result_cmd_line.post_processing_rel_paths,last_file_processed);
    	for(auto p : new_seg){
    		auto pp = as_struct(p);
    		if (pp != nullptr && ( name(*pp) == "Statemachine" || name(*pp) == "statemachine" || name(*pp) == "sm" ) ) {
    		 Nodeset n(pp->children());
    		 process_statemachine(n,"",nullptr,1,0);
    		} else if (pp == nullptr) {
    		 Scope ac_seq;ac_seq.owns_children_ = false;ac_seq.children().push_back(p);
    		 execute_action_seq(nullptr,&ac_seq);
    		}
    	}
    }

	if (result_cmd_line.print_evaluated_postprocessing_tree){
		std::cout << ceps::ast::Nodebase::pretty_print << current_universe();
	}


}



bool State_machine_simulation_core::register_raw_frame_generator_gen_msg(std::string frame_id,char* (*fn)(size_t& )){
	 auto it = frame_generators().find(frame_id);
	 if (it == frame_generators().end()) fatal_(-1,"sender declaration: Unknown frame with id '"+frame_id+"'");
	 it->second->set_native_impl_gen_msg(fn);
	return true;
}


bool State_machine_simulation_core::register_raw_frame_generator_framectxt(std::string frame_id,sm4ceps_plugin_int::Framecontext* f){
	auto it = frame_generators().find(frame_id);
	if (it == frame_generators().end()) fatal_(-1,"sender declaration: Unknown frame with id '"+frame_id+"'");
	it->second->frame_ctxt = f;
	return true;
}
