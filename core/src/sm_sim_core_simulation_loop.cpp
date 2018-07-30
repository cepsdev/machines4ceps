#include "core/include/state_machine_simulation_core.hpp"

#include "core/include/base_defs.hpp"
#include "core/include/sm_livelog_storage_utils.hpp"

#define DONT_PRINT_DEBUG
#define PRINT_LOG_SIM_LOOP

constexpr bool PRINT_DEBUG = true;

static void invariant(std::string const &){}

void executionloop_context_t::do_enter_impl(State_machine_simulation_core* smc,int sms,std::vector<executionloop_context_t::state_present_rep_t> const & v){
		if (get_inf(sms,executionloop_context_t::VISITED)) return;
		set_inf(sms,executionloop_context_t::VISITED,true);

		if (!v[sms]) return;

		for(auto i = state_to_children[sms]+1;children[i];++i){
			if (!is_sm(children[i])) continue;
			do_enter_impl(smc,children[i],v);
		}
		if (!current_states[sms] /*&& on_enter.size() > (size_t)sms && on_enter[sms]*/) {
			smc->current_smp() = get_assoc_sm(sms);
            //std::cout << "##### Calling on_enter B" << std::endl;
			if (on_enter.size() > (size_t)sms && on_enter[sms])
			 on_enter[sms]();
			else{
				auto it = smc->current_smp()->find_action("on_enter");
				if (it && it->body_ != nullptr)
				 smc->execute_action_seq(smc->current_smp(),it->body());
			}
		}
	}
void executionloop_context_t::do_enter(State_machine_simulation_core* smc,int* sms,int n,std::vector<executionloop_context_t::state_present_rep_t> const & v){

		if (n){
			for(int i = 0;i != number_of_states+1;++i) set_inf(i,executionloop_context_t::VISITED,false);
			for(int j = 0; j != n;++j) do_enter_impl(smc,*(sms+j),v);
		}
	}


void executionloop_context_t::do_exit_impl(State_machine_simulation_core* smc,int sms,std::vector<executionloop_context_t::state_present_rep_t> const & v){
		if (get_inf(sms,executionloop_context_t::VISITED)) return;
		set_inf(sms,executionloop_context_t::VISITED,true);
		if (!current_states[sms]) return;

		for(auto i = state_to_children[sms]+1;children[i];++i){
			if (!is_sm(children[i])) continue;
			do_exit_impl(smc,children[i],v);
		}
		if (on_exit.size() > (size_t)sms && on_exit[sms]) {
			smc->current_smp() = get_assoc_sm(sms);
			on_exit[sms]();
        } else {
            smc->current_smp() = get_assoc_sm(sms);
            auto it = smc->current_smp()->find_action("on_exit");
            if (it && it->body_ != nullptr)
             smc->execute_action_seq(smc->current_smp(),it->body());
        }
	}
void executionloop_context_t::do_exit(State_machine_simulation_core* smc,int* sms,int n,std::vector<executionloop_context_t::state_present_rep_t> const & v){

		if (n){
			//std::cout << "do_enter() n="<<n<<" sms[0] == "<< sms[0] << "\n";
			for(int i = 0;i != number_of_states+1;++i) set_inf(i,executionloop_context_t::VISITED,false);
			for(int j = 0; j != n;++j) do_exit_impl(smc,*(sms+j),v);
		}
	}



static void map_ev_payload_to_variant(State_machine_simulation_core::event_t const & ev,std::vector<sm4ceps_plugin_int::Variant>& r){
    if (ev.payload_native_.size()) {r = ev.payload_native_; return;}
    for(auto v : ev.payload_){
      if (v->kind() == ceps::ast::Ast_node_kind::int_literal)
         r.push_back(sm4ceps_plugin_int::Variant(value(as_int_ref(v))));
      else if (v->kind() == ceps::ast::Ast_node_kind::float_literal)
          r.push_back(sm4ceps_plugin_int::Variant(value(as_double_ref(v))));
      else if (v->kind() == ceps::ast::Ast_node_kind::string_literal)
          r.push_back(sm4ceps_plugin_int::Variant(value(as_string_ref(v))));
      }
}
static bool compute_event_signature_match(State_machine_simulation_core* smc,
		std::vector<State_machine_simulation_core::event_signature>& v,
		State_machine_simulation_core::event_signature** ev_sig,State_machine_simulation_core::event_t* ev);
static void print_event_signatures(std::ostream& os,State_machine_simulation_core* smc,std::vector<State_machine_simulation_core::event_signature>& v);

