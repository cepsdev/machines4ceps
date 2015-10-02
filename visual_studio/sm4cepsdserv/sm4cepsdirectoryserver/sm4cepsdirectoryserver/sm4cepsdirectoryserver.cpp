/*
Copyright 2015, cpsdev (cepsdev@hotmail.com).
All rights reserved.
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

* Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above
copyright notice, this list of conditions and the following
disclaimer in the documentation and/or other materials provided
with the distribution.
* Neither the name of Google Inc. nor the names of its
contributors may be used to endorse or promote products derived
from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#include "core/include/cmdline_utils.hpp"
#include <iostream>
#include <sys/stat.h>
#include <signal.h>
#include <sys/types.h>
#include <limits>
#include <cstring>
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <thread>
#include <set>
#include <sstream>



#ifdef __gnu_linux__ || __CYGWIN__ || __OpenBSD__
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
static inline int write(SOCKET s, const void* buf, int len, int flags = 0) { return send(s, (char*)buf, len, flags); }
static inline int close(SOCKET s) { return closesocket(s); }

#endif
#endif

#ifndef _WIN32

#include <dirent.h>
#include <unistd.h>

#else

#include "windows.h"

#endif

bool SHUTDOWN_SIGNAL = false;

void fatal(std::string const & msg)
{
	std::cerr<<"***Error: "<<  msg;
	exit(-1);
}

namespace sm4ceps {
	namespace directory_service {
		struct Nodedescriptor {
			std::string name;
			std::string path;
			std::string ip;
			std::string port;

			bool operator < (Nodedescriptor const & rhs) const {
				return path + name < rhs.path + rhs.name;
			}
		};

		class Nodedirectory {
			std::set<Nodedescriptor> nodes_;
			mutable std::recursive_mutex nodesm_;
		public:
			void register_node(std::string name,
				std::string path,
				std::string ip,
				std::string port);
		};
	}
}

void sm4ceps::directory_service::Nodedirectory::register_node(std::string name,
	std::string path,
	std::string ip,
	std::string port) 
{
	std::lock_guard<std::recursive_mutex> g(nodesm_);
	nodes_.insert(Nodedescriptor{name,path,ip,port});
}

void directory_server_main(std::string port, sm4ceps::directory_service::Nodedirectory* nd)
{
	std::vector<std::thread*> client_handler_threads;
	struct addrinfo hints;
	struct addrinfo* result, *rp;
	int lfd;
	
	memset(&hints, 0, sizeof(hints));
	hints.ai_canonname = nullptr;
	hints.ai_addr = nullptr;
	hints.ai_next = nullptr;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_family = AF_INET;

	hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;
	hints.ai_protocol = IPPROTO_TCP;

	if (getaddrinfo(nullptr, port.c_str(), &hints, &result) != 0)
		fatal("getaddrinfo failed");

	int optval = 1;

	for (rp = result; rp; rp = rp->ai_next)
	{
		lfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (lfd == -1) continue;
		if (setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, (char*)&optval, sizeof(optval))) fatal( "setsockopt");
		if (bind(lfd, rp->ai_addr, rp->ai_addrlen) == 0) break;
		close(lfd);
	}
	if (rp == nullptr) fatal( "comm_dispatcher_thread:Could not bind socket to any address.port=" + port);

	if (listen(lfd, 5) == -1)fatal( "listen");

	freeaddrinfo(result);

	for (; !SHUTDOWN_SIGNAL;)
	{
	
		socklen_t addrlen = sizeof(struct sockaddr_storage);
		struct sockaddr_storage claddr;
		
		int cfd = accept(lfd, (struct sockaddr*) &claddr, &addrlen);
		if (cfd == -1) continue;
		
		//client_handler_threads.push_back(new std::thread(*handler_fn, id, smc, claddr, cfd));
	}
	for (auto tp : client_handler_threads) tp->join();

}

int main(int argc, char* argv[])
{
	auto result_cmd_line = process_cmd_line(argc, argv);
#ifdef _WIN32
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;
	wVersionRequested = MAKEWORD(2, 2);
	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0) {
		std::cerr << "***Error: WSAStartup failed(" << err << ")\n";
		return 1;
	}

	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
		std::cerr << "***Error:Could not find a usable version of Winsock.dll\n";
		WSACleanup();
		return 1;
	}
#else
	signal(SIGPIPE, SIG_IGN);
#endif

	sm4ceps::directory_service::Nodedirectory nodedirectory;

	std::thread mt(directory_server_main, result_cmd_line.port, &nodedirectory);

	mt.join();
    return 0;
}

