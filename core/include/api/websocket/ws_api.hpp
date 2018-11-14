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
    void handle_subscribe_coverage(int sck);
    int next_subscribe_channel_id = 0;
    void handle_subscribe_coverage_thread(int subscribe_channel_id,int sck);
    std::map<int,std::thread*> subscribe_channels2thread;
public:
    Websocket_interface(State_machine_simulation_core* smc,std::string port = "8181"):smc_{smc},port_{port}{}
    void start();
    std::string& port() {return port_;}
    static std::string query(State_machine_simulation_core* smc,std::string);
};

#endif
