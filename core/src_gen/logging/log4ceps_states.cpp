#include "log4ceps_states.hpp"


std::mutex log4ceps_states::global_lock;
log4ceps::State<int> log4ceps_states::Current_event(0);
log4ceps::State<log4ceps::Dynamic_bitset> log4ceps_states::Current_states(32);
log4ceps::State<timespec> log4ceps_states::Timestamp;

void log4ceps_states::write_state_Current_event( log4ceps_states::Current_event_t const & v)
{
  std::unique_lock<std::mutex> lock(global_lock);
  Current_event = v;
}

log4ceps_states::Current_event_t   log4ceps_states::read_state_Current_event()
{
  std::unique_lock<std::mutex> lock(global_lock);
  return log4ceps_states::Current_event;
}
log4ceps_states::Current_event_t   log4ceps_states::read_state_Current_event_no_lock()
{
  return log4ceps_states::Current_event;
}
void log4ceps_states::write_state_Current_states( log4ceps_states::Current_states_t const & v)
{
  std::unique_lock<std::mutex> lock(global_lock);
  Current_states = v;
}

log4ceps_states::Current_states_t   log4ceps_states::read_state_Current_states()
{
  std::unique_lock<std::mutex> lock(global_lock);
  return log4ceps_states::Current_states;
}
log4ceps_states::Current_states_t   log4ceps_states::read_state_Current_states_no_lock()
{
  return log4ceps_states::Current_states;
}
void log4ceps_states::write_state_Timestamp( log4ceps_states::Timestamp_t const & v)
{
  std::unique_lock<std::mutex> lock(global_lock);
  Timestamp = v;
}

log4ceps_states::Timestamp_t   log4ceps_states::read_state_Timestamp()
{
  std::unique_lock<std::mutex> lock(global_lock);
  return log4ceps_states::Timestamp;
}
log4ceps_states::Timestamp_t   log4ceps_states::read_state_Timestamp_no_lock()
{
  return log4ceps_states::Timestamp;
}
