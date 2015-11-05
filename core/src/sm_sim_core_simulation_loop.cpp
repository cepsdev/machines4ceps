#include "core/include/state_machine_simulation_core.hpp"

#include "core/include/base_defs.hpp"


void State_machine_simulation_core::simulate(ceps::ast::Nodeset sim,states_t& states_in,ceps::Ceps_Environment& ceps_env,ceps::ast::Nodeset& universe)
{
	DEBUG_FUNC_PROLOGUE


	current_event().id_= {};

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


	log()<< "[SIMULATION STARTED]\n";	log()<<"[START STATES] " ;	print_info(states);

	int pos = 0;	int loop_ctr = 0;	bool quit = false;

	//Each simulation step starts with a phase where only epsilon transitions are taken until no further transitions are found,
	//followed by the consumption of an event.
	bool taking_epsilon_transitions = true;

	//start_processing_init_script(sim,pos,states);

	for(;!quit && !shutdown();)
	{
		bool ev_read = false;
		current_event().id_= {};
		log()<< "[SIMULATION_LOOP][" << ++loop_ctr << "]\n";

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
					 log() << "[ON_ENTER]"<<get_fullqualified_id(state_rep_t(true,true,sm,sm->id()) ) << "\n";
					 auto it = sm->actions_.find(State_machine::Transition::Action("on_enter"));
					 if (it == sm->actions_.end()) continue;
					 if (it->body_ == nullptr) continue;
					 execute_action_seq(sm,it->body());
				 }//for
			   }

			   taking_epsilon_transitions = true;
			   continue;
			 } else {
			  log()<< "[NO EVENT FOUND => COMPUTATION COMPLETE]\n";
			  break;
			 }
		 } else {
			 if (ev.sid_ == "@@queued_action")
			 {
				 ceps::ast::Scope scope;
				 scope.children() = ev.payload_;scope.owns_children() = false;
				 execute_action_seq(nullptr,&scope);
				 scope.children().clear();
				 continue;
			 }
			 ev_read = true;
		 }
		 current_event() = ev;
		 log()<< "[FETCHED_EVENT][" << ev.sid_ << "]\n";
		}

		states_t states_without_transition;
		current_states() = states;
		states_t triggered_kernel_states(states.begin(),states.end());
		std::map<state_rep_t,state_rep_t> pred;
		std::map<state_rep_t,std::vector<State_machine::Transition::Action>> associated_kernerl_states_action;
		std::vector<State_machine::Transition::Action> on_enter_sm_derived_action_list;

		bool transitions_taken = compute_successor_states_kernel_under_event(ev,
													triggered_kernel_states,
													pred,
													states_without_transition,ceps_env,
													universe,
													associated_kernerl_states_action
													);
		if (!transitions_taken && taking_epsilon_transitions)
		{
			log() <<"[NO FURTHER EPSILON TRANSITIONS FOUND => FETCH EVENT]\n";
			taking_epsilon_transitions = false;
			continue;
		}
		if (taking_epsilon_transitions)
		{
			log() <<"[EPSILON TRANSITIONS FOUND]\n";
		}

		if (print_debug_info_){DEBUG <<"[INITIAL KERNEL STATES]"; print_info(triggered_kernel_states);DEBUG <<"\n";}

		//std::cerr<< "##########";print_info(states_without_transition);
		std::set<State_machine*> sms_exited;
		std::vector<State_machine*> on_exit_seq;
		std::vector<State_machine*> on_enter_seq;

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


		//std::cerr<< "########## new_states_set (before enter)=";
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


		//std::cerr<< "########## new_states_set (after enter)=";
		//print_info(std::vector<state_rep_t>(new_states_set.begin(),new_states_set.end() ));


		auto new_states = states_t(new_states_set.begin(),new_states_set.end());

		print_info(states,new_states,new_states_triggered_set,std::set<state_rep_t>(  states_without_transition.begin(),states_without_transition.end()) );

		if(new_states_triggered_set.size() == 0) log() << "[NO TRANSITIONS]\n";
		if (on_exit_seq.size())
		{
			for(auto const & sm : on_exit_seq){
				log() << "[ON_EXIT]"<<get_fullqualified_id(state_rep_t(true,true,sm,sm->id()) ) <<"\n";
				auto it = sm->actions_.find(State_machine::Transition::Action("on_exit"));
				if (it == sm->actions_.end()) continue;
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
				log() << "[EXECUTE ACTION][ID="<< act_it->id_ <<"]"  << "\n";
				if (act_it->body() == nullptr) continue;
				execute_action_seq(act_it->associated_sm_,act_it->body());
			}
		}

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

		for(auto const & a : on_enter_sm_derived_action_list)
		{
			log() << "[EXECUTE ACTION:on_enter_sm_derived_action_list][ID="<< a.id_ <<"]"  << "\n";
			if (a.body() == nullptr) continue;
			execute_action_seq(a.associated_sm_,a.body());
		}

		update_asserts(states);update_asserts(new_states);

		//Call CALL

		if (ev_read) for(auto p: event_triggered_sender()){
			if (p.event_id_ != current_event().id_) continue;
			size_t data_size;
			char* data = (char*)p.frame_gen_->gen_msg(this,data_size);

			/*std::cout << "*******************\n";
			std::cout << data_size <<"\n";
			std::cout << data <<"\n";

			std::cout << "*******************\n";*/

			DEBUG << "[State_machine_simulation_core::simulate][PUSH_FRAME_TO_SENDER_QUEUE]\n";
			if (data != nullptr) p.frame_queue_->push(std::make_pair(data,data_size));
		}

		if (ev_read && global_event_call_back_fn_) {
			global_event_call_back_fn_(current_event());
		}
		if(ev_read && ev.sid_ == "EXIT") quit = true;

		states = new_states;
		taking_epsilon_transitions = true;//Next loop starts with epsilon transitions
	}

	{log() << "[END STATES] "; print_info(states);}

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

	log()<< "[SIMULATION TERMINATED]\n\n";
	step_handler_ = nullptr;

}
