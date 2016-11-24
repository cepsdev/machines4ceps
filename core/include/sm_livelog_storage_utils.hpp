#include "core/include/livelog/livelogger.hpp"
#include "core/include/sm_livelog_storage_ids.hpp"
#include "core/include/state_machine.hpp"
#include "core/include/sm_execution_ctxt.hpp"
#include "core/include/state_machine_simulation_core_plugin_interface.hpp"
#include <stdexcept>

#ifndef SM_LIVELOG_STORAGE_UTILS_INC
#define SM_LIVELOG_STORAGE_UTILS_INC

namespace sm4ceps{
  void livelog_write(livelog::Livelogger& live_logger,executionloop_context_t::states_t const &  states);
  void livelog_write(livelog::Livelogger& live_logger,std::pair<std::string,std::vector<sm4ceps_plugin_int::Variant>> const & event);
  void livelog_write_console(livelog::Livelogger& live_logger,std::string const &);
  void livelog_write_info(livelog::Livelogger& live_logger,std::string const &);

  void storage_write(livelog::Livelogger::Storage& storage,std::map<int,std::string> i2s, std::mutex* mtx);
  bool storage_read_entry(std::map<int,std::string>& i2s, char * data);

  void storage_write_event(livelog::Livelogger::Storage& storage,
   std::string ev_name,
   std::vector<sm4ceps_plugin_int::Variant> const & params,
   std::mutex* mtx);

  std::size_t storage_write_event(char * data,
   std::string ev_name,
   std::vector<sm4ceps_plugin_int::Variant> const & params,
   bool write=false);

  std::size_t storage_write(char * data,
   std::vector<sm4ceps_plugin_int::Variant> const & v,
   bool write=false);

  std::size_t storage_write(char * data,
   std::string const & s,
   bool write=false);

  std::size_t storage_write(char * data,
   std::int64_t,
   bool write=false);

  std::size_t storage_write(char * data,
   double,
   bool write=false);

  std::size_t storage_read(char * data,
   std::vector<sm4ceps_plugin_int::Variant> & v);

  std::size_t storage_read(char * data,
   std::string & s);

  std::size_t storage_read(char * data,
   std::int64_t& v);

  std::size_t storage_read(char * data,
   double &);

  std::size_t storage_read_variant(char * data,sm4ceps_plugin_int::Variant& v );

  bool storage_read_event(std::string& ev_name,
   std::vector<sm4ceps_plugin_int::Variant>& params,
   char * data);


  class Livelogger_source{
	  livelog::Livelogger* livelogger_ = nullptr;
  public:
	  Livelogger_source() = default;
	  Livelogger_source(livelog::Livelogger* livelogger):livelogger_{livelogger} {}
	  void log_current_states(executionloop_context_t & ec) {
		if (!livelogger_) throw std::runtime_error("Livelogger_source is detached");
		livelog_write(*livelogger_,ec.current_states);
	  }
	  void log_event(std::pair<std::string,std::vector<sm4ceps_plugin_int::Variant>> const & v) {
	 	if (!livelogger_) throw std::runtime_error("Livelogger_source is detached");
	 	livelog_write(*livelogger_,v);
	  }
	  void log_console(std::string const & v) {
	 	if (!livelogger_) throw std::runtime_error("Livelogger_source is detached");
	    livelog_write_console(*livelogger_,v);
	  }
	  void log_info(std::string const & v) {
		if (!livelogger_) throw std::runtime_error("Livelogger_source is detached");
                livelog_write_info(*livelogger_,v);
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

 template <typename F> bool extract_event_raw(char * data, std::size_t len, F f){
        std::pair<std::string,std::vector<sm4ceps_plugin_int::Variant>> ev;
        storage_read_event(std::get<0>(ev),std::get<1>(ev),data);
        f(ev);
        return true;
 }

 template <typename F> bool extract_string_raw(char * data, std::size_t len, F f){
         std::string s;
         storage_read(data+sizeof(short),s);
         f(s);
         return true;
  }
};

#endif
