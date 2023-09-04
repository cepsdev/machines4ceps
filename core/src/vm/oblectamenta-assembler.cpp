/*
Copyright 2023 Tomas Prerovsky (cepsdev@hotmail.com).

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

#include "core/include/vm/vm_base.hpp"
#include "core/include/vm/oblectamenta-assembler.hpp"
#include <stdlib.h>
#include <iostream>
#include <ctype.h>
#include <chrono>
#include <sstream>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <queue>
#include <unordered_map>
#include <cstring>

namespace ceps::vm::oblectamenta{

struct patch_entry{
 char id[64] = {0};
 size_t text_loc{};
};

static std::vector<patch_entry> patch_entries;

void oblectamenta_assembler(ceps::vm::oblectamenta::VMEnv& vm, std::vector<ceps::ast::node_t> mnemonics)
{
 using namespace ceps::ast; using namespace std;

 map<int32_t,size_t> immediate2loc; // immediate values => location in storage
 map<string,size_t> codelabel2loc; // code label => location in storage
                       
 
 for (size_t stmt_pos{}; stmt_pos < mnemonics.size(); ++stmt_pos){
    
    auto e{mnemonics[stmt_pos]};
	std::string sym_name;
	std::string sym_kind;
	std::vector<node_t> args;
    
    if(is<Ast_node_kind::symbol>(e) && kind(as_symbol_ref(e)) == "OblectamentaCodeLabel" ){
        auto lbl{name(as_symbol_ref(e))};
        //cerr << lbl << " patch_entries.size(): " << patch_entries.size() << '\n';  
        codelabel2loc[lbl] = vm.text().size();
        if (patch_entries.size())
            for(size_t pe{}; pe < patch_entries.size(); ++pe)
              if ( 0 == strcmp(patch_entries[pe].id, lbl.c_str())){
                patch_entries[pe].id[0] = char{}; //mark entry as free
                //cerr << "patch! patch_entries[pe].text_loc:" << patch_entries[pe].text_loc <<" vm.text().size()" << vm.text().size() << '\n' ;
                ceps::vm::oblectamenta::patch(vm.text(),patch_entries[pe].text_loc, vm.text().size());
              }

    }else if(is<Ast_node_kind::symbol>(e) && kind(as_symbol_ref(e)) == "OblectamentaOpcode" ){
        auto& mnemonic{name(as_symbol_ref(e))};
        auto it{ceps::vm::oblectamenta::mnemonics.find(mnemonic)};
        if (it == ceps::vm::oblectamenta::mnemonics.end()) 
         throw std::string{"oblectamenta_assembler: unknown opcode: '"+ mnemonic+"'" };
        auto v{it->second};
        if (get<2>(v)) get<2>(v)(vm.text());      
    } else if (is_a_symbol_with_arguments( e,sym_name,sym_kind,args)) {
        if (sym_kind == "OblectamentaOpcode"){
            auto& mnemonic{sym_name};
            std::string sym_name2;
	        std::string sym_kind2;
	        std::vector<node_t> args2;
            
            auto it{ceps::vm::oblectamenta::mnemonics.find(mnemonic)};
            if (it == ceps::vm::oblectamenta::mnemonics.end()) 
               throw std::string{"oblectamenta_assembler: unknown opcode: '"+ mnemonic+"'" };
            auto v{it->second};
            
            if (args.size() == 1 && is<Ast_node_kind::int_literal>(args[0])){
                auto arg{value(as_int_ref(args[0]))};
                auto loc_it{immediate2loc.find(arg)};
                size_t addr {};
                if (loc_it != immediate2loc.end()) addr = loc_it->second;
                else {
                    addr = vm.store(arg);
                    immediate2loc[arg] = addr;
                }
                    
                if (get<3>(v)) 
                 get<3>(v)(vm.text(),addr); 
                else throw std::string{"oblectamenta_assembler: illformed parameter list for '"+ mnemonic+"'" };
            } else if (args.size() == 1 && is<Ast_node_kind::symbol>(args[0]) && kind(as_symbol_ref(args[0]))=="OblectamentaDataLabel") {
                auto data_label_it{vm.data_labels().find(name(as_symbol_ref(args[0])))};
                if (data_label_it == vm.data_labels().end()) 
                    throw std::string{"oblectamenta_assembler: unknown data label: '"+ name(as_symbol_ref(args[0])) +"'" };
                if (get<3>(v)) 
                     get<3>(v)(vm.text(),data_label_it->second); 
                else throw std::string{"oblectamenta_assembler: illformed parameter list for '"+ mnemonic+"'" };
            } else if (args.size() == 1 && is<Ast_node_kind::symbol>(args[0]) && kind(as_symbol_ref(args[0]))=="OblectamentaCodeLabel") {
                auto code_label_it{codelabel2loc.find(name(as_symbol_ref(args[0])))};

                size_t loc{};
                size_t backpatch_loc{};
                bool backpatch{};

                if (code_label_it == codelabel2loc.end()){ 
                    //throw std::string{"oblectamenta_assembler: unknown code label: '"+ name(as_symbol_ref(args[0])) +"'" };
                    backpatch = true;
                } else loc = code_label_it->second; 

                if (get<3>(v)) 
                     backpatch_loc = get<3>(v)(vm.text(),loc); 
                else throw std::string{"oblectamenta_assembler: illformed parameter list for '"+ mnemonic+"'" };
                if (backpatch){
                    size_t pe{};
                    for(;pe < patch_entries.size() && patch_entries[pe].id[0] != 0; ++pe);
                    if (pe == patch_entries.size()) patch_entries.push_back({});
                    patch_entries[pe].text_loc = backpatch_loc;
                    strncpy(patch_entries[pe].id, name(as_symbol_ref(args[0])).c_str(), sizeof(patch_entry::id)); 
                    //std::cerr << pe << " " << patch_entries.size() << " loc= " << loc << " "<< patch_entries[pe].id <<'\n';
                }
            } else if (args.size() == 1 && is_a_symbol_with_arguments( args[0],sym_name2,sym_kind2,args2)){
                if (sym_kind2 == "OblectamentaModifier" && sym_name2 == "addr" && args2.size() == 1 && is<Ast_node_kind::int_literal>(args2[0]) ) {
                    if (get<3>(v)) 
                     get<3>(v)(vm.text(),value(as_int_ref(args2[0]))); 
                    else throw std::string{"oblectamenta_assembler: illformed parameter list for '"+ mnemonic+"'" };
                }
            }
        }
    } 
 } //for
}//function


}