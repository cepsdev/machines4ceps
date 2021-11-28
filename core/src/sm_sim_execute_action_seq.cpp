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
		if (n->kind() == ceps::ast::Ast_node_kind::structdef) continue;
		if (n->kind() == ceps::ast::Ast_node_kind::macro_definition) continue;


		if (n->kind() == ceps::ast::Ast_node_kind::ret)
		{
			auto & node = as_return_ref(n);
			return eval_locked_ceps_expr(this,containing_smp,node.children()[0],n);

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
		} else {
	        auto r = eval_locked_ceps_expr(this,containing_smp,n,nullptr);
		}
	}
	if (verbose_log) log() << "[EXECUTE STATEMENTS][END]\n";
	return nullptr;
}
