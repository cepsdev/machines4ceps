/*
Copyright 2023 Tomas Prerovsky (cepsdev@hotmail.com).

Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to iswp128b8n writing, software
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
#include <sstream>


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

static string escape_str(string const & s)
{
	string result;
	for(unsigned int i = 0; i != s.length();++i)
	{
		if (s[i] == '\n')
			result+="\\n";
		else if (s[i] == '\\')
			result+="\\\\";
		else if (s[i] == '\t')
			result+="\\t";
		else if (s[i] == '\r')
			result+="\\r";
		else if (s[i] == '"')
			result+="\\\"";
		else if (std::isprint(s[i]))
			result += s[i];
	}
	return result;
}


static size_t find_trailing_zero(char* buffer, size_t size)
{
    size_t r{};
    for (;r < size && buffer[r]; ++r);
    return r;
}

size_t deserialize_event_payload(char* buffer, size_t size, std::string& res){
    string r;
    if (size == 0) return {};
    
    if (size < sizeof(msg_node)) return {};

    msg_node& root{ *(msg_node*)buffer };
    size_t len_extra_info{};
    if (root.what == msg_node::NODE){
        auto t = find_trailing_zero(buffer + sizeof(msg_node), size - sizeof(msg_node));
        len_extra_info = t + 1;
    }

    auto hd_size = sizeof(msg_node) + len_extra_info;
    
    if (root.size <  hd_size) return 0;
    auto content_size = root.size - hd_size;
    size_t consumed_content_bytes{};
    
    if (root.what == msg_node::ROOT || root.what == msg_node::NODE){
        string prefix,suffix;
        if (root.what == msg_node::NODE)
        {
            prefix = string{"\""}+ (char*)((msg_node_ex*)buffer)->name +"\":";
        } else {
            prefix = "{";
            suffix = "}";
        }
        string inner;
        bool contains_nodes {};
        
        if (content_size){
            for (;consumed_content_bytes < content_size;){
                msg_node& n{ *(msg_node*)(buffer + hd_size + consumed_content_bytes)};
                string t;
                contains_nodes |= (n.what == msg_node::NODE); 
                consumed_content_bytes += 
                 deserialize_event_payload(buffer+hd_size+consumed_content_bytes, content_size - consumed_content_bytes, t);
                inner += t;
                if (consumed_content_bytes < content_size )inner += ",";
            }
        }
        if (root.what == msg_node::NODE ) 
         if (inner.size() == 0) inner = "{}";
         else if (contains_nodes) inner = "{" + inner + "}"; 
        res = prefix  + inner + suffix;   
    } else if (root.what == msg_node::INT32){
        msg_node_int32& m{ *(msg_node_int32*)&root};
        stringstream ss;
        ss << m.value;
        res = ss.str();
        return sizeof(msg_node_int32);
    } else if (root.what == msg_node::INT64){
        msg_node_int64& m{ *(msg_node_int64*)&root};
        stringstream ss;
        ss << m.value;
        res = ss.str();
        return sizeof(msg_node_int64);
    } else if (root.what == msg_node::F64){
        msg_node_f64& m{ *(msg_node_f64*)&root};
        stringstream ss;
        ss << m.value;
        res = ss.str();
        return sizeof(msg_node_f64);
    } else if (root.what == msg_node::SZ){
        msg_node_sz& m{ *(msg_node_sz*)&root};
        res = "\"" + escape_str(m.value)+ "\"";
        return sizeof(msg_node) + res.size() + 1;
    }
    return hd_size + content_size;
}

static bool is_a_msgdefdirective(ceps::ast::node_t n){
    using namespace ceps::ast; using namespace std; using namespace ceps::vm::oblectamenta;
    return is<Ast_node_kind::structdef>(n) && children(as_struct_ref(n)).size() && 
    children(as_struct_ref(n))[0] && is<Ast_node_kind::symbol>(children(as_struct_ref(n))[0]) &&  
    "OblectamentaMsgDefDirective" == kind(as_symbol_ref(children(as_struct_ref(n))[0]));
}

static bool is_a_msgreaddirective(ceps::ast::node_t n){
    using namespace ceps::ast; using namespace std; using namespace ceps::vm::oblectamenta;
    bool is_msgreaddir = is<Ast_node_kind::structdef>(n) && children(as_struct_ref(n)).size() && 
    children(as_struct_ref(n))[0] && is<Ast_node_kind::symbol>(children(as_struct_ref(n))[0]) &&  
    "OblectamentaMsgReadDirective" == kind(as_symbol_ref(children(as_struct_ref(n))[0]));
    return is_msgreaddir;
}

static std::optional<std::string> static_mem_location(ceps::vm::oblectamenta::VMEnv& vm, ceps::ast::node_t msg_directive){
    using namespace ceps::ast; using namespace std; using namespace ceps::vm::oblectamenta;
    for (auto e: children(as_struct_ref(msg_directive)))
        if(is<Ast_node_kind::symbol>(e) && "OblectamentaDataLabel" == kind(as_symbol_ref(e))){
            auto data_label_it{vm.data_labels().find(name(as_symbol_ref(e)))};
            if (data_label_it != vm.data_labels().end()) return data_label_it->first;
        }
    return {};
}

static std::optional<std::vector<ceps::ast::node_t>> global_error_handler(ceps::ast::node_t msg_directive){
    using namespace ceps::ast; using namespace std; using namespace ceps::vm::oblectamenta;
    for (auto e: children(as_struct_ref(msg_directive)))
        if(is<Ast_node_kind::structdef>(e)){
            for (auto ee: children(as_struct_ref(e)))         
             if(is<Ast_node_kind::symbol>(ee) && "OblectamentaMsgOnError" == kind(as_symbol_ref(ee))){
                std::vector<ceps::ast::node_t> r;
                for (auto eee: children(as_struct_ref(e)))
                  if(!is<Ast_node_kind::symbol>(eee) || "OblectamentaMsgOnError" != kind(as_symbol_ref(eee)))
                   r.push_back(eee);
                return r;
             }
        }
    return {};
}

static ceps::ast::node_t gen_mnemonic(std::string name){
    using namespace ceps::ast; 
    return mk_symbol(name, "OblectamentaOpcode");
} 

static ceps::ast::node_t gen_mnemonic(std::string name, int v){
    using namespace ceps::ast; 
    return mk_func_call(mk_symbol(name, "OblectamentaOpcode"), ceps::interpreter::mk_int_node(v));
} 

static ceps::ast::node_t gen_mnemonic_sym_arg(std::string name, std::string sym_name, std::string sym_kind){
    using namespace ceps::ast; 
    return mk_func_call(mk_symbol(name, "OblectamentaOpcode"), mk_symbol(sym_name, sym_kind));
} 


//emit_mnemonic_with_sym_arg

static std::vector<ceps::ast::node_t>& emwsa(std::vector<ceps::ast::node_t>& r,std::string name, std::string sym, std::string kind){
    r.push_back(gen_mnemonic_sym_arg(name,sym,kind));
    return r;
}

//emit_mnemonic_with_arg

static std::vector<ceps::ast::node_t>& emwa(std::vector<ceps::ast::node_t>& r, std::string name, int v){
    r.push_back(gen_mnemonic(name, v));
    return r;
}


//emit_mnemonic

static std::vector<ceps::ast::node_t>& em(std::vector<ceps::ast::node_t>& r, ceps::ast::node_t n){
    r.push_back(n);
    return r;
}

static std::vector<ceps::ast::node_t>& em(std::vector<ceps::ast::node_t>& r, std::string name){
    r.push_back(gen_mnemonic(name));
    return r;
}

static std::vector<ceps::ast::node_t>& em(std::vector<ceps::ast::node_t>& r, std::string name, int v){
    return emwa(r, name, v);
}

static std::vector<ceps::ast::node_t>& emlbl(std::vector<ceps::ast::node_t>& r, std::string name){
    using namespace ceps::ast;
    r.push_back(mk_symbol(name,"OblectamentaCodeLabel"));
    return r;
}

static void oblectamenta_assembler_preproccess (ceps::vm::oblectamenta::VMEnv& vm,
     ceps::ast::node_t mnemonic,
     std::vector<ceps::ast::node_t>& r){
    using namespace ceps::ast; using namespace std; using namespace ceps::vm::oblectamenta;
    //INVARIANT: ARG0 contains destination address
    //result.push_back(gen_mnemonic("ldi64", 8));
    //if (!is<Ast_node_kind::structdef>(mnemonic)) return;

    string node_name{};
    if (is<Ast_node_kind::structdef>(mnemonic)) node_name = name(as_struct_ref(mnemonic));
    vector<node_t> chldrn;
    if (is<Ast_node_kind::structdef>(mnemonic)) chldrn = children(as_struct_ref(mnemonic));
    else chldrn = children(as_scope_ref(mnemonic));
    
    const auto addr_node_name = vm.mem.heap - vm.mem.base; 
    vm.store(node_name);vm.store('\0');
    
        r.push_back(gen_mnemonic("ldi32", msg_node::NODE));
        r.push_back(gen_mnemonic_sym_arg("ldi64","ARG0","OblectamentaReg"));
        r.push_back(gen_mnemonic("stsi32")); 
        r.push_back(gen_mnemonic("ldi64", sizeof(msg_node) + node_name.length() + 1));
        r.push_back(gen_mnemonic_sym_arg("ldi64","ARG0","OblectamentaReg"));
        r.push_back(gen_mnemonic("ldi64", sizeof(msg_node::what)));
        r.push_back(gen_mnemonic("addi64"));
        r.push_back(gen_mnemonic("stsi64"));
        //lbl_counter
        r.push_back(gen_mnemonic("ldi64", 0));
        //copy node name into message header
        string lbl = string{"__msgdef_"} +to_string(vm.lbl_counter++);               // || Copy routine for zero terminated strings
        r.push_back(mk_symbol(lbl,"OblectamentaCodeLabel"));                         // |
        r.push_back(gen_mnemonic("duptopi64"));                                      // |
        r.push_back(gen_mnemonic("duptopi64"));                                      // |
        
        
        r.push_back(gen_mnemonic("ldi64", addr_node_name));                          // |
        r.push_back(gen_mnemonic("addi64"));                                         // |
        r.push_back(gen_mnemonic("swpi64"));                                         // |
        r.push_back(gen_mnemonic_sym_arg("ldi64","ARG0","OblectamentaReg"));         // |
        r.push_back(gen_mnemonic("ldi64", sizeof(msg_node)));                        // |
        r.push_back(gen_mnemonic("addi64"));                                         // |
        r.push_back(gen_mnemonic("addi64"));                                         // |
        r.push_back(gen_mnemonic("swpi64"));                                         // |
        r.push_back(gen_mnemonic("ldsi8"));                                          // |
        r.push_back(gen_mnemonic("duptopi8"));                                       // |
        r.push_back(gen_mnemonic("swpi16i64"));                                      // |
        r.push_back(gen_mnemonic("stsi8"));                                          // |
        r.push_back(gen_mnemonic("ui8toui64"));                                      // |
        r.push_back(gen_mnemonic("swpi64"));                                         // |
        r.push_back(gen_mnemonic("ldi64", 1));                                       // |
        r.push_back(gen_mnemonic("addi64"));                                         // |
        r.push_back(gen_mnemonic("swpi64"));                                         // |
        r.push_back(gen_mnemonic_sym_arg("bnzeroi64",lbl,"OblectamentaCodeLabel"));  // |
        r.push_back(gen_mnemonic("discardtopi64"));

         //Create stack frame  
         r.push_back(gen_mnemonic_sym_arg("pushi64","FP","OblectamentaReg"));          // pushi64(FP);----|
         r.push_back(gen_mnemonic_sym_arg("ldi64","SP","OblectamentaReg"));            // ldi64(SP);      |
         r.push_back(gen_mnemonic_sym_arg("sti64","FP","OblectamentaReg"));            // sti64(FP);      |-------------------------------------| FP <- SP
         constexpr int rel_addr_root = sizeof(VMEnv::registers_t::file[0]);
         constexpr int rel_addr_content_size = rel_addr_root + sizeof(addr_t);

         r.push_back(gen_mnemonic("ldi64", sizeof(addr_t) + sizeof(msg_node::size)));  // ldi64(sizeof(addr_t) + sizeof(msg_node::size));   ----|
         r.push_back(gen_mnemonic_sym_arg("ldi64","SP","OblectamentaReg"));            // ldi64(SP);      |-------------------------------------|
         r.push_back(gen_mnemonic("subi64"));                                          // subi64;         |
         r.push_back(gen_mnemonic_sym_arg("sti64","SP","OblectamentaReg"));            // sti64(SP);      | SP <- SP - 8 // make room for address to root


         // [FP - rel_addr_content_size] = 0
         r.push_back(gen_mnemonic("ldi64", 0));
         r.push_back(gen_mnemonic("ldi64", rel_addr_content_size));                       
         r.push_back(gen_mnemonic_sym_arg("ldi64","FP","OblectamentaReg"));             
         r.push_back(gen_mnemonic("subi64"));                                                   
         r.push_back(gen_mnemonic("stsi64"));                                          

         //[FP - 8] <-  & msg_node
         r.push_back(gen_mnemonic_sym_arg("ldi64","ARG0","OblectamentaReg"));
         r.push_back(gen_mnemonic("ldi64", rel_addr_root));                       
         r.push_back(gen_mnemonic_sym_arg("ldi64","FP","OblectamentaReg"));             
         r.push_back(gen_mnemonic("subi64"));                                                   
         r.push_back(gen_mnemonic("stsi64"));
                                                   
         for(auto child: chldrn ){
            if (is<Ast_node_kind::structdef>(child) || is<Ast_node_kind::scope>(child) ){
             r.push_back(gen_mnemonic("ldi64", rel_addr_root));                       
             r.push_back(gen_mnemonic_sym_arg("ldi64","FP","OblectamentaReg"));             
             r.push_back(gen_mnemonic("subi64"));
             r.push_back(gen_mnemonic("ldsi64"));
             r.push_back(gen_mnemonic("ldi64", sizeof(msg_node) + node_name.length() + 1));
             r.push_back(gen_mnemonic("addi64"));
             r.push_back(gen_mnemonic("ldi64", rel_addr_content_size));
             

             r.push_back(gen_mnemonic_sym_arg("ldi64","FP","OblectamentaReg"));
             r.push_back(gen_mnemonic("subi64"));
             r.push_back(gen_mnemonic("ldsi64"));
             r.push_back(gen_mnemonic("addi64"));
             
             r.push_back(gen_mnemonic_sym_arg("sti64","ARG0","OblectamentaReg"));
             oblectamenta_assembler_preproccess(vm,child,r);
             r.push_back(gen_mnemonic_sym_arg("ldi64","RES","OblectamentaReg"));
             r.push_back(gen_mnemonic("ldi64", rel_addr_content_size));
             r.push_back(gen_mnemonic_sym_arg("ldi64","FP","OblectamentaReg"));
             r.push_back(gen_mnemonic("subi64"));
             r.push_back(gen_mnemonic("ldsi64"));
             r.push_back(gen_mnemonic("addi64"));
             r.push_back(gen_mnemonic("ldi64", rel_addr_content_size));
             r.push_back(gen_mnemonic_sym_arg("ldi64","FP","OblectamentaReg"));
             r.push_back(gen_mnemonic("subi64"));
             r.push_back(gen_mnemonic("stsi64")); // [FP - rel_addr_content_size] += RES

            } else if (is<Ast_node_kind::symbol>(child) && (kind(as_symbol_ref(child)) == "OblectamentaMessageTag")){
                if ( "i32" == name (as_symbol_ref(child)) ){
                  r.push_back(gen_mnemonic("ldi64", rel_addr_root));                       
                  r.push_back(gen_mnemonic_sym_arg("ldi64","FP","OblectamentaReg"));             
                  r.push_back(gen_mnemonic("subi64"));
                  r.push_back(gen_mnemonic("ldsi64"));
                  r.push_back(gen_mnemonic("ldi64", sizeof(msg_node) + node_name.length() + 1));
                  r.push_back(gen_mnemonic("addi64"));
                  r.push_back(gen_mnemonic("ldi64", rel_addr_content_size));
                  r.push_back(gen_mnemonic_sym_arg("ldi64","FP","OblectamentaReg"));
                  r.push_back(gen_mnemonic("subi64"));
                  r.push_back(gen_mnemonic("ldsi64"));
                  r.push_back(gen_mnemonic("addi64")); //next free offset on cs
                  r.push_back(gen_mnemonic("duptopi64"));
                  r.push_back(gen_mnemonic("ldi32", msg_node::INT32));
                  r.push_back(gen_mnemonic("swpi32i64"));
                  r.push_back(gen_mnemonic("stsi32")); // node type written
                  r.push_back(gen_mnemonic("duptopi64"));
                  r.push_back(gen_mnemonic("ldi64", sizeof(msg_node::what)));
                  r.push_back(gen_mnemonic("addi64"));
                  r.push_back(gen_mnemonic("ldi64", sizeof(msg_node_int32)));
                  r.push_back(gen_mnemonic("swpi64"));
                  r.push_back(gen_mnemonic("stsi64"));
                  r.push_back(gen_mnemonic("ldi64", sizeof(msg_node)));
                  r.push_back(gen_mnemonic("addi64"));
                  
                  r.push_back(gen_mnemonic("stsi32"));
                  r.push_back(gen_mnemonic("ldi64", sizeof(msg_node_int32)));
                  r.push_back(gen_mnemonic("ldi64", rel_addr_content_size));
                  r.push_back(gen_mnemonic_sym_arg("ldi64","FP","OblectamentaReg"));
                  r.push_back(gen_mnemonic("subi64"));
                  r.push_back(gen_mnemonic("ldsi64"));
                  r.push_back(gen_mnemonic("addi64"));
                  r.push_back(gen_mnemonic("ldi64", rel_addr_content_size));
                  r.push_back(gen_mnemonic_sym_arg("ldi64","FP","OblectamentaReg"));
                  r.push_back(gen_mnemonic("subi64"));
                  r.push_back(gen_mnemonic("stsi64")); // [FP - rel_addr_content_size] += RES     
                } else if ( "i64" == name (as_symbol_ref(child)) ){
                    r.push_back(gen_mnemonic("ldi64", rel_addr_root));                       
                    r.push_back(gen_mnemonic_sym_arg("ldi64","FP","OblectamentaReg"));             
                    r.push_back(gen_mnemonic("subi64"));
                    r.push_back(gen_mnemonic("ldsi64"));
                    r.push_back(gen_mnemonic("ldi64", sizeof(msg_node) + node_name.length() + 1));
                    r.push_back(gen_mnemonic("addi64"));
                    r.push_back(gen_mnemonic("ldi64", rel_addr_content_size));
                    r.push_back(gen_mnemonic_sym_arg("ldi64","FP","OblectamentaReg"));
                    r.push_back(gen_mnemonic("subi64"));
                    r.push_back(gen_mnemonic("ldsi64"));
                    r.push_back(gen_mnemonic("addi64")); //next free offset on cs
                    r.push_back(gen_mnemonic("duptopi64"));
                    r.push_back(gen_mnemonic("ldi32", msg_node::INT64));
                    r.push_back(gen_mnemonic("swpi32i64"));
                    r.push_back(gen_mnemonic("stsi32")); // node type written
                    r.push_back(gen_mnemonic("duptopi64"));
                    r.push_back(gen_mnemonic("ldi64", sizeof(msg_node::what)));
                    r.push_back(gen_mnemonic("addi64"));
                    r.push_back(gen_mnemonic("ldi64", sizeof(msg_node_int64)));
                    r.push_back(gen_mnemonic("swpi64"));
                    r.push_back(gen_mnemonic("stsi64"));
                    r.push_back(gen_mnemonic("ldi64", sizeof(msg_node)));
                    r.push_back(gen_mnemonic("addi64"));
                    r.push_back(gen_mnemonic("stsi64"));
                    r.push_back(gen_mnemonic("ldi64", sizeof(msg_node_int64)));
                    r.push_back(gen_mnemonic("ldi64", rel_addr_content_size));
                    r.push_back(gen_mnemonic_sym_arg("ldi64","FP","OblectamentaReg"));
                    r.push_back(gen_mnemonic("subi64"));
                    r.push_back(gen_mnemonic("ldsi64"));
                    r.push_back(gen_mnemonic("addi64"));
                    r.push_back(gen_mnemonic("ldi64", rel_addr_content_size));
                    r.push_back(gen_mnemonic_sym_arg("ldi64","FP","OblectamentaReg"));
                    r.push_back(gen_mnemonic("subi64"));
                    r.push_back(gen_mnemonic("stsi64")); // [FP - rel_addr_content_size] += RES 
                 } else if ( "f64" == name (as_symbol_ref(child)) ){
                    r.push_back(gen_mnemonic("ldi64", rel_addr_root));                       
                    r.push_back(gen_mnemonic_sym_arg("ldi64","FP","OblectamentaReg"));             
                    r.push_back(gen_mnemonic("subi64"));
                    r.push_back(gen_mnemonic("ldsi64"));
                    r.push_back(gen_mnemonic("ldi64", sizeof(msg_node) + node_name.length() + 1));
                    r.push_back(gen_mnemonic("addi64"));
                    r.push_back(gen_mnemonic("ldi64", rel_addr_content_size));
                    r.push_back(gen_mnemonic_sym_arg("ldi64","FP","OblectamentaReg"));
                    r.push_back(gen_mnemonic("subi64"));
                    r.push_back(gen_mnemonic("ldsi64"));
                    r.push_back(gen_mnemonic("addi64")); //next free offset on cs
                    r.push_back(gen_mnemonic("duptopi64"));
                    r.push_back(gen_mnemonic("ldi32", msg_node::F64));
                    r.push_back(gen_mnemonic("swpi32i64"));
                    r.push_back(gen_mnemonic("stsi32")); // node type written
                    r.push_back(gen_mnemonic("duptopi64"));
                    r.push_back(gen_mnemonic("ldi64", sizeof(msg_node::what)));
                    r.push_back(gen_mnemonic("addi64"));
                    r.push_back(gen_mnemonic("ldi64", sizeof(msg_node_f64)));
                    r.push_back(gen_mnemonic("swpi64"));
                    r.push_back(gen_mnemonic("stsi64"));
                    r.push_back(gen_mnemonic("ldi64", sizeof(msg_node)));
                    r.push_back(gen_mnemonic("addi64"));
                    r.push_back(gen_mnemonic("stsdbl"));
                    r.push_back(gen_mnemonic("ldi64", sizeof(msg_node_f64)));
                    r.push_back(gen_mnemonic("ldi64", rel_addr_content_size));
                    r.push_back(gen_mnemonic_sym_arg("ldi64","FP","OblectamentaReg"));
                    r.push_back(gen_mnemonic("subi64"));
                    r.push_back(gen_mnemonic("ldsi64"));
                    r.push_back(gen_mnemonic("addi64"));
                    r.push_back(gen_mnemonic("ldi64", rel_addr_content_size));
                    r.push_back(gen_mnemonic_sym_arg("ldi64","FP","OblectamentaReg"));
                    r.push_back(gen_mnemonic("subi64"));
                    r.push_back(gen_mnemonic("stsi64")); // [FP - rel_addr_content_size] += RES 
                 } else if ( "sz" == name (as_symbol_ref(child)) ){
                  r.push_back(gen_mnemonic("ldi64", rel_addr_root));                       
                  r.push_back(gen_mnemonic_sym_arg("ldi64","FP","OblectamentaReg"));             
                  r.push_back(gen_mnemonic("subi64"));
                  r.push_back(gen_mnemonic("ldsi64"));

                  r.push_back(gen_mnemonic("ldi64", sizeof(msg_node) + node_name.length() + 1));
                  r.push_back(gen_mnemonic("addi64"));

                  r.push_back(gen_mnemonic("ldi64", rel_addr_content_size));
                  r.push_back(gen_mnemonic_sym_arg("ldi64","FP","OblectamentaReg"));
                  r.push_back(gen_mnemonic("subi64"));
                  r.push_back(gen_mnemonic("ldsi64"));
                  r.push_back(gen_mnemonic("addi64")); //next free offset on cs

                  r.push_back(gen_mnemonic("duptopi64"));
                  r.push_back(gen_mnemonic("ldi32", msg_node::SZ));
                  r.push_back(gen_mnemonic("swpi32i64"));
                  r.push_back(gen_mnemonic("stsi32")); // node type written
                  r.push_back(gen_mnemonic("ldi64", sizeof(msg_node)));
                  r.push_back(gen_mnemonic("addi64"));
                  
                  
                  // source addr | destination addr
                  r.push_back(gen_mnemonic("swpi64"));
                  // destination addr | source addr
                  r.push_back(gen_mnemonic("duptopi64"));
                  // destination addr | source addr | source addr
                  r.push_back(gen_mnemonic("swp128i64"));
                  // source addr | source addr | destination addr
                  string lbl = string{"__msgdef_"} +to_string(vm.lbl_counter++);               // || Copy routine for zero terminated strings
                  r.push_back(mk_symbol(lbl,"OblectamentaCodeLabel"));                         // |
                  r.push_back(gen_mnemonic("swpi64"));
                  r.push_back(gen_mnemonic("duptopi64"));
                  r.push_back(gen_mnemonic("ldsi8"));
                  // source addr | destination addr | source addr | *((int8_t*)source addr)

                  r.push_back(gen_mnemonic("duptopi8"));
                  // source addr | destination addr | source addr | *((int8_t*)source addr) | *((int8_t*)source addr) 
                  r.push_back(gen_mnemonic("swp80i64"));
                  // source addr | source addr | *((int8_t*)source addr) | *((int8_t*)source addr) | destination addr

                  r.push_back(gen_mnemonic("duptopi64"));
                  // source addr | source addr | *((int8_t*)source addr) | *((int8_t*)source addr) | destination addr | destination addr
                  r.push_back(gen_mnemonic("swpi64b72")); 
                  // source addr | source addr | *((int8_t*)source addr) | destination addr| *((int8_t*)source addr) | destination addr                 
                  r.push_back(gen_mnemonic("stsi8"));
                  // source addr | source addr | *((int8_t*)source addr) | destination addr
                  r.push_back(gen_mnemonic("ldi64", 1));
                  r.push_back(gen_mnemonic("addi64"));
                  // source addr | source addr | *((int8_t*)source addr) | destination addr + 1
                  r.push_back(gen_mnemonic("swp72i64"));
                  // source addr | *((int8_t*)source addr) | destination addr + 1 | source addr 
                  r.push_back(gen_mnemonic("ldi64", 1));
                  r.push_back(gen_mnemonic("addi64"));
                  // source addr | *((int8_t*)source addr) | destination addr + 1 | source addr + 1
                  r.push_back(gen_mnemonic("swpi64"));
                  // source addr |  *((int8_t*)source addr) | source addr + 1 | destination addr + 1 
                  r.push_back(gen_mnemonic("swp128b8"));
                  // source addr | source addr + 1 | destination addr + 1 | *((int8_t*)source addr)

                  r.push_back(gen_mnemonic("ui8toui32"));
                  r.push_back(gen_mnemonic_sym_arg("bnzeroi32",lbl,"OblectamentaCodeLabel")); 
                  // source addr | source addr + len of string | destination addr + len of string
                  r.push_back(gen_mnemonic("discardtopi64"));
                  // source addr | source addr + len of string
                  r.push_back(gen_mnemonic("subi64"));
                  // len of string + 1
                  r.push_back(gen_mnemonic("duptopi64"));
                  // len of string + 1 | len of string + 1
                  r.push_back(gen_mnemonic("ldi64", rel_addr_content_size));
                  r.push_back(gen_mnemonic_sym_arg("ldi64","FP","OblectamentaReg"));
                  r.push_back(gen_mnemonic("subi64"));
                  r.push_back(gen_mnemonic("ldsi64"));
                  r.push_back(gen_mnemonic("addi64"));
                  r.push_back(gen_mnemonic("ldi64", sizeof(msg_node) ));
                  r.push_back(gen_mnemonic("addi64"));
                  r.push_back(gen_mnemonic("ldi64", rel_addr_content_size));
                  r.push_back(gen_mnemonic_sym_arg("ldi64","FP","OblectamentaReg"));
                  r.push_back(gen_mnemonic("subi64"));
                  r.push_back(gen_mnemonic("stsi64")); // [FP - rel_addr_content_size] += len of string + 1
                  r.push_back(gen_mnemonic("duptopi64"));
                  r.push_back(gen_mnemonic("ldi64", rel_addr_root));                       
                  r.push_back(gen_mnemonic_sym_arg("ldi64","FP","OblectamentaReg"));             
                  r.push_back(gen_mnemonic("subi64"));
                  r.push_back(gen_mnemonic("ldsi64"));

                  r.push_back(gen_mnemonic("ldi64", sizeof(msg_node) + node_name.length() + 1));
                  r.push_back(gen_mnemonic("addi64"));

                  r.push_back(gen_mnemonic("ldi64", rel_addr_content_size));
                  r.push_back(gen_mnemonic_sym_arg("ldi64","FP","OblectamentaReg"));
                  r.push_back(gen_mnemonic("subi64"));
                  r.push_back(gen_mnemonic("ldsi64"));
                  r.push_back(gen_mnemonic("addi64")); 
                  r.push_back(gen_mnemonic("subi64")); 
                  r.push_back(gen_mnemonic("ldi64", sizeof(msg_node::size)));
                  r.push_back(gen_mnemonic("swpi64")); 
                  r.push_back(gen_mnemonic("subi64"));//pointer to current sz node's size field

                  r.push_back(gen_mnemonic("swpi64"));
                  r.push_back(gen_mnemonic("ldi64", sizeof(msg_node) ));
                  r.push_back(gen_mnemonic("addi64"));
                  r.push_back(gen_mnemonic("swpi64"));
                  //r.push_back(gen_mnemonic("dbg_print_cs_and_regs", 0));
                  //r.push_back(gen_mnemonic("halt"));
                  r.push_back(gen_mnemonic("stsi64"));
                }
            } 
            else r.push_back(child);
        }
        // update msg_node::size

        r.push_back(gen_mnemonic("ldi64", rel_addr_content_size));
        r.push_back(gen_mnemonic_sym_arg("ldi64","FP","OblectamentaReg"));
        r.push_back(gen_mnemonic("subi64"));
        r.push_back(gen_mnemonic("ldsi64"));
        
        
        r.push_back(gen_mnemonic("ldi64", rel_addr_root));                       
        r.push_back(gen_mnemonic_sym_arg("ldi64","FP","OblectamentaReg"));             
        r.push_back(gen_mnemonic("subi64"));
        r.push_back(gen_mnemonic("ldsi64"));
        r.push_back(gen_mnemonic("ldi64",sizeof(msg_node::what)));
        r.push_back(gen_mnemonic("addi64"));
        r.push_back(gen_mnemonic("ldsi64"));
       // r.push_back(gen_mnemonic("dbg_print_cs_and_regs",0));
        r.push_back(gen_mnemonic("addi64"));

        r.push_back(gen_mnemonic_sym_arg("sti64","RES","OblectamentaReg"));
        r.push_back(gen_mnemonic_sym_arg("ldi64","RES","OblectamentaReg"));

        r.push_back(gen_mnemonic("ldi64", rel_addr_root));                       
        r.push_back(gen_mnemonic_sym_arg("ldi64","FP","OblectamentaReg"));             
        r.push_back(gen_mnemonic("subi64"));
        r.push_back(gen_mnemonic("ldsi64"));
        r.push_back(gen_mnemonic("ldi64",sizeof(msg_node::what)));
        r.push_back(gen_mnemonic("addi64"));
        r.push_back(gen_mnemonic("stsi64"));

        r.push_back(gen_mnemonic_sym_arg("ldi64","FP","OblectamentaReg"));
        r.push_back(gen_mnemonic_sym_arg("sti64","SP","OblectamentaReg"));
        r.push_back(gen_mnemonic_sym_arg("popi64","FP","OblectamentaReg"));
}

template <typename T> static const bool all_modfifier(T const & v){
    using namespace ceps::ast;
    for(auto e : v)
     if (is<Ast_node_kind::symbol>(e) && (kind(as_symbol_ref(e)) == "OblectamentaMessageModifier")
                                      && (name(as_symbol_ref(e)) == "all")) return true;
    return false;
}

static void oblectamenta_assembler_preproccess_match (std::string node_name, 
                                                      ceps::vm::oblectamenta::VMEnv& vm, 
                                                      std::vector<ceps::ast::node_t>& r, 
                                                      std::vector<ceps::ast::node_t>& mnemonics){
    using namespace ceps::ast;

    //Top of CS: |addr of current child|content-size|

    const auto addr_node_name = vm.mem.heap - vm.mem.base; 
    vm.store(node_name);vm.store('\0');

    string lbl_start = string{"__msgdef_"} +to_string(vm.lbl_counter++);
    string lbl_next_node = string{"__msgdef_"} +to_string(vm.lbl_counter++);
    string lbl_names_equal = string{"__msgdef_"} +to_string(vm.lbl_counter++);
    string lbl_check_strings = string{"__msgdef_"} +to_string(vm.lbl_counter++);
    string lbl_check_strings_unequal = string{"__msgdef_"} +to_string(vm.lbl_counter++);
    string lbl_wrong_node_type_i32_expected = string{"__msgdef_"} +to_string(vm.lbl_counter++);
    string lbl_not_enough_space = string{"__msgdef_"} +to_string(vm.lbl_counter++);
    string lbl_done = string{"__msgdef_"} +to_string(vm.lbl_counter++);
    string lbl_not_found = string{"__msgdef_"} +to_string(vm.lbl_counter++);
    string lbl_match_failed = string{"__msgdef_"} +to_string(vm.lbl_counter++);

    em(r,"swpi64");                        // |content-size|addr of current child|
    emlbl(r,lbl_next_node);                // WHILE NODE NOT FOUND DO
    em(r,"swpi64");                        // |addr of current child|content-size|
    //em(r,"dbg_print_topi64");              // debug: print top i64 value
    em(r,"duptopi64");                     // |addr of current child|content-size|content-size|
    emwsa(r,"bzeroi64",lbl_not_found, "OblectamentaCodeLabel");
                                           // |addr of current child|content-size|
    em(r,"swpi64");                        // |content-size|addr of current child|


    //Check remaining content size, if content-size <= 0 then we haven't found the requested node
    /*em(r,"swpi64");      //|addr of current child|content-size|
    em(r,"duptopi64");   //|addr of current child|content-size|content-size|
    em(r,"dbg_print_topi64");
    emwa(r, "halt", 5);*/
    em(r,"duptopi64");
    //|content-size|addr of current child|addr of current child|
    //XXXUpdate content-size, i.e. content-size = content-size - size of current child node 
    //em(r,"ldi64",sizeof(msg_node::what));em(r,"addi64");em(r,"ldsi64");em(r,"swp128i64");em(r,"subi64");em(r,"swpi64");
    //em(r,"duptopi64");    
    em(r,"ldsi32"); 
    //|content-size|addr of current child|node type
    em(r,"ldi32",msg_node::NODE);
    em(r,"subi32");
    emwsa(r,"bnzeroi32",lbl_next_node, "OblectamentaCodeLabel");//TODO: Error-Handling
    // check name
    em(r,"duptopi64");
    //|content-size|addr of current child|addr of first child|
    em(r,"ldi64",sizeof(msg_node));
    em(r,"addi64");
    //|content-size|addr of current child|addr of node-name|
    em(r,"ldi64",addr_node_name);
    //|content-size|addr of current child|addr of node-name|addr of name|
    
    emlbl(r,lbl_check_strings);
    em(r,"duptopi128");
    em(r,"ldsi8");
    em(r,"swpi8i64");
    em(r,"ldsi8");
    //|content-size|addr of current child|addr of node-name|addr of name|name[i]|node-name[i]
    emwsa(r,"bneqi8",lbl_check_strings_unequal,"OblectamentaCodeLabel");
    //|content-size|addr of first child|addr of node-name|addr of name|
    em(r,"duptopi64");
    em(r,"ldsi8");
    //|content-size|addr of current child|addr of node-name|addr of name|name[i]|
    
    

    emwsa(r,"bzeroi8",lbl_names_equal,"OblectamentaCodeLabel");
    
    //em(r,"dbg_print_cs_and_regs", 0);//em(r,"halt");

    em(r,"ldi64",1);em(r,"addi64");em(r,"swpi64");
    em(r,"ldi64",1);em(r,"addi64");em(r,"swpi64");
    //|content-size|addr of current child|addr of node-name+1|addr of name+1|
    emwsa(r,"buc",lbl_check_strings,"OblectamentaCodeLabel");
    
    emlbl(r,lbl_check_strings_unequal);    // CASE: node's name doesn't match     
                                           //|content-size|addr of current child|addr of node-name|addr of name
    
    em(r,"discardtopi64");em(r,"discardtopi64");
    //|content-size|addr of current child|
    //1. Update content-size
    em(r,"duptopi64");
    em(r,"ldi64",sizeof(msg_node::what));
    em(r,"addi64");
    em(r,"ldsi64");
    //|content-size|addr of current child|size of current child|
    em(r,"swp128i64");
    //|addr of current child|size of current child|content-size|
    em(r,"subi64");
    em(r,"swpi64");
    //|content-size|addr of current child|

    //2. Update addr to current node
    em(r,"duptopi64");                     //|content-size|addr of current child|addr of current child|
    em(r,"ldi64",sizeof(msg_node::what));  //|content-size|addr of current child|addr of current child|sizeof(msg_node::what)|
    em(r,"addi64");                        //|content-size|addr of current child|addr of current child+sizeof(msg_node::what)|
    em(r,"ldsi64");                        //|content-size|*((int64_t*)addr of current child+sizeof(msg_node::what)) |
    em(r,"addi64");                        //|content-size|addr of current child+*((int64_t*)addr of current child+sizeof(msg_node::what))|
                                           //|content-size|addr of next child|

    emwsa(r,"buc",lbl_next_node,"OblectamentaCodeLabel");
    emlbl(r,lbl_names_equal);
    //if (node_name.length() == 0) em(r,"dbg_print_cs_and_regs", 0);
    //|content-size|addr of current child|addr of node-name|addr of name
    //Invariant: we found our node, match the rest
    //Discard address of name which fulfilled its duty
    em(r,"discardtopi64");
    //|content-size|addr of current child|addr of node-name|
    em(r,"swpi64");
    em(r,"duptopi64");
    //|content-size|addr of node-name|addr of current child|addr of current child|
    em(r,"swp128i64");
    //|content-size|addr of current child|addr of current child|addr of node-name|
    em(r,"swpi64");
    em(r,"ldi64",sizeof(msg_node));
    em(r,"addi64");
    em(r,"swpi64");
    em(r,"subi64");
    em(r,"ldi64",1);
    em(r,"addi64");
    //|content-size|addr of node node_name|len of name+1|
    em(r,"duptopi128");
    //|content-size|addr of node node_name|len of name+1|addr of node node_name|len of name+1|
    em(r,"ldi64",sizeof(msg_node));
    //|content-size|addr of node node_name|len of name+1|addr of node node_name|len of name+1|sizeof(msg_node)|
    em(r,"addi64");
    //|content-size|addr of node node_name|len of name+1|addr of node node_name|len of name+1+sizeof(msg_node)|
    em(r,"addi64");
    //|content-size|addr of node node_name|len of name+1|addr of first child of node_name|
    em(r,"swpi64b128");
    //|content-size|addr of first child of node_name|addr of node node_name|len of name+1|
    //Compute content size
    em(r,"swpi64");
    em(r,"duptopi64");
    em(r,"ldi64",sizeof(msg_node::what));
    em(r,"addi64");
    em(r,"ldsi64");
    //|content-size|addr of first child of node_name|len of name+1|addr of node node_name|total size of current child|
    

    em(r,"swp128i64");
    em(r,"swpi64");
    em(r,"subi64");
    em(r,"ldi64",sizeof(msg_node));
    em(r,"swpi64");
    em(r,"subi64");
    //|content-size|addr of first child of node_name|addr of node node_name|content size|
    em(r,"swp128i64");
    em(r,"swpi64");
    em(r,"ldi64",0);
    bool tag_read {};
    //|content-size|addr of node node_name|addr of first child of node_name|content size|offset|
    for(auto mn: mnemonics){
        //|content-size|addr of node node_name|addr of ith child of node_name|content size|offset|
        if(is<Ast_node_kind::structdef>(mn) || is<Ast_node_kind::scope>(mn)){
            //|content-size|addr of node node_name|addr of first child of node_name|content size|offset|
            tag_read = true;
            em(r,"swpi64i192");

            //|content-size|offset|addr of node node_name|addr of first child of node_name|content size|

            //em(r,"dbg_print_cs_and_regs", 0);
            //Invariant: |addr of message|addr of last non matched child|content-size|
            if(is<Ast_node_kind::structdef>(mn)) 
             oblectamenta_assembler_preproccess_match(name(as_struct_ref(mn)), vm, r, children(as_struct_ref(mn)));
            else
             oblectamenta_assembler_preproccess_match("", vm, r, children(as_scope_ref(mn)));

            em(r,"bzeroi32");
            //Invariant:|content-size|offset|addr of message|addr of matched node|remaining content-size| Node Matched (flag) |
            emwsa(r,"bzeroi64",lbl_match_failed,"OblectamentaCodeLabel");

            //em(r,"dbg_print_cs_and_regs", 0);
            //Compute address of next child
            //|content-size|offset|addr of message|addr of matched node|remaining content-size including matched node|
            em(r,"swpi64");
            //|content-size|offset|addr of message|remaining content-size including matched node|addr of matched node|
            em(r,"duptopi64");
            //|content-size|offset|addr of message|remaining content-size including matched node|addr of matched node|addr of matched node|
            em(r,"ldi64",sizeof(msg_node)- sizeof(msg_node::size));
            em(r,"addi64");
            em(r,"ldsi64");
            //|content-size|offset|addr of message|remaining content-size including matched node|addr of matched node|size of matched node|
            em(r,"duptopi64");
            //|content-size|offset|addr of message|remaining content-size including matched node|addr of matched node|size of matched node|size of matched node|
            em(r,"swp128i64");
            //|content-size|offset|addr of message|remaining content-size including matched node|size of matched node|size of matched node|addr of matched node|
            em(r,"addi64");
            //|content-size|offset|addr of message|remaining content-size including matched node|size of matched node|addr of next node|
            em(r,"swpi64b128");
            //|content-size|offset|addr of message|addr of next node|remaining content-size including matched node|size of matched node|
            em(r,"swpi64");
            //|content-size|offset|addr of message|addr of next node|size of matched node|remaining content-size including matched node|
            em(r,"subi64");
            //|content-size|offset|addr of message|addr of next node|remaining content-size|
            em(r,"swp192i64");
            //|content-size|addr of message|addr of next node|remaining content-size|offset|

            //em(r,"dbg_print_cs_and_regs", 0);
            //em(r,"halt");
            //|content-size|addr of node node_name|addr of first child of node_name|content size|offset|

        } else if (is<Ast_node_kind::symbol>(mn) && (kind(as_symbol_ref(mn)) == "OblectamentaMessageTag")){
            if ("i32" == name(as_symbol_ref(mn))){
                tag_read = true;
                //ToDo: Check size
                em(r,"swp128i64");
                em(r,"duptopi64");
                //|content-size|addr of node node_name|content size|offset|addr of first child of node_name|addr of first child of node_name|
                em(r,"ldsi32");
                //|content-size|addr of node node_name|content size|offset|addr of first child of node_name|child of node_name.what|
                em(r,"ldi32",msg_node::INT32);
                //|content-size|addr of node node_name|content size|offset|addr of ith child of node_name|child of node_name.what|msg_node::INT32|
                emwsa(r,"bneq",lbl_wrong_node_type_i32_expected,"OblectamentaCodeLabel");
                //|content-size|addr of node node_name|content size|offset|addr of ith child of node_name|
                em(r,"ldi64",sizeof(msg_node));
                //|content-size|addr of node node_name|content size|offset|addr of ith child of node_name|sizeof(msg_node)|
                em(r,"addi64");
                //|content-size|addr of node node_name|content size|offset|addr of ith child's payload|
                em(r,"duptopi64");
                //|content-size|addr of node node_name|content size|offset|addr of ith child's payload|addr of ith child's payload|
                em(r,"ldsi32");
                //|content-size|addr of node node_name|content size|offset|addr of ith child's payload|ith child's payload|
                em(r,"swpi32i64");
                //|content-size|addr of node node_name|content size|offset|ith child's payload|addr of ith child's payload|
                em(r,"ldi64",sizeof(msg_node_int32::value));
                em(r,"addi64");
                //|content-size|addr of node node_name|content size|offset|ith child's payload|addr of it+1h child|
                em(r,"swp96i64");
                //|content-size|addr of node node_name|content size|ith child's payload|addr of it+1h child|offset|
                em(r,"ldi64",sizeof(msg_node_int32));
                em(r,"addi64");
                //|content-size|addr of node node_name|content size|ith child's payload|addr of it+1h child|new offset|
                em(r,"swpi160i64");
                //|content-size|addr of node node_name|ith child's payload|addr of it+1h child|new offset|content size|
                em(r,"swpi64");
                //|content-size|addr of node node_name|ith child's payload|addr of it+1h child|content size|new offset|
                em(r,"swpi192i32");
                //|content-size|addr of node node_name|addr of it+1h child|content size|new offset|ith child's payload|
            } else if ("f64" == name(as_symbol_ref(mn))){
                tag_read = true;
                //ToDo: Check size
                em(r,"swp128i64");
                em(r,"duptopi64");
                //|content-size|addr of node node_name|content size|offset|addr of first child of node_name|addr of first child of node_name|
                em(r,"ldsi32");
                //|content-size|addr of node node_name|content size|offset|addr of first child of node_name|child of node_name.what|
                em(r,"ldi32",msg_node::F64);
                //|content-size|addr of node node_name|content size|offset|addr of ith child of node_name|child of node_name.what|msg_node::INT32|
                emwsa(r,"bneq",lbl_wrong_node_type_i32_expected,"OblectamentaCodeLabel");
                //|content-size|addr of node node_name|content size|offset|addr of ith child of node_name|
                em(r,"ldi64",sizeof(msg_node));
                //|content-size|addr of node node_name|content size|offset|addr of ith child of node_name|sizeof(msg_node)|
                em(r,"addi64");
                //|content-size|addr of node node_name|content size|offset|addr of ith child's payload|
                em(r,"duptopi64");
                //|content-size|addr of node node_name|content size|offset|addr of ith child's payload|addr of ith child's payload|
                em(r,"ldsdbl");
                //|content-size|addr of node node_name|content size|offset|addr of ith child's payload|ith child's payload|
                em(r,"swpi64");
                //|content-size|addr of node node_name|content size|offset|ith child's payload|addr of ith child's payload|
                em(r,"ldi64",sizeof(msg_node_f64::value));
                em(r,"addi64");
                //|content-size|addr of node node_name|content size|offset|ith child's payload|addr of it+1h child|
                em(r,"swp128i64");
                //|content-size|addr of node node_name|content size|ith child's payload|addr of it+1h child|offset|
                em(r,"ldi64",sizeof(msg_node_f64));
                em(r,"addi64");
                //|content-size|addr of node node_name|content size|ith child's payload|addr of it+1h child|new offset|
                em(r,"swp192i64");
                //|content-size|addr of node node_name|ith child's payload|addr of it+1h child|new offset|content size|
                em(r,"swpi64");
                //|content-size|addr of node node_name|ith child's payload|addr of it+1h child|content size|new offset|
                em(r,"swp192i64");
                //|content-size|addr of node node_name|addr of it+1h child|content size|new offset|ith child's payload|                
            } else if ("sz" == name(as_symbol_ref(mn))){
                tag_read = true;
                //ToDo: Check size
                em(r,"swp128i64");
                em(r,"duptopi64");
                //|content-size|addr of node node_name|content size|offset|addr of first child of node_name|addr of first child of node_name|
                em(r,"ldsi32");
                //|content-size|addr of node node_name|content size|offset|addr of first child of node_name|child of node_name.what|
                em(r,"ldi32",msg_node::SZ);
                //|content-size|addr of node node_name|content size|offset|addr of ith child of node_name|child of node_name.what|msg_node::INT32|
                emwsa(r,"bneq",lbl_wrong_node_type_i32_expected,"OblectamentaCodeLabel");
                //|content-size|addr of node node_name|content size|offset|addr of ith child of node_name|
                em(r,"ldi64",sizeof(msg_node));
                //|content-size|addr of node node_name|content size|offset|addr of ith child of node_name|sizeof(msg_node)|
                em(r,"addi64");
                //|content-size|addr of node node_name|content size|offset|addr of ith child's payload|
                em(r,"duptopi64");
                //|content-size|addr of node node_name|content size|offset|addr of ith child's payload|addr of ith child's payload == ith child's payload |
                //|content-size|addr of node node_name|content size|offset|addr of ith child's payload|ith child's payload (pointer)|
                em(r,"swpi64");
                //|content-size|addr of node node_name|content size|offset|ith child's payload|addr of ith child's payload|
                //Compute address of next child
                em(r,"ldi64",sizeof(msg_node::size));
                //|content-size|addr of node node_name|content size|offset|ith child's payload|addr of ith child's payload|sizeof(msg_node::size)|
                em(r,"swpi64");
                //|content-size|addr of node node_name|content size|offset|ith child's payload|sizeof(msg_node::size)|addr of ith child's payload|
                em(r,"subi64");
                //|content-size|addr of node node_name|content size|offset|ith child's payload|addr of ith child's size field|
                em(r,"duptopi64");
                //|content-size|addr of node node_name|content size|offset|ith child's payload|addr of ith child's size field|addr of ith child's size field|
                em(r,"ldsi64");
                //|content-size|addr of node node_name|content size|offset|ith child's payload|addr of ith child's size field|ith child's size|
                em(r,"swpi64");
                //|content-size|addr of node node_name|content size|offset|ith child's payload|ith child's size|addr of ith child's size field|
                em(r,"ldi64",sizeof(msg_node::what));
                //|content-size|addr of node node_name|content size|offset|ith child's payload|ith child's size|addr of ith child's size field|sizeof(msg_node::what))|
                em(r,"swpi64");
                //|content-size|addr of node node_name|content size|offset|ith child's payload|ith child's size|sizeof(msg_node::what))|addr of ith child's size field|
                em(r,"subi64");
                //|content-size|addr of node node_name|content size|offset|ith child's payload|ith child's size|addr of ith child|
                em(r,"addi64");
                //|content-size|addr of node node_name|content size|offset|ith child's payload|addr of ith+1 child|

                //|content-size|addr of node node_name|content size|offset|ith child's payload|addr of it+1h child|
                em(r,"swp128i64");
                //|content-size|addr of node node_name|content size|ith child's payload|addr of it+1h child|offset|
                em(r,"ldi64",sizeof(msg_node_sz));
                em(r,"addi64");
                //|content-size|addr of node node_name|content size|ith child's payload|addr of it+1h child|new offset|
                em(r,"swp192i64");
                //|content-size|addr of node node_name|ith child's payload|addr of it+1h child|new offset|content size|
                em(r,"swpi64");
                //|content-size|addr of node node_name|ith child's payload|addr of it+1h child|content size|new offset|
                em(r,"swp192i64");
                //|content-size|addr of node node_name|addr of it+1h child|content size|new offset|ith child's payload|                
            }
        } else em(r,mn);
    }
    if (tag_read){
        //Invariant: |content-size|addr of node node_name|addr of it+1h child|content size|new offset|
        //
        //em(r,"dbg_print_data",0);em(r,"dbg_print_cs_and_regs", 0);//em(r,"halt");
        em(r,"discardtopi64");em(r,"discardtopi64");em(r,"discardtopi64");
        //Invariant: |content-size|addr of node node_name|
        em(r,"swpi64");
        //Invariant: |addr of node node_name|content-size|
        //em(r,"dbg_print_cs_and_regs", 0);em(r,"halt");
    }
    //Ready to bounce back
    //Write Result
    em(r,"ldi64",1);                       // |content-size|addr of current child| result == false |
    emwsa(r,"buc",lbl_done,"OblectamentaCodeLabel");

    emlbl(r,lbl_not_enough_space);
    //|content-size|addr of node node_name|addr of ith child of node_name|content size|offset|

    emlbl(r,lbl_wrong_node_type_i32_expected);
    //|content-size|addr of node node_name|content size|offset|addr of first child of node_name|
  
    emlbl(r,lbl_next_node);
    emlbl(r,lbl_not_found);
                                           // |addr of current child|content-size|
    em(r,"swpi64");                        // |content-size|addr of current child|
    em(r,"ldi64",0);                       // |content-size|addr of current child| result == false |
    
    emwsa(r,"buc",lbl_done,"OblectamentaCodeLabel");
    emlbl(r,lbl_match_failed);
    //Invariant:|content-size|offset|addr of message|addr of matched node|remaining content-size|
    em(r,"discardtopi64");em(r,"discardtopi64");
    //Invariant:|content-size|offset|addr of message|
    em(r,"swpi64");em(r,"discardtopi64");
    //Invariant:|content-size|addr of message|
    em(r,"ldi64",0);                       // |content-size|addr of current child| result == false |    
    emwsa(r,"buc",lbl_done,"OblectamentaCodeLabel");

    emlbl(r,lbl_done);

    em(r,"noop");
    //Invariant: |content-size|addr of node node_name| Result |
    
    //em(r,"dbg_print_cs_and_regs", 0);
    //em(r,"halt");
}

