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
#include <iterator>
#include <regex>
#include <future>
#include <chrono>
#include<sstream>



#ifdef __gnu_linux__
 #include <arpa/inet.h>
 #include <sys/socket.h>
 #include <netdb.h>
 #include <unistd.h>
 #include <endian.h>
 #include <sys/timerfd.h>
 #include <sys/types.h>
 #include <ifaddrs.h>
 #include <stdio.h>
 #include <stdlib.h>
 #include <unistd.h>
 #include <string.h>
 #include <net/if.h>
 #include <sys/types.h>
 #include <sys/socket.h>
 #include <sys/ioctl.h>
 #include <linux/can.h>
 #include <linux/can/raw.h>
 #define UNIX_API 
#else
 #ifdef _WIN32
  using ssize_t = long long;
  #define WIN_API
  #define PCAN_API
  #ifndef WIN32_LEAN_AND_MEAN
   #define WIN32_LEAN_AND_MEAN
  #endif
  #include <windows.h>
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #include <iphlpapi.h>
  inline int close(int s) { return closesocket(s); }
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

#ifdef WIN_API
#define INIT_SYS_ERR_HANDLING int syscall_result = 0;
#define STORE_SYS_ERR syscall_result = WSAGetLastError();
#define THROW_ERR_INET { char* buffer=nullptr;\
 auto r = FormatMessage( \
  FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER, \
  NULL, \
  syscall_result, \
  0, \
  (LPSTR)&buffer, \
  0,\
  NULL); \
 throw net::exceptions::err_inet{r!=0?buffer:""}; \
 }
#endif

 template<typename F> struct cleanup {
	 F f_;
	 cleanup(F f) :f_(f) {}
	 ~cleanup() { f_(); }
 };

#define CLEANUP(x)  auto destructor = x; cleanup<decltype(destructor)> trigger_destructor{ destructor };

 namespace ceps {
	 namespace cloud {
		 extern std::vector<std::string> sys_info_available_ifs;
		 extern std::vector<std::string> check_available_ifs();

		 using Local_Interface = std::string;
		 using Remote_Interface = std::string;
		 using Remote_Interface_Type = std::string;
		 using Stream_Mapping = std::pair<Local_Interface, Remote_Interface>;
		 using Downstream_Mapping = Stream_Mapping;
		 using Upstream_Mapping = Downstream_Mapping;
		 using Hostname = std::string;
		 using Port = std::string;
		 using Simulation_Core = std::pair<Hostname, Port>;

		 struct ctrl_thread_info {
			 std::atomic_bool shutdown{ false };
			 std::thread* ctrl_thread;
		 };


		 struct gateway_thread_info {
			 std::atomic_bool shutdown{ false };
			 std::thread* gateway_thread;
			 std::atomic_bool down{ false };
		 };

		 using mappings_downstream_t = std::map<Simulation_Core, std::vector< Downstream_Mapping > >;
		 using mappings_upstream_t = std::map<Simulation_Core, std::vector< Upstream_Mapping > >;
		 using info_out_channels_t = std::map<Simulation_Core, std::vector< std::pair<Remote_Interface, Remote_Interface_Type> > >;
		 using info_in_channels_t = std::map<Simulation_Core, std::vector< std::pair<Remote_Interface, Remote_Interface_Type> > >;
		 using ctrl_threads_t = std::map<Simulation_Core, std::shared_ptr<ctrl_thread_info>  >;
		 using downstream_threads_t = std::map<Simulation_Core, std::vector< std::shared_ptr<gateway_thread_info> >  >;

		 extern std::map<Simulation_Core, std::vector< Downstream_Mapping > > mappings_downstream;
		 extern std::map<Simulation_Core, std::vector< Upstream_Mapping > > mappings_upstream;
		 extern std::map<Simulation_Core, std::vector< std::pair<Remote_Interface, Remote_Interface_Type> > > info_out_channels;
		 extern std::map<Simulation_Core, std::vector< std::pair<Remote_Interface, Remote_Interface_Type> > > info_in_channels;
		 extern std::map<Simulation_Core, std::shared_ptr<ctrl_thread_info>  > ctrl_threads;
		 extern downstream_threads_t downstream_threads;
		 extern Simulation_Core current_core;
		 extern std::mutex global_mutex;

		 extern void ctrl_thread_fn(Simulation_Core sim_core,
			 std::shared_ptr<ctrl_thread_info> ctrl);


		 extern void gateway_fn(
			 Simulation_Core sim_core,
			 std::shared_ptr<gateway_thread_info> ctrl,
			 int local_sck,
			 int extended_can,
			 std::string remote_interface);

		 std::tuple<bool, std::string, std::vector<std::pair<std::string, std::string>>> read_virtual_can_msg(int sck, std::string& unconsumed_data);
		 std::pair<bool, std::string> get_virtual_can_attribute_content(std::string attr, std::vector<std::pair<std::string, std::string>> const & http_header);
		 Simulation_Core cmdline_read_remote_host(int argc, char* argv[]);
		 namespace exceptions{
			 class err_vcan_api : public std::runtime_error {
			 public:
				 err_vcan_api(std::string what) : runtime_error{ what } {}
			 };
		 }
		 namespace vcan_api {
			 std::vector<std::pair<Remote_Interface, std::string>> fetch_out_channels(Simulation_Core sim_core);
			 std::tuple<bool, std::string, std::vector<std::pair<std::string, std::string>>> send_cmd(int sock, std::string command);
		 }
	 }
	 namespace misc {
		 template <typename T> T & sort_and_remove_duplicates(T & v) {
			 std::sort(v.begin(), v.end());
			 auto it = std::unique(v.begin(), v.end());
			 v.erase(it, v.end());
			 return v;
		 }
	 }
 }

 namespace net {
	 namespace inet {
		 int establish_inet_stream_connect(std::string remote, std::string port);
	 }
	 namespace can {
		 struct can_info {
			 enum BAUD_RATE {
				 BAUD_1M, BAUD_800K, BAUD_500K, BAUD_250K, BAUD_125K,
				 BAUD_100K, BAUD_95K, BAUD_83K, BAUD_50K, BAUD_47K,
				 BAUD_33K, BAUD_20K, BAUD_10K, BAUD_5K
			 };
			 BAUD_RATE br = BAUD_1M;
		 };
		 ceps::cloud::Local_Interface get_local_endpoint(std::string);
		 int get_local_endpoint_handle(std::string s);
		 can_info get_local_endpoint_info(std::string);
		 void set_local_endpoint_info(std::string, can_info info);
	 }
	 namespace exceptions {
		 class err_inet : public std::runtime_error {
		 public:
			 err_inet(std::string what) : runtime_error{ what } {}
		 };
		 class err_getaddrinfo : public err_inet {
		 public:
			 err_getaddrinfo(std::string what) :err_inet{ what } {}
		 };
		 class err_connect : public err_inet {
		 public:
			 err_connect(std::string what) :err_inet{ what } {}
		 };
		 class err_send : public err_inet {
		 public:
			 err_send(std::string what) :err_inet{ what } {}
		 };
		 class err_recv : public err_inet {
		 public:
			 err_recv(std::string what) :err_inet{ what } {}
		 };
		 class err_can : public std::runtime_error {
		 public:
			 err_can(std::string what) : runtime_error{ what } {}
		 };
	 }
 }

