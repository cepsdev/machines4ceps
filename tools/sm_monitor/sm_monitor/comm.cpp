#include <vector>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/types.h>
#include <limits>
#include <cstring>
#include <map>
#include <algorithm>
#include <set>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>

#include "ceps_all.hh"

#include "sm4ceps/core/include/sm_ev_comm_layer.hpp"


extern void fatal(int,std::string);
extern void warn(int,std::string);
extern int controlled_thread_id = -1;
extern int running_thread_id = -1;

std::mutex sysstates_table_m;
std::vector<std::pair<std::string,ceps::ast::Nodebase_ptr>> sysstates_table;

std::mutex comm_threads_m;
std::vector<std::thread*> comm_threads;
std::vector<bool> comm_threads_stop_signals;
std::vector<int> comm_threads_status;

extern const int COMM_STATUS_CONNECTING = 0;
extern const int COMM_STATUS_CONNECTED = 1;
extern const int COMM_STATUS_TERMINATED = 2;
extern const int COMM_STATUS_ERR_ALLOC_SOCKET_FAILED = -1;
extern const int COMM_STATUS_ERR_GETADDRINFO_FAILED = -2;
extern const int COMM_STATUS_ERR_WRITE_FAILED = -3;
extern const int COMM_STATUS_ERR_READ_FAILED = -4;
extern const int COMM_STATUS_ERR_INVALID_PROTOCOL = -5;


extern std::string send_command;
extern std::mutex send_command_mtx;

