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


#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <stdlib.h>

static Ism4ceps_plugin_interface* plugin_master = nullptr;


static ceps::ast::Nodebase_ptr ev2sctp_plugin(ceps::ast::Call_parameters* params){
    using namespace ceps::ast;
    std::cout << *params << std::endl;
    return nullptr;
}

extern "C" void init_plugin(IUserdefined_function_registry* smc)
{
  (plugin_master = smc->get_plugin_interface())->reg_ceps_plugin("route_events_sctp",ev2sctp_plugin);
  std::cerr << "Registered ev2sctp plugin" << std::endl;
}