static void oblectamenta_assembler_preproccess (ceps::vm::oblectamenta::VMEnv& vm, std::vector<ceps::ast::node_t>& mnemonics){
    using namespace ceps::ast; using namespace std; using namespace ceps::vm::oblectamenta;
    for (size_t pos{}; pos < mnemonics.size(); ++pos){
        auto mnem{mnemonics[pos]};
        if (is_a_msgreaddirective(mnem)){ //Case: Deserialization
         auto mem_loc = static_mem_location(vm,mnem); //Fetch buffer address
         if (!mem_loc) { 
          stringstream offending_msg;
          offending_msg << *mnem;
          throw string{"oblectamenta_assembler: Message Directive contains no data label (hence I don't know where to read the bytes from). Offending message directive is >>>"+ offending_msg.str() +"<<<" };
         }
         auto glb_err_handler = global_error_handler(mnem);

         vector<node_t> r; // r contains the generated code
         emwsa(r,"lea",*mem_loc,"OblectamentaDataLabel");
         em(r,"duptopi64");
         //|addr of message|addr of message|
         em(r,"ldsi32"); 
         //|addr of message|type of message|
         emwa(r,"ldi32",msg_node::ROOT);
         //|addr of message|type of message|msg_node::ROOT
         em(r,"subi32");
         string lbl_error_root_type_wrong = string{"__msgdef_"} +to_string(vm.lbl_counter++);
         string lbl_done = string{"__msgdef_"} +to_string(vm.lbl_counter++);
         string lbl_error = string{"__msgdef_"} +to_string(vm.lbl_counter++);

         emwsa(r,"bnzeroi32",lbl_error_root_type_wrong,"OblectamentaCodeLabel");
         //|addr of message|
         //INVARIANT: Type = ROOT

         //|addr of message|
         em(r,"duptopi64");
         //|addr of message|addr of message|
         em(r,"ldi64", sizeof(msg_node::what));
         em(r,"addi64");
         //|addr of message|addr of node-size|
         em(r,"ldsi64");
         //|addr of message|node-size|
         em(r,"ldi64", sizeof(msg_node));
         em(r,"swpi64");
         em(r,"subi64");
         //|addr of message|content-size|
         em(r,"swpi64");
         em(r,"duptopi64");
         em(r,"swp128i64");
         //|addr of message|addr of message|content-size|
         em(r,"swpi64");
         em(r,"ldi64", sizeof(msg_node));
         em(r,"addi64");
         em(r,"swpi64");
         //|addr of message|addr of first child|content-size|



         for(auto child: children(as_struct_ref(mnem)) ){
            if (is<Ast_node_kind::structdef>(child) || is<Ast_node_kind::scope>(child)  ){
             vector<node_t> chldrn;
             if (is<Ast_node_kind::structdef>(child)) chldrn = children(as_struct_ref(child));
             else chldrn = children(as_scope_ref(child));

             auto loop_over_all{all_modfifier(chldrn)};

             //emwa(r,"dbg_print_cs_and_regs", 0);//em(r,"halt");
             //Invariant: |addr of message|addr of last non matched child|content-size|
             //em(r,"dbg_print_cs_and_regs", 0);
             //Find node name(as_struct_ref(child))
             //emwa(r,"dbg_print_cs_and_regs", 0);
             string lbl_loop,lbl_loop_end;
             if (loop_over_all){ 
                lbl_loop = string{"__msgdef_"} +to_string(vm.lbl_counter++);
                lbl_loop_end = string{"__msgdef_"} +to_string(vm.lbl_counter++);
                emlbl(r,lbl_loop);
             }

             if (is<Ast_node_kind::structdef>(child)) 
              oblectamenta_assembler_preproccess_match(name(as_struct_ref(child)), vm, r, chldrn);
            else
              oblectamenta_assembler_preproccess_match("", vm, r, chldrn);
             //emwa(r,"dbg_print_cs_and_regs", 0);
             //Invariant: |addr of message|addr of matched node|remaining content-size| Node Matched (flag) |
             //Check whether successful or not

             string lbl_continue = string{"__msgdef_"} +to_string(vm.lbl_counter++);
             emwsa(r,"bnzeroi64",lbl_continue,"OblectamentaCodeLabel");
             if (loop_over_all){
              emwsa(r,"buc",lbl_loop_end,"OblectamentaCodeLabel");
             } 
             em(r,"discardtopi64");em(r,"discardtopi64");em(r,"discardtopi64");
             //Invariant: CS points to old value before entering message read directive
             
             emwsa(r,"buc",lbl_error,"OblectamentaCodeLabel");
             emlbl(r,lbl_continue);
             //Invariant: |addr of message|addr of matched node|remaining content-size|
             //Move pointer to next node
             em(r,"swpi64");
             //|addr of message|remaining content-size|addr of matched node|
             em(r,"duptopi64");
             em(r,"ldi64", sizeof(msg_node::what));
             em(r,"addi64");
             em(r,"ldsi64");
             //|addr of message|remaining content-size|addr of matched node| size of matched node|
             em(r,"swpi64");
             em(r,"swpi64b128");
             em(r,"swpi64");
             em(r,"subi64");
             //|addr of message|addr of matched node|remaining content-size|
             em(r,"swpi64");
             //|addr of message|remaining content-size|addr of matched node|
             em(r,"duptopi64");
             em(r,"ldi64", sizeof(msg_node::what));
             em(r,"addi64");
             em(r,"ldsi64");
             em(r,"addi64");
             em(r,"swpi64");
             //|addr of message|addr of next unmatched node|remaining content-size|
             if (loop_over_all){
              emwsa(r,"buc",lbl_loop,"OblectamentaCodeLabel");
              emlbl(r,lbl_loop_end); em(r,"noop");
             }
             //emwa(r,"dbg_print_cs_and_regs", 0);em(r,"halt");
            }
         }
         
         //Invariant: |addr of message|addr of next child|remaining content-size|
         //emwa(r,"dbg_print_cs_and_regs", 0);em(r,"halt");
         em(r,"discardtopi64");em(r,"discardtopi64");em(r,"discardtopi64");

         emwsa(r,"buc",lbl_done,"OblectamentaCodeLabel");
         emlbl(r,lbl_error_root_type_wrong);em(r,"halt");
         emlbl(r,lbl_error);if (glb_err_handler) for (auto e: *glb_err_handler) r.push_back(e);
         emlbl(r,lbl_done);
         mnemonics.insert(mnemonics.begin() + pos, r.begin(), r.end());
         pos += r.size();
        } else if (is_a_msgdefdirective(mnem)){
         // write case
         auto mem_loc = static_mem_location(vm,mnem);
         if (!mem_loc) { 
          stringstream offending_msg;
          offending_msg << *mnem;
          throw string{"oblectamenta_assembler: Message Dirctive contains no data label (hence I don't know where to write the bytes to). Offending message directive is >>>"+ offending_msg.str() +"<<<" };
         }
         //INVARIANT: mnem is a message directive, and mem_loc holds a pointer into static memory to hold the serialized result.
         //Next: replace message directive with Oblectamenta Machine Code which will generate the message at runtime
         //mnem = CODE;
         vector<node_t> r;
         //write node type
         r.push_back(gen_mnemonic("ldi32", msg_node::ROOT));                           // ldi32(1);
         r.push_back(gen_mnemonic_sym_arg("lea",*mem_loc,"OblectamentaDataLabel"));    // lea(msg_buffer);
         r.push_back(gen_mnemonic("stsi32"));                                          // stsi32 (write node type)
         //write node size
         r.push_back(gen_mnemonic("ldi32", sizeof(msg_node)));                         // ldi32(sizeof(msg_node))
         r.push_back(gen_mnemonic("ui32toui64"));                                      // ui32toui64
         r.push_back(gen_mnemonic("ldi32",sizeof(msg_node::what)));                    // ldi32(sizeof(msg_node::what))
         r.push_back(gen_mnemonic("ui32toui64"));                                      // ui32toui64
         r.push_back(gen_mnemonic_sym_arg("lea",*mem_loc,"OblectamentaDataLabel"));    // lea(*mem_loc)
         r.push_back(gen_mnemonic("addi64"));                                          // addi64
         r.push_back(gen_mnemonic("stsi64"));                                          // stsi64
         
         //Create stack frame  
         r.push_back(gen_mnemonic_sym_arg("pushi64","FP","OblectamentaReg"));          // pushi64(FP);----|
         r.push_back(gen_mnemonic_sym_arg("ldi64","SP","OblectamentaReg"));            // ldi64(SP);      |
         r.push_back(gen_mnemonic_sym_arg("sti64","FP","OblectamentaReg"));            // sti64(FP);      |-------------------------------------| FP <- SP
         constexpr int rel_addr_root_size = sizeof(VMEnv::registers_t::file[0]);
         constexpr int rel_addr_content_size = rel_addr_root_size  + sizeof(addr_t);

         r.push_back(gen_mnemonic("ldi64", sizeof(addr_t) + sizeof(msg_node::size)));  // ldi64(sizeof(addr_t) + sizeof(msg_node::size));   ----|
         r.push_back(gen_mnemonic_sym_arg("ldi64","SP","OblectamentaReg"));            // ldi64(SP);      |-------------------------------------|
         r.push_back(gen_mnemonic("subi64"));                                          // subi64;         |
         r.push_back(gen_mnemonic_sym_arg("sti64","SP","OblectamentaReg"));            // sti64(SP);      | SP <- SP - 8 // make room for address to root

         r.push_back(gen_mnemonic_sym_arg("lea",*mem_loc,"OblectamentaDataLabel"));    // lea(msg_buffer) |
         r.push_back(gen_mnemonic("ldi64", sizeof(msg_node::what)));                   // ldi32(1);       |
         r.push_back(gen_mnemonic("addi64"));                                          // addi64          |
         r.push_back(gen_mnemonic("ldi64", rel_addr_root_size));                       // ldi64(8);       |
         r.push_back(gen_mnemonic_sym_arg("ldi64","FP","OblectamentaReg"));            // ldii64(FP);     | 
         r.push_back(gen_mnemonic("subi64"));                                          // subi64;         |         
         r.push_back(gen_mnemonic("stsi64"));                                          // stsi64          | [FP - 8] <-  & mem_loc->size

         // [FP - rel_addr_content_size] = 0
         r.push_back(gen_mnemonic("ldi64", 0));
         r.push_back(gen_mnemonic("ldi64", rel_addr_content_size));                       
         r.push_back(gen_mnemonic_sym_arg("ldi64","FP","OblectamentaReg"));             
         r.push_back(gen_mnemonic("subi64"));                                                   
         r.push_back(gen_mnemonic("stsi64"));                                          
         
         for(auto child: children(as_struct_ref(mnem)) ){
            if (is<Ast_node_kind::structdef>(child) || is<Ast_node_kind::scope>(child)){
             r.push_back(gen_mnemonic_sym_arg("lea",*mem_loc,"OblectamentaDataLabel")); 
             r.push_back(gen_mnemonic("ldi64", sizeof(msg_node)));
             r.push_back(gen_mnemonic("addi64"));
             r.push_back(gen_mnemonic("ldi64", rel_addr_content_size));
             r.push_back(gen_mnemonic_sym_arg("ldi64","FP","OblectamentaReg"));
             r.push_back(gen_mnemonic("subi64"));
             r.push_back(gen_mnemonic("ldsi64"));
             r.push_back(gen_mnemonic("addi64"));
             r.push_back(gen_mnemonic_sym_arg("sti64","ARG0","OblectamentaReg"));
             oblectamenta_assembler_preproccess(vm,child,r);
             r.push_back(gen_mnemonic_sym_arg("ldi64","RES","OblectamentaReg"));
             r.push_back(gen_mnemonic("ldi64", rel_addr_content_size));
             r.push_back(gen_mnemonic_sym_arg("ldi64","FP","OblectamentaReg"));
             r.push_back(gen_mnemonic("subi64"));
             r.push_back(gen_mnemonic("ldsi64"));
             r.push_back(gen_mnemonic("addi64"));
             r.push_back(gen_mnemonic("ldi64", rel_addr_content_size));
             r.push_back(gen_mnemonic_sym_arg("ldi64","FP","OblectamentaReg"));
             r.push_back(gen_mnemonic("subi64"));
             r.push_back(gen_mnemonic("stsi64")); // [FP - rel_addr_content_size] += RES
            }
        }
        // update msg_node::size
        r.push_back(gen_mnemonic("ldi64", rel_addr_content_size));
        r.push_back(gen_mnemonic_sym_arg("ldi64","FP","OblectamentaReg"));
        r.push_back(gen_mnemonic("subi64"));
        r.push_back(gen_mnemonic("ldsi64"));
        r.push_back(gen_mnemonic_sym_arg("lea",*mem_loc,"OblectamentaDataLabel"));
        r.push_back(gen_mnemonic("ldi64",sizeof(msg_node::what)));
        r.push_back(gen_mnemonic("addi64"));
        r.push_back(gen_mnemonic("ldsi64"));
        r.push_back(gen_mnemonic("addi64"));
        r.push_back(gen_mnemonic_sym_arg("lea",*mem_loc,"OblectamentaDataLabel"));
        r.push_back(gen_mnemonic("ldi64",sizeof(msg_node::what)));
        r.push_back(gen_mnemonic("addi64"));
        r.push_back(gen_mnemonic("stsi64"));

        r.push_back(gen_mnemonic_sym_arg("ldi64","FP","OblectamentaReg"));            
        r.push_back(gen_mnemonic_sym_arg("sti64","SP","OblectamentaReg"));
        r.push_back(gen_mnemonic_sym_arg("popi64","FP","OblectamentaReg"));
        mnemonics.insert(mnemonics.begin() + pos, r.begin(), r.end());               // INVARIANT : ROOT node with zero content at address *mem_loc
        pos += r.size();
     }

    }
}

