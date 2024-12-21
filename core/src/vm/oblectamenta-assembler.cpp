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

    if ( text_loc + 2*max_opcode_width >= vm.text_size){
        //vm.resize_text(8192);
        vm.resize_text(  2*max_opcode_width + assembler::text_growth_factor * (double)vm.text_size );
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
                //auto loc_it{immediate2loc.find(arg)};
                size_t addr {arg};
                //if (loc_it != immediate2loc.end()) addr = loc_it->second;
                //else {
                //   addr = vm.store(arg);
                //    immediate2loc[arg] = addr;
               // }
                    
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



namespace ceps::vm::oblectamenta::ast{

template<> bool check<ceps::vm::oblectamenta::VMEnv>(ceps::ast::Struct & s)
{
    using namespace ceps::ast;
    return name(s) == "vm";
}

template<> bool check<ser_wrapper_stack>(ceps::ast::Struct & s)
{
    using namespace ceps::ast;
    return name(s) == "stack";
}

template<> ser_wrapper_stack fetch<ser_wrapper_stack>(ceps::ast::Struct& s)
{
    OBLECTAMENTA_AST_PROC_PROLOGUE    
    ser_wrapper_stack r{make_shared<VMEnv>()};
    if(children(s).size()) 
    for(size_t i{children(s).size()}; i > 0; --i){
     auto e {children(s)[i-1]};
     if (is<Ast_node_kind::int_literal>(e)){ 
        auto v{value(as_int_ref(e))};
        r.vm->registers.file[VMEnv::registers_t::SP] -= sizeof(int32_t);
        *(int*)&(r.vm->mem.base[r.vm->registers.file[VMEnv::registers_t::SP]]) = v;
     } else if (is<Ast_node_kind::uint8>(e)){
        auto v{value(as_uint8_ref(e))};
        r.vm->registers.file[VMEnv::registers_t::SP] -= sizeof(v);
        *(decltype(v)*)&(r.vm->mem.base[r.vm->registers.file[VMEnv::registers_t::SP]]) = v;
     }
    }        
    return r;
}

template<> bool check<ser_wrapper_text>(ceps::ast::Struct & s)
{
    using namespace ceps::ast;
    return name(s) == "text";
}


template<> ser_wrapper_text fetch<ser_wrapper_text>(ceps::ast::Struct& s)
{
    OBLECTAMENTA_AST_PROC_PROLOGUE

    ser_wrapper_text r{make_shared<VMEnv>()};
    try{
        for(auto e : children(s))
         if (is<Ast_node_kind::structdef>(e) && name(as_struct_ref(e)) == "asm" ){
          ceps::vm::oblectamenta::oblectamenta_assembler(*r.vm,children(as_struct_ref(e)));
         } else if (is<Ast_node_kind::uint8>(e)){
            auto v{value(as_uint8_ref(e))};
            r.vm->text[r.vm->text_loc++] = v;
         }
    } catch (std::string const& msg){
        std::cerr << "***Error oblectamenta_assembler:" <<  msg << '\n' << '\n' <<"Erroneous segment:\n" << s << '\n' << '\n';
    }
    return r;
}

template<> ser_wrapper_text fetch<ser_wrapper_text>(ceps::ast::Struct& s, ceps::vm::oblectamenta::VMEnv& vm)
{
    OBLECTAMENTA_AST_PROC_PROLOGUE    
    ser_wrapper_text r{};

    size_t tot_size{};
    // compute total size
    for(auto e : children(s))
        if (is<Ast_node_kind::uint8>(e))
            ++tot_size;
    if (tot_size > vm.text_size) vm.resize_text(tot_size);

    try{
        for(auto e : children(s))
         if (is<Ast_node_kind::structdef>(e) && name(as_struct_ref(e)) == "asm" ){
          ceps::vm::oblectamenta::oblectamenta_assembler(vm,children(as_struct_ref(e)));
         } else if (is<Ast_node_kind::uint8>(e)){
            auto v{value(as_uint8_ref(e))};
            vm.text[vm.text_loc++] = v;
         }
    } catch (std::string const& msg){
        std::cerr << "***Error oblectamenta_assembler:" <<  msg << '\n' << '\n' <<"Erroneous segment:\n" << s << '\n' << '\n';
    }
    return r;
}

template<> bool check<ser_wrapper_data>(ceps::ast::Struct & s)
{
    using namespace ceps::ast;
    return name(s) == "data";
}

template<> ser_wrapper_data fetch<ser_wrapper_data>(ceps::ast::Struct& s)
{
    OBLECTAMENTA_AST_PROC_PROLOGUE    
    ser_wrapper_data r{make_shared<VMEnv>()};
    for(auto e: children(s)){
        if (is<Ast_node_kind::int_literal>(e)) r.vm->store(value(as_int_ref(e)));
        else if (is<Ast_node_kind::float_literal>(e)) r.vm->store(value(as_double_ref(e)));
        else if (is<Ast_node_kind::string_literal>(e)) r.vm->store(value(as_string_ref(e)));
        else if (is<Ast_node_kind::symbol>(e) && kind(as_symbol_ref(e))=="OblectamentaDataLabel"){
                r.vm->data_labels()[name(as_symbol_ref(e))] = r.vm->mem.heap - r.vm->mem.base;
        } else if (is<Ast_node_kind::uint8>(e)){
            auto v{value(as_uint8_ref(e))};
            r.vm->store(v);
        }
    }        
    return r;
}

template<> bool check<ser_wrapper_cstack>(ceps::ast::Struct & s)
{
    using namespace ceps::ast;
    return name(s) == "compute_stack";
}

template<> ser_wrapper_cstack fetch<ser_wrapper_cstack>(ceps::ast::Struct& s)
{
    OBLECTAMENTA_AST_PROC_PROLOGUE    
    ser_wrapper_cstack r{make_shared<VMEnv>()};
    for(auto e: children(s)){
        if (is<Ast_node_kind::int_literal>(e)) r.vm->push_cs(value(as_int_ref(e)));
        else if (is<Ast_node_kind::float_literal>(e)) r.vm->push_cs(value(as_double_ref(e)));
        else if (is<Ast_node_kind::string_literal>(e)) r.vm->push_cs(value(as_string_ref(e)));
        else if (is<Ast_node_kind::uint8>(e)) r.vm->push_cs(value(as_uint8_ref(e)));
    }
    return r;
}

template<> bool check<ceps::vm::oblectamenta::VMEnv::registers_t>(ceps::ast::Struct & s)
{
    using namespace ceps::ast;
    return name(s) == "registers";
}

template<> ceps::vm::oblectamenta::VMEnv::registers_t fetch<ceps::vm::oblectamenta::VMEnv::registers_t>(ceps::ast::Struct& s)
{
    OBLECTAMENTA_AST_PROC_PROLOGUE
    VMEnv r{};
    auto regs{r.registers};

    for (auto e : children(s)){
     if (!is<Ast_node_kind::structdef>(e)) continue;
     auto it{regs.reg_mnemonic2idx.find(name(*as_struct_ptr(e)))};
     if (it == regs.reg_mnemonic2idx.end()) continue;
     if (!children(*as_struct_ptr(e)).size()) continue;
     if (is<Ast_node_kind::int_literal>(children(*as_struct_ptr(e))[0]) )
      regs.file[it->second] = value(as_int_ref(children(*as_struct_ptr(e))[0]));
     else if (is<Ast_node_kind::long_literal>(children(*as_struct_ptr(e))[0]))
      regs.file[it->second] = value(as_int64_ref(children(*as_struct_ptr(e))[0]));     
    }
   return regs;
}

template<> ceps::vm::oblectamenta::VMEnv fetch<ceps::vm::oblectamenta::VMEnv>(ceps::ast::Struct& s)
{
    OBLECTAMENTA_AST_PROC_PROLOGUE
    VMEnv r{};
    optional<Struct*> st;

    for (auto e : children(s)){
     if (!is<Ast_node_kind::structdef>(e)) continue;
     if (name(*as_struct_ptr(e)) == "stack") {
        auto stack_opt{read_value<ser_wrapper_stack>(*as_struct_ptr(e))};
        if (!stack_opt) continue;
        copy_stack( *(*stack_opt).vm, r );
     } 
     else if (name(*as_struct_ptr(e)) == "data" ){
        auto data_opt{read_value<ser_wrapper_data>(*as_struct_ptr(e))};
        if (!data_opt) continue;
        copy_data( *(*data_opt).vm, r );             
     }
     else if ( name(*as_struct_ptr(e)) == "compute_stack") {
        auto cs_opt{read_value<ser_wrapper_cstack>(*as_struct_ptr(e))};
        if (!cs_opt) continue;
        copy_compute_stack( *(*cs_opt).vm, r );
     } else if ( name(*as_struct_ptr(e)) == "text") {
        auto text_opt{read_value<ser_wrapper_text>(*as_struct_ptr(e),r)};
        if (!text_opt) continue;
        /*r.text = (*text_opt).vm->text;
        r.text_loc = (*text_opt).vm->text_loc;
        (*text_opt).vm->text = nullptr;*/
     } 
 
    }
   return r;
}

template<> ceps::ast::node_t ast_rep (ser_wrapper_stack stack, ceps::vm::oblectamenta::VMEnv& vm ){
    OBLECTAMENTA_AST_PROC_PROLOGUE    
    auto result = mk_struct("stack");
    auto& ch {children(*result)};
    auto mem_size{vm.mem.end -vm.mem.base};
    for (ssize_t i = 0; i < mem_size  - vm.registers.file[VMEnv::registers_t::SP]; ++i )
        ch.push_back(mk_uint8( vm.mem.base[vm.registers.file[VMEnv::registers_t::SP] + i] ));
    return result;
}

template<> ceps::ast::node_t ast_rep (ser_wrapper_data data, ceps::vm::oblectamenta::VMEnv& vm){
    OBLECTAMENTA_AST_PROC_PROLOGUE
    vector< pair<string, size_t> > lbls{vm.data_labels().rbegin(), vm.data_labels().rend()};
    sort(lbls.begin(), lbls.end(), [](pair<string, size_t> const & lhs, pair<string, size_t> const & rhs ){return lhs.second < rhs.second;});
  
    auto result = mk_struct("data");
    auto& ch {children(*result)};
    size_t static_mem_size{ (size_t)(vm.mem.heap -vm.mem.base)};
    size_t cur_lbl = 0;
    for (size_t i = 0; i < static_mem_size; ++i ){
         while(cur_lbl < lbls.size() && lbls[cur_lbl].second == i){
            ch.push_back(ceps::ast::mk_symbol(lbls[cur_lbl].first,"OblectamentaDataLabel"));
            ++cur_lbl;
        } 
        ch.push_back(mk_uint8( vm.mem.base[i] ));
    }
    return result;
}


template<> ceps::ast::node_t ast_rep (ser_wrapper_cstack cstack, ceps::vm::oblectamenta::VMEnv& vm ){
    OBLECTAMENTA_AST_PROC_PROLOGUE    
    auto result = mk_struct("compute_stack");
    auto& ch {children(*result)};
    for(size_t i = 0; i < (size_t)vm.registers.file[VMEnv::registers_t::CSP] ; ++i)
     ch.push_back(mk_uint8(vm.compute_stack[i]));
    return result;
}

// AST representation ser_wrapper_text

template<> ceps::ast::node_t ast_rep (ser_wrapper_text text, ceps::vm::oblectamenta::VMEnv& vm ){
    OBLECTAMENTA_AST_PROC_PROLOGUE    
    auto result = mk_struct("text");
    auto& ch {children(*result)};
    for (size_t loc = 0; loc < vm.text_loc; ++loc)
     ch.push_back(mk_uint8(vm.text[loc]));
    return result;
}

template<> ceps::ast::node_t ast_rep (ser_wrapper_text text){
    OBLECTAMENTA_AST_PROC_PROLOGUE    
    auto result = mk_struct("text");
    return result;
}

// AST representation ceps::vm::oblectamenta::VMEnv::registers_t
template<> ceps::ast::node_t ast_rep (ceps::vm::oblectamenta::VMEnv::registers_t regs){
    OBLECTAMENTA_AST_PROC_PROLOGUE   
    auto result = mk_struct("registers");
    auto& ch {children(*result)};
    for (auto reg : regs.reg_mnemonic2idx)
    {
        auto t{mk_struct(reg.first)}; ch.push_back(t);
        children(*t).push_back(mk_int64_node(regs.file[reg.second]));
    }
    return result;
}

// AST representation ceps::vm::oblectamenta::VMEnv
template<> ceps::ast::node_t ast_rep<ceps::vm::oblectamenta::VMEnv&> (ceps::vm::oblectamenta::VMEnv& vm){
    OBLECTAMENTA_AST_PROC_PROLOGUE
    
    auto result = mk_struct("vm");
    children(*result).push_back(ast_rep(ser_wrapper_stack{},vm));
    children(*result).push_back(ast_rep(ser_wrapper_data{}, vm));
    children(*result).push_back(ast_rep(ser_wrapper_text{},vm));
    children(*result).push_back(ast_rep(ser_wrapper_cstack{}, vm));
    children(*result).push_back(ast_rep(vm.registers)); 
    return result;
}

template<> ceps::ast::node_t ast_rep<ceps::vm::oblectamenta::VMEnv> (ceps::vm::oblectamenta::VMEnv vm){
    OBLECTAMENTA_AST_PROC_PROLOGUE
    
    auto result = mk_struct("vm");
    children(*result).push_back(ast_rep(ser_wrapper_stack{},vm));
    children(*result).push_back(ast_rep(ser_wrapper_data{}, vm));
    children(*result).push_back(ast_rep(ser_wrapper_text{}, vm));
    children(*result).push_back(ast_rep(ser_wrapper_cstack{}, vm));
    children(*result).push_back(ast_rep(vm.registers));
    return result;
}
}