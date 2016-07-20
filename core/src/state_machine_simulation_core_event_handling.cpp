#include "core/include/state_machine_simulation_core.hpp"
#include "core/include/base_defs.hpp"

#include <thread>
#include "cepsparserdriver.hh"


void State_machine_simulation_core::queue_event(std::string ev_name,std::initializer_list<sm4ceps_plugin_int::Variant> vl){
#ifdef SHORTCIRCUIT_EVENTQUEUE
	if (vl.size() == 0 && enforce_native()){
		auto& ctxt = executionloop_context();
		auto evid = ctxt.ev_to_id[ev_name];
		if (evid > 0){
			ctxt.push_ev_sync_queue(evid);
			return;
		}
	}
#endif
	event_t ev(ev_name);
	ev.already_sent_to_out_queues_ = false;
	ev.unique_ = this->unique_events().find(ev.id_) != this->unique_events().end();
	ev.payload_native_.insert(ev.payload_native_.cbegin(),vl.begin(),vl.end());


	enqueue_event(ev,true);
}


void State_machine_simulation_core::sync_queue_event(int ev_id){
	executionloop_context().push_ev_sync_queue(ev_id);
}


size_t State_machine_simulation_core::argc(){
	if (enforce_native() && executionloop_context().current_ev_id != 0) return 0;
	return std::max(current_event().payload_.size(),current_event().payload_native_.size());
}

sm4ceps_plugin_int::Variant State_machine_simulation_core::argv(size_t j){
 if (0 == j) {
	 if (enforce_native() && executionloop_context().current_ev_id != 0)
		 return sm4ceps_plugin_int::Variant{executionloop_context().id_to_ev[executionloop_context().current_ev_id]};
	 return sm4ceps_plugin_int::Variant{current_event().id_};
 }
 auto i = j - 1;
 if (std::max(current_event().payload_.size(),current_event().payload_native_.size()) <= i )
	 fatal_(-1,"Access to argv: Out of bounds.");
 if (current_event().payload_native_.size()){
	 return current_event().payload_native_[i];
 }
 auto r = current_event().payload_[i];
 if (r->kind() == ceps::ast::Ast_node_kind::int_literal)
	 return sm4ceps_plugin_int::Variant{ceps::ast::value(ceps::ast::as_int_ref(r))};
 if (r->kind() == ceps::ast::Ast_node_kind::float_literal)
 	 return sm4ceps_plugin_int::Variant{ceps::ast::value(ceps::ast::as_double_ref(r))};
 if (r->kind() == ceps::ast::Ast_node_kind::string_literal)
 	 return sm4ceps_plugin_int::Variant{ceps::ast::value(ceps::ast::as_string_ref(r))};
 fatal_(-1,"Access to argv: Unsupported type.");
}

bool State_machine_simulation_core::is_global_event(std::string const & ev_name){
	auto symtab = ceps_env_current().get_global_symboltable();
	auto sym = symtab.lookup(ev_name,false,false,false);

	if (sym && sym->category == ceps::parser_env::Symbol::SYMBOL && sym->payload && ((ceps::parser_env::Symbol*)sym->payload)->name == "Event")
		return true;
	return false;
}

void State_machine_simulation_core::enqueue_event(event_t ev, bool update_out_queues)
{
	ev.already_sent_to_out_queues_ = update_out_queues;
	this->main_event_queue().push(ev);
	if (!update_out_queues) return;
	std::lock_guard<std::mutex> lk(out_event_queues_m_);
	for(auto oq : out_event_queues_){
		oq->push(ev);
	}
}

std::string compose_err_msg_assert(State_machine_simulation_core* smp, state_rep_t state, State_machine_simulation_core::states_t& states){
	std::stringstream ss;
	ss << "\nExpected to be in state " << smp->get_fullqualified_id(state) <<", current states are:\n";
	if (smp->enforce_native()){
	 for(size_t i = 0;i!=smp->executionloop_context().current_states.size();++i)
		 if (smp->executionloop_context().current_states[i]) ss << " " << smp->executionloop_context().idx_to_state_id[i] << "\n";
	} else{
	 for(auto const & s : states)
	 {
		ss << " " << smp->get_fullqualified_id(s) << "\n";
	 }
	}
	ss <<".";
	return ss.str();
}

