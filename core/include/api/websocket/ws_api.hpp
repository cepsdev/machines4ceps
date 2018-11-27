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
#include "core/include/threadsafequeue.hpp"
#include "core/include/state_machine.hpp"
#include "core/include/sm_execution_ctxt.hpp"

class State_machine_simulation_core;

class Websocket_interface{
public:
    struct coverage_update_msg_t{
     enum what_t {INIT,UPDATE,NEW_CHANNEL};
     what_t what;
     union {
         struct {
             int id;
             int socket;
         } channel;
     } payload;
     std::vector<int> coverage_state_table;
     std::vector<int> coverage_transitions_table;
     std::vector<executionloop_context_t::state_present_rep_t> current_states;
     executionloop_context_t::enter_times_t enter_times;
     executionloop_context_t::exit_times_t exit_times;
     std::chrono::time_point<std::chrono::high_resolution_clock> time_stamp;
    };

    struct subscribe_coverage_handler_ctxt_t{
        State_machine_simulation_core* smc;
        threadsafe_queue< coverage_update_msg_t,
                          std::queue<coverage_update_msg_t> >* q;
        int subscribe_channel_id;
        std::chrono::time_point<std::chrono::high_resolution_clock> last;
        long long delta_ms;
    };

private:
    threadsafe_queue<coverage_update_msg_t,std::queue<coverage_update_msg_t>>* coverage_channel_queue = nullptr;
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
    void handle_subscribe_coverage_thread(threadsafe_queue<coverage_update_msg_t,std::queue<coverage_update_msg_t>>* q);
    std::map<int,std::thread*> subscribe_channels2thread;
public:
    Websocket_interface(State_machine_simulation_core* smc,std::string port = "8181"):smc_{smc},port_{port}{}
    void start();
    std::string& port() {return port_;}
    static std::string query(State_machine_simulation_core* smc,std::string);
};

#endif
