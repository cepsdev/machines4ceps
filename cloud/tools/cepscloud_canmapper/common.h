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

 extern std::vector<std::string> sys_info_available_ifs;

using Local_Interface = std::string;
using Remote_Interface = std::string;
using Downstream_Mapping = std::pair<Local_Interface,Remote_Interface>;
using Upstream_Mapping = std::pair<Remote_Interface,Local_Interface>;
using Hostname = std::string;
using Port = std::string;
using Simulation_Core = std::pair<Hostname,Port>;
struct ctrl_thread_info {
 std::atomic_bool shutdown{false};
 std::thread* ctrl_thread;
} ;

extern std::map<Simulation_Core,std::vector< Downstream_Mapping > > mappings_downstream;
extern std::map<Simulation_Core,std::vector< Upstream_Mapping > > mappings_upstream;
extern std::map<Simulation_Core, std::shared_ptr<ctrl_thread_info>  > ctrl_threads;
extern Simulation_Core current_core;

void ctrl_thread_fn(std::shared_ptr<ctrl_thread_info>);

template <typename T> T & sort_and_remove_duplicates(T & v){
    std::sort(v.begin(),v.end());
    auto it = std::unique (v.begin(), v.end());
    v.erase(it,v.end());
    return v;
}

#endif // COMMON_H
