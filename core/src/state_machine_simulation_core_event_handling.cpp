#include "core/include/state_machine_simulation_core.hpp"
#ifdef __GNUC__
#define __funcname__ __PRETTY_FUNCTION__
#else
#define __funcname__ __FUNCSIG__
#endif
#define DEBUG_FUNC_PROLOGUE 	Debuglogger debuglog(__funcname__,this,this->print_debug_info_);
#define DEBUG (debuglog << "[DEBUG]", debuglog)

#include <thread>
#include "cepsparserdriver.hh"


bool State_machine_simulation_core::is_global_event(std::string const & ev_name){
	auto symtab = ceps_env_current().get_global_symboltable();
	auto sym = symtab.lookup(ev_name,false,false,false);

	if (sym && sym->category == ceps::parser_env::Symbol::SYMBOL && sym->payload && ((ceps::parser_env::Symbol*)sym->payload)->name == "Event")
		return true;
	return false;
}

void State_machine_simulation_core::enqueue_event(event_t ev, bool update_out_queues)
{

	if(ev.delta_time_ != clock_type::duration::zero())
	{
		if (timed_events_active_== 0)base_time_point_ = clock_.now();
		else ev.delta_time_ += clock_.now() - base_time_point_;
		++timed_events_active_;
	}


	ev.already_sent_to_out_queues_ = update_out_queues;
	this->main_event_queue().push(ev);
	if (!update_out_queues) return;
	std::lock_guard<std::mutex> lk(out_event_queues_m_);
	for(auto oq : out_event_queues_){
		oq->push(ev);
	}
}