static void check_for_events(State_machine_simulation_core* smc,
		                     ceps::ast::Nodeset& sim,
							 std::size_t& pos,
							 bool& ev_read,
							 executionloop_context_t & execution_ctxt,
							 bool& taking_epsilon_transitions,
							 int& ev_id,
							 bool& quit,
							 bool& do_continue,
							 bool& do_break,
							 State_machine_simulation_core::event_signature** ev_sig){

	State_machine_simulation_core::states_t new_states_fetch;
	smc->current_event().id_= {};
	event_rep_t ev;
	std::vector<State_machine*> on_enter_seq;
	bool fetch_made_states_update = false;
	if (!smc->fetch_event(ev,sim,pos,new_states_fetch,fetch_made_states_update,on_enter_seq)) {
	 if (fetch_made_states_update){
      for(auto const & s : new_states_fetch){
		execution_ctxt.current_states[s.id_] = 1;
		execution_ctxt.visit_state(s.id_);
	  }
 	  if (on_enter_seq.size()){
	   for(auto const & sm : on_enter_seq){
		 //Handle on_enter
		 auto it = sm->find_action("on_enter");
		 if (it == nullptr) continue;
         if (it->native_func()){
          smc->current_smp() = it->associated_sm_;
          it->native_func()();
         } else{
          if (it->body_ == nullptr) continue;
          if (smc->enforce_native())
           smc->fatal_(-1,"Expecting native implementation (--enforce_native) (on_enter_1):"+it->id());
          smc->execute_action_seq(sm,it->body());
         }
	    }//for
	  }
	  taking_epsilon_transitions = true;
	  do_continue = true;return;
	 } else {
       do_break = true;return;
	 }
	} else {
      if (ev.sid_ == "@@queued_action"){
		if (ev.glob_func_ != nullptr){
		 ev.glob_func_(); }
		else {
           if (smc->enforce_native())
            smc->fatal_(-1,"Expecting native implementation (--enforce_native):@@queued_action");

		    ceps::ast::Scope scope;
			scope.children() = ev.payload_;scope.owns_children() = false;
			smc->execute_action_seq(nullptr,&scope);
			scope.children().clear();
		}
		do_continue = true;return;
	  } else if (ev.sid_ == "@@set_state"){
		  if (smc->enforce_native()){

		  } else {
			std::lock_guard<std::recursive_mutex>g(smc->states_mutex());
			std::string s = ev.payload_native_[0].sv_;
			auto w = smc->get_global_states()[s];
			smc->global_systemstates_prev()[s] = w;

			if (w == nullptr || w->kind() != ceps::ast::Ast_node_kind::float_literal)
				smc->get_global_states()[s] =
			 		new ceps::ast::Double(ev.payload_native_[1].dv_, ceps::ast::all_zero_unit(), nullptr, nullptr, nullptr);
			else {
				auto old_value = ceps::ast::value(ceps::ast::as_double_ref(smc->get_global_states()[s]));
				if (old_value != ev.payload_native_[1].dv_) smc->global_systemstates_prev()[s] = nullptr;
				ceps::ast::value(ceps::ast::as_double_ref(smc->get_global_states()[s])) = ev.payload_native_[1].dv_;
			}
		  }

		  taking_epsilon_transitions = true;
		  do_continue = true;return;
	  }
	  ev_read = true;
	 }
	smc->current_event() = ev;
	if (ev_read) {
		ev_id = execution_ctxt.ev_to_id[smc->current_event().id_];
		if (ev.sid_ == "EXIT" ) {quit = true;return;}
		if (ev_id <= 0) return;
		//Check signature
		auto sigs_it = smc->event_signatures().find(ev_id);
		if (sigs_it == smc->event_signatures().end()) return;
		if(!compute_event_signature_match (smc,sigs_it->second,ev_sig,&smc->current_event()) ){
		 std::stringstream ss; ss << smc->current_event().id_;
		 ss << "\n"; print_event_signatures(ss,smc,sigs_it->second);
		 ss << "Event " << smc->current_event().id_ << " is of type\n";
		 for(auto e : smc->current_event().payload_){
			 ss << "  " << ceps::ast::ast_node_kind_to_text[(int)e->kind()] << "\n";
		 }
		 smc->fatal_(-1,"No matching overload (event signature declaration) found for event-ctor "+ss.str());
		}
	}
}

static bool compute_event_signature_match(State_machine_simulation_core* smc,
		std::vector<State_machine_simulation_core::event_signature>& v,
		State_machine_simulation_core::event_signature** ev_sig, State_machine_simulation_core::event_t* ev){
 for(auto & e : v){
  if (e.entries.size() != ev->payload_.size()) continue;
  bool match_found = true;
  for (std::size_t i = 0; i != e.entries.size(); ++i ){
	  if (e.entries[i].kind == ceps::ast::Ast_node_kind::undefined) continue;
	  if (e.entries[i].kind != ev->payload_[i]->kind()){match_found = false; break;}
  }
  if (match_found){
	  *ev_sig = &e;
	  return true;
  }
 }
 return false;
}

static void print_event_signatures(std::ostream& os,State_machine_simulation_core* smc,std::vector<State_machine_simulation_core::event_signature>& v){
 int ctr = 0;
 for(auto & e : v){
	 os << "Event Signature Overload #" << ++ctr << ":\n";
	 for (std::size_t i = 0; i != e.entries.size(); ++i ){
		 os << "  "<< e.entries[i].arg_name << " : ";
		 if (e.entries[i].kind == ceps::ast::Ast_node_kind::undefined) os << "ANY";
		 else os << ceps::ast::ast_node_kind_to_text[(int)e.entries[i].kind];
		 os << "\n";
	 }
 }
}

