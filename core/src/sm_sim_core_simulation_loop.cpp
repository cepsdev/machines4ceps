#include "core/include/state_machine_simulation_core.hpp"

#include "core/include/base_defs.hpp"


void State_machine_simulation_core::simulate(ceps::ast::Nodeset sim,states_t& states_in,ceps::Ceps_Environment& ceps_env,ceps::ast::Nodeset& universe)
{
	DEBUG_FUNC_PROLOGUE


	current_event().id_= {};
	assert_in_end_states_.clear();
	assert_not_in_end_states_.clear();


	states_t states;
	trans_hull_of_containment_rel(states_in,states);
	std::string testcase;
	std::string testname;

	auto const& testcase_ = sim["TestCase"];
	auto const& testname_ = sim["Test"];
	if (testcase_.size()==1 && testname_.size()==1 &&
		testcase_.nodes()[0]->kind() == ceps::ast::Ast_node_kind::identifier && testname_.nodes()[0]->kind() == ceps::ast::Ast_node_kind::identifier )
	{
		log() << "[TESTCASE]";
		log() << "[" << (testcase = name(ceps::ast::as_id_ref(testcase_.nodes()[0]))) <<"]\n";
		log() << "[TEST]";
	    log() << "[" << (testname = name(ceps::ast::as_id_ref(testname_.nodes()[0]))) <<"]\n";
	}


	auto fixture = universe[testcase];
	if (!fixture.empty())
	{
		log()<< "[FIXTURE(S) FOUND]\n";
		sim.nodes().insert(sim.nodes().begin(),fixture.nodes().begin(),fixture.nodes().end());
	}

	if (user_supplied_global_init() == nullptr){
		auto globals = universe["Globals"];
		if (!globals.empty())
		{
			sim.nodes().insert(sim.nodes().begin(),globals.nodes().begin(),globals.nodes().end());
		}
		globals = universe["globals"];
		if (!globals.empty())
		{
			sim.nodes().insert(sim.nodes().begin(),globals.nodes().begin(),globals.nodes().end());
		}
	}


	{
		std::lock_guard<std::recursive_mutex>g(states_mutex());
		global_systemstates_prev().insert(get_global_states().begin(),get_global_states().end());
	}


	request_start_for_all_dispatchers();


	log()<< "[SIMULATION STARTED]\n";	//log()<<"[START STATES] " ;	print_info(states);
        if (user_supplied_global_init() != nullptr){
                map_ceps_payload_to_native_=true;
                delete_ceps_payload_=true;

		user_supplied_global_init()();
        }

	int pos = 0;	int loop_ctr = 0;	bool quit = false;

	//Each simulation step starts with a phase where only epsilon transitions are taken until no further transitions are found,
	//followed by the consumption of an event.
	bool taking_epsilon_transitions = true;

	//start_processing_init_script(sim,pos,states);

	sm4ceps_plugin_int::Variant (*post_proc_native)() = nullptr;

	{
		auto it = glob_funcs().find("post_event_processing");
		if (it != glob_funcs().end()){
			post_proc_native = it->second;
		}
	}

	for(;!quit && !shutdown();)
	{
		bool ev_read = false;
		current_event().id_= {};
		if (print_debug_info_)log()<< "[SIMULATION_LOOP][" << ++loop_ctr << "]\n";

		event_rep_t ev;
		bool fetch_made_states_update = false;

		if (!taking_epsilon_transitions)
		{
		 if (step_handler_) quit = step_handler_();
		 std::vector<State_machine*> on_enter_seq;
		 if (!fetch_event(ev,sim,pos,states,fetch_made_states_update,on_enter_seq)) {
			 if (fetch_made_states_update)
			 {
			  if (on_enter_seq.size())
				{
				 for(auto const & sm : on_enter_seq){
				 	 //Handle on_enter
					 if (print_debug_info_)log() << "[ON_ENTER]"<<get_fullqualified_id(state_rep_t(true,true,sm,sm->id()) ) << "\n";
					 auto it = sm->find_action("on_enter");
					 if (it == nullptr) continue;
                                         if (it->native_func()){
                                           current_smp() = it->associated_sm_;
                                           it->native_func()();
                                         } else{
                                          if (it->body_ == nullptr) continue;
                                          execute_action_seq(sm,it->body());
                                         }
				 }//for
			   }

			   taking_epsilon_transitions = true;
			   continue;
			 } else if (!remove_states_.empty()){
			   taking_epsilon_transitions = true;
			   continue;
		     } else {
		    	 if (print_debug_info_)log()<< "[NO EVENT FOUND => COMPUTATION COMPLETE]\n";
			  break;
			 }
		 } else {
			 if (ev.sid_ == "@@queued_action")
			 {
				 if (ev.glob_func_ != nullptr){
					 ev.glob_func_();
				 } else {
				  ceps::ast::Scope scope;
				  scope.children() = ev.payload_;scope.owns_children() = false;
				  execute_action_seq(nullptr,&scope);
				  scope.children().clear();
				 }
				 continue;
			 }
			 ev_read = true;
		 }
		 current_event() = ev;
		 log()<< "[FETCHED_EVENT][" << ev.sid_ << "]\n";
		}


		bool ignore_ev = false;
		if (ev_read && not_transitional_events().find(ev.sid_) != not_transitional_events().end() ) ignore_ev = true;

		states_t states_without_transition;
		current_states() = states;
		call_states_visitors();
		if (active_states_logger()){
			std::lock_guard<std::recursive_mutex> g (this->active_states_logger_mutex_);
			std::vector<int> v;
			//for(auto & s: states){
				//if (s.is_sm_) v.push_back(s.smp_->idx_) else
			//}
			++active_states_logger_ctr_;
		}

		if (!ignore_ev) {
		 states_t triggered_kernel_states(states.begin(),states.end());
		 std::map<state_rep_t,state_rep_t> pred;
	 	 std::map<state_rep_t,std::vector<State_machine::Transition::Action>> associated_kernerl_states_action;
		 std::vector<State_machine::Transition::Action> on_enter_sm_derived_action_list;
		 std::set<state_rep_t> removed_states;

		 bool transitions_taken = compute_successor_states_kernel_under_event(ev,
													triggered_kernel_states,
													pred,
													states_without_transition,ceps_env,
													universe,
													associated_kernerl_states_action,
													remove_states_,
													removed_states
													);


		 if (!transitions_taken && taking_epsilon_transitions && removed_states.empty())
		 {
			 if (print_debug_info_)log() <<"[NO FURTHER EPSILON TRANSITIONS FOUND => FETCH EVENT]\n";
			remove_states_.clear();
			taking_epsilon_transitions = false;
			continue;
		 }
		 if (taking_epsilon_transitions)
		 {
			 if (print_debug_info_)log() <<"[EPSILON TRANSITIONS FOUND]\n";
		 }

		 if (print_debug_info_){DEBUG <<"[INITIAL KERNEL STATES]"; print_info(triggered_kernel_states);DEBUG <<"\n";}

		 std::set<State_machine*> sms_exited;
		 std::vector<State_machine*> on_exit_seq;
		 std::vector<State_machine*> on_enter_seq;

		 for(auto& st : removed_states){
			if (!st.is_sm_) continue;
			leave_sm(st.smp_,states,sms_exited,on_exit_seq);
		 }
		 remove_states_.clear();

		 for(auto const & s : triggered_kernel_states)
		 {
			bool leave = false;
			if(pred[s].is_sm_ /* && s.containing_sm() != s.smp_*/) leave = true;
			if(!pred[s].is_sm_ && s.containing_sm() != pred[s].containing_sm()) leave = true;
			if(!leave) continue;
			leave_sm(pred[s].smp_,states,sms_exited,on_exit_seq);
		 }

		 std::set<state_rep_t> new_states_set;
		 std::set<state_rep_t> new_states_triggered_set;

		 for(auto const & s : triggered_kernel_states)
		 {
			if (sms_exited.find(s.smp_) != sms_exited.end()) continue;
			new_states_set.insert(s);
			new_states_triggered_set.insert(s);
		 }
		 std::set<State_machine*> sms_entered;
		 for(auto const & s : states_without_transition)
		 {
			if (sms_exited.find(s.smp_) != sms_exited.end()) continue;
			new_states_set.insert(s);
			sms_entered.insert(s.smp_);
		 }
		 for(auto const & s : triggered_kernel_states)
		 {
			//if (sms_exited.find(s.smp_) != sms_exited.end()) continue;
			sms_entered.insert(pred[s].containing_sm());
		 }

		 if(print_debug_info_) { DEBUG <<  "[new_states_triggered_set (before enter_sm())]"; print_info(std::vector<state_rep_t>(new_states_triggered_set.begin(),new_states_triggered_set.end() )); DEBUG << "\n";}
		 if(print_debug_info_) { DEBUG << "[new_states_set (before enter_sm())]"; print_info(std::vector<state_rep_t>(new_states_set.begin(),new_states_set.end() )); DEBUG << "\n";}


		 for(auto const& s : new_states_triggered_set)
		 {
			//climb up until you reach an already entered sm (or null)

			auto top = s.smp_;

			std::vector<State_machine*> path_of_sms;
			for(;top->parent_;top = top->parent_)
			{
				if (sms_entered.find(top->parent_) != sms_entered.end()) break;
				path_of_sms.push_back(top->parent_);
			}
			for(auto rit = path_of_sms.rbegin(); rit != path_of_sms.rend();++rit )
				enter_sm(false,*rit,sms_entered,on_enter_seq,new_states_set,on_enter_sm_derived_action_list,states);

			assert(s.is_sm_ || s.smp_ != nullptr);
			enter_sm(!s.is_sm_,s.smp_,sms_entered,on_enter_seq,new_states_set,on_enter_sm_derived_action_list,states);
		 }

		if(print_debug_info_) { DEBUG << "[new_states_set (after enter_sm())]";
		print_info(std::vector<state_rep_t>(new_states_set.begin(),new_states_set.end() )); DEBUG << "\n";}



		auto new_states = states_t(new_states_set.begin(),new_states_set.end());

		if(print_debug_info_)print_info(states,new_states,new_states_triggered_set,std::set<state_rep_t>(  states_without_transition.begin(),states_without_transition.end()) );

		if(new_states_triggered_set.size() == 0) if(print_debug_info_)log() << "[NO TRANSITIONS]\n";
		if (on_exit_seq.size())
		{
			for(auto const & sm : on_exit_seq){
				log() << "[ON_EXIT]"<<get_fullqualified_id(state_rep_t(true,true,sm,sm->id()) ) <<"\n";
				auto it = sm->find_action("on_exit");
				if (it == nullptr) continue;
				if (it->native_func()){
				 current_smp() = it->associated_sm_;
                                 it->native_func()();
				 continue;
				}
				if (it->body_ == nullptr) continue;
				execute_action_seq(sm,it->body());
			}
		}

		for(auto const & s : new_states)
		{
			auto it = associated_kernerl_states_action.find(s);
			if (it == associated_kernerl_states_action.end()) continue;
			for(auto act_it = it->second.begin(); act_it != it->second.end();++act_it)
			{
				if (print_debug_info_)log() << "[EXECUTE ACTION][ID="<< act_it->id_ <<"]"  << "\n";
				if (act_it->native_func()){
					if (print_debug_info_)log() << "[EXECUTE NATIVE ACTION]\n";
					current_smp() = act_it->associated_sm_;
					act_it->native_func()();
					continue;
				}
				if (act_it->body() == nullptr) continue;
				execute_action_seq(act_it->associated_sm_,act_it->body());
			}
		}

/*
		if (on_enter_seq.size())
		{
			for(auto const & sm : on_enter_seq)
			{
				//Handle on_enter
				log() << "[ON_ENTER]"<<get_fullqualified_id(state_rep_t(true,true,sm,sm->id()) ) << "\n";
				auto it = sm->actions_.find(State_machine::Transition::Action("on_enter"));
				if (it == sm->actions_.end()) continue;
				if (it->body_ == nullptr) continue;
				execute_action_seq(sm,it->body());
			}
		}
*/
		 for(auto const & a : on_enter_sm_derived_action_list)
		 {
			 if (print_debug_info_)log() << "[EXECUTE ACTION:on_enter_sm_derived_action_list][ID="<< a.id_ <<"]"  << "\n";
			 if (a.native_func()){
			  current_smp() = a.associated_sm_;
			  a.native_func()();
			  continue;
			 }
			if (a.body() == nullptr) continue;
			execute_action_seq(a.associated_sm_,a.body());
		 }


		 update_asserts(states);update_asserts(new_states);

		 if(ev_read && ev.sid_ == "EXIT") quit = true;

		 states = new_states;
		}//if(!ignore_ev)

		if (ev_read){
			if (post_proc_native) post_proc_native();
			else if (!post_event_processing().empty()){
			 ceps::ast::Scope scope;
			 scope.owns_children() = false;
			 scope.children() = post_event_processing().nodes();
			 execute_action_seq(nullptr,&scope);
			}
		}

		//Call CALL

		if (ev_read) for(auto p: event_triggered_sender()){
			if (p.event_id_ != current_event().id_) continue;
			size_t data_size;
			char* data = (char*)p.frame_gen_->gen_msg(this,data_size);
			DEBUG << "[State_machine_simulation_core::simulate][PUSH_FRAME_TO_SENDER_QUEUE]\n";
			if (data != nullptr) p.frame_queue_->push(std::make_tuple(data,data_size,p.frame_gen_->header_length()));
		 }

		 if (ev_read && global_event_call_back_fn_ && is_export_event(current_event().id_)) {
                        //std::cout << ">>" << current_event().id_ << std::endl;
			if (current_event().payload_native_.size()){
                         for(auto & v : current_event().payload_native_){
                          if (v.what_ == sm4ceps_plugin_int::Variant::Int)
                            current_event().payload_.push_back(new ceps::ast::Int(v.iv_,ceps::ast::all_zero_unit(), nullptr, nullptr, nullptr));
                          else if (v.what_ == sm4ceps_plugin_int::Variant::Double)
                            current_event().payload_.push_back(new ceps::ast::Double(v.dv_,ceps::ast::all_zero_unit(), nullptr, nullptr, nullptr));
                          else if (v.what_ == sm4ceps_plugin_int::Variant::String)
                            current_event().payload_.push_back(new ceps::ast::String(v.sv_,nullptr, nullptr, nullptr));
			 }
                         global_event_call_back_fn_(current_event());
                         for(auto v : current_event().payload_) delete v;
                         current_event().payload_.clear();
			} else
			global_event_call_back_fn_(current_event());
		 }


		taking_epsilon_transitions = true;//Next loop starts with epsilon transitions
		{log() << "[ACTIVE_STATES] "; print_info(states);}
		if (logtrace()){
			clock_gettime(CLOCK_REALTIME, &log4kmw::get_value<0>(log4kmw_states::Timestamp));

			log4kmw::get_value<0>(log4kmw_states::Current_states).clear();
			for(auto & s : states){
				log4kmw::get_value<0>(log4kmw_states::Current_states).set(s.id());
			}
			log4kmw_loggers::log_event(log4kmw_events::Step(), log4kmw_loggers::logger_Trace);
		}
	}

	{log() << "[ACTIVE_STATES(After Termination)] "; print_info(states);}

	if (active_asserts_.size())
	{
		std::stringstream ss;
		bool not_satisfied_asserts_found = false;
		for(auto const & a : active_asserts_)
		{
			if (a.satisfied_) continue;
			not_satisfied_asserts_found = true;
			ss << "\nExpected to eventually reach state(s): { ";
			for(size_t j = 0; j < a.states_.size(); ++j)
			{
				ss << get_fullqualified_id(a.states_[j]);
				if(j < a.states_.size() - 1) ss << ",";
			}
			ss << " } .";
		}

		if (not_satisfied_asserts_found)
								fatal_(-1,"\nASSERTION(S) not satisfied (ASSERT_EVENTUALLY_VISIT_STATES): "+ss.str());
	}

	if (!assert_in_end_states_.empty()){
		for(auto const & st : assert_in_end_states_){
			bool bfound = false;
			for(auto const & st2 : states)
			{
				if (st2 == st){bfound=true;break;}
			}
			if (!bfound){
				std::stringstream ss;ss << "\nExpected to be finally in state:  ";ss  << get_fullqualified_id(st);
				fatal_(-1,"\nASSERTION(S) not satisfied (ASSERT_END_STATES_CONTAINS): "+ss.str());
			}
		}
	}

	if (!assert_not_in_end_states_.empty()){
			for(auto const & st : assert_not_in_end_states_){
				bool bfound = false;
				for(auto const & st2 : states)
				{
					if (st2 == st){bfound=true;break;}
				}
				if (bfound){
					std::stringstream ss;ss << "\nExpected NOT to be finally in state:  ";ss  << get_fullqualified_id(st);
					fatal_(-1,"\nASSERTION(S) not satisfied (ASSERT_END_STATES_CONTAINS_NOT): "+ss.str());
				}
			}
		}
	log()<< "[SIMULATION TERMINATED]\n\n";
	step_handler_ = nullptr;

}