bool State_machine_simulation_core::fetch_event(event_rep_t& ev,
												ceps::ast::Nodeset& sim,
												int& pos,
												states_t& states,
												bool& states_updated,
												bool ignore_handler,
												bool ignore_ev_queue,
												bool exit_if_start_found)
{
	DEBUG_FUNC_PROLOGUE
	using namespace ceps::ast;
	bool pending_timed_event;
	states_updated = false;
	size_t timer = 0;
do{
	pending_timed_event = false;
	if (!ignore_ev_queue)
	{
		//global_event_queue.size();

		std::unique_lock<std::mutex> lk(main_event_queue().data_mutex());
		if (!main_event_queue().data().empty()) {
			auto& next_event = main_event_queue().data().top();
			bool timed_event = false;
			bool periodic_event = false;
			if (next_event.delta_time_ != clock_type::duration::zero())
			{
				timed_event = true;
				periodic_event = next_event.periodic_;
				if(clock_.now()-this->base_time_point_ < next_event.delta_time_)
				 { pending_timed_event = true;}
			}
			if(!pending_timed_event)
			{
			 if(timed_event&&!periodic_event) --timed_events_active_;
			 auto eev = main_event_queue().data().top();
			 main_event_queue().data().pop();
			 lk.unlock();
			 if (periodic_event){

				 eev.delta_time_ = eev.delta_time_orig_;
				 enqueue_event(eev,true);
			 }

			 ev.sid_ = eev.id_;
			 ev.payload_ = eev.payload_;

			 if (!eev.already_sent_to_out_queues_)
			 {
				 for(auto oq : out_event_queues_)oq->push(eev);
			 }

			 return true;
			}
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
		else if (node_raw->kind() == ceps::ast::Ast_node_kind::structdef && ceps::ast::name(ceps::ast::as_struct_ref(node_raw)) == "Start"){
		  if (exit_if_start_found){++pos; return false;}
		  else
		  {
			  DEBUG << "[START_FOUND]\n";
			  states_t new_states;
			  for(auto const& n: ceps::ast::as_struct_ref(node_raw).children())
			  	{
				    DEBUG << "[START_FOUND][INSERT_STATE]\n";
			  		if (n->kind() != ceps::ast::Ast_node_kind::identifier && n->kind() != ceps::ast::Ast_node_kind::binary_operator) continue;
			  		if (n->kind() == ceps::ast::Ast_node_kind::binary_operator && op(ceps::ast::as_binop_ref(n)) != '.') continue;
			  		auto state = resolve_state_qualified_id(n,nullptr);
			  		std::stringstream ss; ss << ceps::ast::Nodeset(n);
			  		if (!state.valid()) fatal_(-1,"Expression doesn't evaluate to an existing state: "+ss.str());
			  		new_states.push_back(state);
			  	}//for
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
				DEBUG <<"[PRE][resolve_state_qualified_id]\n";
				auto state = resolve_state_qualified_id(n,nullptr);
				DEBUG <<"[POST][resolve_state_qualified_id]\n";
				ass.states_.push_back(state);
				DEBUG << "[ASSERT_EVENTUALLY_VISIT_STATES]["<<get_fullqualified_id(state)<<"\n" ;
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
				DEBUG <<"[PRE][resolve_state_qualified_id]\n";
				auto state = resolve_state_qualified_id(n,nullptr);
				DEBUG <<"[POST][resolve_state_qualified_id]\n";
				ass.states_.push_back(state);
				DEBUG << "[ASSERT_EVENTUALLY_VISIT_STATES]["<<get_fullqualified_id(state)<<"\n" ;
			}
			active_asserts_.push_back(ass);
		} else if (node_raw->kind() == ceps::ast::Ast_node_kind::structdef && ceps::ast::name(ceps::ast::as_struct_ref(node_raw)) == "ASSERT_CURRENT_STATES_CONTAINS")
		{
			ceps::ast::Struct_ptr assert = ceps::ast::as_struct_ptr(node_raw);

			for(auto const& n: assert->children())
				{
					if (n->kind() != ceps::ast::Ast_node_kind::identifier && n->kind() != ceps::ast::Ast_node_kind::binary_operator) continue;
					if (n->kind() == ceps::ast::Ast_node_kind::binary_operator && op(ceps::ast::as_binop_ref(n)) != '.') continue;
					auto state = resolve_state_qualified_id(n,nullptr);
					std::stringstream ss; ss << ceps::ast::Nodeset(n);
					if (!state.valid()) fatal_(-1,"Expression doesn't evaluate to an existing state: "+ss.str());
					bool found = false;
					for(auto const & s : states)
					{
						if (s == state) {found = true; break;}
					}
					if (!found) {
						std::stringstream ss;
						ss << "\nExpected to be in state " << get_fullqualified_id(state) <<", current states are:\n";
						for(auto const & s : states)
						{
							ss << " " << get_fullqualified_id(s) << "\n";
						}
						ss <<".";
						fatal_(-1,"\nASSERTION not satisfied (ASSERT_CURRENT_STATES_CONTAINS): "+ss.str());
					}
					//states.push_back(state);
				}//for

		} else if (node_raw->kind() == ceps::ast::Ast_node_kind::structdef && ceps::ast::name(ceps::ast::as_struct_ref(node_raw)) == "ASSERT_CURRENT_STATES_CONTAINS_NOT")
		{

			ceps::ast::Struct_ptr assert = ceps::ast::as_struct_ptr(node_raw);

			for(auto const& n: assert->children())
				{
					if (n->kind() != ceps::ast::Ast_node_kind::identifier && n->kind() != ceps::ast::Ast_node_kind::binary_operator) continue;
					if (n->kind() == ceps::ast::Ast_node_kind::binary_operator && op(ceps::ast::as_binop_ref(n)) != '.') continue;
					auto state = resolve_state_qualified_id(n,nullptr);
					std::stringstream ss; ss << ceps::ast::Nodeset(n);
					if (!state.valid()) fatal_(-1,"Expression doesn't evaluate to an existing state: "+ss.str());
					bool found = false;
					for(auto const & s : states)
					{
						if (s == state) {found = true; break;}
					}
					if (found) {
						std::stringstream ss;
						ss << "\nExpected NOT to be in state " << get_fullqualified_id(state) <<", current states are:\n";
						for(auto const & s : states)
						{
							ss << " " << get_fullqualified_id(s) << "\n";
						}
						ss <<".";
						fatal_(-1,"\nASSERTION not satisfied (ASSERT_CURRENT_STATES_CONTAINS_NOT): "+ss.str());
					}
					//states.push_back(state);
				}//for

		}
		auto cev = resolve_event_qualified_id(sim.nodes()[pos],nullptr);
		if (!cev.valid() || cev.sid_.length() == 0) {
			ceps::ast::Scope scope(node_raw);
			execute_action_seq(nullptr,&scope);
			//REMARK:Last call could have triggered an event, so we need to examine the queue first.
			++pos;
			return fetch_event( ev,
					sim,
					pos,
					states,
					states_updated,
					ignore_handler,
					ignore_ev_queue,
					exit_if_start_found);

			/*pending_timed_event = true;
			continue;*/
		}
		ev = cev;
		if (running_as_node()) {
			event_t real_ev(ev.sid_);
			std::lock_guard<std::mutex> lk(out_event_queues_m_);
			for(auto oq : out_event_queues_){
				oq->push(real_ev);
			}
		}
		++pos;
		return true;
	}



	if (pending_timed_event) std::this_thread::sleep_for(std::chrono::microseconds(1));
	else if (pos >= (int)sim.nodes().size() && running_as_node()){
		DEBUG << "[FETCH_EVENT_LOOP][WAIT_FOR_DATA]\n";
		this->main_event_queue().wait_for_data();
	}
} while (pending_timed_event || running_as_node());
	return false;
}
