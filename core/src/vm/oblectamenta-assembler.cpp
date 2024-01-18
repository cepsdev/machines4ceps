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


static std::optional<std::tuple<VMEnv::reg_t, VMEnv::reg_offs_t>> is_register_offset_expression(std::vector<ceps::ast::node_t> args){
    using namespace ceps::ast; using namespace std; using namespace ceps::vm::oblectamenta;
    if (args.size() == 1 && is<Ast_node_kind::symbol>(args[0]) && kind(as_symbol_ref(args[0]))=="OblectamentaReg"){
        return std::tuple<VMEnv::reg_t, VMEnv::reg_offs_t>{VMEnv::reg_t{VMEnv::registers_t{}.reg_mnemonic2idx[name(as_symbol_ref(args[0]))] }, VMEnv::reg_offs_t{}};
    } else if (args.size() == 1 
               && is<Ast_node_kind::binary_operator>(args[0]) 
               && is<Ast_node_kind::symbol>((as_binop_ref(args[0]).left()) )
               && kind(as_symbol_ref(as_binop_ref(args[0]).left()))=="OblectamentaReg"
               && is<Ast_node_kind::int_literal>((as_binop_ref(args[0]).right()) )
              )
    {
        VMEnv::reg_t reg{VMEnv::registers_t{}.reg_mnemonic2idx[name(as_symbol_ref(as_binop_ref(args[0]).left()))]};
        VMEnv::reg_offs_t reg_offs{value(as_int_ref(as_binop_ref(args[0]).right()))};
        if (op_val(as_binop_ref(args[0])) == "-") reg_offs= -reg_offs;
        return std::tuple<VMEnv::reg_t, VMEnv::reg_offs_t>{reg, reg_offs};        
    }
    return {};
}

