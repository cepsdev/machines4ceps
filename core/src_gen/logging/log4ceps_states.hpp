#ifndef LOG4CEPS_STATES_HPP_INC
#define LOG4CEPS_STATES_HPP_INC

#include "../log4ceps/include/log4ceps_state.hpp"
#include "../log4ceps/include/log4ceps_dynamic_bitset.hpp"
#include<string>
#include<thread>
#include<mutex>
#include<time.h>

namespace log4ceps_states{
 extern std::mutex global_lock;
 extern log4ceps::State<int> Current_event;
 extern log4ceps::State<log4ceps::Dynamic_bitset> Current_states;
 extern log4ceps::State<timespec> Timestamp;

 typedef log4ceps::State<int> Current_event_t;
 typedef log4ceps::State<log4ceps::Dynamic_bitset> Current_states_t;
 typedef log4ceps::State<timespec> Timestamp_t;

 void write_state_Current_event(Current_event_t const &);
 Current_event_t   read_state_Current_event(); 
 Current_event_t   read_state_Current_event_no_lock(); 
 void write_state_Current_states(Current_states_t const &);
 Current_states_t   read_state_Current_states(); 
 Current_states_t   read_state_Current_states_no_lock(); 
 void write_state_Timestamp(Timestamp_t const &);
 Timestamp_t   read_state_Timestamp(); 
 Timestamp_t   read_state_Timestamp_no_lock(); 
}

#endif