void run_as_monitor(int id,std::string ip, std::string port){

  long update_interval = 100000;
  int cfd;

  struct addrinfo hints;
  struct addrinfo *result, *rp;

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_canonname = NULL;
  hints.ai_addr = NULL;
  hints.ai_next = NULL;
  hints.ai_family = AF_UNSPEC;

  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_NUMERICSERV;
  if (getaddrinfo(ip.c_str(), port.c_str(), &hints, &result) != 0){
    std::lock_guard<std::mutex> g(comm_threads_m);
   comm_threads_status[id] = COMM_STATUS_ERR_GETADDRINFO_FAILED;
   return;
   }


  for (rp = result; rp != NULL; rp = rp->ai_next) {
   cfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
   if (cfd == -1)	continue;
   if (connect(cfd, rp->ai_addr, rp->ai_addrlen) != -1)break;
   close(cfd);
  }
  freeaddrinfo(result);
  if (rp == nullptr) {
   std::lock_guard<std::mutex> g(comm_threads_m);
   comm_threads_status[id] = COMM_STATUS_ERR_ALLOC_SOCKET_FAILED;
   return;
  }

  {
     std::lock_guard<std::mutex> g(comm_threads_m);
     comm_threads_status[id] = COMM_STATUS_CONNECTED;
  }

  for(;;)
  {
   {
    std::lock_guard<std::mutex> g(comm_threads_m);
    if (comm_threads_stop_signals[id])break;
   }

    nmp_header header;
    std::string command;

    {
      std::lock_guard<std::mutex> g(send_command_mtx);
      command=send_command;
      send_command.clear();
    }
    if (command.length()){
     header.id = htonl(NMP_EVAL_CEPS_EXPRESSION);
     header.len = htonl(command.length());
    } else {
    header.id = htonl(NMP_REQ_SYSTEMSTATES);
    header.len = htonl(0);
    }

    if (write(cfd, &header,sizeof(header) ) !=	 sizeof(nmp_header)){
    close(cfd);
    std::lock_guard<std::mutex> g(comm_threads_m);
    comm_threads_status[id] = COMM_STATUS_ERR_WRITE_FAILED;
    return;
    }

    if (command.length()){
      if (write(cfd, command.c_str(),command.length() ) !=command.length()){
       close(cfd);
       std::lock_guard<std::mutex> g(comm_threads_m);
       comm_threads_status[id] = COMM_STATUS_ERR_WRITE_FAILED;
       return;
      }
    }

    auto r = recv(cfd,&header,sizeof(nmp_header),0);
    if (r != sizeof(nmp_header)) {
     close(cfd);
     std::lock_guard<std::mutex> g(comm_threads_m);
     comm_threads_status[id] = COMM_STATUS_ERR_READ_FAILED;
     return;
    }
    header.id = htonl(header.id);
    header.len = htonl(header.len);

    if (header.id == NMP_SYSTEMSTATES){
      auto data = new char[header.len];
      if (data == nullptr){fatal(-1,"[ERROR_CLIENT_HANDLER_THREAD][ALLOC_FAILED].");close(cfd);return;}
      ssize_t already_read = 0;
      ssize_t n = 0;
      for(; (already_read < header.len) && (n = recv(cfd,data+already_read,header.len-already_read,0)) > 0;already_read+=n);
      if(already_read < header.len){fatal(-1, "[ERROR_CLIENT_HANDLER_THREAD][READ_FAILED].");close(cfd);return;}
      size_t offs = 0;

      std::lock_guard<std::mutex> g(sysstates_table_m);sysstates_table.clear();

      for(;offs < header.len;)
      {
        int id = ntohl(*((int*)(data+offs)));offs+=sizeof(int);
        if (id == NMP_PAYLOAD_SYSTEMSTATE){
          std::string name;
          offs+=ceps::deserialize_value(name, data+offs, header.len-offs);
          int nmp_payload_id = ntohl(*(int*)(data+offs));
          if (nmp_payload_id == NMP_PAYLOAD_INT)	{
            offs+=sizeof(int);
            int v;
            ceps::ast::Unit_rep::sc_t m,kg,s,ampere,kelvin,mol,candela;
            auto r = ceps::deserialize_value(v, data+offs, header.len-offs);offs+=r;
            v = ntohl(v);
            r = ceps::deserialize_value(m, data+offs, header.len-offs);m = ntohl(m);offs+=r;
            r = ceps::deserialize_value(kg, data+offs, header.len-offs);kg = ntohl(kg);offs+=r;
            r = ceps::deserialize_value(s, data+offs, header.len-offs);s = ntohl(s);offs+=r;
            r = ceps::deserialize_value(ampere, data+offs, header.len-offs);ampere = ntohl(ampere);offs+=r;
            r = ceps::deserialize_value(kelvin, data+offs, header.len-offs);kelvin = ntohl(kelvin);offs+=r;
            r = ceps::deserialize_value(mol, data+offs, header.len-offs);mol = ntohl(mol);offs+=r;
            r = ceps::deserialize_value(candela, data+offs, header.len-offs);candela = ntohl(candela);offs+=r;
            sysstates_table.push_back(
                                        std::make_pair(
                                                name,
                                                new ceps::ast::Int( v, ceps::ast::Unit_rep(m,kg,s,ampere,kelvin,mol,candela), nullptr, nullptr, nullptr)
                                        )
                                );
          } else if (nmp_payload_id == NMP_PAYLOAD_DOUBLE)	{
            offs+=sizeof(int);
            double v;
            ceps::ast::Unit_rep::sc_t m,kg,s,ampere,kelvin,mol,candela;
            auto r = ceps::deserialize_value(v, data+offs, header.len-offs);offs+=r;
            r = ceps::deserialize_value(m, data+offs, header.len-offs);m = ntohl(m);offs+=r;
            r = ceps::deserialize_value(kg, data+offs, header.len-offs);kg = ntohl(kg);offs+=r;
            r = ceps::deserialize_value(s, data+offs, header.len-offs);s = ntohl(s);offs+=r;
            r = ceps::deserialize_value(ampere, data+offs, header.len-offs);ampere = ntohl(ampere);offs+=r;
            r = ceps::deserialize_value(kelvin, data+offs, header.len-offs);kelvin = ntohl(kelvin);offs+=r;
            r = ceps::deserialize_value(mol, data+offs, header.len-offs);mol = ntohl(mol);offs+=r;
            r = ceps::deserialize_value(candela, data+offs, header.len-offs);candela = ntohl(candela);offs+=r;
            sysstates_table.push_back(
                                        std::make_pair(name,
                                        new ceps::ast::Double( v, ceps::ast::Unit_rep(m,kg,s,ampere,kelvin,mol,candela), nullptr, nullptr, nullptr))
                                );
          } else if (nmp_payload_id == NMP_PAYLOAD_STRING)	{
            offs+=sizeof(int);
            std::string v;
            auto r = ceps::deserialize_value(v, data+offs, header.len-offs);offs+=r;
            sysstates_table.push_back(
                                        std::make_pair(name,
                                                new ceps::ast::String( v, nullptr, nullptr, nullptr)
                                ));
          } else {

              close(cfd);delete[] data;
              std::lock_guard<std::mutex> g(comm_threads_m);
              comm_threads_status[id] = COMM_STATUS_ERR_INVALID_PROTOCOL;
              return;

          }
         } else break;
        }//for

	}
	std::this_thread::sleep_for(std::chrono::microseconds(update_interval));
	}//for
	close(cfd);
	comm_threads_status[id] = COMM_STATUS_TERMINATED;
}

