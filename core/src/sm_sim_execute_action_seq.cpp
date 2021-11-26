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

ceps::ast::node_t 
sm_action_print(State_machine* active_smp, 
				ceps::parser_env::Symboltable* sym_table,
				std::vector<ceps::ast::node_t>const & args)
{
	using namespace ceps::ast;
    bool do_flush = false;
	for(auto n : args)
	{
		if (n == nullptr) continue;
        if (n->kind() == Ast_node_kind::byte_array){
			auto& seq = ceps::ast::bytes(ceps::ast::as_byte_array_ref(n));
            for(auto c: seq){
                std::cout << (int)c << " ";
            }
        } else if (n->kind() == Ast_node_kind::symbol &&
                    kind(as_symbol_ref(n)) == "IOManip" &&
                    name(as_symbol_ref(n)) == "endl" ) 
				do_flush = true;
		else if (is<Ast_node_kind::nodeset>(n) && as_ast_nodeset_ref(n).children().size() == 1 )
				std::cout << default_text_representation(as_ast_nodeset_ref(n).children()[0]);
        else std::cout << default_text_representation(n);
    }//for

	if (do_flush) std::cout.flush();
	return mk_none();
}

ceps::ast::node_t sm_action_assignment(	ceps::ast::Binary_operator_ptr binop,  	  	  	  	  	  	  	  	  
										ceps::ast::Nodebase_ptr lhs ,
	 	 	 	  	  	  	  	  	  	ceps::ast::Nodebase_ptr rhs,
										State_machine_simulation_core* sm_core, 
									    ceps::parser_env::Symboltable* sym_table)
{
	using namespace ceps::ast; 

	if ( is<Ast_node_kind::symbol>(lhs) && kind(as_symbol_ref(lhs)) == "Guard") {
		std::lock_guard<std::recursive_mutex> g{sm_core->states_mutex()};			
		sm_core->guards()[name(as_symbol_ref(lhs))] = rhs;
	} else if (is<Ast_node_kind::symbol>(lhs) && kind(as_symbol_ref(lhs)) == "Systemstate") {
		node_t old_value {nullptr};
		auto& state_id = name(as_symbol_ref(lhs));
		{ 
			std::lock_guard<std::recursive_mutex> g{sm_core->states_mutex()};
			old_value = sm_core->get_global_states()[state_id];
			sm_core->get_global_states()[state_id] = rhs->clone();
		}					  
		if (old_value) delete old_value;
	} else {
		std::stringstream ss;ss << *binop;
		sm_core->fatal_(-1,"Unsupported assignment:"+ss.str()+"\n");
	}
	return rhs;
}


