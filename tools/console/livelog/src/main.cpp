#include <iostream>
#include "core/include/livelog/livelogger.hpp"
#include "core/include/sockets/rdwrn.hpp"
#include "core/include/sm_livelog_storage_utils.hpp"


livelog::Livelogger* log_entries_peer;

void comm_stream_thread(int id,
			     std::string ip,
			     std::string port)
{
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
		 std::cout <<"len="<< len << std::endl;
		 if (len == 0) //sentinel read
		  break;
		 if (buffer_size < len){
			 if (buffer != nullptr) delete[] buffer;
			 buffer = new char[buffer_size = len * 2];
		 }
		 int r=0;
		 if(len != (r=readn(cfd,buffer,len))) { auto e= errno;std::cout <<"error:" << e << " r=" << r << std::endl;break;}
		 auto ch = ((livelog::Livelogger::Storage::chunk*) buffer);
		 std::cout << "data:"<<std::endl;
		 std::cout <<"id="<< ch->id_ << " len=" << ch->len_ << " what="<<ch->what_<<" value="<< *((int*)ch->data()) << std::endl;

		}

		std::this_thread::sleep_for(2.0s);

	}
	if (conn_established)close(cfd);
}



int main(int argc, char * argv[]){
 using namespace std::chrono_literals;
 log_entries_peer = new livelog::Livelogger(200,20000000);
 sm4ceps::Livelogger_sink livelogger_sink(log_entries_peer);
 livelogger_sink.run();
 livelog::Livelogger::Storage::id_t next_id = -1;
 for(;;){
	 std::this_thread::sleep_for(.25s);log_entries_peer->flush();
	 {
		 std::lock_guard<std::mutex> lk1(log_entries_peer->mutex_trans2consumer());
		 for_each(log_entries_peer->trans_storage(), [&](char* data,livelog::Livelogger::Storage::id_t id,livelog::Livelogger::Storage::what_t what, livelog::Livelogger::Storage::len_t len ){
			 if (next_id > id) return true;
			 next_id = id+1;
			 if (what == sm4ceps::STORAGE_WHAT_CURRENT_STATES){
				 sm4ceps::extract_current_states_raw(data,len,[&](std::vector<int> states){
					 for(auto const & e : states) std::cout << e << " ";
					 std::cout << std::endl;
				 });
			 }
		 });
	 }
 }
 //comm_stream_thread(0,"127.0.0.1","3000");
 return 0;
}