template <typename F> void handle_current_states_assertion(ceps::ast::Nodebase_ptr node_raw,
		                             State_machine_simulation_core* smp,
									 State_machine_simulation_core::states_t& states,
									 std::string what, F f){

 ceps::ast::Struct_ptr assert = ceps::ast::as_struct_ptr(node_raw);
 for(auto const& n: assert->children())
 {
  if (n->kind() != ceps::ast::Ast_node_kind::identifier && n->kind() != ceps::ast::Ast_node_kind::binary_operator) continue;
  if (n->kind() == ceps::ast::Ast_node_kind::binary_operator && op(ceps::ast::as_binop_ref(n)) != '.') continue;
  auto state = smp->resolve_state_qualified_id(n,nullptr);
  if (!state.valid()) {
	std::stringstream ss; ss << ceps::ast::Nodeset(n);
	if (smp->conf_ignore_unresolved_state_id_in_directives())
		smp->log() << "****Warning: Expression doesn't evaluate to an existing state: "<<ss.str();
		else smp->fatal_(-1,"Expression doesn't evaluate to an existing state: "+ss.str());
		return;
  }
  bool found = false;
  if (smp->enforce_native()){
		found = smp->executionloop_context().current_states[state.id_];
  } else{
	 for(auto const & s : states)
	 {
		if (s == state) {found = true; break;}
	 }
  }
  if (f(found)) {
	auto err_msg = compose_err_msg_assert(smp,  state,  states);
	std::cout << std::endl;
    smp->fatal_(-1,"\nASSERTION not satisfied ("+what+"): "+err_msg);
  }
 }//for
}


