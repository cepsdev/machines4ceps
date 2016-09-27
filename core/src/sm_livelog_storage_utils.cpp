#include "core/include/livelog/livelogger.hpp"
#include "core/include/sm_livelog_storage_ids.hpp"
#include "core/include/sm_livelog_storage_utils.hpp"
#include "core/include/sockets/rdwrn.hpp"

void sm4ceps::livelog_write(livelog::Livelogger& live_logger,executionloop_context_t::states_t const &  states){
 std::size_t len = 0;
 for(auto const & s : states) if (s != 0) len+=sizeof(std::uint32_t);
 live_logger.write_ext(sm4ceps::STORAGE_WHAT_CURRENT_STATES,[&](char * data){
	 std::uint32_t counter = 0;
	 for(auto const & s : states){
		 if (s != 0) {*((std::uint32_t*)data) = counter; data += sizeof(std::uint32_t);} ++counter;
	 }
  },len);
}

void sm4ceps::storage_write(livelog::Livelogger::Storage& storage,std::map<int,std::string> i2s, std::mutex* mtx){
	std::size_t len = sizeof(std::size_t);
	for(auto const & s : i2s)  len+=sizeof(std::int32_t)+s.second.length()+sizeof(std::int32_t);
	 storage.write_ext(sm4ceps::STORAGE_WHAT_INT32_TO_STRING_MAP,[&](char * data){
		 std::uint32_t counter = 0;
		 *((std::size_t*) data) = i2s.size();
		 data+=sizeof(std::size_t);
		 for(auto const & s : i2s){
			 *((std::int32_t*)data) = s.first;
			 data += sizeof(std::int32_t);
			 *((std::int32_t*)data) = s.second.length();
			 data += sizeof(std::int32_t);
			 memcpy(data,s.second.c_str(),s.second.length());
		 }
	  },len,mtx);
}

void sm4ceps::Livelogger_sink::comm_stream_fn(int id,
			     std::string ip,
			     std::string port){
	using namespace std::chrono_literals;

	int cfd;
	struct addrinfo hints;
	struct addrinfo *result, *rp;
	bool conn_established = false;

	for(;;)
	{
		rp = nullptr;result = nullptr;

		if (!conn_established)
		{
			for(;rp == nullptr;)
			{

			memset(&hints, 0, sizeof(struct addrinfo));
			hints.ai_canonname = NULL;
			hints.ai_addr = NULL;
			hints.ai_next = NULL;
			hints.ai_family = AF_INET;

			hints.ai_socktype = SOCK_STREAM;
            //hints.ai_flags = AI_NUMERICSERV;
			if (getaddrinfo(ip.c_str(), port.c_str(), &hints, &result) != 0){
				std::this_thread::sleep_for(std::chrono::microseconds(1000));continue;
			}

			if (result == nullptr) {
				std::this_thread::sleep_for(std::chrono::microseconds(1000)); continue;
			}
			 for (rp = result; rp != NULL; rp = rp->ai_next) {
			  cfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
			  if (cfd == -1) {  continue; }
			  if (connect(cfd, rp->ai_addr, rp->ai_addrlen) != -1)break;
			  close(cfd);
			 }
			 if (result != nullptr) freeaddrinfo(result);
			 if (rp == nullptr) {
				 std::this_thread::sleep_for(std::chrono::microseconds(1000000));continue;
			 }
			}
			conn_established = true;
		}

		auto cmd = livelog::CMD_GET_NEW_LOG_ENTRIES;
		cmd = htonl(cmd);

		if ( ( write(cfd, (char*) &cmd,sizeof(cmd) )) !=	 sizeof(cmd) )
		{
			close(cfd);
			conn_established=false;
			continue;
		}

		livelog::Livelogger::Storage::len_t len = 0;
		char * buffer = nullptr;
		std::size_t buffer_size = 0;
		for(;;){
		 if ( sizeof(len) != read(cfd,&len,sizeof(len)) ){
			break;
		 }
		 if (len == 0) //sentinel read
		  break;
		 if (buffer_size < len){
			 if (buffer != nullptr) delete[] buffer;
			 buffer = new char[buffer_size = len * 2];
		 }
		 int r=0;
		 if(len != (r=readn(cfd,buffer,len))) { auto e= errno;std::cout <<"error:" << e << " r=" << r << std::endl;break;}

		 if (livelogger_){
			 auto ch = (livelog::Livelogger::Storage::chunk*)buffer;
			 auto data = buffer + sizeof(livelog::Livelogger::Storage::chunk);
			 livelogger_->write(ch->what(),data,ch->len(),ch->id());
		 }
		}
		std::this_thread::sleep_for(0.1s);
	}
	if (conn_established)close(cfd);
}

void sm4ceps::Livelogger_sink::run_sync(){
	comm_stream_fn(-1,ip_,port_);
}

void sm4ceps::Livelogger_sink::run(){
	attached_thread_ = new std::thread{&sm4ceps::Livelogger_sink::comm_stream_fn,this,-1,ip_,port_};
}
