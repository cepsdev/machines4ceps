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
extern std::vector<std::string> available_ifs;

#endif // COMMON_H