static void log_triggered_transitions(State_machine_simulation_core* smc,
		                              executionloop_context_t & execution_ctxt,
									  std::vector<int> & triggered_transitions,std::vector<int>::iterator end_of_trans_it){
return;
if (smc->live_logger() || !smc->quiet_mode()){
   std::stringstream ss;
   ss << "Triggered Transitions: ";
   for(auto p = triggered_transitions.begin();p != end_of_trans_it;++p ){
		auto t = *p;
      auto const & trans = execution_ctxt.transitions[t];
      ss << "  " << execution_ctxt.idx_to_state_id[trans.from] <<" => " << execution_ctxt.idx_to_state_id[trans.to];
   }
   smc->info(ss.str());
  }
}


static void log_current_states(State_machine_simulation_core* smc){
if (smc->live_logger()){
      sm4ceps::livelog_write(*smc->live_logger(),smc->executionloop_context().current_states);
  } else if(!smc->quiet_mode() && smc->log_verbosity > 1){
	  smc->info("Active States: ",false);
  	for(size_t i = 0; i != smc->executionloop_context().current_states.size();++i)
  		if (smc->executionloop_context().current_states[i]) smc->info(smc->executionloop_context().idx_to_state_id[i]+" ",false );
  	smc->info("");
  }
}

static bool  handle_event_triggered_senders(State_machine_simulation_core* smc,executionloop_context_t & execution_ctxt,bool& ev_read){
  bool processed = false;
  if (ev_read && smc->event_triggered_sender().size() && smc->current_event().id_.length()) {
  	for(auto p: smc->event_triggered_sender()){
      if (p.event_id_ != smc->current_event().id_) continue;
  	  processed = true;
	  size_t data_size;
	  char* data = (char*) std::get<1>(p.frame_gen_->gen_msg(smc,data_size,{}));
	  if (data != nullptr) p.frame_queue_->push(std::make_tuple(Rawframe_generator::gen_msg_return_t{Rawframe_generator::IS_BINARY,(void*)data},data_size,p.frame_gen_->header_length(),0));
	  break;
	}
  }
  return processed;
}

static void log_event(State_machine_simulation_core* smc,executionloop_context_t & execution_ctxt, bool ev_read,int ev_id){
if ( (smc->live_logger() || !smc->quiet_mode()) && ev_read){
       std::vector<sm4ceps_plugin_int::Variant> v;
       map_ev_payload_to_variant(smc->current_event(),v);
       if (smc->live_logger()) smc->live_logger_out()->log_event(std::make_pair(execution_ctxt.id_to_ev[ev_id],v));
       else if (smc->log_verbosity > 1) {
      	 std::string p;
      	 if (v.size()){
      		 p.append("(");
      		 size_t j = 0;
      		 for (auto & e : v){
                if (e.what_ == sm4ceps_plugin_int::Variant::Int) p.append(std::to_string(e.iv_));
                else if (e.what_ == sm4ceps_plugin_int::Variant::Double) p.append(std::to_string(e.dv_));
                else p.append("'"+e.sv_+"'");
                if (j+1 != v.size()) p.append(", ");
                ++j;
      		 }
      		 p.append(")");
      	 }
      	smc->info("Event "+execution_ctxt.id_to_ev[ev_id]+p+" fetched.");
       }
  }
}

static void compute_triggered_transitions(State_machine_simulation_core* smc,ceps::Ceps_Environment& ceps_env,
		                                   executionloop_context_t & execution_ctxt,
										   executionloop_context_t::states_t & temp,
										   size_t max_number_of_active_transitions,
										   std::vector<int> & triggered_transitions,
										   int& triggered_transitions_end,
										   int cur_states_size,
										   int ev_id){
 for (int s = 0; s != cur_states_size && max_number_of_active_transitions != (size_t)triggered_transitions_end;++s){
   if (execution_ctxt.current_states[s] == 0) continue;
		ssize_t i = execution_ctxt.state_to_first_transition[s];
		int smp = execution_ctxt.transitions[i].smp;

		bool triggered = false;
		for(;(std::size_t)i != execution_ctxt.transitions.size();++i){
			auto const & t = execution_ctxt.transitions[i];
			if (t.props & executionloop_context_t::TRANS_PROP_ABSTRACT) continue;
			if (t.smp != smp) break;
			if (temp[t.to] && ev_id == 0 && t.ev == 0 && t.from == t.to && t.guard == nullptr && t.script_guard.length() == 0) continue;
			if (t.ev == ev_id && t.from == s){
                if (!t.script_guard.empty()){
					State_machine_simulation_core::states_t st;
                    bool r = smc->eval_guard(ceps_env,t.script_guard,st);
                    if (r){triggered_transitions[triggered_transitions_end++]=i;triggered=true;}
				} else if (t.guard == nullptr || (*t.guard)() )  {
					triggered_transitions[triggered_transitions_end++]=i;triggered=true;
				}
				if (triggered) execution_ctxt.visit_transition(i);
			}
		}
		if (!triggered) {temp[s] = 1;}
	}
}



