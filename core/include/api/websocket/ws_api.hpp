#ifndef INC_WS_API_INC
#define INC_WS_API_INC

#include <string> 
#include <vector>
#include <map>
#include <algorithm>
#include <set> 
#include "ceps_all.hh"
#include "core/include/serialization.hpp"
#include "core/include/state_machine_simulation_core_plugin_interface.hpp"
class State_machine_simulation_core;

class Websocket_interface{
    void dispatcher();
    void handler(int sck);
    State_machine_simulation_core* smc_ = nullptr;
    std::string port_;
    std::thread* dispatcher_thread_ = nullptr;
    std::mutex handler_threads_status_mutex_;
    using thread_status_type = std::tuple<std::thread*,bool,int>;
    std::vector< thread_status_type > handler_threads_status_;
public:
    Websocket_interface(State_machine_simulation_core* smc,std::string port = "8181"):smc_{smc},port_{port}{}
    void start();
    std::string& port() {return port_;}
};

#endif