ceps::ast::Nodebase_ptr State_machine_simulation_core::execute_action_seq(
		State_machine* containing_smp,
		ceps::ast::Nodebase_ptr ac_seq)
{
	using namespace std;
	using namespace std::chrono;
	DEBUG_FUNC_PROLOGUE
	const auto verbose_log = false;

	if (ac_seq == nullptr) return nullptr;
	if (ac_seq->kind() != ceps::ast::Ast_node_kind::structdef && ac_seq->kind() != ceps::ast::Ast_node_kind::scope) return nullptr;
	auto actions = ceps::ast::nlf_ptr(ac_seq);
	if (verbose_log) log() << "[EXECUTE STATEMENTS][START]\n";
	for(auto & n : actions->children())
	{
		if (verbose_log)log() << "[EXECUTE STATEMENT]" << *n << "\n";

		if (n->kind() == ceps::ast::Ast_node_kind::ret)
		{
			auto & node = as_return_ref(n);
			return eval_locked_ceps_expr(this,containing_smp,node.children()[0],n);

		} else	if ( is_assignment_op(n) )
		{
			auto & node = as_binop_ref(n);
			sm_action_assignment(	&node,  	  	  	  	  	  	  	  	  
									node.left() ,
	 	 	 	  	  	  	  	  	eval_locked_ceps_expr(this,containing_smp,node.right(),n),
									this,
									nullptr);
		} else if (n->kind() == ceps::ast::Ast_node_kind::identifier) {
			if (containing_smp != nullptr)
			{
				auto it = containing_smp->find_action(ceps::ast::name(ceps::ast::as_id_ref(n)));
				if (it != /*containing_smp->actions().end()*/nullptr && it->body() != nullptr){
					execute_action_seq(containing_smp,it->body());
					continue;
				}
			}

			auto it = global_funcs().find(name(ceps::ast::as_id_ref(n)));
			if (it != global_funcs().end()){
				auto body = it->second;
				execute_action_seq(nullptr,body);continue;
			}
			std::stringstream ss;ss << *n;
			fatal_(-1,"Invalid statement:"+ss.str());
		} else if (n->kind() == ceps::ast::Ast_node_kind::ifelse) {
			using namespace ceps::ast;

			auto ifelse = as_ifelse_ptr(n);
			Nodebase_ptr cond = eval_locked_ceps_expr(this,containing_smp,ifelse->children()[0],n);
			bool take_left_branch{true};
			bool erroneous_cond{false};

			if (is<Ast_node_kind::nodeset>(cond)){
				auto& set_of_nodes{as_ast_nodeset_ref(cond)};
				if (!set_of_nodes.children().size()) take_left_branch = false;
				else{
					auto p = set_of_nodes.children()[0];
					if (is<Ast_node_kind::int_literal>(p)) take_left_branch = value(as_int_ref(p)) != 0;
					else if (is<Ast_node_kind::float_literal>(p)) take_left_branch = value(as_double_ref(p)) != 0;
					else if (is<Ast_node_kind::string_literal>(p)) take_left_branch = value(as_string_ref(p)) != "1";
					else erroneous_cond = true;
				}
			} else if (is<Ast_node_kind::int_literal>(cond) )
				take_left_branch = value(as_int_ref(cond)) != 0;
			else if (is<Ast_node_kind::float_literal>(cond) ) 
			    take_left_branch = value(as_double_ref(cond)) != 0;
			else  erroneous_cond = true;
			

			if (erroneous_cond){
				std::stringstream ss; ss << *cond;
				fatal_(-1,"Expression in conditional illformed: >>"+ ss.str()+"<<.");
			}
			
			Nodebase_ptr branch_to_take = nullptr;

			if (take_left_branch && ifelse->children().size() > 1) branch_to_take = ifelse->children()[1];
			else if (!take_left_branch && ifelse->children().size() > 2) branch_to_take = ifelse->children()[2];
			if (branch_to_take == nullptr) continue;
			Nodebase_ptr result_of_branch = nullptr;
			if (branch_to_take->kind() != Ast_node_kind::structdef && branch_to_take->kind() != Ast_node_kind::scope)
			{
				Scope scope(branch_to_take);scope.owns_children() = false;
				result_of_branch=execute_action_seq(containing_smp,&scope);
				scope.children().clear();
			} else { result_of_branch=execute_action_seq(containing_smp,branch_to_take);}
			if (result_of_branch != nullptr) return result_of_branch;
		} else if (n->kind() == ceps::ast::Ast_node_kind::symbol && ceps::ast::kind(ceps::ast::as_symbol_ref(n)) == "Event")
		{
			//log() << "[QUEUEING EVENT][" << ceps::ast::name(ceps::ast::as_symbol_ref(n)) <<"]" << "\n";
			event_t ev(ceps::ast::name(ceps::ast::as_symbol_ref(n)));
			ev.unique_ = this->unique_events().find(ev.id_) != this->unique_events().end();
			ev.already_sent_to_out_queues_ = false;
			enqueue_event(ev,true);
		} else if (n->kind() == ceps::ast::Ast_node_kind::func_call)
		{
			std::vector<ceps::ast::Nodebase_ptr> args;
			std::string  func_name;
			if (!sm_action_read_func_call_values(this,n, func_name,args)){
				std::stringstream ss;
				ss << *n << "\n";
				fatal_(-1,"Internal Error:State_machine_simulation_core::execute_action_seq:"+ss.str());
			}

			if (is_global_event(func_name))
			{
				//log() << "[QUEUEING EVENT WITH PAYLOAD][" << func_name <<"]" << "\n";
				{
					for(size_t i = 0; i != args.size(); ++i)
					{
						args[i] = eval_locked_ceps_expr(this,containing_smp,args[i],n);

						//args[i]  = ceps::interpreter::evaluate(args[i],ceps_env_current().get_global_symboltable(),ceps_env_current().interpreter_env(),n	);
					}
				}
				event_t ev(func_name,args);
				ev.already_sent_to_out_queues_ = false;
				ev.unique_ = this->unique_events().find(ev.id_) != this->unique_events().end();
				enqueue_event(ev,true);
			}

			else if (func_name == "timer" || func_name == "start_timer" || func_name == "start_periodic_timer")
			{
				for(size_t i = 0; i != args.size(); ++i)
				 {
					args[i] = eval_locked_ceps_expr(this,containing_smp,args[i],n);
				 }
				exec_action_timer(args,func_name == "start_periodic_timer");
			}
            else if (func_name == "print"){
				for(size_t i = 0; i != args.size(); ++i)
					args[i] = eval_locked_ceps_expr(this,containing_smp,args[i],n);
			 	sm_action_print(containing_smp, nullptr, args);
			}
			else if (func_name == "kill_timer" || func_name == "stop_timer")
			{

				if (args.size() == 0){
					DEBUG << "[KILLING ALL TIMERS]\n";

					this->kill_named_timer(std::string{});
				}else{
					std::string timer_id;
					if (args[0]->kind() != ceps::ast::Ast_node_kind::identifier)
						fatal_(-1,"stop_timer: first argument (the timer id) has to be an unbound identifier.\n");
					timer_id = ceps::ast::name(ceps::ast::as_id_ref(args[0]));
					DEBUG << "[KILLING NAMED TIMERS][TIMER_ID="<< timer_id <<"]\n";
					this->kill_named_timer(timer_id);
				}
			} else{
                auto r = eval_locked_ceps_expr(this,containing_smp,n,nullptr);
				//if (r) std::cerr <<"===> "<< *r << std::endl;
                //if(r) delete r;
			}
		} else {
            auto r = eval_locked_ceps_expr(this,containing_smp,n,nullptr);
		}
	}
	if (verbose_log) log() << "[EXECUTE STATEMENTS][END]\n";
	return nullptr;
}
