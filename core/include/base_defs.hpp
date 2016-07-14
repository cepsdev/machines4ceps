#ifndef INC_BASE_DEFS_INC
#define INC_BASE_DEFS_INC

#include <mutex>
#include <thread>
#include <iostream>

const auto CEPS_REP_PUGI_XML_DOC = 12;
const auto CEPS_REP_PUGI_XML_PATH = 13;
const auto CEPS_REP_PUGI_XML_NODE_SET = 14;

#ifdef __gnu_linux__
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <endian.h>
#include <sys/timerfd.h>
static inline int closesocket(int s) { return close(s); }
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
static inline int write(SOCKET s, const void* buf, int len, int flags = 0) { return send(s, (char*)buf, len, flags); }
static inline int close(SOCKET s) { return closesocket(s); }
#ifndef __GNUC__
 typedef std::int64_t ssize_t;
#endif
#include <intrin.h>


static inline std::uint64_t be64toh(std::uint64_t i) { return _byteswap_uint64(i); }
static inline std::uint64_t htobe64(std::uint64_t i) { return _byteswap_uint64(i); }
static inline std::uint32_t htobe32(std::uint32_t i) { return htonl(i); }
static inline  void bzero(void *s, size_t n) { memset(s, 0, n); }

#endif
#endif

struct Debuglogger{
	std::string toprint_;
	bool p_;
	static std::recursive_mutex mtx_;

	Debuglogger(std::string const & s,bool p = false):toprint_(s),p_(p)
	{
		if(p_) std::cout << "[DEBUG][ENTER]["<< toprint_.c_str()<< "]\n";
	}

	~Debuglogger()
	{
		if(p_) std::cout << "[DEBUG][LEAVE]["<< toprint_.c_str()<< "]\n";
	}

	template<typename T> Debuglogger& operator <<(T const & v)
	{
		if (!p_) return *this;
		std::cout << v;
		return *this;
	}
};
#ifdef __GNUC__
#define __funcname__ __PRETTY_FUNCTION__
#else
#define __funcname__ __FUNCSIG__
#endif

extern bool PRINT_DEBUG_INFO;

#define DEBUG_FUNC_PROLOGUE 	Debuglogger debuglog(__funcname__,PRINT_DEBUG_INFO);
#define DEBUG (std::lock_guard<std::recursive_mutex>(debuglog.mtx_),debuglog << "[DEBUG]", debuglog)
#define INFO (std::lock_guard<std::recursive_mutex>(debuglog.mtx_),debuglog << "[INFO]", debuglog)
#define ERRORLOG (std::lock_guard<std::recursive_mutex>(debuglog.mtx_),debuglog << "[ERROR]", debuglog)
#define DEBUG_FUNC_PROLOGUE2 	Debuglogger debuglog(__funcname__,PRINT_DEBUG_INFO);

#define NODEBUG_PRINT



#endif
