#include "core/include/livelog/livelogger.hpp"
#include "core/include/sm_livelog_storage_ids.hpp"
#include "core/include/state_machine.hpp"
#include "core/include/sm_execution_ctxt.hpp"
#include <stdexcept>

#ifndef SM_LIVELOG_STORAGE_UTILS_INC
#define SM_LIVELOG_STORAGE_UTILS_INC

namespace sm4ceps{
  void livelog_write(livelog::Livelogger& live_logger,executionloop_context_t::states_t const &  states);
  void storage_write(livelog::Livelogger::Storage& storage,std::map<int,std::string> i2s, std::mutex* mtx);
  bool storage_read_entry(std::map<int,std::string>& i2s, char * data);

  class Livelogger_source{
	  livelog::Livelogger* livelogger_ = nullptr;
  public:
	  Livelogger_source() = default;
	  Livelogger_source(livelog::Livelogger* livelogger):livelogger_{livelogger} {}
	  void log_current_states(executionloop_context_t & ec) {
		  if (!livelogger_) throw std::runtime_error("Livelogger_source is detached");
		  livelog_write(*livelogger_,ec.current_states);
	  }
  };
  class Livelogger_sink{
      livelog::Livelogger* livelogger_ = nullptr;
      void comm_stream_fn(int id,
      			     std::string ip,
      			     std::string port);
      std::string port_ = "3000";
      std::string ip_   = "127.0.0.1";
      std::thread* attached_thread_ = nullptr;

  public:
	  Livelogger_sink() = default;
	  Livelogger_sink(livelog::Livelogger* livelogger):livelogger_{livelogger} {}
	  void run_sync();
	  void run();
	  void set_peer_addr(std::string ip, std::string port) {ip_=ip;port_=port;}
  };

  template <typename F> bool extract_current_states_raw(char * data, std::size_t len, F f){
	  std::size_t num = len / sizeof(std::int32_t);
	  std::vector<int> v;v.resize(num);
	  for(std::size_t j = 0; j != num; ++j)v[j] = *(((int*)data) + j);
	  f(v);
	  return true;
  }

};

#endif