#ifdef PCAN_API
#include "PCANBasic.h"

 typedef TPCANStatus(__stdcall *CAN_GetValue_t)(
	 TPCANHandle,
	 TPCANParameter,
	 void*,
	 DWORD);

 typedef TPCANStatus(__stdcall *CAN_Initialize_t)(
	 TPCANHandle Channel,
	 TPCANBaudrate Btr0Btr1,
	 TPCANType HwType /*_DEF_ARG*/,
	 DWORD IOPort /*_DEF_ARG*/,
	 WORD Interrupt /*_DEF_ARG*/);


 typedef TPCANStatus(__stdcall *CAN_InitializeFD_t)(
	 TPCANHandle Channel,
	 TPCANBitrateFD BitrateFD);


 typedef TPCANStatus(__stdcall *CAN_Uninitialize_t)(
	 TPCANHandle Channel);

 typedef TPCANStatus(__stdcall *CAN_Reset_t)(
	 TPCANHandle Channel);

 typedef TPCANStatus(__stdcall *CAN_Read_t)(
	 TPCANHandle Channel,
	 TPCANMsg* MessageBuffer,
	 TPCANTimestamp* TimestampBuffer);

 typedef TPCANStatus(__stdcall *CAN_ReadFD_t)(
	 TPCANHandle Channel,
	 TPCANMsgFD* MessageBuffer,
	 TPCANTimestampFD *TimestampBuffer);

 typedef TPCANStatus(__stdcall *CAN_Write_t)(
	 TPCANHandle Channel,
	 TPCANMsg* MessageBuffer);

 typedef TPCANStatus(__stdcall *CAN_WriteFD_t)(
	 TPCANHandle Channel,
	 TPCANMsgFD* MessageBuffer);

 typedef TPCANStatus(__stdcall *CAN_FilterMessages_t)(
	 TPCANHandle Channel,
	 DWORD FromID,
	 DWORD ToID,
	 TPCANMode Mode);

 typedef TPCANStatus(__stdcall *CAN_SetValue_t)(
	 TPCANHandle Channel,
	 TPCANParameter Parameter,
	 void* Buffer,
	 DWORD BufferLength);

 typedef TPCANStatus(__stdcall *CAN_GetErrorText_t)(
	 TPCANStatus Error,
	 WORD Language,
	 LPSTR Buffer);

 typedef TPCANStatus(__stdcall *CAN_GetValue_t)(
	 TPCANHandle,
	 TPCANParameter,
	 void*,
	 DWORD);


 namespace pcan_api {
	 extern CAN_GetValue_t getvalue;
	 extern CAN_Initialize_t initialize;
	 extern CAN_InitializeFD_t initialize_fd;
	 extern CAN_Uninitialize_t uninitialize;
	 extern CAN_Reset_t reset;
	 extern CAN_Read_t read;
	 extern CAN_ReadFD_t read_fd;
	 extern CAN_Write_t write;
	 extern CAN_WriteFD_t write_fd;
	 extern CAN_FilterMessages_t filtermessages;
	 extern CAN_SetValue_t setvalue;
	 extern CAN_GetErrorText_t geterrortext;
	 extern std::vector< std::pair<unsigned int, std::string> > all_channels;
	 extern std::vector< std::pair<std::string, net::can::can_info> > all_channels_info;
 }

#endif


#endif // COMMON_H
