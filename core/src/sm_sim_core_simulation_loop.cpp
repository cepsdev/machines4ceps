#include "core/include/state_machine_simulation_core.hpp"

#include "core/include/base_defs.hpp"

#define DONT_PRINT_DEBUG
#define PRINT_LOG_SIM_LOOP


void executionloop_context_t::do_enter_impl(int sms,std::vector<int> const & v){
		if (get_inf(sms,executionloop_context_t::VISITED)) return;
		set_inf(sms,executionloop_context_t::VISITED,true);
		if (!v[sms]) return;

		for(auto i = state_to_children[sms]+1;children[i];++i){
			if (!is_sm(children[i])) continue;
			do_enter_impl(children[i],v);
		}
		if (!current_states[sms] && on_enter[sms]) on_enter[sms]();
	}
void executionloop_context_t::do_enter(int* sms,int n,std::vector<int> const & v){

		if (n){
			//std::cout << "do_enter() n="<<n<<" sms[0] == "<< sms[0] << "\n";
			for(int i = 0;i != number_of_states+1;++i) set_inf(i,executionloop_context_t::VISITED,false);
			for(int j = 0; j != n;++j) do_enter_impl(*(sms+j),v);
		}
	}


void executionloop_context_t::do_exit_impl(int sms,std::vector<int> const & v){
		if (get_inf(sms,executionloop_context_t::VISITED)) return;
		set_inf(sms,executionloop_context_t::VISITED,true);
		if (!current_states[sms]) return;

		for(auto i = state_to_children[sms]+1;children[i];++i){
			if (!is_sm(children[i])) continue;
			do_exit_impl(children[i],v);
		}
		if (on_exit[sms]) on_exit[sms]();
	}
void executionloop_context_t::do_exit(int* sms,int n,std::vector<int> const & v){

		if (n){
			//std::cout << "do_enter() n="<<n<<" sms[0] == "<< sms[0] << "\n";
			for(int i = 0;i != number_of_states+1;++i) set_inf(i,executionloop_context_t::VISITED,false);
			for(int j = 0; j != n;++j) do_exit_impl(*(sms+j),v);
		}
	}


