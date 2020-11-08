#include "log4ceps_records.hpp"

void log4ceps_records::make_record_from_states(Trace& rec)
{
 std::unique_lock<std::mutex> lock(log4ceps_states::global_lock);
 rec.record() = log4ceps_records::Trace_t(
   log4ceps_states::read_state_Timestamp_no_lock(),
   log4ceps_states::read_state_Current_states_no_lock()
 );
}
