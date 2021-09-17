#ifndef LOG4CEPS_RECORDS_HPP_INC
#define LOG4CEPS_RECORDS_HPP_INC

#include "../log4ceps/include/log4ceps_state.hpp"
#include "log4ceps_states.hpp"
#include "log4ceps_events.hpp"
#include "../log4ceps/include/log4ceps_record.hpp"
#include<string>

namespace log4ceps_records{
 using Trace_t = log4ceps::State_record <
   log4ceps_states::Timestamp_t,
   log4ceps_states::Current_states_t
 >;
struct Trace{Trace_t rec;Trace_t& record(){return rec;} };

 void make_record_from_states(Trace& rec);

}
#endif