static bool compute_transition_kernel(std::vector<int>& triggered_transitions,
		                              std::vector<int>::iterator end_of_trans_it,
									  executionloop_context_t & execution_ctxt,
									  executionloop_context_t::states_t & temp,
									  std::vector<executionloop_context_t::state_rep_t> & triggered_thread_regions,
									  int triggered_thread_regions_end
									  ){
 bool possible_exit_or_enter_occured = false;
 execution_ctxt.triggered_shadow_transitions.clear();
 for(auto i : triggered_transitions){
  auto st = i;
  for(st = execution_ctxt.shadow_transitions[st];st > 0;st = execution_ctxt.shadow_transitions[st])
   execution_ctxt.triggered_shadow_transitions.push_back(st);
 }

 auto handle_final_states = [&] (int to){
  if (execution_ctxt.get_inf(to,executionloop_context_t::IN_THREAD) && execution_ctxt.get_inf(to,executionloop_context_t::FINAL)){
   auto region = execution_ctxt.get_parent(execution_ctxt.get_parent(to));
   int i = 0;
   for(; i != triggered_thread_regions_end;++i) if (triggered_thread_regions[i] == region) break;
   if (i == triggered_thread_regions_end)
	triggered_thread_regions[triggered_thread_regions_end++] = region;
  }
 };

 auto process_transition = [&] (int t){
   auto const & trans = execution_ctxt.transitions[t];

   if (!possible_exit_or_enter_occured && (execution_ctxt.get_parent(trans.to) != execution_ctxt.get_parent(trans.from)))
    possible_exit_or_enter_occured = true;
   if (!possible_exit_or_enter_occured && (execution_ctxt.is_sm(trans.to) || execution_ctxt.is_sm(trans.from)))
	possible_exit_or_enter_occured = true;

   temp[trans.from] = 0;
   temp[trans.to] = 1;
   execution_ctxt.visit_state(trans.to);

   handle_final_states(trans.to);
  };

  for(auto p = triggered_transitions.begin();p != end_of_trans_it;++p ){
   auto ti = *p;
   auto const & t = execution_ctxt.transitions[ti];
   process_transition(ti);
   auto shadow_from = execution_ctxt.shadow_state[t.from];auto shadow_to = execution_ctxt.shadow_state[t.to];
   if (shadow_from <= 0 && shadow_to <= 0) continue;
   if (shadow_from <= 0 && shadow_to > 0){
	for(auto st = shadow_to; st > 0; st = execution_ctxt.shadow_state[st]) {
		temp[st] = 1;execution_ctxt.visit_state(st);handle_final_states(st);
		//if(!possible_exit_or_enter_occured && execution_ctxt.is_sm(st)) possible_exit_or_enter_occured = true;
	}
	possible_exit_or_enter_occured = true;
   } else if (shadow_from > 0 && shadow_to <= 0){

    for(auto st = shadow_from; st > 0; st = execution_ctxt.shadow_state[st]) temp[st] = 0;
    possible_exit_or_enter_occured = true;

   }
  }//for
  for (auto t : execution_ctxt.triggered_shadow_transitions){
   process_transition(t);execution_ctxt.visit_transition(t);
  }//for

  if (triggered_thread_regions_end){
   for(int i = 0; i != triggered_thread_regions_end; ++i){
    auto region = triggered_thread_regions[i];
	if (!execution_ctxt.get_inf(region,executionloop_context_t::JOIN)) continue;
	if (execution_ctxt.join_states[region] == 0) continue;
	bool all_threads_in_final = true;
	for(auto j = execution_ctxt.state_to_children[region]+1;execution_ctxt.children[j];++j){
	 int child;
	 if (!execution_ctxt.is_sm(child = execution_ctxt.children[j])) continue;
	 if (!execution_ctxt.get_inf(child,executionloop_context_t::THREAD)) continue;
	 if (execution_ctxt.final_state[child] == 0){all_threads_in_final = false; break;}
	 if (!temp[execution_ctxt.final_state[child]]){all_threads_in_final = false; break;}
	}//for
	if (all_threads_in_final){
	 for(auto j = execution_ctxt.state_to_children[region]+1;execution_ctxt.children[j];++j){
	  int child;
	  if (!execution_ctxt.is_sm(child = execution_ctxt.children[j])) continue;
	  if (!execution_ctxt.get_inf(child,executionloop_context_t::THREAD)) continue;
	  temp[child] = 0;
	 }
    possible_exit_or_enter_occured = true;
	temp[execution_ctxt.join_states[region]] = 1;
	execution_ctxt.visit_state(execution_ctxt.join_states[region]);
	}
   }//for
  }
  return possible_exit_or_enter_occured;
}

static void run_triggered_actions(State_machine_simulation_core* smc,
		                          std::vector<int>& triggered_transitions,
                                  std::vector<int>::iterator end_of_trans_it,
		                          executionloop_context_t & execution_ctxt){
 for(auto p = triggered_transitions.begin();p != end_of_trans_it;++p ){
   auto t = *p;
   auto const & trans = execution_ctxt.transitions[t];
   smc->current_smp() = execution_ctxt.get_assoc_sm(trans.smp);
   if(!trans.native) {
    if(trans.a1_script) smc->execute_action_seq(smc->current_smp(),(ceps::ast::Nodebase_ptr)trans.a1_script);
    if(trans.a2_script) smc->execute_action_seq(smc->current_smp(),(ceps::ast::Nodebase_ptr)trans.a2_script);
    if(trans.a3_script) smc->execute_action_seq(smc->current_smp(),(ceps::ast::Nodebase_ptr)trans.a3_script);
   } else {
    auto a1 = trans.a1;auto a2 = trans.a2; auto a3 = trans.a3;
    if (a1) a1();  if (a2) a2(); if (a3) a3();
   }
 }
}

