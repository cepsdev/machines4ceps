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


// The following routines serialize message{...} definitions.
static size_t dispatch_serialize_event_payload(ceps::ast::node_t msg, char* buffer, size_t remaining_bytes){
    using namespace ceps::ast; using namespace std; using namespace ceps::vm::oblectamenta;
    size_t written{};
    vector<node_t> args;
    string sym_name, sym_kind;
    if (is<Ast_node_kind::structdef>(msg)){
        auto the_name {name(as_struct_ref(msg))};
        if (buffer) if (remaining_bytes < the_name.size() + 1 + sizeof(msg_node)) return {};
        if (buffer) if (msg_node_ex::MAX_NAME < sizeof(msg_node) + 1) return {};
        
		msg_node* n{ buffer ? (msg_node*)buffer : nullptr };
        
		if(n) n->what = msg_node::NODE;
        written = the_name.size() + 1 + sizeof(msg_node);
        msg_node_ex* nn{ buffer? (msg_node_ex*)buffer : nullptr};

        if(nn) strncpy(nn->name,the_name.c_str(),the_name.size()+1);
        for (auto e: children(as_struct_ref(msg))){
            auto r = dispatch_serialize_event_payload(e, buffer ? buffer + written : nullptr, buffer ? remaining_bytes - written : 0);
            written += r;
            if (buffer) if (written > remaining_bytes) return {};
        }
        if(nn) nn->size = written;
    } else if (is_a_symbol_with_arguments( msg,sym_name,sym_kind,args)){
        if (sym_kind == "OblectamentaMessageTagInt32" && args.size() && is<Ast_node_kind::int_literal>(args[0])){
            if (buffer) if (sizeof(msg_node_int32) > remaining_bytes) return {};
            msg_node_int32* n{ buffer ? (msg_node_int32*)buffer : nullptr };
			written = sizeof(msg_node_int32);
            if(n) {
				n->what = msg_node::INT32;
            	n->size = written;
            	n->value = value(as_int_ref(args[0]));
			}
        } else if (sym_kind == "OblectamentaMessageTagZeroTerminatedString" && args.size() && is<Ast_node_kind::string_literal>(args[0])){
            if (buffer) if (sizeof(msg_node) + 1 + value(as_string_ref(args[0])).size() > remaining_bytes) return {};
            msg_node_sz* n{ buffer ? (msg_node_sz*)buffer : nullptr};
			written = sizeof(msg_node) + 1 + value(as_string_ref(args[0])).size();
            if(n){
				n->what = msg_node::SZ;
            	n->size = written;
            	strncpy(n->value,value(as_string_ref(args[0])).c_str(),value(as_string_ref(args[0])).size()+1);
			}
        } 
    } else if (is<Ast_node_kind::int_literal>(msg)){
		if (buffer) if (sizeof(msg_node_int32) > remaining_bytes) return {};
		msg_node_int32* n{ buffer ? (msg_node_int32*)buffer : nullptr };
		written = sizeof(msg_node_int32);
		if(n) {
			n->what = msg_node::INT32;
			n->size = written;
			n->value = value(as_int_ref(msg));
		}
	} else if (is<Ast_node_kind::string_literal>(msg)){
		if (buffer) if (sizeof(msg_node) + 1 + value(as_string_ref(msg)).size() > remaining_bytes) return {};
		msg_node_sz* n{ buffer ? (msg_node_sz*)buffer : nullptr};
		written = sizeof(msg_node) + 1 + value(as_string_ref(msg)).size();
		if(n){
			n->what = msg_node::SZ;
			n->size = written;
			strncpy(n->value,value(as_string_ref(msg)).c_str(),value(as_string_ref(msg)).size()+1);
		}	
	}
    return written;
}

static size_t serialize_oblectamenta_msg(ceps::ast::node_t msg, char* buffer, size_t remaining_bytes){
    using namespace ceps::ast; using namespace std; using namespace ceps::vm::oblectamenta;
	if (buffer) if (sizeof(msg_node) > remaining_bytes) return 0;
	msg_node* root{nullptr};
    if (buffer) {
		root =  (msg_node*)buffer;
		root->what = msg_node::ROOT;
	}
    size_t written{};
    written = sizeof(msg_node);
    for (auto e: children(as_struct_ref(msg))){
        auto r = dispatch_serialize_event_payload(e, buffer ? buffer + written: nullptr, buffer? remaining_bytes - written : 0);
        written += r;
    }
	if (root) root->size = written; 
    return written;
}

namespace ceps::vm::oblectamenta{
	size_t deserialize_event_payload(char* buffer, size_t size, std::string& res);
}

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
					else if (is<Ast_node_kind::symbol>(e) && kind(as_symbol_ref(e))=="OblectamentaDataLabel")
					{  //Label for data
						smc->vm.data_labels()[name(as_symbol_ref(e))] = smc->vm.mem.heap - smc->vm.mem.base;
					} else if (is<Ast_node_kind::uint8>(e)){ // Bytes
						auto v{value(as_uint8_ref(e))};
						smc->vm.store(v);
					} else if (is<Ast_node_kind::structdef>(e)){
						if (name(as_struct_ref(e)) == "msg"){
							//cerr << serialize_oblectamenta_msg(e,nullptr,0) << "\n";
							auto required_space = serialize_oblectamenta_msg(e,nullptr,0);
							auto ofs = smc->vm.reserve(required_space);
							if (!ofs) {
								stringstream ss;ss << *e;
								smc->fatal_(2,"[Oblectementa Assembler][data section] message too large.\n"+ss.str());
							}
							char* buffer = (char*)smc->vm.mem.base + *ofs;
							if ( required_space != serialize_oblectamenta_msg(e,buffer,required_space)){
								stringstream ss;ss << *e;
								smc->fatal_(2,"[Oblectementa Assembler][data section] Failed to serialize message.\n"+ss.str());
							}
						}
					} else if (is<Ast_node_kind::symbol>(e) && kind(as_symbol_ref(e))=="OblectamentaGlobalEventPayload")
					{  
						smc->oblectamenta_global_event_buffer_offs = smc->vm.mem.heap - smc->vm.mem.base;
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