void State_machine_simulation_core::simulate_purly_native(ceps::ast::Nodeset sim,
		                                     states_t& states_in,
		                                     ceps::Ceps_Environment& ceps_env,
		                                     ceps::ast::Nodeset& universe){



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


sm4ceps_plugin_int::Variant (*post_proc_native)() = nullptr;
{
	auto it = glob_funcs().find("post_event_processing");
	if (it != glob_funcs().end()){
		post_proc_native = it->second;
	}
}

for(auto const & s : states){
  executionloop_context().current_states.push_back(s.id_);
}

executionloop_context().current_states.resize(executionloop_context().number_of_states+1);
auto new_states = executionloop_context().current_states;
auto temp = executionloop_context().current_states;
auto entering_sms = executionloop_context().current_states;
auto exiting_sms = executionloop_context().current_states;

constexpr auto max_number_of_active_transitions = 1024;

std::vector<int> triggered_transitions;
triggered_transitions.resize(max_number_of_active_transitions);



auto & execution_ctxt = executionloop_context();

execution_ctxt.current_states_init_and_clear();

int cur_states_size = execution_ctxt.current_states.size();

for(;!quit && !shutdown();)
{

 bool ev_read = false;int ev_id = 0;
 //std::cout << current_event().id_ << std::endl;


 bool fetch_made_states_update = false;

 if (!taking_epsilon_transitions && !execution_ctxt.ev_sync_queue_empty())
 {
	ev_read = true;
	ev_id   = execution_ctxt.front_ev_sync_queue();
	execution_ctxt.pop_ev_sync_queue();
 } else if(!taking_epsilon_transitions) {
	 if (step_handler_) quit = step_handler_();
	 std::vector<State_machine*> on_enter_seq;
	 states_t new_states_fetch;
	 current_event().id_= {};
	 event_rep_t ev;
	 if (!fetch_event(ev,sim,pos,new_states_fetch,fetch_made_states_update,on_enter_seq)) {
		 if (fetch_made_states_update)
		 {
		  for(auto const & s : new_states_fetch){
			  execution_ctxt.current_states[s.id_] = 1;

		  }
		  if (on_enter_seq.size())
			{
			 for(auto const & sm : on_enter_seq){
			  //Handle on_enter
			  auto it = sm->find_action("on_enter");
			  if (it == nullptr) continue;
              if (it->native_func()){
                current_smp() = it->associated_sm_;
                it->native_func()();
              } else{
                if (it->body_ == nullptr) continue;
                if (enforce_native())
                  fatal_(-1,"Expecting native implementation (--enforce_native) (on_enter_1):"+it->id());
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
               #ifdef PRINT_LOG_SIM_LOOP
		    	 if (print_debug_info_)log()<< "[NO EVENT FOUND => COMPUTATION COMPLETE]\n";
               #endif
			  break;
			 }
	 } else {
      if (ev.sid_ == "@@queued_action"){
		if (ev.glob_func_ != nullptr){
		 ev.glob_func_(); }
		else {
           if (enforce_native())
            fatal_(-1,"Expecting native implementation (--enforce_native):@@queued_action");

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
	 if (ev_read) ev_id = execution_ctxt.ev_to_id[current_event().id_];
    }

	/*std::cout << "states = ";
	for(int z = 0; z != current_states_end; ++z){
	 if (!executionloop_context().current_states[z]) continue;
	 std::cout << z << " ";
	}
	std::cout << std::endl;*/



	//std::cout << "event_id=" <<ev_id << std::endl;
	//std::memset(triggered_transitions.data(),0,triggered_transitions.size()*sizeof(int));
	//std::memset(new_states.data(),0,new_states.size()*sizeof(int));
	memcpy(temp.data(),executionloop_context().current_states.data(),cur_states_size*4);


	//std::cout << "Computing triggered transitions.\n";
	std::cout << "ev_id == " << ev_id << std::endl;
	int triggered_transitions_end = 0;
	for (int s = 0; s != cur_states_size && max_number_of_active_transitions != triggered_transitions_end;++s){
		if (execution_ctxt.current_states[s] == 0) continue;


		int i = execution_ctxt.state_to_first_transition[s];
		int smp = execution_ctxt.transitions[i].smp;

		bool triggered = false;
		for(;i != execution_ctxt.transitions.size();++i){
			auto const & t = execution_ctxt.transitions[i];
			if (t.smp != smp) break;
			if (t.ev == ev_id && t.from == s)
				if (!t.guard || (t.guard && (*t.guard)()))  {
					triggered_transitions[triggered_transitions_end++]=i;triggered=true;
				}
		}
		if (!triggered) {temp[s] = 1;}
	}


	if (triggered_transitions_end == 0){
		std::cout << "No triggered transitions found!\n";
		taking_epsilon_transitions = !taking_epsilon_transitions;continue;
	}

	if(ev_read) taking_epsilon_transitions = true;

	std::sort(triggered_transitions.begin(),triggered_transitions.begin()+triggered_transitions_end);
	auto end_of_trans_it = unique( triggered_transitions.begin(), triggered_transitions.begin()+triggered_transitions_end );

	std::cout << "Triggered transitions:\n";
	for(auto p = triggered_transitions.begin();p != end_of_trans_it;++p ){
		auto t = *p;
		auto const & trans = execution_ctxt.transitions[t];
		std::cout << trans.from << " -> " << trans.to << std::endl;
	}

	bool possible_exit_or_enter_occured = false;

	for(auto p = triggered_transitions.begin();p != end_of_trans_it;++p ){
		auto t = *p;
		auto const & trans = execution_ctxt.transitions[t];

		if (!possible_exit_or_enter_occured && (execution_ctxt.get_parent(trans.to) != execution_ctxt.get_parent(trans.from)))
			possible_exit_or_enter_occured = true;
		if (!possible_exit_or_enter_occured && (execution_ctxt.is_sm(trans.to) || execution_ctxt.is_sm(trans.from))) possible_exit_or_enter_occured = true;
		temp[trans.from] = 0;temp[trans.to] = 1;
	}

	std::cout << "possible_exit_occured="<< possible_exit_or_enter_occured << std::endl;
	/*std::cout << "temp states (before enter)= ";
	for(int z = 0; z !=  temp.size(); ++z){
		 if (!temp[z]) continue;
		 std::cout << z << " ";
	}
	std::cout << std::endl;

	std::cout << "new states = ";
	for(int z = 0; z != new_states_end; ++z){
		 if (!new_states[z]) continue;
		 std::cout << z << " ";
	}
	std::cout << std::endl;*/

	if (!possible_exit_or_enter_occured){
		for(auto p = triggered_transitions.begin();p != end_of_trans_it;++p ){
			auto t = *p;
			auto const & trans = execution_ctxt.transitions[t];

			auto a1 = trans.a1;
			auto a2 = trans.a2;
			auto a3 = trans.a3;
			if (a1) a1();
			if (a2) a2();
			if (a3) a3();
		}
		memcpy(execution_ctxt.current_states.data(),temp.data(),cur_states_size*4);
	} else{

	 //Compute transitively entered states

		 std::cout << "temp states (before computation entered states)= ";
		 for(int z = 0; z != temp.size(); ++z){
			 if (!temp[z]) continue;
			 std::cout << z << " ";
		 }
		 std::cout << std::endl;

	 for(int i = 0; i != execution_ctxt.number_of_states+1;++i) {
		execution_ctxt.set_inf(i,executionloop_context_t::VISITED,false);
	 }

	 //std::memset(entering_sms.data(),0,entering_sms.size()*sizeof(int));
	 int entering_sms_next = 0;
	 for(auto state = 0; state != temp.size();++state ){
		 if (execution_ctxt.get_inf(state,executionloop_context_t::VISITED)) continue;
		 if(!temp[state] || execution_ctxt.current_states[state]) continue;
		 execution_ctxt.set_inf(state,executionloop_context_t::VISITED,true);

		 //Newly added state
		 if(execution_ctxt.is_sm(state)){
			temp[state] = 1;
			if (execution_ctxt.initial_state[state] != 0) {
				temp[execution_ctxt.initial_state[state]] = 1;
				execution_ctxt.set_inf(execution_ctxt.initial_state[state],executionloop_context_t::VISITED,true);
			}
		 } else {
		   if (execution_ctxt.current_states[execution_ctxt.get_parent(state)]) continue;
		   temp[execution_ctxt.current_states[execution_ctxt.get_parent(state)]] = 1;
		   execution_ctxt.set_inf(execution_ctxt.get_parent(state),executionloop_context_t::VISITED,true);
		 }
		 auto p = state;
		 for(;execution_ctxt.get_parent(p) != 0 && !execution_ctxt.current_states[execution_ctxt.get_parent(p)];){
			 p = execution_ctxt.get_parent(p);
			 temp[p] = 1;execution_ctxt.set_inf(p,executionloop_context_t::VISITED,true);
			 if (execution_ctxt.initial_state[p] != 0){
				 temp[execution_ctxt.initial_state[p]] = 1;
				 execution_ctxt.set_inf(execution_ctxt.initial_state[p],executionloop_context_t::VISITED,true);
			 }
		 }
		 entering_sms[entering_sms_next++] = p;
	 }

	 std::cout << "temp states (after computation entered states)= ";
	 for(int z = 0; z != temp.size(); ++z){
		 if (!temp[z]) continue;
		 std::cout << z << " ";
	 }
	 std::cout << std::endl;


	 //Compute transitively exit states

	 for(int i = 0; i != execution_ctxt.number_of_states+1;++i) {
		execution_ctxt.set_inf(i,executionloop_context_t::VISITED,false);
	 }

	 //std::memset(exiting_sms.data(),0,exiting_sms.size()*sizeof(int));
	 int exiting_sms_next = 0;
	 for(auto state = 0; state != temp.size();++state ){
		 if (!execution_ctxt.is_sm(state)) continue;
		 if (execution_ctxt.get_inf(state,executionloop_context_t::VISITED)) continue;
		 execution_ctxt.set_inf(state,executionloop_context_t::VISITED,true);

		 if(temp[state]){
		  if (!execution_ctxt.empty(state,temp)) continue;
		  std::cout << "!!!!" << state << "\n";
		  temp[state] = 0;
		 } else {
		   if (execution_ctxt.current_states[state]) execution_ctxt.remove_children(state,temp);
		   continue;
		 }

		 auto p = state;
		 for(;execution_ctxt.get_parent(p) != 0 && execution_ctxt.current_states[execution_ctxt.get_parent(p)];){
			 p = execution_ctxt.get_parent(p);
			 if (!execution_ctxt.empty(p,temp)) break;
			 temp[p] = 0;
		 }
		 exiting_sms[exiting_sms_next++] = p;
	 }


	 /*std::cout << "temp states (after enter)= ";
	 for(int z = 0; z != temp.size(); ++z){
		 if (!temp[z]) continue;
		 std::cout << z << " ";
	 }
	 std::cout << std::endl;*/
	 //Compute transitively exit states

	 execution_ctxt.do_exit(exiting_sms.data(),exiting_sms_next,temp);

	 for(auto p = triggered_transitions.begin();p != end_of_trans_it;++p ){
	 			auto t = *p;
	 			auto const & trans = execution_ctxt.transitions[t];

	 			auto a1 = trans.a1;
	 			auto a2 = trans.a2;
	 			auto a3 = trans.a3;
	 			if (a1) a1();
	 			if (a2) a2();
	 			if (a3) a3();
	 }

	 execution_ctxt.do_enter(entering_sms.data(),entering_sms_next,temp);
	 memcpy(execution_ctxt.current_states.data(),temp.data(),cur_states_size*4);
	}


	std::cout << "**** current states = ";
		for(int z = 0; z != execution_ctxt.current_states.size(); ++z){
			 if (!execution_ctxt.current_states[z]) continue;
			 std::cout << z << " ";
		}
	std::cout << std::endl;
	/*std::cout << "taking_epsilon_transitions == " << taking_epsilon_transitions << std::endl;
   */
 }
}


void State_machine_simulation_core::simulate(ceps::ast::Nodeset sim,
		                                     states_t& states_in,
		                                     ceps::Ceps_Environment& ceps_env,
		                                     ceps::ast::Nodeset& universe)
{
#ifdef PRINT_DEBUG
	DEBUG_FUNC_PROLOGUE
#endif

	current_event().id_= {};
	assert_in_end_states_.clear();
	assert_not_in_end_states_.clear();


	states_t states;
	trans_hull_of_containment_rel(states_in,states);
	//std::cout << "states_in.size()="<< states.size() << std::endl;
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
        #ifdef PRINT_LOG_SIM_LOOP
		 if (print_debug_info_)log()<< "[SIMULATION_LOOP][" << ++loop_ctr << "]\n";
        #endif

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
                     #ifdef PRINT_LOG_SIM_LOOP
					  if (print_debug_info_)log() << "[ON_ENTER]"<<get_fullqualified_id(state_rep_t(true,true,sm,sm->id(),sm->idx_) ) << "\n";
                     #endif
					 auto it = sm->find_action("on_enter");
					 if (it == nullptr) continue;
                                         if (it->native_func()){
                                           current_smp() = it->associated_sm_;
                                           it->native_func()();
                                         } else{
                                          if (it->body_ == nullptr) continue;
                                          if (enforce_native())
                                            fatal_(-1,"Expecting native implementation (--enforce_native) (on_enter_1):"+it->id());
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
               #ifdef PRINT_LOG_SIM_LOOP
		    	 if (print_debug_info_)log()<< "[NO EVENT FOUND => COMPUTATION COMPLETE]\n";
               #endif
			  break;
			 }
		 } else {
			 if (ev.sid_ == "@@queued_action")
			 {
				 if (ev.glob_func_ != nullptr){
					 ev.glob_func_();
				 } else {

                                  if (enforce_native())
                                    fatal_(-1,"Expecting native implementation (--enforce_native):@@queued_action");

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
         #ifdef PRINT_LOG_SIM_LOOP
		  log()<< "[FETCHED_EVENT][" << ev.sid_ << "]\n";
         #endif
		}


		bool ignore_ev = false;
		if (ev_read && not_transitional_events().find(ev.sid_) != not_transitional_events().end() ) ignore_ev = true;

		states_t states_without_transition;
		current_states() = states;
		call_states_visitors();
		if (active_states_logger()){
			std::lock_guard<std::recursive_mutex> g (this->active_states_logger_mutex_);
			std::vector<int> v;

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
             #ifdef PRINT_LOG_SIM_LOOP
			  if (print_debug_info_)log() <<"[EPSILON TRANSITIONS FOUND]\n";
             #endif
		 }
         #ifdef PRINT_DEBUG
		  if (print_debug_info_){DEBUG <<"[INITIAL KERNEL STATES]"; print_info(triggered_kernel_states);DEBUG <<"\n";}
         #endif
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
         #ifdef PRINT_DEBUG
		  if(print_debug_info_) { DEBUG <<  "[new_states_triggered_set (before enter_sm())]"; print_info(std::vector<state_rep_t>(new_states_triggered_set.begin(),new_states_triggered_set.end() )); DEBUG << "\n";}
		  if(print_debug_info_) { DEBUG << "[new_states_set (before enter_sm())]"; print_info(std::vector<state_rep_t>(new_states_set.begin(),new_states_set.end() )); DEBUG << "\n";}
         #endif

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

			enter_sm(!s.is_sm_,s.smp_,sms_entered,on_enter_seq,new_states_set,on_enter_sm_derived_action_list,states);
		 }
        #ifdef PRINT_DEBUG
		 if(print_debug_info_) { DEBUG << "[new_states_set (after enter_sm())]";
		 print_info(std::vector<state_rep_t>(new_states_set.begin(),new_states_set.end() )); DEBUG << "\n";}
        #endif


		auto new_states = states_t(new_states_set.begin(),new_states_set.end());
        #ifdef PRINT_LOG_SIM_LOOP
		 if(print_debug_info_)print_info(states,new_states,new_states_triggered_set,std::set<state_rep_t>(  states_without_transition.begin(),states_without_transition.end()) );
        #endif

        #ifdef PRINT_LOG_SIM_LOOP
		 if(new_states_triggered_set.size() == 0) if(print_debug_info_)log() << "[NO TRANSITIONS]\n";
        #endif

		if (on_exit_seq.size())
		{
			for(auto const & sm : on_exit_seq){
                #ifdef PRINT_LOG_SIM_LOOP
				 log() << "[ON_EXIT]"<<get_fullqualified_id(state_rep_t(true,true,sm,sm->id(),sm->idx_) ) <<"\n";
                #endif
				auto it = sm->find_action("on_exit");
				if (it == nullptr) continue;
				if (it->native_func()){
                                 current_smp() = sm;
                                 it->native_func()();
				 continue;
                                }
				if (it->body_ == nullptr) continue;
                                if (enforce_native())
                                 fatal_(-1,"Expecting native implementation (--enforce_native)(on_exit_1):"+it->id());

				execute_action_seq(sm,it->body());
			}
		}

		for(auto const & s : new_states)
		{
			auto it = associated_kernerl_states_action.find(s);
			if (it == associated_kernerl_states_action.end()) continue;
			for(auto act_it = it->second.begin(); act_it != it->second.end();++act_it)
			{
                #ifdef PRINT_LOG_SIM_LOOP
				 if (print_debug_info_)log() << "[EXECUTE ACTION][ID="<< act_it->id_ <<"]"  << "\n";
                #endif

				 if (act_it->native_func()){
                    #ifdef PRINT_LOG_SIM_LOOP
					 if (print_debug_info_)log() << "[EXECUTE NATIVE ACTION]\n";
                    #endif
					current_smp() = act_it->associated_sm_;
					act_it->native_func()();
					continue;
				}
				if (act_it->body() == nullptr) continue;
                                if ( enforce_native())
                                 fatal_(-1,"Expecting native implementation (--enforce_native)(act_1):"+act_it->id());

				execute_action_seq(act_it->associated_sm_,act_it->body());
			}
		}

		 for(auto const & a : on_enter_sm_derived_action_list)
		 {
             #ifdef PRINT_LOG_SIM_LOOP
			  if (print_debug_info_)log() << "[EXECUTE ACTION:on_enter_sm_derived_action_list][ID="<< a.id_ <<"]"  << "\n";
             #endif

			 if (a.native_func()){
			  current_smp() = a.associated_sm_;
			  a.native_func()();
			  continue;
			 }
			if (a.body() == nullptr) continue;
                        if (enforce_native())
                                 fatal_(-1,"Expecting native implementation (--enforce_native)(derived_1):"+a.id());

			execute_action_seq(a.associated_sm_,a.body());
		 }


		 if (!enforce_native())
		  {update_asserts(states);update_asserts(new_states);}

		 if(ev_read && ev.sid_ == "EXIT") quit = true;

		 states = new_states;
		}//if(!ignore_ev)

		if (ev_read){
			if (post_proc_native) post_proc_native();
			else if (!post_event_processing().empty()){
                         if (enforce_native())
                                 fatal_(-1,"Expecting native implementation (--enforce_native):post_processing");

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
            #ifdef PRINT_DEBUG
			 DEBUG << "[State_machine_simulation_core::simulate][PUSH_FRAME_TO_SENDER_QUEUE]\n";
            #endif
			if (data != nullptr) p.frame_queue_->push(std::make_tuple(data,data_size,p.frame_gen_->header_length()));
		 }

		 if (ev_read && global_event_call_back_fn_ && is_export_event(current_event().id_)) {
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
		{
            #ifdef PRINT_LOG_SIM_LOOP
			 log() << "[ACTIVE_STATES] "; print_info(states);
            #endif
		}
		if (logtrace()){
			clock_gettime(CLOCK_REALTIME, &log4kmw::get_value<0>(log4kmw_states::Timestamp));

			log4kmw::get_value<0>(log4kmw_states::Current_states).clear();
			for(auto & s : states){
				log4kmw::get_value<0>(log4kmw_states::Current_states).set(s.id());
			}
			log4kmw_loggers::log_event(log4kmw_events::Step(), log4kmw_loggers::logger_Trace);
		}
	}

    #ifdef PRINT_LOG_SIM_LOOP
	{log() << "[ACTIVE_STATES(After Termination)] "; print_info(states);}
    #endif

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
    #ifdef PRINT_LOG_SIM_LOOP
	 log()<< "[SIMULATION TERMINATED]\n\n";
    #endif
	step_handler_ = nullptr;
}
