#ifndef COMMON_H
#define COMMON_H

#include <mutex>
#include <thread>
#include <atomic>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <set>
#include <unordered_set>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>


#ifdef __gnu_linux__
 #include <arpa/inet.h>
 #include <sys/socket.h>
 #include <netdb.h>
 #include <unistd.h>
 #include <endian.h>
 #include <sys/timerfd.h>
 #include <sys/types.h>
 #include <ifaddrs.h>
#else
 #ifdef _WIN32
  #ifndef WIN32_LEAN_AND_MEAN
   #define WIN32_LEAN_AND_MEAN
  #endif
  #include <windows.h>
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #include <iphlpapi.h>
  #undef min
  #undef max
 #pragma comment(lib, "Ws2_32.lib")
 #include <intrin.h>
 static inline std::uint64_t be64toh(std::uint64_t i) { return _byteswap_uint64(i); }
 static inline std::uint64_t htobe64(std::uint64_t i) { return _byteswap_uint64(i); }
 static inline std::uint32_t htobe32(std::uint32_t i) { return htonl(i); }
 static inline  void bzero(void *s, size_t n) { memset(s, 0, n); }
#endif
#endif


/*std::string host;
std::string port;
std::mutex global_mutex;
std::condition_variable global_mutex_cv;
*/

class QCoreApplication;
class QMainWindow;

extern std::vector<std::string> sys_info_available_ifs;

using Local_Interface = std::string;
using Remote_Interface = std::string;
using Remote_Interface_Type = std::string;
using Stream_Mapping = std::pair<Local_Interface,Remote_Interface>;
using Downstream_Mapping = Stream_Mapping;
using Upstream_Mapping = Downstream_Mapping;
using Hostname = std::string;
using Port = std::string;
using Simulation_Core = std::pair<Hostname,Port>;
struct ctrl_thread_info {
 std::atomic_bool shutdown{false};
 std::thread* ctrl_thread;
};



using mappings_downstream_t = std::map<Simulation_Core,std::vector< Downstream_Mapping > >;
using mappings_upstream_t = std::map<Simulation_Core,std::vector< Upstream_Mapping > >;
using info_out_channels_t = std::map<Simulation_Core,std::vector< std::pair<Remote_Interface,Remote_Interface_Type> > >;
using info_in_channels_t = std::map<Simulation_Core,std::vector< std::pair<Remote_Interface,Remote_Interface_Type> > >;
using ctrl_threads_t = std::map<Simulation_Core, std::shared_ptr<ctrl_thread_info>  >;

extern std::map<Simulation_Core,std::vector< Downstream_Mapping > > mappings_downstream;
extern std::map<Simulation_Core,std::vector< Upstream_Mapping > > mappings_upstream;
extern std::map<Simulation_Core,std::vector< std::pair<Remote_Interface,Remote_Interface_Type> > > info_out_channels;
extern std::map<Simulation_Core,std::vector< std::pair<Remote_Interface,Remote_Interface_Type> > > info_in_channels;
extern std::map<Simulation_Core, std::shared_ptr<ctrl_thread_info>  > ctrl_threads;
extern Simulation_Core current_core;
extern std::mutex global_mutex;

void ctrl_thread_fn(QCoreApplication* core_app,
                    QMainWindow* main_window,
                    Simulation_Core sim_core,
                    std::shared_ptr<ctrl_thread_info> ctrl);

template <typename T> T & sort_and_remove_duplicates(T & v){
    std::sort(v.begin(),v.end());
    auto it = std::unique (v.begin(), v.end());
    v.erase(it,v.end());
    return v;
}


#include <QEvent>

const auto CTRL_THREAD_ERROR_GETADDR_FAILED = 111;
const auto CTRL_THREAD_STATUS_CONNECTION_ESTABLISHED = 115;

class QCtrlThreadConnectionFailed : public QEvent{
   public:
   QCtrlThreadConnectionFailed():QEvent((QEvent::Type)(QEvent::User+CTRL_THREAD_ERROR_GETADDR_FAILED)){}
   std::string reason_;
   std::string& reason(){return reason_;}
};

class QCtrlThreadConnectionEstablished : public QEvent{
   public:
   QCtrlThreadConnectionEstablished():QEvent((QEvent::Type)(QEvent::User+CTRL_THREAD_STATUS_CONNECTION_ESTABLISHED)){}
   std::string msg_;
   std::string& msg(){return msg_;}
};

class MapSelectionEventConsumer{
 public:
    virtual void mapping_selection_changed(bool) = 0;
};


#endif // COMMON_H