void oblectamenta_assembler(ceps::vm::oblectamenta::VMEnv& vm, 
                            std::vector<ceps::ast::node_t>& mnemonics, 
                            std::map<std::string, int> const & ev_to_id,
                            bool append_halt)
{
 using namespace ceps::ast; using namespace std; using namespace ceps::vm::oblectamenta;
 
 //First Step: preprocess assembler text and expand macros/structures like message serializations
 oblectamenta_assembler_preproccess(vm,mnemonics);
 
 map<int32_t,size_t> immediate2loc; // immediate values => location in storage
 map<string,size_t> codelabel2loc; // code label => location in storage
 size_t& text_loc = vm.text_loc;

 bool postponed_msg_with_payload{};
 size_t postponed_buffer{};
 int postponed_ev{};
 size_t postponed_insert_after{};
 ceps::vm::oblectamenta::mnemonics_t::mapped_type postponed_mnem;

 for (size_t stmt_pos{}; stmt_pos < mnemonics.size(); ++stmt_pos){
    if ( text_loc + 10*max_opcode_width >= vm.text_size){
        //vm.resize_text(8192);
        vm.resize_text(  10*max_opcode_width + assembler::text_growth_factor * (double)vm.text_size );
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
    } else if(is<Ast_node_kind::symbol>(e) && kind(as_symbol_ref(e)) == "Event" ){
        //Events are mapped to msg
        auto& event_name{name(as_symbol_ref(e))};
        auto it{ceps::vm::oblectamenta::mnemonics.find("msg")};
        if (it == ceps::vm::oblectamenta::mnemonics.end()) 
         throw std::string{"oblectamenta_assembler: Internal Error (msg opcode not found)" };
        auto v{it->second};
        auto ev_id_it{ev_to_id.find(event_name)};
        if (ev_id_it == ev_to_id.end())
         throw std::string{"oblectamenta_assembler: event '"+event_name+"' has no encoding" };

        if (get<3>(v)) text_loc = get<3>(v)(vm,text_loc,ev_id_it->second);      
    } else if (is_a_symbol_with_arguments( e,sym_name,sym_kind,args)) {
        if (sym_kind == "Event"){
            if (args.size() && is<Ast_node_kind::structdef>(args[0]) && "msg" == name(as_struct_ref(args[0]))) {
                // Case : message
                //cout << *e << '\n';
                vector<node_t> mnems;
                mnems.push_back(args[0]);
                oblectamenta_assembler_preproccess (vm, mnems);
                mnemonics.insert(mnemonics.begin() + stmt_pos + 1, mnems.begin(), mnems.end());
                
                
                //INVARIANT: Code for payload generation is in place
                auto event_name{sym_name};
                auto it{ceps::vm::oblectamenta::mnemonics.find("msg@Event@@OblectamentaDataLabel@")};
                if (it == ceps::vm::oblectamenta::mnemonics.end()) 
                 throw std::string{"oblectamenta_assembler: Internal Error (msg@Event@@OblectamentaDataLabel@ opcode not found)" };
                auto v{it->second};
                postponed_mnem = v;
                auto ev_id_it{ev_to_id.find(event_name)};
                if (ev_id_it == ev_to_id.end())
                 throw std::string{"oblectamenta_assembler: event '"+event_name+"' has no encoding" };
                // get the buffer address
                string buffer_name{};
                for(auto e: children(as_struct_ref(args[0]))){
                    if (is<Ast_node_kind::symbol>(e) && kind(as_symbol_ref(e)) == "OblectamentaDataLabel" ){
                        buffer_name = name(as_symbol_ref(e));
                        break;
                    }
                }//for
                if (buffer_name.length() == 0) {
                 stringstream offending_msg;
                 offending_msg << *args[0];
                 throw 
                  string{"oblectamenta_assembler: Message Directive contains no data label (hence I don't know where to read the bytes from). Offending message directive is >>>"+ offending_msg.str() +"<<<" };
                }
                auto data_label_it{vm.data_labels().find(buffer_name)};
                if (data_label_it == vm.data_labels().end()) 
                    throw std::string{"oblectamenta_assembler: unknown data label: '"+ buffer_name +"'" };

                postponed_msg_with_payload = true;
                postponed_buffer = data_label_it->second;
                postponed_ev = ev_id_it->second;
                postponed_insert_after = stmt_pos + mnems.size();
                //for (auto e: mnems) cout << *e << '\n';
                //cout << "\n\n\n\n";
                //for(size_t i = stmt_pos + 1; i < mnemonics.size(); ++i ) cout << *mnemonics[i] << '\n';
                //for (auto e: mnems) cout << *e << '\n';
                /*char buffer[512];
                auto written_bytes = serialize_event_payload(args[0],buffer,512);
                string json_msg;
                deserialize_event_payload(buffer,written_bytes,json_msg);*/
                //cerr <<">>" << json_msg << "<<" << "\n";
            }
        }else if (sym_kind == "OblectamentaOpcode"){
            auto& mnemonic{sym_name};
            std::string sym_name2;
	        std::string sym_kind2;
	        std::vector<node_t> args2;
                        
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
                size_t addr {arg};
                
                auto it{ceps::vm::oblectamenta::mnemonics.find(mnemonic+"imm")};
                if (it == ceps::vm::oblectamenta::mnemonics.end()) 
                    throw std::string{"oblectamenta_assembler: unknown opcode: '"+ mnemonic+"imm'" };
                auto v{it->second};
                    
                if (get<3>(v)) 
                 text_loc = get<3>(v)(vm,text_loc,addr); 
                else throw std::string{"oblectamenta_assembler: illformed parameter list for '"+ mnemonic+"'" };
            } else if (args.size() == 1 && is<Ast_node_kind::float_literal>(args[0])){
                auto arg{value(as_double_ref(args[0]))};
                
                auto it{ceps::vm::oblectamenta::mnemonics.find(mnemonic+"imm")};
                if (it == ceps::vm::oblectamenta::mnemonics.end()) 
                    throw std::string{"oblectamenta_assembler: unknown opcode: '"+ mnemonic+"imm'" };
                auto v{it->second};
                    
                if (get<5>(v)) 
                 text_loc = get<5>(v)(vm,text_loc,arg); 
                else throw std::string{"oblectamenta_assembler: illformed parameter list for '"+ mnemonic+"'" };
            } else if (args.size() == 1 && is<Ast_node_kind::symbol>(args[0]) && kind(as_symbol_ref(args[0]))=="OblectamentaDataLabel") {
                auto data_label_it{vm.data_labels().find(name(as_symbol_ref(args[0])))};
                if (data_label_it == vm.data_labels().end()) 
                    throw std::string{"oblectamenta_assembler: unknown data label: '"+ name(as_symbol_ref(args[0])) +"'" };
                auto it{ceps::vm::oblectamenta::mnemonics.find(mnemonic)};
                if (it == ceps::vm::oblectamenta::mnemonics.end()){
                    mnemonic+="@"+kind(as_symbol_ref(args[0]))+"@";
                    it = ceps::vm::oblectamenta::mnemonics.find(mnemonic);
                    if (it == ceps::vm::oblectamenta::mnemonics.end())
                     throw std::string{"oblectamenta_assembler: unknown opcode: '"+ mnemonic+"'" };
                }
                auto opcode{it->second};
    
                if (get<3>(opcode)) 
                     text_loc = get<3>(opcode)(vm,text_loc,data_label_it->second); 
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

                auto it{ceps::vm::oblectamenta::mnemonics.find(mnemonic)};
                if (it == ceps::vm::oblectamenta::mnemonics.end()) 
                   throw std::string{"oblectamenta_assembler: unknown opcode: '"+ mnemonic+"'" };
                auto opcode{it->second};
    
    

                if (get<3>(opcode)) 
                     backpatch_loc = text_loc = get<3>(opcode)(vm,text_loc,loc); 
                else throw std::string{"oblectamenta_assembler: illformed parameter list for '"+ mnemonic+"'" };
                if (backpatch){
                    size_t pe{};
                    for(;pe < patch_entries.size() && patch_entries[pe].id[0] != 0; ++pe);
                    if (pe == patch_entries.size()) patch_entries.push_back({});
                    patch_entries[pe].text_loc = backpatch_loc;
                    strncpy(patch_entries[pe].id, name(as_symbol_ref(args[0])).c_str(), sizeof(patch_entry::id)); 
                }
            } else if (args.size() == 1 && is<Ast_node_kind::symbol>(args[0]) && kind(as_symbol_ref(args[0]))=="OblectamentaExternalFunc") {
                // Handle case when statement is of the form: call(external function name);
                auto ref_ext_name{name(as_symbol_ref(args[0]))};
                auto query_opcode_x_version_it{ceps::vm::oblectamenta::mnemonics.find(mnemonic+"x")};
                if (query_opcode_x_version_it == ceps::vm::oblectamenta::mnemonics.end()) 
                    throw std::string{"oblectamenta_assembler: Opcode doesn't support external references: '"+ mnemonic+"'" };
                auto op_code_entry{query_opcode_x_version_it->second};
                
                auto ref_external_it{vm.exfuncs.begin()};
				for (;ref_external_it != vm.exfuncs.end();++ref_external_it) if (ref_external_it->name == ref_ext_name) break;
				if(ref_external_it == vm.exfuncs.end()) 
                    throw std::string{"oblectamenta_assembler: '"+ref_ext_name+"'not declared as external" };

                if (get<3>(op_code_entry)) text_loc = get<3>(op_code_entry)(vm,text_loc,(size_t)ref_external_it->addr); 
                    else throw std::string{"oblectamenta_assembler: illformed parameter list for '"+ mnemonic+"'" };
                    
            } else if (args.size() == 1 && is_a_symbol_with_arguments( args[0],sym_name2,sym_kind2,args2)){
                if (sym_kind2 == "OblectamentaModifier" 
                    && sym_name2 == "addr" 
                    && args2.size() == 1 
                    && is<Ast_node_kind::int_literal>(args2[0]) ) 
                {
                    auto it{ceps::vm::oblectamenta::mnemonics.find(mnemonic)};
                    if (it == ceps::vm::oblectamenta::mnemonics.end()) 
                       throw std::string{"oblectamenta_assembler: unknown opcode: '"+ mnemonic+"'" };
                    auto opcode{it->second};
    
                    if (get<3>(opcode)) 
                     text_loc = get<3>(opcode)(vm,text_loc,value(as_int_ref(args2[0]))); 
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
                } else {
                    
                }
            } else if (args.size() == 1) {
                auto mangled_mnemonic{mnemonic};
                for(auto ee: args){
                    if (is<Ast_node_kind::int_literal>(ee) || is<Ast_node_kind::long_literal>(ee)) mangled_mnemonic+="imm";
                    else if (is<Ast_node_kind::string_literal>(ee)) mangled_mnemonic+="sz";
                    else {
                        stringstream ss;
                        ss << "Illformed opcode. In '" << *e << " the offending argument is ";
                        ss << *ee << "\n";
                        throw std::string{ss.str()};
                    }
                }
                //Get opcode
                auto it{ceps::vm::oblectamenta::mnemonics.find(mangled_mnemonic)};
                if (it == ceps::vm::oblectamenta::mnemonics.end()) 
                    throw std::string{"oblectamenta_assembler: unknown opcode: '"+mangled_mnemonic+"'" };
                auto opcode{it->second};
                
                if (get<MNEM_IMM>(opcode)){
                    addr_t imm;
                    auto e = args[0];
                    if (is<Ast_node_kind::int_literal>(e) || is<Ast_node_kind::long_literal>(e)) 
                     imm = value(as_int_ref(e));
                    else if (is<Ast_node_kind::string_literal>(e)){
                     auto t{vm.store(value(as_string_ref(e)))};
                     vm.store('\0');
                     imm = t;
                    }
                    text_loc = get<MNEM_IMM>(opcode)(vm,text_loc, imm);
                }  else 
                 throw std::string{"oblectamenta_assembler: illformed parameter list for '"+ mnemonic+"'" }; 
            } else if (args.size() == 2){
                auto mangled_mnemonic{mnemonic};                
                for(auto e: args){
                    if (is<Ast_node_kind::int_literal>(e) || is<Ast_node_kind::long_literal>(e)) mangled_mnemonic+="imm";
                    else if (is<Ast_node_kind::string_literal>(e)) mangled_mnemonic+="sz";
                    else if (is<Ast_node_kind::symbol>(e)) mangled_mnemonic+="@"+kind(as_symbol_ref(e))+"@";
                }
                //Get opcode
                auto it{ceps::vm::oblectamenta::mnemonics.find(mangled_mnemonic)};
                if (it == ceps::vm::oblectamenta::mnemonics.end()) 
                    throw std::string{"oblectamenta_assembler: unknown opcode: '"+mangled_mnemonic+"'" };
                auto opcode{it->second};
                if (get<MNEM_IMM_IMM>(opcode)){
                    addr_t imm[2];
                    for (size_t i{}; i < 2; ++i){
                        auto e = args[i];
                        if (is<Ast_node_kind::int_literal>(e) || is<Ast_node_kind::long_literal>(e)) imm[i] = value(as_int_ref(e));
                        else if (is<Ast_node_kind::string_literal>(e)){
                            auto t{vm.store(value(as_string_ref(e)))};
                            vm.store('\0');
                            imm[i] = t;
                        } else if (is<Ast_node_kind::symbol>(e) && kind(as_symbol_ref(e)) == "OblectamentaDataLabel" ){
                            auto data_label_it{vm.data_labels().find(name(as_symbol_ref(e)))};
                            if (data_label_it == vm.data_labels().end()) 
                              throw std::string{"oblectamenta_assembler: unknown data label: '"+ name(as_symbol_ref(e)) +"'" };
                              imm[1] = data_label_it->second;
                        } else if (is<Ast_node_kind::symbol>(e) && kind(as_symbol_ref(e)) == "OblectamentaReg" ){
                            auto reg_it = vm.registers.reg_mnemonic2idx.find(name(as_symbol_ref(e)));
                            if (reg_it == vm.registers.reg_mnemonic2idx.end()){
                             stringstream ss;
                             ss <<  *e << '\n';
                             throw std::string{"oblectamenta_assembler: unknown register '"+name(as_symbol_ref(e))+"' in " + ss.str()};
                            }
                            imm[i] = reg_it->second;
                        }
                    }
                    text_loc = get<MNEM_IMM_IMM>(opcode)(vm,text_loc, imm[0], imm[1]);
                } 
                else 
                 throw std::string{"oblectamenta_assembler: illformed parameter list for '"+ mnemonic+"'" };
            } else throw std::string{"oblectamenta_assembler: illformed mnemonic '"+ mnemonic+"'" };
        }
    } 

    if (postponed_msg_with_payload  && postponed_insert_after == stmt_pos){
        if (get<6>(postponed_mnem)) text_loc = get<6>(postponed_mnem)(vm,text_loc,postponed_ev,postponed_buffer);
        else throw std::string{"oblectamenta_assembler: Internal Error (msg@Event@@OblectamentaDataLabel@ opcode emitter not found)" };
        postponed_msg_with_payload = false;
        postponed_buffer = 0;
        postponed_ev = 0;
        postponed_insert_after = 0;
    }
 } //for
 if (append_halt)
    text_loc = emit<Opcode::halt>(vm,text_loc);
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
          ceps::vm::oblectamenta::oblectamenta_assembler(*r.vm,children(as_struct_ref(e)),{});
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
          ceps::vm::oblectamenta::oblectamenta_assembler(vm,children(as_struct_ref(e)),{});
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