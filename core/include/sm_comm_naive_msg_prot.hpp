#ifndef INC_SM_COMM_NAIVE_MSG_PROT_HPP
#define INC_SM_COMM_NAIVE_MSG_PROT_HPP

#include <string> 
#include <vector>
#include <map>
#include <algorithm>
#include <set>
#ifdef __gnu_linux__
 #include <arpa/inet.h>
 #include <sys/socket.h>
 #include <netdb.h>
 #include <unistd.h>
#else
#ifdef _WIN32
 #ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN
 #endif
 #include <windows.h>
 #include <winsock2.h>
 #include <ws2tcpip.h>
 #include <iphlpapi.h>
 #pragma comment(lib, "Ws2_32.lib")
#endif
#endif





class State_machine_simulation_core;

struct nmp_header{
	uint32_t id;
	uint32_t len;
};

void comm_dispatcher_thread(int id,
							State_machine_simulation_core* smc,
							std::string ip,
							std::string port,
							void (*handler_fn) (int,State_machine_simulation_core* , sockaddr_storage,int));
void comm_sender_thread(int id,State_machine_simulation_core* smc,std::string ip,std::string port, bool passive = true);
bool comm_dispatcher_thread_started();
bool nmp_send_raw(State_machine_simulation_core* smc,std::string ip, std::string port, uint32_t what, uint32_t len, const char * data);
void register_dispatcher_handler(uint32_t id,void (State_machine_simulation_core::*)(nmp_header,char*));
void nmp_consumer_thread_fn(int id,State_machine_simulation_core* smc,sockaddr_storage claddr,int sck);
void nmp_monitoring_thread_fn(int id,State_machine_simulation_core* smc,sockaddr_storage claddr,int sck);


const uint32_t NMP_USER_DEFINED = 9999;

const uint32_t NMP_REQ_SYSTEMSTATES = 100;
const uint32_t NMP_SYSTEMSTATES = 101;
const uint32_t NMP_ACK = 0;
const uint32_t NMP_ERR = 1;

const auto NMP_PAYLOAD_INT = 0;
const auto NMP_PAYLOAD_DOUBLE = 1;
const auto NMP_PAYLOAD_STRING = 2;
const auto NMP_PAYLOAD_SYSTEMSTATE = 3;

#endif