void oblectamenta_assembler(ceps::vm::oblectamenta::VMEnv& vm, std::vector<ceps::ast::node_t> mnemonics)
{

 using namespace ceps::ast; using namespace std; using namespace ceps::vm::oblectamenta;

 map<int32_t,size_t> immediate2loc; // immediate values => location in storage
 map<string,size_t> codelabel2loc; // code label => location in storage
 size_t& text_loc = vm.text_loc;

 for (size_t stmt_pos{}; stmt_pos < mnemonics.size(); ++stmt_pos){

    if ( text_loc + max_opcode_width >= vm.text_size){
        vm.resize_text( assembler::text_growth_factor * (double)vm.text_size );
    }
      
    auto e{mnemonics[stmt_pos]};
    std::string sym_name;
	std::string sym_kind;
	std::vector<node_t> args;
    
    if(is<Ast_node_kind::symbol>(e) && kind(as_symbol_ref(e)) == "OblectamentaCodeLabel"){
        auto lbl{name(as_symbol_ref(e))};
        codelabel2loc[lbl] = text_loc;
        if (patch_entries.size())
            for(size_t pe{}; pe < patch_entries.size(); ++pe)
              if ( 0 == strcmp(patch_entries[pe].id, lbl.c_str())){
                patch_entries[pe].id[0] = char{}; //mark entry as free
                patch(vm,patch_entries[pe].text_loc - sizeof(addr_t), text_loc);
              }
    } else if(is<Ast_node_kind::symbol>(e) && kind(as_symbol_ref(e)) == "OblectamentaOpcode" ){
        auto& mnemonic{name(as_symbol_ref(e))};
        auto it{ceps::vm::oblectamenta::mnemonics.find(mnemonic)};
        if (it == ceps::vm::oblectamenta::mnemonics.end()) 
         throw std::string{"oblectamenta_assembler: unknown opcode: '"+ mnemonic+"'" };
        auto v{it->second};
        if (get<2>(v)) text_loc = get<2>(v)(vm,text_loc);      
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

            if (auto r = is_register_offset_expression(args)){
                auto it{ceps::vm::oblectamenta::mnemonics.find(mnemonic+"reg")};
                if (it == ceps::vm::oblectamenta::mnemonics.end()) 
                    throw std::string{"oblectamenta_assembler: unknown opcode: '"+ mnemonic+"reg'" };
                auto v{it->second};

                //std::cerr << "register_offset_expression (A) reg: "<<get<0>(*r)<< " offs:"<< get<1>(*r) << "\n";
                if (get<4>(v)) {
                 text_loc = get<4>(v)(vm,text_loc,get<0>(*r),get<1>(*r) );
                 //std::cerr << "register_offset_expression(B) reg: "<<get<0>(*r)<< " offs:"<< get<1>(*r) << "\n";
                }
            } else if (args.size() == 1 && is<Ast_node_kind::int_literal>(args[0])){
                auto arg{value(as_int_ref(args[0]))};
                auto loc_it{immediate2loc.find(arg)};
                size_t addr {};
                if (loc_it != immediate2loc.end()) addr = loc_it->second;
                else {
                    addr = vm.store(arg);
                    immediate2loc[arg] = addr;
                }
                    
                if (get<3>(v)) 
                 text_loc = get<3>(v)(vm,text_loc,addr); 
                else throw std::string{"oblectamenta_assembler: illformed parameter list for '"+ mnemonic+"'" };
            } else if (args.size() == 1 && is<Ast_node_kind::symbol>(args[0]) && kind(as_symbol_ref(args[0]))=="OblectamentaDataLabel") {
                auto data_label_it{vm.data_labels().find(name(as_symbol_ref(args[0])))};
                if (data_label_it == vm.data_labels().end()) 
                    throw std::string{"oblectamenta_assembler: unknown data label: '"+ name(as_symbol_ref(args[0])) +"'" };
                if (get<3>(v)) 
                     text_loc = get<3>(v)(vm,text_loc,data_label_it->second); 
                else throw std::string{"oblectamenta_assembler: illformed parameter list for '"+ mnemonic+"'" };
            } else if (args.size() == 1 && is<Ast_node_kind::symbol>(args[0]) && kind(as_symbol_ref(args[0]))=="OblectamentaCodeLabel") {
                auto label_name{name(as_symbol_ref(args[0]))};
                auto code_label_it{codelabel2loc.find(label_name)};

                size_t loc{};
                size_t backpatch_loc{};
                bool backpatch{};

                if (code_label_it == codelabel2loc.end()){ 
                    backpatch = true;
                } else loc = code_label_it->second; 

                if (get<3>(v)) 
                     backpatch_loc = text_loc = get<3>(v)(vm,text_loc,loc); 
                else throw std::string{"oblectamenta_assembler: illformed parameter list for '"+ mnemonic+"'" };
                if (backpatch){
                    size_t pe{};
                    for(;pe < patch_entries.size() && patch_entries[pe].id[0] != 0; ++pe);
                    if (pe == patch_entries.size()) patch_entries.push_back({});
                    patch_entries[pe].text_loc = backpatch_loc;
                    strncpy(patch_entries[pe].id, name(as_symbol_ref(args[0])).c_str(), sizeof(patch_entry::id)); 
                }
            } else if (args.size() == 1 && is_a_symbol_with_arguments( args[0],sym_name2,sym_kind2,args2)){
                if (sym_kind2 == "OblectamentaModifier" && sym_name2 == "addr" && args2.size() == 1 && is<Ast_node_kind::int_literal>(args2[0]) ) {
                    if (get<3>(v)) 
                     text_loc = get<3>(v)(vm,text_loc,value(as_int_ref(args2[0]))); 
                    else throw std::string{"oblectamenta_assembler: illformed parameter list for '"+ mnemonic+"'" };
                } else  if (sym_kind2 == "OblectamentaModifier" && 
                            sym_name2 == "reg" &&
                            args2.size() == 1 && 
                            is<Ast_node_kind::symbol>(args2[0]) &&
                            kind(as_symbol_ref(args2[0])) == "OblectamentaReg" ) {
                    string reg_name{name(as_symbol_ref(args2[0]))};
                    auto query_reg_it{vm.registers.reg_mnemonic2idx.find(reg_name)};
                    if (query_reg_it == vm.registers.reg_mnemonic2idx.end()) 
                     throw std::string{"oblectamenta_assembler: unknown register: '"+reg_name +"'" };
                    //INVARIANT: query_reg_it is valid, i.e. register name known
                    auto query_opcode_reg_version_it{ceps::vm::oblectamenta::mnemonics.find(mnemonic+"reg")};
                    if (query_opcode_reg_version_it == ceps::vm::oblectamenta::mnemonics.end()) 
                        throw std::string{"oblectamenta_assembler: Opcode doesn't support register access: '"+ mnemonic+"'" };
                    //INVARIANT: reg, opcode valid
                    auto op_code_entry{query_opcode_reg_version_it->second};
                    if (get<3>(op_code_entry)) text_loc = get<3>(op_code_entry)(vm,text_loc,query_reg_it->second); 
                    else throw std::string{"oblectamenta_assembler: illformed parameter list for '"+ mnemonic+"'" };
                }
            }
        }
    } 
 } //for
 
}//function


}