static void log_state_changes(State_machine_simulation_core* smc,executionloop_context_t & execution_ctxt, executionloop_context_t::states_t & temp){
 if (!smc->quiet_mode() || smc->live_logger()){
  auto changes = false;
  for(size_t z = 0; z != execution_ctxt.current_states.size(); ++z){
	if (execution_ctxt.current_states[z] == temp[z]) continue;
	changes = true;break;
  }
  if (!changes) return;
  std::stringstream ss;
  //ss << "Set of states changed: ";
  for(size_t z = 0; z != execution_ctxt.current_states.size(); ++z){
   if (execution_ctxt.current_states[z] == temp[z]) continue;
   ss << execution_ctxt.idx_to_state_id[z];
   if (temp[z]) ss<< "+ "; else ss<<"- ";
  }
  smc->info(ss.str());
 }
}

static void compute_entered_states(State_machine_simulation_core* smc,
		                           executionloop_context_t & execution_ctxt,
								   executionloop_context_t::states_t & temp,
								   std::vector<executionloop_context_t::state_rep_t>& entering_sms,
								   int& entering_sms_next){
 for(int i = 0; i != execution_ctxt.number_of_states+1;++i) {
  execution_ctxt.set_inf(i,executionloop_context_t::VISITED,false);
 }

 for(std::size_t state = 0; state != temp.size();++state ){
  if (execution_ctxt.get_inf(state,executionloop_context_t::VISITED)) continue;
  if(!temp[state] || execution_ctxt.current_states[state]) continue;
  //invariant : temp[state] && !execution_ctxt.current_states[state]
  execution_ctxt.set_inf(state,executionloop_context_t::VISITED,true);
  //Newly added state
  if(execution_ctxt.is_sm(state)){
   temp[state] = 1;execution_ctxt.visit_state(state);
   int init_state = execution_ctxt.initial_state[state];
   if ( init_state ) {
	temp[init_state ] = 1;execution_ctxt.visit_state(init_state);
	execution_ctxt.set_inf(init_state ,executionloop_context_t::VISITED,true);
   }
   if (execution_ctxt.get_inf(state,executionloop_context_t::REGION)){
	for(auto ti =execution_ctxt.state_to_children[state]+1;execution_ctxt.children[ti];++ti){
	 int thread;
	 if (!execution_ctxt.is_sm(thread = execution_ctxt.children[ti])) continue;
	 if (!execution_ctxt.get_inf(thread,executionloop_context_t::THREAD)) continue;
	 temp[thread] = 1;execution_ctxt.visit_state(thread);
	 execution_ctxt.set_inf(thread,executionloop_context_t::VISITED,true);
	 if(execution_ctxt.initial_state[thread] != 0){
	  temp[execution_ctxt.initial_state[thread]] = 1;execution_ctxt.visit_state(execution_ctxt.initial_state[thread]);
	  execution_ctxt.set_inf(execution_ctxt.initial_state[thread],executionloop_context_t::VISITED,true);
	 }
	}//for
   }
  } else {
   auto prnt = execution_ctxt.get_parent(state);
   if (execution_ctxt.current_states[prnt]) continue;
   temp[prnt] = 1;execution_ctxt.visit_state(prnt);
   execution_ctxt.set_inf(prnt,executionloop_context_t::VISITED,true);
  }
  auto p = state;
  for(;;){
   auto prnt = execution_ctxt.get_parent(p);
   if (prnt == 0 || execution_ctxt.current_states[prnt] == 1) break;
   p = prnt;
   temp[p] = 1;execution_ctxt.visit_state(p);execution_ctxt.set_inf(p,executionloop_context_t::VISITED,true);
   if (execution_ctxt.initial_state[p] != 0){
    temp[execution_ctxt.initial_state[p]] = 1;execution_ctxt.visit_state(execution_ctxt.initial_state[p]);
	execution_ctxt.set_inf(execution_ctxt.initial_state[p],executionloop_context_t::VISITED,true);
   }
  }//for
  if(execution_ctxt.is_sm(p)) entering_sms[entering_sms_next++] = p;
 }
}

static void compute_exit_states(State_machine_simulation_core* smc,
        executionloop_context_t & execution_ctxt,
		executionloop_context_t::states_t & temp,
		std::vector<executionloop_context_t::state_rep_t>& exiting_sms,
		int& exiting_sms_next){
 for(int i = 0; i != execution_ctxt.number_of_states+1;++i) {
  execution_ctxt.set_inf(i,executionloop_context_t::VISITED,false);
 }
 for(std::size_t state = 0; state != temp.size();++state ){
  if (!execution_ctxt.is_sm(state)) continue;
  if (execution_ctxt.get_inf(state,executionloop_context_t::VISITED)) continue;
  execution_ctxt.set_inf(state,executionloop_context_t::VISITED,true);

  if(temp[state]){
   if (!execution_ctxt.empty(state,temp)) continue;
   temp[state] = 0;
  } else {
   if (execution_ctxt.current_states[state]) {
    execution_ctxt.remove_children(state,temp);
   } else continue;
  }
  auto p = state;
  for(;execution_ctxt.get_parent(p) != 0 && execution_ctxt.current_states[execution_ctxt.get_parent(p)];){
   auto pp = execution_ctxt.get_parent(p);
   if (!execution_ctxt.empty(pp,temp)) break;
   temp[p = pp] = 0;
  }//for
  exiting_sms[exiting_sms_next++] = p;
 }
}

