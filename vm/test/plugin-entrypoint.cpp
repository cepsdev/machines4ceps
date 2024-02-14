
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
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <stdlib.h>
#include <map>
#include <algorithm>
#include <future>
#include <memory>
#include <netinet/sctp.h> 
#include <vector>
#include <string>
#include <optional>

#include "ceps_ast.hh"
#include "core/include/state_machine_simulation_core.hpp"

#include "core/include/vm/vm_base.hpp"
#include "core/include/vm/oblectamenta-assembler.hpp"
#include "core/include/vm/oblectamenta-comp-graph.hpp"

#define ast_proc_prolog  using namespace ceps::ast;\
    using namespace ceps::vm::oblectamenta;\
    using namespace ceps::interpreter;\
    using namespace std;

namespace cepsplugin{
    static Ism4ceps_plugin_interface* plugin_master = nullptr;
    static const std::string version_info = "INSERT_NAME_HERE v0.1";
    static constexpr bool print_debug_info{true};
    ceps::ast::node_t operation(ceps::ast::node_callparameters_t params);
    ceps::ast::node_t obj(ceps::ast::node_callparameters_t params);
}


template<typename T> T 
    fetch(ceps::ast::Struct&);
template<typename T> T 
    fetch(ceps::ast::node_t);
template<typename T> T 
    fetch(ceps::ast::Struct&, ceps::vm::oblectamenta::VMEnv&);

template<> ceps::vm::oblectamenta::VMEnv fetch<ceps::vm::oblectamenta::VMEnv>(ceps::ast::Struct& );


template<typename T> 
    std::optional<T> read_value(ceps::ast::Struct& s);
template<typename T> 
    std::optional<T> read_value(ceps::ast::Struct& s, ceps::vm::oblectamenta::VMEnv& vm);
template<typename T> 
    std::optional<T> read_value(size_t idx, ceps::ast::Struct& s);
template<typename T> 
    bool check(ceps::ast::Struct&);
template<typename T> 
    bool check(ceps::ast::node_t);

template<typename T> 
    std::optional<T> read_value(size_t idx, ceps::ast::Struct& s){
        auto & v{children(s)};
        if(v.size() <= idx || !check<T>(v[idx])) return {};
        return fetch<T>(v[idx]);
    }

template<typename T> 
    std::optional<T> read_value(ceps::ast::Struct& s){
        if(!check<T>(s)) return {};
        return fetch<T>(s);
    }

template<typename T> 
    std::optional<T> read_value(ceps::ast::Struct& s, ceps::vm::oblectamenta::VMEnv& vm){
        if(!check<T>(s)) return {};
        return fetch<T>(s,vm);
    }

template<typename T> ceps::ast::node_t ast_rep (T entity);
template<typename T> ceps::ast::node_t ast_rep (T entity, ceps::vm::oblectamenta::VMEnv&);

////////

struct ser_wrapper_stack{
    std::shared_ptr<ceps::vm::oblectamenta::VMEnv> vm;
};

struct ser_wrapper_data{
    std::shared_ptr<ceps::vm::oblectamenta::VMEnv> vm;
};

struct ser_wrapper_text{
    std::shared_ptr<ceps::vm::oblectamenta::VMEnv> vm;    
};

