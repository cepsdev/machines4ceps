#ifndef LOG4CEPS_LOGGERS_HPP_INC
#define LOG4CEPS_LOGGERS_HPP_INC

#include "../log4ceps/include/log4ceps_state.hpp"
#include "log4ceps_states.hpp"
#include "log4ceps_events.hpp"
#include "log4ceps_records.hpp"
#include "../log4ceps/include/log4ceps_logger.hpp"
#include "log4ceps_events.hpp"
#include <string>
#include <thread>
#include <mutex>

namespace log4ceps_loggers{
 struct logger_Trace_t{log4ceps::Logger<log4ceps_records::Trace_t, log4ceps::persistence::memory_mapped_file>log;
 log4ceps::Logger<log4ceps_records::Trace_t, log4ceps::persistence::memory_mapped_file>& logger(){return log;}};
 extern logger_Trace_t logger_Trace;
 extern std::mutex logger_mutex_Trace;
 void log_event(log4ceps_events::Step const &, logger_Trace_t &);
}
#endif