static bool is_a_simulation_directive(ceps::ast::Nodebase_ptr n){
 if (n->kind() == ceps::ast::Ast_node_kind::structdef){
	 auto& nn = ceps::ast::as_struct_ref(n);
	 auto nm = ceps::ast::name(nn);
	 if (nm == "Start") return true;
	 if (nm == "start") return true;
     if (nm == "ASSERT_EVENTUALLY_VISIT_STATES") return true;
     if (nm == "ASSERT_CURRENT_STATES_CONTAINS") return true;
     if (nm == "ASSERT_CURRENT_STATES_CONTAIN") return true;
     if (nm == "ASSERT_CURRENT_STATES_CONTAINS_NOT") return true;
     if (nm == "ASSERT_CURRENT_STATES_CONTAIN_NOT") return true;
     if (nm == "ASSERT_END_STATES_CONTAINS") return true;
     if (nm == "ASSERT_END_STATES_CONTAIN") return true;
     if (nm == "ASSERT_END_STATES_CONTAINS_NOT") return true;
     if (nm == "ASSERT_END_STATES_CONTAIN_NOT") return true;
 }
 return false;
}

void State_machine_simulation_core::run_simulation(ceps::ast::Nodeset sim,
		                                     states_t& states_in,
		                                     ceps::Ceps_Environment& ceps_env,
		                                     ceps::ast::Nodeset& universe){

 auto & execution_ctxt = executionloop_context();
 int ev_id;
 event_t dummy_ev;
 event_t dummy_ev2;

 current_event().id_= {};
 assert_in_end_states_.clear();
 assert_not_in_end_states_.clear();
 std::vector<ceps::ast::Int*> callback_ceps_int_vec;
  for(int i = 0; i != 16;++i)callback_ceps_int_vec.push_back(new ceps::ast::Int(0,ceps::ast::all_zero_unit(), nullptr, nullptr, nullptr));
 std::vector<ceps::ast::Double*> callback_ceps_double_vec;
  for(int i = 0; i != 16;++i)callback_ceps_double_vec.push_back(new ceps::ast::Double(0,ceps::ast::all_zero_unit(), nullptr, nullptr, nullptr));
 std::vector<ceps::ast::String*> callback_ceps_str_vec;
  for(int i = 0; i != 16;++i)callback_ceps_str_vec.push_back(new ceps::ast::String("", nullptr, nullptr, nullptr));


 auto global_ev_cllbck = [this,&ev_id,&dummy_ev,&dummy_ev2,&execution_ctxt,&callback_ceps_int_vec,&callback_ceps_double_vec,&callback_ceps_str_vec](){
	if (execution_ctxt.current_ev_id == ev_id){
		dummy_ev.id_ = execution_ctxt.id_to_ev[ev_id];
		global_event_call_back_fn_(dummy_ev);
	} else {
	  dummy_ev2.payload_.clear();
	  dummy_ev2.id_ = execution_ctxt.id_to_ev[ev_id];

      if (current_event().payload_native_.size()){
    	int iint=0,idouble=0,istr=0,imax=16;

	    for(auto & v : current_event().payload_native_){
		  if (v.what_ == sm4ceps_plugin_int::Variant::Int && iint < imax){
		   ceps::ast::value(*callback_ceps_int_vec[iint]) = v.iv_;
		   dummy_ev2.payload_.push_back(callback_ceps_int_vec[iint++]);
		  } else if (v.what_ == sm4ceps_plugin_int::Variant::Double){
		   ceps::ast::value(*callback_ceps_double_vec[idouble]) = v.dv_;
		   dummy_ev2.payload_.push_back(callback_ceps_double_vec[idouble++]);
		  }else if (v.what_ == sm4ceps_plugin_int::Variant::String){
		   ceps::ast::value(*callback_ceps_str_vec[istr]) = v.sv_;
		   dummy_ev2.payload_.push_back(callback_ceps_str_vec[istr++]);
		  }
	  }//for
	}
    global_event_call_back_fn_(dummy_ev2);
  }
 };

 states_t states;

 std::string testcase;
 std::string testname;

 auto const& testcase_ = sim["TestCase"];
 auto const& testname_ = sim["Test"];
 if (testcase_.size()==1 && testname_.size()==1 &&
	testcase_.nodes()[0]->kind() == ceps::ast::Ast_node_kind::identifier && testname_.nodes()[0]->kind() == ceps::ast::Ast_node_kind::identifier )
 {
    info("Running test '"+ ( testname = name(ceps::ast::as_id_ref(testname_.nodes()[0]) ))  +"' of test case '"+ (testcase = name(ceps::ast::as_id_ref(testcase_.nodes()[0]))) + "'" );
 }

 auto fixture = universe[testcase];
 if (!fixture.empty())
 {
    info("Fixture(s) loaded");
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


 //info("Simulation started.");
 if (user_supplied_global_init() != nullptr){
      map_ceps_payload_to_native_=true;
      delete_ceps_payload_=true;
	user_supplied_global_init()();
 }

 std::size_t pos = 0;bool quit = false;
 //Execute possible initializations
 for(;pos!=sim.size();++pos){
  auto node_raw = sim.nodes()[pos];
  if (is_a_simulation_directive(node_raw)) break;
  bool executed = false;
  if (is_assignment_op(node_raw)) {
   auto & node = as_binop_ref(node_raw);
   std::string state_id;
   if (is_assignment_to_guard(node)){
    eval_guard_assign(node);
   }
   else if (is_assignment_to_state(node,state_id)){
    eval_state_assign(node,state_id);
   } else {
	std::stringstream ss;
	ss << *node_raw;
	fatal_(-1,"Unsupported assignment:"+ss.str());
   }
   executed = true;
  } else {
   auto cev = resolve_event_qualified_id(sim.nodes()[pos],nullptr);
   if ( (!cev.valid() || cev.sid_.length() == 0) ) {//No Event read
	executed = true;
    ceps::ast::Scope scope(node_raw);scope.owns_children() = false;
	execute_action_seq(nullptr,&scope);
   }
  }
  if(!executed)break;
 }//for

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

 execution_ctxt.current_states_init_and_clear();

 auto temp = executionloop_context().current_states;

 std::vector<executionloop_context_t::state_rep_t> entering_sms;entering_sms.resize( executionloop_context().current_states.size());
 std::vector<executionloop_context_t::state_rep_t> exiting_sms;exiting_sms.resize( executionloop_context().current_states.size());
 std::vector<executionloop_context_t::state_rep_t> triggered_thread_regions;triggered_thread_regions.resize( executionloop_context().current_states.size());

 constexpr auto max_number_of_active_transitions = 4096;

 std::vector<int> triggered_transitions;
 triggered_transitions.resize(max_number_of_active_transitions);
 int cur_states_size = execution_ctxt.current_states.size();
 taking_epsilon_transitions = false;

 if(execution_ctxt.start_of_covering_states_valid()){
	 bool changed = false;
	 for(int i = execution_ctxt.start_of_covering_states; i != execution_ctxt.number_of_states;++i){
      if (!execution_ctxt.get_inf(i,executionloop_context_t::SM)) continue;
      if (execution_ctxt.get_parent(i) != 0) continue;
      if (!changed && temp[i] == 0) changed = true;
      temp[i] = 1;
	 }
	 if (changed){
	  log_current_states(this);
      int entering_sms_next = 0; int exiting_sms_next = 0;
	  compute_entered_states(this,execution_ctxt,temp,entering_sms,entering_sms_next);
	  compute_exit_states(this,execution_ctxt,temp,exiting_sms,exiting_sms_next);
	  execution_ctxt.do_exit(this,exiting_sms.data(),exiting_sms_next,temp);
	  execution_ctxt.do_enter(this,entering_sms.data(),entering_sms_next,temp);
	  log_state_changes(this,execution_ctxt, temp);
	  memcpy(execution_ctxt.current_states.data(),temp.data(),cur_states_size*sizeof(executionloop_context_t::state_present_rep_t));
	  taking_epsilon_transitions = true;
	 }
 }

 for(;!quit && !shutdown();)
 {
  bool ev_read = false;
  ev_id = 0;
  current_smp() = nullptr;
  execution_ctxt.current_ev_id = 0;
  State_machine_simulation_core::event_signature* ev_sig = nullptr;

  if (!taking_epsilon_transitions)
  {
	if (execution_ctxt.ev_sync_queue_empty()){
	 bool do_continue = false;	 bool do_break = false;
	 check_for_events(this,sim,pos,ev_read,execution_ctxt,taking_epsilon_transitions,ev_id,quit,do_continue,do_break,&ev_sig);
	 if (do_continue) continue;	 if (do_break) break;
	} else {
	 ev_read = true;ev_id   = execution_ctxt.front_ev_sync_queue();
	 execution_ctxt.pop_ev_sync_queue(); execution_ctxt.current_ev_id = ev_id;
	}
  }


  auto restart_event = ev_read && current_event().id_=="@@restart_state_machine";
  //auto sm_to_be_restarted = restart_event ? current_event().payload_native_[0].iv_ : 0;

  log_current_states(this);
  handle_event_triggered_senders(this,execution_ctxt,ev_read);

  memcpy(temp.data(),executionloop_context().current_states.data(),cur_states_size*sizeof(executionloop_context_t::state_present_rep_t));
  log_event(this,execution_ctxt,ev_read,ev_id);
  int triggered_transitions_end = 0;
  auto triggered_thread_regions_end = triggered_transitions_end;
  compute_triggered_transitions(this,ceps_env,execution_ctxt,temp,
								max_number_of_active_transitions,triggered_transitions,
								triggered_transitions_end,cur_states_size,ev_id);
  bool no_transitions_triggered = triggered_transitions_end == 0;
  if (no_transitions_triggered && !restart_event){
	if (this->current_event().error_ != nullptr){
		info("Unhandled error:"+current_event().error_->what_ + "["+std::to_string(current_event().error_->errno_)+"]");
		break;
	}
	if (taking_epsilon_transitions) taking_epsilon_transitions = false;
	else taking_epsilon_transitions = true;
    if (ev_read && post_proc_native) post_proc_native();
	if (global_event_call_back_fn_!=nullptr && ev_id != 0 && execution_ctxt.exported_events.find(ev_id) != execution_ctxt.exported_events.end())
			global_ev_cllbck();
	continue;
  }
  invariant("at least one transition or the restart of a sm was triggered");
  taking_epsilon_transitions = true;

  std::sort(triggered_transitions.begin(),triggered_transitions.begin()+triggered_transitions_end);
  auto end_of_trans_it = unique( triggered_transitions.begin(), triggered_transitions.begin()+triggered_transitions_end );
  if (ev_id > 0 && ev_sig){
	//update scopes
	for(auto it = triggered_transitions.begin();it != end_of_trans_it;++it)
	{
     auto sms = execution_ctxt.get_assoc_sm(execution_ctxt.transitions[*it].root_sms);
     sms->visited_flag = false;
	}
	for(auto it = triggered_transitions.begin();it != end_of_trans_it;++it)
	{
     auto sms = execution_ctxt.get_assoc_sm(execution_ctxt.transitions[*it].root_sms);
     if (sms->visited_flag) continue;  sms->visited_flag = true;
     if (!sms->global_scope) { sms->global_scope = std::make_shared<ceps::parser_env::Scope>();	}
     auto & scope = *sms->global_scope;
     for (std::size_t i = 0; i!= ev_sig->entries.size();++i){
    	 auto var_name = ev_sig->entries[i].arg_name;
    	 auto val = this->current_event().payload_[i];
    	 auto sym = scope.insert(var_name);
    	 sym->category = ceps::parser_env::Symbol::Category::VAR;
    	 sym->payload = val;
     }
	}
  }

  log_triggered_transitions(this,execution_ctxt,triggered_transitions,end_of_trans_it);
  bool possible_exit_or_enter_occured = restart_event || compute_transition_kernel(triggered_transitions,
          end_of_trans_it,
		  execution_ctxt,
		  temp,
		  triggered_thread_regions,
		  triggered_thread_regions_end
		  );

 if (!possible_exit_or_enter_occured){
  invariant("No state machine was entered nor left.");
  run_triggered_actions(this,triggered_transitions,end_of_trans_it,execution_ctxt);
 } else {
  int entering_sms_next = 0; int exiting_sms_next = 0;
  auto compute_enter_and_exit_states = [&](){
      compute_entered_states(this,execution_ctxt,temp,entering_sms,entering_sms_next);
      compute_exit_states(this,execution_ctxt,temp,exiting_sms,exiting_sms_next);
      execution_ctxt.do_exit(this,exiting_sms.data(),exiting_sms_next,temp);
      run_triggered_actions(this,triggered_transitions,end_of_trans_it,execution_ctxt);
      execution_ctxt.do_enter(this,entering_sms.data(),entering_sms_next,temp);
  };
  if(restart_event) {
      for(auto sm_to_be_restarted_ : current_event().payload_native_){
          auto sm_to_be_restarted = sm_to_be_restarted_.iv_;
          temp[sm_to_be_restarted] = 0;
      }
      compute_enter_and_exit_states();
      memcpy(execution_ctxt.current_states.data(),temp.data(),cur_states_size*sizeof(executionloop_context_t::state_present_rep_t));
      for(auto sm_to_be_restarted_ : current_event().payload_native_){
          auto sm_to_be_restarted = sm_to_be_restarted_.iv_;
          temp[sm_to_be_restarted] = 1;
      }
      compute_enter_and_exit_states();
      memcpy(execution_ctxt.current_states.data(),temp.data(),cur_states_size*sizeof(executionloop_context_t::state_present_rep_t));
  } else compute_enter_and_exit_states();
 }
 log_state_changes(this,execution_ctxt, temp);
 memcpy(execution_ctxt.current_states.data(),temp.data(),cur_states_size*sizeof(executionloop_context_t::state_present_rep_t));
 if(ev_read && post_proc_native) post_proc_native();
 if (global_event_call_back_fn_!=nullptr && ev_id != 0 && execution_ctxt.exported_events.find(ev_id) != execution_ctxt.exported_events.end())
		global_ev_cllbck();
 }
 //info("Simulation finished.");
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

	std::size_t pos = 0;
	int loop_ctr = 0;
	bool quit = false;

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
			auto data = p.frame_gen_->gen_msg(this,data_size,{});
            #ifdef PRINT_DEBUG
			 DEBUG << "[State_machine_simulation_core::simulate][PUSH_FRAME_TO_SENDER_QUEUE]\n";
            #endif
			if (std::get<0>(data)) p.frame_queue_->push(std::make_tuple(data,data_size,p.frame_gen_->header_length(),0));
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
