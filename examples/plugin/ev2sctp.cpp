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
#include "ceps_all.hh"
#include "core/include/state_machine_simulation_core_reg_fun.hpp"
#include "core/include/state_machine_simulation_core_plugin_interface.hpp"
#include "core/include/state_machine_simulation_core.hpp"


#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <stdlib.h>

static Ism4ceps_plugin_interface* plugin_master = nullptr;


static ceps::ast::Nodebase_ptr ev2sctp_plugin(ceps::ast::Call_parameters* params){
    using namespace ceps::ast;
    //std::cout << *params << std::endl;

    ceps::parser_env::Scope scope;
    auto sym = scope.insert("mme_type");
    sym->category = ceps::parser_env::Symbol::Category::VAR;
    sym->payload = ceps::interpreter::mk_int_node(1);

    ceps::ast::Nodebase_ptr p = static_cast<ceps::ast::Nodebase_ptr>(plugin_master->evaluate_fragment_in_global_context(params->children()[0],&scope));

    if (p == nullptr) std::cerr << "**********" << std::endl;
    p = p->clone();

    std::cout << *p << std::endl;
    
    return nullptr;
}

extern "C" void init_plugin(IUserdefined_function_registry* smc)
{
  (plugin_master = smc->get_plugin_interface())->reg_ceps_plugin("route_events_sctp",ev2sctp_plugin);
  std::cerr << "Registered ev2sctp plugin" << std::endl;
}
