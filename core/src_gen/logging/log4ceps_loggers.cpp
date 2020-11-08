#include "log4ceps_records.hpp"
#include "log4ceps_loggers.hpp"

 log4ceps_loggers::logger_Trace_t log4ceps_loggers::logger_Trace;
std::mutex log4ceps_loggers::logger_mutex_Trace;
 void log4ceps_loggers::log_event(log4ceps_events::Step const & ev, log4ceps_loggers::logger_Trace_t & log)
{
 if(!log.logger()) throw std::runtime_error("Logger 'Trace' invalid (not initialized?)");
 log4ceps_records::Trace rec;
 log4ceps_records::make_record_from_states(rec);
 {
  std::unique_lock<std::mutex> lock(log4ceps_loggers::logger_mutex_Trace);
  log.logger().append(rec.record());
 }
}

