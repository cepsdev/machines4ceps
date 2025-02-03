/*
Copyright 2025 Tomas Prerovsky (cepsdev@hotmail.com).

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

#include "core/include/state_machine_simulation_core.hpp"

#ifdef __gnu_linux__

#include <dlfcn.h>

#endif

// Helper routine which processes all oblectamenta - data blocks and oblectamenta guards in document order.
void handle_oblectamenta_blocks(State_machine_simulation_core* smc,Result_process_cmd_line const& result_cmd_line, ceps::ast::Nodeset& ns){
	using namespace ceps::ast;
	using namespace std;

	for(auto p : ns.nodes())
	{
		if (is<Ast_node_kind::binary_operator>(p) && is<Ast_node_kind::symbol>(as_binop_ref(p).left())
		    && kind(as_symbol_ref(as_binop_ref(p).left())) == "Guard" 
			&& op_val(as_binop_ref(p)) == "="){
		} else if (is<Ast_node_kind::structdef>(p) && name(as_struct_ref(p)) == "oblectamenta"){
		for (auto obl_sec : children(as_struct_ref(p))){

		if (!is<Ast_node_kind::structdef>(obl_sec) || name(as_struct_ref(obl_sec)) != "global" ) continue;
		for (auto sub_sec : children(as_struct_ref(obl_sec)))
			if (is<Ast_node_kind::structdef>(sub_sec) && name(as_struct_ref(sub_sec)) == "data" ){ 
			//handle case: oblectamenta{... global{ ...data{}... }...}
				for (auto e: children(as_struct_ref(sub_sec))){
					if (is<Ast_node_kind::int_literal>(e)) smc->vm.store(value(as_int_ref(e))); // int data
					else if (is<Ast_node_kind::float_literal>(e)) smc->vm.store(value(as_double_ref(e))); //double data (we call them float)
					else if (is<Ast_node_kind::string_literal>(e)) smc->vm.store(value(as_string_ref(e))); // string data
					else if (is<Ast_node_kind::symbol>(e) && kind(as_symbol_ref(e))=="OblectamentaDataLabel"){  //Label for data
						smc->vm.data_labels()[name(as_symbol_ref(e))] = smc->vm.mem.heap - smc->vm.mem.base;
					} else if (is<Ast_node_kind::uint8>(e)){ // Bytes
						auto v{value(as_uint8_ref(e))};
						smc->vm.store(v);
					}
				}
			} else if (is<Ast_node_kind::structdef>(sub_sec) && name(as_struct_ref(sub_sec)) == "extern" ) { 
			//handle case: oblectamenta{... global{ ...extern{}... }...}
				for (auto e: children(as_struct_ref(sub_sec))){
					if (!is<Ast_node_kind::binary_operator>(e) || op_val(as_binop_ref(e)) != ":") continue;
					auto return_type = as_binop_ref(e).right();
					auto signature = as_binop_ref(e).left();

					string sym_name, sym_kind;
					vector<Nodebase_ptr> args;
					if(!is_a_symbol_with_arguments( signature, sym_name, sym_kind, args)) continue;
					if(sym_kind != "OblectamentaExternalFunc") continue;
					auto it{smc->vm.exfuncs.begin()};
					for (;it != smc->vm.exfuncs.end();++it) if (it->name == sym_name) break;
					if(it != smc->vm.exfuncs.end()) continue;

					ceps::vm::oblectamenta::VMEnv::exfuncdescr_t exfuncdescr;
					exfuncdescr.name = sym_name;
					exfuncdescr.stack_size = 0;

					auto it_dll{smc->loaded_dlls.begin()};
					for(; it_dll != smc->loaded_dlls.end();++it_dll){
						dlerror();
						exfuncdescr.addr = dlsym(it_dll->first, exfuncdescr.name.c_str());
						if (dlerror() == nullptr) break;
					}
					if (it_dll ==  smc->loaded_dlls.end())
						//couldn't resolve symbol
						smc->fatal_(-1,"Oblectamenta: external reference '"+ sym_name +"' couldn't be resolved.");

					for(size_t i{}; i != args.size();++i){
						exfuncdescr.call_regs |= 1 << i;						
					}

					smc->vm.exfuncs.push_back(exfuncdescr);
				}
			}
		}// for all nodes in oblectamenta struct
		}// if is oblectamenta struct
	}
}
