#include <string> 
#include <vector>
#include <map>
#include <algorithm>
#include <set> 
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "ceps_all.hh"

#include "log4kmw_state.hpp"
#include "log4kmw_record.hpp"
#include "log4kmw_logger.hpp"
#include "log4kmw_states.hpp"
#include "log4kmw_records.hpp"
#include "log4kmw_loggers.hpp"

namespace log4kmw_test{ namespace meta_info{void log_print_ceps( log4kmw_loggers::logger_Trace_t& logger) ;}}
int main(int argc, char **argv){
	log4kmw_loggers::logger_Trace.logger().init(log4kmw::persistence::memory_mapped_file("trace.bin", 1024*1024*8, false));
	log4kmw_test::meta_info::log_print_ceps(log4kmw_loggers::logger_Trace);
	return 0;
}