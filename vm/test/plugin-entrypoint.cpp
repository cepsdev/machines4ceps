
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
    ceps::ast::node_t plugin_entrypoint(ceps::ast::node_callparameters_t params);
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

ceps::ast::node_t cepsplugin::plugin_entrypoint(ceps::ast::node_callparameters_t params){
    using namespace std;
    using namespace ceps::ast;
    using namespace ceps::interpreter;
    compile_and_run();

    auto data = get_first_child(params);    
    if (!is<Ast_node_kind::structdef>(data)) return nullptr;
    auto& ceps_struct = *as_struct_ptr(data);
    cout << "cepsplugin::plugin_entrypoint:\n";
    for(auto e : children(ceps_struct)){
        cout <<"\t"<< * e << "\n";
    }
    cout <<"\n\n";
    auto result = mk_struct("result");
    children(*result).push_back(mk_int_node(42));
    return result;
}

extern "C" void init_plugin(IUserdefined_function_registry* smc)
{
  cepsplugin::plugin_master = smc->get_plugin_interface();
  cepsplugin::plugin_master->reg_ceps_phase0plugin("INSERT_NAME_FOR_FUNCTION_HERE", cepsplugin::plugin_entrypoint);
}					
				