struct ser_wrapper_cstack{
    std::shared_ptr<ceps::vm::oblectamenta::VMEnv> vm;
};

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
    ast_proc_prolog    
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
    ast_proc_prolog    
    ser_wrapper_text r{make_shared<VMEnv>()};
    try{
        for(auto e : children(s))
         if (is<Ast_node_kind::structdef>(e) && name(as_struct_ref(e)) == "asm" ){
          oblectamenta_assembler(*r.vm,children(as_struct_ref(e)));
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
    ast_proc_prolog    
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
          oblectamenta_assembler(vm,children(as_struct_ref(e)));
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
    ast_proc_prolog    
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
    ast_proc_prolog    
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
    ast_proc_prolog
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
    ast_proc_prolog
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
    ast_proc_prolog    
    auto result = mk_struct("stack");
    auto& ch {children(*result)};
    auto mem_size{vm.mem.end -vm.mem.base};
    for (ssize_t i = 0; i < mem_size  - vm.registers.file[VMEnv::registers_t::SP]; ++i )
        ch.push_back(mk_uint8( vm.mem.base[vm.registers.file[VMEnv::registers_t::SP] + i] ));
    return result;
}

template<> ceps::ast::node_t ast_rep (ser_wrapper_data data, ceps::vm::oblectamenta::VMEnv& vm){
    ast_proc_prolog
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
    ast_proc_prolog    
    auto result = mk_struct("compute_stack");
    auto& ch {children(*result)};
    for(size_t i = 0; i < (size_t)vm.registers.file[VMEnv::registers_t::CSP] ; ++i)
     ch.push_back(mk_uint8(vm.compute_stack[i]));
    return result;
}

// AST representation ser_wrapper_text

template<> ceps::ast::node_t ast_rep (ser_wrapper_text text, ceps::vm::oblectamenta::VMEnv& vm ){
    ast_proc_prolog    
    auto result = mk_struct("text");
    auto& ch {children(*result)};
    for (size_t loc = 0; loc < vm.text_loc; ++loc)
     ch.push_back(mk_uint8(vm.text[loc]));
    return result;
}

template<> ceps::ast::node_t ast_rep (ser_wrapper_text text){
    ast_proc_prolog    
    auto result = mk_struct("text");
    return result;
}

// AST representation ceps::vm::oblectamenta::VMEnv::registers_t
template<> ceps::ast::node_t ast_rep (ceps::vm::oblectamenta::VMEnv::registers_t regs){
    ast_proc_prolog    
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
    ast_proc_prolog
    
    auto result = mk_struct("vm");
    children(*result).push_back(ast_rep(ser_wrapper_stack{},vm));
    children(*result).push_back(ast_rep(ser_wrapper_data{}, vm));
    children(*result).push_back(ast_rep(ser_wrapper_text{},vm));
    children(*result).push_back(ast_rep(ser_wrapper_cstack{}, vm));
    children(*result).push_back(ast_rep(vm.registers)); 
    return result;
}

template<> ceps::ast::node_t ast_rep<ceps::vm::oblectamenta::VMEnv> (ceps::vm::oblectamenta::VMEnv vm){
    ast_proc_prolog
    
    auto result = mk_struct("vm");
    children(*result).push_back(ast_rep(ser_wrapper_stack{},vm));
    children(*result).push_back(ast_rep(ser_wrapper_data{}, vm));
    children(*result).push_back(ast_rep(ser_wrapper_text{}, vm));
    children(*result).push_back(ast_rep(ser_wrapper_cstack{}, vm));
    children(*result).push_back(ast_rep(vm.registers));
    return result;
}

/**
 * @brief ceps entry point for the creation of Oblectamenta VM's objects.
 * 
 * @param params 
 * @return ceps::ast::node_t 
 */
ceps::ast::node_t cepsplugin::obj(ceps::ast::node_callparameters_t params){
    ast_proc_prolog
    
    static auto fns{ map<string, node_t (*)(Struct&)>{
        {"vm", [] (Struct& s) -> node_t { 
                if(!children(s).size()) return ast_rep(VMEnv{}); 
                auto vm_opt{read_value<VMEnv>(s)};
                if (vm_opt) return ast_rep(*vm_opt);
                return mk_undef();
               } 
        },
        {   "stack", 
            [] (Struct& s) -> node_t 
                    { 
                        VMEnv vm;
                        auto v{read_value<ser_wrapper_stack>(s)}; 
                        if (!v) return mk_undef(); 
                        return ast_rep(*v,*(*v).vm); 
                    } 
        },
        {"data", [] (Struct& s) -> node_t { VMEnv vm; auto v{read_value<ser_wrapper_data>(s)}; if (!v) return mk_undef(); return ast_rep(*v,*(*v).vm); } },
        {"text", [] (Struct& s) -> node_t { VMEnv vm; auto v{read_value<ser_wrapper_text>(s)}; if (!v) return mk_undef(); return ast_rep(*v,*(*v).vm); } },
        {"compute_stack", [] (Struct& s) -> node_t { VMEnv vm; auto v{read_value<ser_wrapper_cstack>(s)}; if (!v) return mk_undef(); return ast_rep(*v,*(*v).vm); } },
        {"registers", [] (Struct& s) -> node_t { auto v{read_value<VMEnv::registers_t>(s)}; if (!v) return mk_undef(); return ast_rep(*v); } }
    }};
    
    auto data{get_first_child(params)};
    if (!is<Ast_node_kind::structdef>(data))
     return mk_undef();

    auto& ceps_struct = *as_struct_ptr(data);
    auto obj_name{name(ceps_struct)};
    auto factory_it{fns.find(obj_name)};
    if (factory_it == fns.end()) return mk_undef();
    return factory_it->second(ceps_struct);
}

class CepsComputeGraphNotationTraverser{
    
        std::vector<ceps::ast::node_t> v; 
    public:
        struct array_ref{
            std::string id;
            size_t idx;
            uint8_t width{8};
        };

        struct expr;

        struct op_expr{
            ceps::ast::node_t root;
            std::string op;
            expr lhs();
            expr rhs();
        };

        struct func_id{
            std::string name;
            std::optional<std::string> sym_kind;
        };
        
        struct noary_or_unary_funccall_expr{
            ceps::ast::node_t root;
            func_id fid;
            ceps::ast::node_t  arg;
            noary_or_unary_funccall_expr() = default;
            noary_or_unary_funccall_expr(ceps::ast::node_t root, func_id fid, ceps::ast::node_t  arg)
            : root{root}, fid{fid}, arg{arg}{}
            expr argument();
        };

        struct funccall_expr{
            ceps::ast::node_t root;
            func_id fid;
            std::vector<ceps::ast::node_t>  args;
            funccall_expr() = default;
            funccall_expr(ceps::ast::node_t root, func_id fid, std::vector<ceps::ast::node_t>  args)
            : root{root}, fid{fid}, args{args}{}
            std::vector<expr> arguments();
        };

        struct expr{
            ceps::ast::node_t root;

            std::optional<int> as_int(){
                using namespace std;
                using namespace ceps::ast;
                if(!is<ceps::ast::Ast_node_kind::int_literal>(root)) return {};
                return value(as_int_ref(root));
            }

            std::optional<std::string> as_id(){
                using namespace std;
                using namespace ceps::ast;
                if(!is<ceps::ast::Ast_node_kind::identifier>(root)) return {};
                return name(as_id_ref(root));
            }

            std::optional<std::pair<std::string,std::string>> as_symbol(){
                using namespace std;
                using namespace ceps::ast;
                if(!is<ceps::ast::Ast_node_kind::symbol>(root)) return {};
                return { make_pair(name(as_symbol_ref(root)), kind(as_symbol_ref(root))) };
            }

            std::optional<noary_or_unary_funccall_expr> as_noary_or_unary_funccall(){
                using namespace std;
                using namespace ceps::ast;
                if(!is<ceps::ast::Ast_node_kind::func_call>(root)) return {};
				
                string func_id;
				string fkind; 
				string sym_name;
				node_t ftarget; 
				vector<node_t> args;
                if (args.size() > 1) return {};

                is_a_funccall(	root, func_id, fkind, sym_name, ftarget, args);
                return noary_or_unary_funccall_expr( root, 
                                               { (fkind == "" ? func_id : sym_name), fkind == "" ? optional<string>{} : optional<string>{fkind}  },
                                               args.size() == 0 ? nullptr : args[0]);

            }
            
            std::optional<funccall_expr> as_funccall(){
                using namespace std;
                using namespace ceps::ast;
                if(!is<ceps::ast::Ast_node_kind::func_call>(root)) return {};
				
                string func_id;
				string fkind; 
				string sym_name;
				node_t ftarget; 
				vector<node_t> args;

                is_a_funccall(	root, func_id, fkind, sym_name, ftarget, args);
                return funccall_expr( root, 
                                               { (fkind == "" ? func_id : sym_name), fkind == "" ? optional<string>{} : optional<string>{fkind}  },
                                               args);

            }

            std::optional<array_ref> as_array_ref () {
                using namespace std;
                using namespace ceps::ast;

                string sym_name;
	            string sym_kind;
	            vector<node_t> args;
                if (is_a_symbol_with_arguments(root, sym_name, sym_kind, args)){
                    if (sym_kind == "OblectamentaDataLabel" && args.size() == 1 && is<Ast_node_kind::int_literal>(args[0])){
                        return array_ref{sym_name,(size_t)value(as_int_ref(args[0])) };                        
                    }                 
                }
                return {};
            }
            std::optional<op_expr> as_binary_operation() const{
                using namespace ceps::ast;
                if(is<Ast_node_kind::binary_operator>(root)){
                    return op_expr{root, op_val(as_binop_ref(root)) };
                }
                return {};
            }

            bool is_leaf() const{
                using namespace std;
                using namespace ceps::ast;
                return is<Ast_node_kind::int_literal>(root) || is<Ast_node_kind::float_literal>(root) || is<Ast_node_kind::string_literal>(root)
                       || is<Ast_node_kind::identifier>(root) || is<Ast_node_kind::uint8>(root) || is<Ast_node_kind::long_literal>(root);
            }
            size_t size() const{
                if (is_leaf()) return 0;
                return children(*nlf_ptr(root)).size();
            }
            expr operator[](size_t i){
                return expr{children(*nlf_ptr(root))[i]};
            } 
        };
        struct stmt{
            ceps::ast::node_t node;
            std::optional<op_expr> is_assign_op() const {
                using namespace ceps::ast; 
                if (!is<Ast_node_kind::binary_operator>(node)) return {};
                if("=" == op_val(as_binop_ref(node))) 
                 return op_expr{node};
                return {};
            }
            expr as_expr(){return expr{node};}

        };
        CepsComputeGraphNotationTraverser(std::vector<ceps::ast::node_t> v):v{v} {}
        stmt operator[](size_t i){
            return {v[i]};            
        }
        size_t size() const {return v.size();}
};

CepsComputeGraphNotationTraverser::expr CepsComputeGraphNotationTraverser::noary_or_unary_funccall_expr::argument(){
    return expr{arg};
}

std::vector<CepsComputeGraphNotationTraverser::expr> CepsComputeGraphNotationTraverser::funccall_expr::arguments(){
    std::vector<CepsComputeGraphNotationTraverser::expr> r;
    for(auto e: args) r.push_back(expr{e});
    return r;
}

CepsComputeGraphNotationTraverser::expr CepsComputeGraphNotationTraverser::op_expr::lhs() { return { ceps::ast::children(ceps::ast::as_binop_ref(root))[0]} ;}
CepsComputeGraphNotationTraverser::expr CepsComputeGraphNotationTraverser::op_expr::rhs() { return { ceps::ast::children(ceps::ast::as_binop_ref(root))[1]} ;}

template <typename F, typename E> 
 void traverse(F f, E e){
    if (f(e)){
        if (e.is_leaf()) return;
        for(size_t i{}; i < e.size(); ++i)
         traverse(f, e[i]);
    }
}

ceps::ast::node_t mk_func_call(ceps::ast::node_t func_id, ceps::ast::node_t arg){
    using namespace ceps::ast;
    Func_call* f = new Func_call();
	f->children_.push_back(func_id);
    Call_parameters* params = new Call_parameters();
	f->children_.push_back(params);
    params->children_.push_back(arg);
    return f;
}

class CepsOblectamentaMnemonicsEmitter{
        std::vector<ceps::ast::node_t> v; 
    public:
        void emitStoreDouble(size_t location){
            v.push_back(
                mk_func_call(
                    ceps::interpreter::mk_symbol("stdbl","OblectamentaOpcode"), 
                    ceps::interpreter::mk_int_node((int)location)
                )            
            );                        
        }
        void emitLoadDouble(size_t location){
            v.push_back(
                mk_func_call(
                    ceps::interpreter::mk_symbol("lddbl","OblectamentaOpcode"), 
                    ceps::interpreter::mk_int_node((int)location)
                )            
            );                        
        }
        void emitMulDouble(){
            v.push_back(ceps::interpreter::mk_symbol("muldbl","OblectamentaOpcode"));
        }
        void emitSubDouble(){
            v.push_back(ceps::interpreter::mk_symbol("subdbl","OblectamentaOpcode"));
        }
        void emitAddDouble(){
            v.push_back(ceps::interpreter::mk_symbol("adddbl","OblectamentaOpcode"));
        }
        void emitDivDouble(){
            v.push_back(ceps::interpreter::mk_symbol("divdbl","OblectamentaOpcode"));
        }
        void emitSinDouble(){
            v.push_back(ceps::interpreter::mk_symbol("sindbl","OblectamentaOpcode"));
        }
        void emitCosDouble(){
            v.push_back(ceps::interpreter::mk_symbol("cosdbl","OblectamentaOpcode"));
        }
        void emitTanDouble(){
            v.push_back(ceps::interpreter::mk_symbol("tandbl","OblectamentaOpcode"));
        }
        void emitAtanDouble(){
            v.push_back(ceps::interpreter::mk_symbol("atandbl","OblectamentaOpcode"));
        }
        void emitExpDouble(){
            v.push_back(ceps::interpreter::mk_symbol("expdbl","OblectamentaOpcode"));
        }


        std::vector<ceps::ast::node_t> listing() const { return v;}
};

template<>
    void ceps::vm::oblectamenta::ComputationGraph<CepsComputeGraphNotationTraverser,CepsOblectamentaMnemonicsEmitter>::compile 
    (CepsComputeGraphNotationTraverser& graph_def,CepsOblectamentaMnemonicsEmitter& emitter){
        using namespace std;
        map<string,pair<size_t,size_t>> addrs;
        map<string,size_t> predef_array_offs;
        

        auto get_addr = [&] (CepsComputeGraphNotationTraverser::expr e) {
            auto array_ref{e.as_array_ref()};
            if (!array_ref) return size_t{};
            auto addr_info{addrs[array_ref->id]}; 
            size_t addr{addr_info.first + array_ref->idx * array_ref->width};
            return addr;
        };
        //First step compute array sizes and addresses

        for(size_t i{0}; i < graph_def.size(); ++i){
            auto stmt{graph_def[i]};
            auto assign_expr{stmt.is_assign_op()};
            if (!assign_expr){
                auto fcall {stmt.as_expr().as_funccall()};
                if("array" == fcall->fid.name){
                    //handle declarations
                    auto args{fcall->arguments()};
                    if (args.size() == 3){
                        auto sym{args[0].as_symbol()};
                        auto id{args[1].as_id()};
                        auto mod{args[2].as_funccall()};
                        if (sym && id  && mod && mod->fid.name == "base" && mod->arguments().size() == 1){
                            auto base_addr{mod->arguments()[0].as_int()};
                            if (base_addr) {
                                predef_array_offs[ sym->first ] = *base_addr;
                            }                    
                        }
                    }                    
                }
                continue;
            }
            auto left_side{assign_expr->lhs()};
            auto right_side{assign_expr->rhs()};
            auto array_ref{left_side.as_array_ref()};
            const auto width = 8;// assume doubles
            if (array_ref){
                auto ar{*array_ref};
                if ( addrs[ar.id].second < width * (ar.idx + 1)) 
                 addrs[ar.id].second = width * (ar.idx + 1);
            }
            traverse(
                [&](decltype(right_side) e){ 
                    auto array_ref{e.as_array_ref()}; 
                    if (array_ref){
                        auto ar{*array_ref};
                        if ( addrs[ar.id].second < width * (ar.idx + 1)) 
                            addrs[ar.id].second = width * (ar.idx + 1);
                        return false;
                    } 
                    return true;
                }, right_side);
        }
        size_t cur_mem_addr{};

        for (auto& e: addrs)
        {
            auto it{predef_array_offs.find(e.first)};
            if (it == predef_array_offs.end()) continue;
            e.second.first = it->second;
            auto t{e.second.first + e.second.second};
            if (t > cur_mem_addr) cur_mem_addr = t;
        }

        for (auto& e: addrs)
        {
            auto it{predef_array_offs.find(e.first)};
            if (it != predef_array_offs.end()) continue;

            e.second.first = cur_mem_addr;
            cur_mem_addr += e.second.second;             
        }

        /*for (auto& e: addrs)
        {

            cout << e.first << " " << e.second.first << " " << e.second.second << '\n';
        }*/

        //second step: generate code

        for(size_t i{0}; i < graph_def.size(); ++i){
            auto stmt{graph_def[i]};
            auto assign_expr{stmt.is_assign_op()};
            if (!assign_expr) continue;
            auto right_side{assign_expr->rhs()};
            //Emit code for right hand side
            function<bool(CepsComputeGraphNotationTraverser::expr)> parse_fn;
            parse_fn = [&](CepsComputeGraphNotationTraverser::expr e){
                    auto aref{e.as_array_ref()};
                    auto bop{e.as_binary_operation()};
                    if (aref){
                        size_t addr  = get_addr(e);
                        emitter.emitLoadDouble(addr);
                        return false;
                    } else if (bop){
                        traverse(parse_fn, bop->rhs());
                        traverse(parse_fn, bop->lhs());
                        if (bop->op == "*"){
                            emitter.emitMulDouble();
                        } else  if (bop->op == "-"){
                            emitter.emitSubDouble();
                        } else  if (bop->op == "+"){
                            emitter.emitAddDouble();
                        } else if (bop->op == "/"){
                            emitter.emitDivDouble();
                        } 
                        return false;
                    }
                    auto simple_funccall{e.as_noary_or_unary_funccall()};
                    if (simple_funccall && simple_funccall->arg ){
                        traverse(parse_fn,simple_funccall->argument());
                        if (simple_funccall->fid.name == "sin")
                         emitter.emitSinDouble();
                        else if (simple_funccall->fid.name == "cos")
                         emitter.emitCosDouble();
                        else if (simple_funccall->fid.name == "tan")
                         emitter.emitTanDouble();
                        else if (simple_funccall->fid.name == "atan")
                         emitter.emitAtanDouble();
                        else if (simple_funccall->fid.name == "exp")
                         emitter.emitExpDouble();
                        return false;

                    }
                    return true;
                };
            traverse(
                parse_fn,
                right_side
            );

            //Emit store
            auto left_side{assign_expr->lhs()};
            size_t addr  = get_addr(left_side);                        
            emitter.emitStoreDouble(addr);
        }       
    }

ceps::ast::node_t cepsplugin::operation(ceps::ast::node_callparameters_t params){
    ast_proc_prolog    
    auto data = get_first_child(params);    
    if (!is<Ast_node_kind::structdef>(data)) {
        return mk_undef();
    }
    auto& ceps_struct = *as_struct_ptr(data);
    if (name(ceps_struct) == "run" && children(ceps_struct).size()){
        for(auto e : children(ceps_struct))
         if (is<Ast_node_kind::structdef>(e) && name(as_struct_ref(e)) == "vm" ){
            auto maybe_vm {read_value<VMEnv>(as_struct_ref(e))};
            if (maybe_vm){
                auto& vm{*maybe_vm};
                vm.run(0);
                return ast_rep<ceps::vm::oblectamenta::VMEnv&> (vm);
            } else {
                return mk_undef();
            }
         }
    } else if (name(ceps_struct) == "compile_diffprog" && children(ceps_struct).size()){
        for(auto e : children(ceps_struct))
            if (is<Ast_node_kind::structdef>(e) && name(as_struct_ref(e)) == "differentiable_program" ){
                auto& diff_struct = as_struct_ref(e);
            for(auto e : children(diff_struct))
                if (is<Ast_node_kind::structdef>(e) && name(as_struct_ref(e)) == "computation_graph" ){
                    auto& computation_graph = as_struct_ref(e);
                    CepsComputeGraphNotationTraverser traverser{children(computation_graph)};
                    CepsOblectamentaMnemonicsEmitter emitter;
                    ComputationGraph<CepsComputeGraphNotationTraverser,CepsOblectamentaMnemonicsEmitter> comp_graph;
                    comp_graph.compile(traverser,emitter); 
                    auto result = mk_struct("asm");
                    ceps::ast::children(*result) = emitter.listing();
                    return result;
                }
            }
    }
    return mk_undef();
}

extern "C" void init_plugin(IUserdefined_function_registry* smc)
{
  cepsplugin::plugin_master = smc->get_plugin_interface();
  cepsplugin::plugin_master->reg_ceps_phase0plugin("operation", cepsplugin::operation);
  cepsplugin::plugin_master->reg_ceps_phase0plugin("obj", cepsplugin::obj);
}					
				