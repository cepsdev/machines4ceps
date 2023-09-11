
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
#include <netinet/sctp.h> 

#include "ceps_ast.hh"
#include "core/include/state_machine_simulation_core.hpp"

#include "core/include/vm/vm_base.hpp"
#include "core/include/vm/oblectamenta-assembler.hpp"

namespace cepsplugin{
    static Ism4ceps_plugin_interface* plugin_master = nullptr;
    static const std::string version_info = "INSERT_NAME_HERE v0.1";
    static constexpr bool print_debug_info{true};
    ceps::ast::node_t run_oblectamenta_bytecode(ceps::ast::node_callparameters_t params);
    ceps::ast::node_t obj(ceps::ast::node_callparameters_t params);
}


template<typename T> T 
    fetch(ceps::ast::Struct&);
template<typename T> T 
    fetch(ceps::ast::node_t);

template<> ceps::vm::oblectamenta::VMEnv fetch<ceps::vm::oblectamenta::VMEnv>(ceps::ast::Struct& );


template<typename T> 
    std::optional<T> read_value(ceps::ast::Struct& s);
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

template<typename T> ceps::ast::node_t ast_rep (T entity);
template<typename T> ceps::ast::node_t ast_rep (T entity, ceps::vm::oblectamenta::VMEnv&);

////////


template<> bool check<ceps::vm::oblectamenta::VMEnv>(ceps::ast::Struct & s)
{
    using namespace ceps::ast;
    return name(s) == "vm";
}

template<> ceps::vm::oblectamenta::VMEnv fetch<ceps::vm::oblectamenta::VMEnv>(ceps::ast::Struct& s)
{
    using namespace ceps::ast;
    using namespace ceps::vm::oblectamenta;
    VMEnv r{};

    //r.text().push_back( (VMEnv::text_t::value_type) ceps::vm::oblectamenta::Opcode::halt);std::cerr << "/////\n";

    for (auto e : children(s)){
     if (!is<Ast_node_kind::structdef>(e)) continue;
     if (name(*as_struct_ptr(e)) == "text" && children(*as_struct_ptr(e)).size() ){
        try{
            oblectamenta_assembler(r,children(*as_struct_ptr(e)));
        } catch (std::string const& msg){
            std::cerr << "***Error oblectamenta_assembler:" <<  msg << '\n' << '\n' <<"Erroneous segment:\n" << *as_struct_ptr(e) << '\n' << '\n';
        }
     }
     else if (name(*as_struct_ptr(e)) == "data" && children(*as_struct_ptr(e)).size() ){
        auto& data_rep{*as_struct_ptr(e)};
        for(auto e: children(data_rep)){
            if (is<Ast_node_kind::int_literal>(e)) r.store(value(as_int_ref(e)));
            else if (is<Ast_node_kind::float_literal>(e)) r.store(value(as_double_ref(e)));
            else if (is<Ast_node_kind::string_literal>(e)) r.store(value(as_string_ref(e)));
            else if (is<Ast_node_kind::symbol>(e) && kind(as_symbol_ref(e))=="OblectamentaDataLabel"){
                r.data_labels()[name(as_symbol_ref(e))] = r.data_size();
            }
        }        
     }
     else if (name(*as_struct_ptr(e)) == "stack" && children(*as_struct_ptr(e)).size() ){
        auto& stack_rep{*as_struct_ptr(e)};
        for(auto e: children(stack_rep)){
            if (is<Ast_node_kind::int_literal>(e)) r.push(value(as_int_ref(e)));
            else if (is<Ast_node_kind::float_literal>(e)) r.push(value(as_double_ref(e)));
            else if (is<Ast_node_kind::string_literal>(e)) r.push(value(as_string_ref(e)));
        }
     }
    }
   return r;
}

struct ser_wrapper_stack{
    ceps::vm::oblectamenta::VMEnv::stack_t value;
};

struct ser_wrapper_data{
    ceps::vm::oblectamenta::VMEnv::data_t value;
};

struct ser_wrapper_text{
    ceps::vm::oblectamenta::VMEnv::text_t value;
};

template<> ceps::ast::node_t ast_rep (ser_wrapper_stack stack, ceps::vm::oblectamenta::VMEnv& vm ){
    using namespace ceps::ast;
    using namespace ceps::interpreter;
    
    auto result = mk_struct("stack");
    auto& ch {children(*result)};
    for (size_t i = 0; i < stack.value.size() && i < vm.stack_top_pos(); ++i )
        ch.push_back(mk_int_node(stack.value[i]));
    return result;
}

template<> ceps::ast::node_t ast_rep (ser_wrapper_data data){
    using namespace ceps::ast;
    using namespace ceps::interpreter;
    
    auto result = mk_struct("data");

    auto& ch {children(*result)};
    for (auto e:data.value)
     ch.push_back(mk_int_node(e));
    
    return result;
}

template<> ceps::ast::node_t ast_rep (ser_wrapper_text text){
    using namespace ceps::ast;
    using namespace ceps::interpreter;
    
    auto result = mk_struct("text");
    //children(*result).push_back(ast_rep(ser_wrapper_stack{vm.stack()}));
    return result;
}


template<> ceps::ast::node_t ast_rep<ceps::vm::oblectamenta::VMEnv&> (ceps::vm::oblectamenta::VMEnv& vm){
    using namespace ceps::ast;
    using namespace ceps::interpreter;
    
    auto result = mk_struct("vm");
    children(*result).push_back(ast_rep(ser_wrapper_stack{vm.stack()},vm));
    children(*result).push_back(ast_rep(ser_wrapper_data{vm.data()}));
    children(*result).push_back(ast_rep(ser_wrapper_text{vm.text()}));
 
    return result;
}


////////

/*template<int n> void h() {
    std::cout << "no partam: "<< n << '\n';
}
template<int n> void h(int i){
    std::cout << "param: " << n << '\n';
}*/
ceps::ast::node_t cepsplugin::obj(ceps::ast::node_callparameters_t params){
    return get_first_child(params);
}


ceps::ast::node_t cepsplugin::run_oblectamenta_bytecode(ceps::ast::node_callparameters_t params){
    using namespace std;
    using namespace ceps::ast;
    using namespace ceps::interpreter;
    using namespace ceps::vm::oblectamenta;



    
    auto data = get_first_child(params);    
    

    if (!is<Ast_node_kind::structdef>(data)) {
        return mk_undef();
    }
    auto& ceps_struct = *as_struct_ptr(data);
    //compile_and_run();
    if (name(ceps_struct) == "vm"){
        auto maybe_vm {read_value<VMEnv>(ceps_struct)};
        
        if (maybe_vm){
            auto& vm{*maybe_vm};
            emit<Opcode::halt>(vm.text());
            vm.run(0);
            return ast_rep<ceps::vm::oblectamenta::VMEnv&> (vm);
        } else {

        }
    }
    return mk_undef();
}

extern "C" void init_plugin(IUserdefined_function_registry* smc)
{
  cepsplugin::plugin_master = smc->get_plugin_interface();
  cepsplugin::plugin_master->reg_ceps_phase0plugin("run_oblectamenta_bytecode", cepsplugin::run_oblectamenta_bytecode);
  cepsplugin::plugin_master->reg_ceps_phase0plugin("obj", cepsplugin::obj);

}					
				