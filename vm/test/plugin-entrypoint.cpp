
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


namespace cepsplugin{
    static Ism4ceps_plugin_interface* plugin_master = nullptr;
    static const std::string version_info = "INSERT_NAME_HERE v0.1";
    static constexpr bool print_debug_info{true};
    ceps::ast::node_t run_oblectamenta_bytecode(ceps::ast::node_callparameters_t params);
}


void compile_and_run(){
    using namespace ceps::vm::oblectamenta;
    VMEnv vm;
    std::vector<int> vars;
    for (auto i : {1,2,3,4,5,6,7,8,9,10}) vars.push_back(vm.store(i));
    for (auto i : {1,2}) vm.push(i);
    auto prog1 = emit<Opcode::addi32>(vm.text());
    emit<Opcode::halt>(vm.text());
    vm.run(prog1);
    vm.dump(std::cout);
    std::cout << "============================\n";
    
    for (auto i : {1,2,3,4,5,6,7,8,9,10}) vm.push(i);vm.dump(std::cout);
    auto prog2 = emit<Opcode::addi32>(vm.text());
    emit<Opcode::addi32>(vm.text());
    emit<Opcode::addi32>(vm.text());
    emit<Opcode::addi32>(vm.text());
    emit<Opcode::addi32>(vm.text());
    emit<Opcode::addi32>(vm.text());
    emit<Opcode::addi32>(vm.text());
    emit<Opcode::addi32>(vm.text());
    emit<Opcode::addi32>(vm.text());
    emit<Opcode::halt>(vm.text());
    vm.run(prog2);
    std::cout << "============================\n";
    vm.dump(std::cout);
    auto prog3 = emit<Opcode::addi32>(vm.text());
    emit<Opcode::halt>(vm.text());
    vm.run(prog3);
    std::cout << "============================\n";
    vm.dump(std::cout);
    std::cout << "============================\n";
    vm.reset();
    vm.dump(std::cout);
    auto prog4 = emit<Opcode::noop>(vm.text());

    emit<Opcode::ldi32>(vm.text(),vars[0]);
    for(size_t i = 1; i < vars.size();++i ){
        emit<Opcode::ldi32>(vm.text(),vars[i]);
        emit<Opcode::addi32>(vm.text());
    }
    int result_prog4 = vm.store(55);
    emit<Opcode::sti32>(vm.text(),result_prog4);
    emit<Opcode::halt>(vm.text());

    for(size_t i = 0; i < 100L; ++i) {
        vm.run(prog4); 
        assert(vm.read_store<int>(result_prog4) == 55);
    }
    vm.dump(std::cout);
    auto prog5 = emit<Opcode::noop>(vm.text());
    int prog5_a = vm.store(550);
    int prog5_b = vm.store(55);
    int prog5_res = vm.store(0);

    emit<Opcode::ldi32>(vm.text(),prog5_b);
    emit<Opcode::ldi32>(vm.text(),prog5_a);
    emit<Opcode::subi32>(vm.text());
    emit<Opcode::sti32>(vm.text(),prog5_res);
    emit<Opcode::halt>(vm.text());
    vm.run(prog5);
    assert(vm.read_store<int>(prog5_res) == 495);

    auto prog6 = emit<Opcode::noop>(vm.text());
    int prog6_a = vm.store(550.1);
    int prog6_b = vm.store(55.0);
    int prog6_res = vm.store(495.1);

    emit<Opcode::lddbl>(vm.text(),prog6_b);
    emit<Opcode::lddbl>(vm.text(),prog6_a);
    emit<Opcode::subdbl>(vm.text());
    emit<Opcode::stdbl>(vm.text(),prog6_res);
    
    emit<Opcode::halt>(vm.text());
    vm.run(prog6);
    vm.dump(std::cout);
   
    assert(vm.read_store<double>(prog6_res) == 495.1);


    /*
    
    for(;prog7_counter < prog7_limit; prog7_counter += prog7_step);
    
    */
    auto prog7 = emit<Opcode::noop>(vm.text());
    int prog7_counter = vm.store(0);
    int prog7_limit = 10;
    int prog7_step = 1;
    int prog7_limit_loc = vm.store(prog7_limit);
    int prog7_step_loc = vm.store(prog7_step);
    emit<Opcode::ldi32>(vm.text(),prog7_counter);
    emit<Opcode::ldi32>(vm.text(),prog7_limit_loc);
    auto backpatch = emit<Opcode::blteq>(vm.text(),0);
    emit<Opcode::ldi32>(vm.text(),prog7_counter);
    emit<Opcode::ldi32>(vm.text(),prog7_step_loc);
    emit<Opcode::addi32>(vm.text());
    emit<Opcode::sti32>(vm.text(),prog7_counter);
    emit<Opcode::buc>(vm.text(),prog7);
    auto last = emit<Opcode::halt>(vm.text());
    patch(vm.text(), backpatch + 1, last );
    vm.run(prog7);
    assert(vm.read_store<int>(prog7_counter) == prog7_limit - (prog7_limit % prog7_step) + (prog7_limit % prog7_step != 0 ? prog7_step : 0)  );
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

void oblectamenta_assembler(ceps::vm::oblectamenta::VMEnv& vm, std::vector<ceps::ast::node_t> mnemonics)
{
 using namespace ceps::ast;
 
 for (size_t stmt_pos{}; stmt_pos < mnemonics.size(); ++stmt_pos){
    
    auto e{mnemonics[stmt_pos]};
	std::string sym_name;
	std::string sym_kind;
	std::vector<node_t> args;
    
    if(is<Ast_node_kind::symbol>(e) && kind(as_symbol_ref(e)) == "OblectamentaOpcode" ){
        auto& mnemonic{name(as_symbol_ref(e))};
        auto it{ceps::vm::oblectamenta::mnemonics.find(mnemonic)};
        if (it == ceps::vm::oblectamenta::mnemonics.end()) 
         throw std::string{"oblectamenta_assembler: unknown opcode: '"+ mnemonic+"'" };
        auto v{it->second};
        if (get<2>(v)) get<2>(v)(vm.text());      
    } else if (is_a_symbol_with_arguments( e,sym_name,sym_kind,args)) {
        if (sym_kind == "OblectamentaOpcode"){
            auto& mnemonic{sym_name};
            if (args.size() == 1 && is<Ast_node_kind::int_literal>(args[0])){
                auto arg{value(as_int_ref(args[0]))};
                auto it{ceps::vm::oblectamenta::mnemonics.find(mnemonic)};
                if (it == ceps::vm::oblectamenta::mnemonics.end()) 
                    throw std::string{"oblectamenta_assembler: unknown opcode: '"+ mnemonic+"'" };
                auto v{it->second};
                if (get<3>(v)) get<3>(v)(vm.text(),arg); else throw std::string{"oblectamenta_assembler: illformed parameter list for '"+ mnemonic+"'" };
            }
        }
    } 
 } //for
}//function

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
            std::cerr << msg << '\n';
        }
     }
     else if (name(*as_struct_ptr(e)) == "data" && children(*as_struct_ptr(e)).size() ){
        auto& data_rep{*as_struct_ptr(e)};
        for(auto e: children(data_rep)){
            if (is<Ast_node_kind::int_literal>(e)) r.store(value(as_int_ref(e)));
            else if (is<Ast_node_kind::float_literal>(e)) r.store(value(as_double_ref(e)));
            else if (is<Ast_node_kind::string_literal>(e)) r.store(value(as_string_ref(e)));
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
}					
				