bool State_machine_simulation_core::fetch_event(event_rep_t& ev,
												ceps::ast::Nodeset& sim,
												int& pos,
												states_t& states,
												bool& states_updated,
												std::vector<State_machine*>& on_enter_seq,
												bool ignore_handler,
												bool ignore_ev_queue,
												bool exit_if_start_found)
{
	using namespace ceps::ast;
	bool pending_timed_event;
	states_updated = false;
do{
	pending_timed_event = timed_events_pending();
	if (!ignore_ev_queue)
	{

		std::unique_lock<std::mutex> lk(main_event_queue().data_mutex());
		if (!main_event_queue().data().empty()) {
			 auto eev = main_event_queue().data().front();
			 main_event_queue().data().pop();
			 lk.unlock();

			 if (eev.frmctxt_ !=nullptr){
				 eev.frmctxt_->update_sysstates();
				 if ( nullptr != eev.frmctxt_->get_handler()){
					 eev.frmctxt_->get_handler()();
					 delete eev.frmctxt_;
				 }
				 continue;
			 }

			 ev.sid_ = eev.id_;

             if (map_ceps_payload_to_native_ && eev.payload_.size()){
                eev.payload_native_.clear();
               for(auto v : eev.payload_){
                 if (v->kind() == ceps::ast::Ast_node_kind::int_literal)
                     eev.payload_native_.push_back(sm4ceps_plugin_int::Variant(value(as_int_ref(v))));
                 else if (v->kind() == ceps::ast::Ast_node_kind::float_literal)
                     eev.payload_native_.push_back(sm4ceps_plugin_int::Variant(value(as_double_ref(v))));
                 else if (v->kind() == ceps::ast::Ast_node_kind::string_literal)
                     eev.payload_native_.push_back(sm4ceps_plugin_int::Variant(value(as_string_ref(v))));
                 if (delete_ceps_payload_) delete v;
                 }
                 eev.payload_.clear();
             }

			 ev.payload_ = eev.payload_;
			 ev.payload_native_ = eev.payload_native_;
			 ev.glob_func_ = eev.glob_func_;

			 if (!eev.already_sent_to_out_queues_)
				 for(auto oq : out_event_queues_)oq->push(eev);

			 return true;
		}
	}

	for(; pos < (int)sim.nodes().size();++pos)
	{
		auto node_raw = sim.nodes()[pos];
		if ( is_assignment_op(node_raw) )
		{
			auto & node = as_binop_ref(node_raw);
			std::string state_id;
			if (is_assignment_to_guard(node))
			{
				eval_guard_assign(node);
			} else if (is_assignment_to_state(node,state_id))
			{
				eval_state_assign(node,state_id);
			}
			else {
			 std::stringstream ss;
			 ss << *node_raw;
			 fatal_(-1,"Unsupported assignment:"+ss.str()+"\n");
			}
			continue;
		}
		else if (node_raw->kind() == ceps::ast::Ast_node_kind::structdef
				&& ceps::ast::name(ceps::ast::as_struct_ref(node_raw)) == "Start"){
		  if (exit_if_start_found){++pos; return false;}
		  else
		  {

			  states_t new_states;
			  std::set<State_machine*> new_sms;
			  for(auto const& n: ceps::ast::as_struct_ref(node_raw).children())
			  	{

			  		if (n->kind() != ceps::ast::Ast_node_kind::identifier && n->kind() != ceps::ast::Ast_node_kind::binary_operator) continue;
			  		if (n->kind() == ceps::ast::Ast_node_kind::binary_operator && op(ceps::ast::as_binop_ref(n)) != '.') continue;
			  		auto state = resolve_state_qualified_id(n,nullptr);
			  		std::stringstream ss; ss << ceps::ast::Nodeset(n);
			  		if (!state.valid())
			  			{
			  			  if (conf_ignore_unresolved_state_id_in_directives())
			  				  log() << "***Warning: Start-directive: Expression doesn't evaluate to an existing state: " << ss.str();
			  			  else fatal_(-1,"Start-directive: Expression doesn't evaluate to an existing state: "+ss.str());
			  			  continue;
			  			}
			  		new_states.push_back(state);
			  		new_sms.insert(state.smp_);
			  	}//for
			  on_enter_seq.clear();
			  on_enter_seq.insert(on_enter_seq.end(),new_sms.begin(),new_sms.end());
			  ++pos;
			  states_updated = true;
			  states_t new_states_hull;
			  trans_hull_of_containment_rel(new_states,new_states_hull);
			  std::set<state_rep_t> states_map(states.begin(),states.end());
			  states_map.insert(new_states_hull.begin(),new_states_hull.end());
			  states.clear();
			  for(auto & s :states_map ) states.push_back(s);

			  return false;
		  }
		}else if (node_raw->kind() == ceps::ast::Ast_node_kind::structdef &&
				  ceps::ast::name(ceps::ast::as_struct_ref(node_raw)) == "ASSERT_EVENTUALLY_VISIT_STATES")
		{

			ceps::ast::Struct_ptr assert = ceps::ast::as_struct_ptr(node_raw);
			assert_t ass;
			ass.satisfied_ = false;
			ass.type_ = ass.EVENTUALLY;

			for(auto const& n: assert->children())
			{
				if (n->kind() != ceps::ast::Ast_node_kind::identifier && n->kind() != ceps::ast::Ast_node_kind::binary_operator) continue;
				if (n->kind() == ceps::ast::Ast_node_kind::binary_operator && op(ceps::ast::as_binop_ref(n)) != '.') continue;

				auto state = resolve_state_qualified_id(n,nullptr);
				if (!state.valid()) {
					std::stringstream ss; ss << ceps::ast::Nodeset(n);
					if (conf_ignore_unresolved_state_id_in_directives())
						log() << "****Warning: Expression doesn't evaluate to an existing state: "<<ss.str();
					else fatal_(-1,"Expression doesn't evaluate to an existing state: "+ss.str());
					continue;
				}

				ass.states_.push_back(state);

			}
			active_asserts_.push_back(ass);
		} else if (node_raw->kind() == ceps::ast::Ast_node_kind::structdef &&
				  ceps::ast::name(ceps::ast::as_struct_ref(node_raw)) == "ASSERT_NEVER_VISIT_STATES")
		{

			ceps::ast::Struct_ptr assert = ceps::ast::as_struct_ptr(node_raw);
			assert_t ass;
			ass.satisfied_ = true;
			ass.type_ = ass.NEVER;

			for(auto const& n: assert->children())
			{
				if (n->kind() != ceps::ast::Ast_node_kind::identifier && n->kind() != ceps::ast::Ast_node_kind::binary_operator) continue;
				if (n->kind() == ceps::ast::Ast_node_kind::binary_operator && op(ceps::ast::as_binop_ref(n)) != '.') continue;

				auto state = resolve_state_qualified_id(n,nullptr);
				if (!state.valid()) {
					std::stringstream ss; ss << ceps::ast::Nodeset(n);
					if (conf_ignore_unresolved_state_id_in_directives())
						log() << "****Warning: Expression doesn't evaluate to an existing state: "<<ss.str();
					else fatal_(-1,"Expression doesn't evaluate to an existing state: "+ss.str());
					continue;
				}

				ass.states_.push_back(state);

			}
			active_asserts_.push_back(ass);
		} else if (node_raw->kind() ==
				ceps::ast::Ast_node_kind::structdef
				&& ceps::ast::name(ceps::ast::as_struct_ref(node_raw)) == "ASSERT_END_STATES_CONTAINS")
		{
			ceps::ast::Struct_ptr assert = ceps::ast::as_struct_ptr(node_raw);

				for(auto const& n: assert->children())
					{
						if (n->kind() != ceps::ast::Ast_node_kind::identifier && n->kind() != ceps::ast::Ast_node_kind::binary_operator) continue;
						if (n->kind() == ceps::ast::Ast_node_kind::binary_operator && op(ceps::ast::as_binop_ref(n)) != '.') continue;
						auto state = resolve_state_qualified_id(n,nullptr);

						if (!state.valid()) {
							std::stringstream ss; ss << ceps::ast::Nodeset(n);
							if (conf_ignore_unresolved_state_id_in_directives())
								log() << "****Warning: Expression doesn't evaluate to an existing state: "<<ss.str();
							else fatal_(-1,"Expression doesn't evaluate to an existing state: "+ss.str());
							continue;
						}
						assert_in_end_states_.insert(state);
					}//for
		}
	      else if (node_raw->kind() ==
				ceps::ast::Ast_node_kind::structdef
				&& ceps::ast::name(ceps::ast::as_struct_ref(node_raw)) == "ASSERT_END_STATES_CONTAINS_NOT")
		{
				ceps::ast::Struct_ptr assert = ceps::ast::as_struct_ptr(node_raw);

					for(auto const& n: assert->children())
						{
							if (n->kind() != ceps::ast::Ast_node_kind::identifier && n->kind() != ceps::ast::Ast_node_kind::binary_operator) continue;
							if (n->kind() == ceps::ast::Ast_node_kind::binary_operator && op(ceps::ast::as_binop_ref(n)) != '.') continue;
							auto state = resolve_state_qualified_id(n,nullptr);

							if (!state.valid()) {
								std::stringstream ss; ss << ceps::ast::Nodeset(n);
								if (conf_ignore_unresolved_state_id_in_directives())
									log() << "****Warning: Expression doesn't evaluate to an existing state: "<<ss.str();
								else fatal_(-1,"Expression doesn't evaluate to an existing state: "+ss.str());
								continue;
							}
							assert_not_in_end_states_.insert(state);

						}//for

		}
		else if (node_raw->kind() == ceps::ast::Ast_node_kind::structdef
				&& ceps::ast::name(ceps::ast::as_struct_ref(node_raw)) == "ASSERT_CURRENT_STATES_CONTAINS")
		{
			handle_current_states_assertion(node_raw,
					                        this,
											states,
											"ASSERT_CURRENT_STATES_CONTAINS",
											[](bool found){return !found;} );


		} else if (node_raw->kind() == ceps::ast::Ast_node_kind::structdef
				&& ceps::ast::name(ceps::ast::as_struct_ref(node_raw)) == "ASSERT_CURRENT_STATES_CONTAINS_NOT")
		{
			handle_current_states_assertion(node_raw,
					                        this,
											states,
											"ASSERT_CURRENT_STATES_CONTAINS",
											[](bool found){return found;} );
		}
		else {
		 auto cev = resolve_event_qualified_id(sim.nodes()[pos],nullptr);
		 if ( (!cev.valid() || cev.sid_.length() == 0) ) {
		  ceps::ast::Scope scope(node_raw);scope.owns_children() = false;

		  if (enforce_native())
                                 fatal_(-1,"Expecting native implementation (--enforce_native):fetch_event");

			execute_action_seq(nullptr,&scope);
			//REMARK:Last call could have triggered an event, so we need to examine the queue first.
			++pos;
			return fetch_event( ev,
					sim,
					pos,
					states,
					states_updated,
					on_enter_seq,
					ignore_handler,
					ignore_ev_queue,
					exit_if_start_found);
		}
		ev = cev;
		if (running_as_node()) {
			event_t real_ev(ev.sid_);
			real_ev.unique_ = this->unique_events().find(real_ev.id_) != this->unique_events().end();
			std::lock_guard<std::mutex> lk(out_event_queues_m_);
			for(auto oq : out_event_queues_){
				oq->push(real_ev);
			}
		}
		++pos;
		return true;
	   }//else
	}



	//std::cout << timed_events_pending() << std::endl;
	if (pos >= (int)sim.nodes().size()){
		if (running_as_node() || timed_events_pending()){
			//DEBUG << "[FETCH_EVENT_LOOP][WAIT_FOR_DATA]\n";
			this->main_event_queue().wait_for_data();
		}
	}

	//if (pending_timed_event) std::this_thread::sleep_for(std::chrono::microseconds(1));
	//else if (pos >= (int)sim.nodes().size() && running_as_node()){
	//	DEBUG << "[FETCH_EVENT_LOOP][WAIT_FOR_DATA]\n";
	//	this->main_event_queue().wait_for_data();
	//}
} while (pending_timed_event || running_as_node());
	return false;
}
