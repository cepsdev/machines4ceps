#include <iostream>
#include "core/include/livelog/livelogger.hpp"


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
		if ( sizeof(len) != read(cfd,&len,sizeof(len)) ){
			break;
		}
		std::cout << len << std::endl;


		std::this_thread::sleep_for(2.0s);

	}
	if (conn_established)close(cfd);
}

int main(int argc, char * argv[]){
 comm_stream_thread(0,"127.0.0.1","3000");
 return 0;
}
