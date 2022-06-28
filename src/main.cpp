/*
Copyright 2014, cepsdev (cepsdev@hotmail.com).
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

#include <signal.h>
#include <sys/types.h>
#include <limits>
#include <cstring>
#include "core/include/base_defs.hpp"


#include "core/include/state_machine.hpp"
#include "core/include/state_machine_simulation_core.hpp"
#include "main.hpp"


#define VERSION_SM4CEPS_MAJOR "0"
#define VERSION_SM4CEPS_MINOR "8.0"

vector < string > generated_sql_file_names;
bool DUMP_PLANTUML_REP_TO_COUT = false;
bool SIMULATION_PRINT_CURRENT_STATES = true;
auto SM_COUNTER = 0;
bool gen_doc = false;
std::string doc_out_dir;
State_machine* entry_machine = nullptr;



ostream& operator << (ostream& os, State_machine::State s)
{
 if (s.is_initial()) return os << "[*]";
 return os << s.id();
}



void dummy(int , std::string const & ) {}

void fatal(int code, std::string const & msg )
{
	stringstream ss;
	if (State_machine_simulation_core::ERR_FILE_OPEN_FAILED == code)
        ss << "Couldn't open '" << msg <<"'.";
	else if (State_machine_simulation_core::ERR_CEPS_PARSER == code)
        ss << "A parser exception occured in '" << msg << "'.";
	else ss << msg;
	throw runtime_error{ss.str()};
}

void warn(int code, std::string const & msg)
{
	if (WARN_XML_PROPERTIES_MISSING_PREFIX_DEFINITION == code)
		cerr << "\n***WARNING. No xml file prefix defined for '"
		<< msg
		<< "', will use default (empty string).\nIf you want different behaviour please add the following to the global namespace:\n\txml{gen_file_prefix{"
		<< msg
		<< "{\"MY_XML_FILENAME_PREFIX_HERE\";};};.\n";
	else if (WARN_CANNOT_FIND_TEMPLATE == code)
		cerr << "\n***WARNING. No template found which matches the path "
		<< msg
		<< "." << endl;
	else if (WARN_NO_INVOCATION_MAPPING_AND_NO_TABLE_DEF_FOUND == code)
		cerr << "\n***WARNING. There exists neither a xml invocation mapping nor a db2 table definition for  "
		<< msg
		<< ". No file for this particular object will be generated." << endl;
	else if (WARN_TESTCASE_ID_ALREADY_DEFINED == code)
		cerr << "\n***WARNING. Id already defined in another testcase precondition: "
		<< msg
		<< "." << endl;
	else if (WARN_TESTCASE_EMPTY == code)
		cerr << "\n***WARNING. Testcase precondition is empty: "
		<< msg
		<< "." << endl;
	else if (WARN_NO_STATEMACHINES == code)
		cerr << "\n***WARNING. No statemachines found"
		<< msg
		<< "." << endl;
	else
		cerr << "\n***WARNING. " << msg <<"."<< endl;

}

void process_simulation(ceps::ast::Nodeset& sim,ceps::Ceps_Environment& ceps_env,ceps::ast::Nodeset& universe);

void run_as_monitor(Result_process_cmd_line const & result_cmd_line){


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
	if (getaddrinfo(result_cmd_line.monitored_node_ip.c_str(), result_cmd_line.monitored_node_port.c_str(), &hints, &result) != 0)
	 fatal(-1,"getaddrinfo");

	for (rp = result; rp != NULL; rp = rp->ai_next) {
	 cfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
	 if (cfd == -1)	continue;
	 if (connect(cfd, rp->ai_addr, rp->ai_addrlen) != -1)break;
 	 close(cfd);
	}
	freeaddrinfo(result);
	if (rp == nullptr) { fatal(-1,"Couldn't create a socket.");}

	for(;;)
	{
		std::vector<std::pair<std::string,ceps::ast::Nodebase_ptr>> sysstates_table;
		nmp_header header;
		header.id = htonl(NMP_REQ_SYSTEMSTATES);
		header.len = htonl(0);

		if (write(cfd, (char*)&header,sizeof(header) ) !=	 sizeof(nmp_header))
			fatal(-1,"Partial/failed write (reqLenStr)");

		auto r = recv(cfd, (char*)&header,sizeof(nmp_header),0);
		if (r != sizeof(nmp_header)) break;
		header.id = htonl(header.id);
		header.len = htonl(header.len);

		if (header.id == NMP_SYSTEMSTATES){
			auto data = new char[header.len];
			if (data == nullptr){fatal(-1,"[ERROR_CLIENT_HANDLER_THREAD][ALLOC_FAILED].");close(cfd);return;}
			size_t already_read = 0;
			size_t n = 0;
			for(; (already_read < header.len) && (n = recv(cfd,data+already_read,header.len-already_read,0)) > 0;already_read+=n);

			if(already_read < header.len){fatal(-1, "[ERROR_CLIENT_HANDLER_THREAD][READ_FAILED].");close(cfd);return;}

			size_t offs = 0;

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
						r = ceps::deserialize_value(m, data+offs, header.len-offs);m = (ceps::ast::Unit_rep::sc_t)ntohl(m);offs+=r;
						r = ceps::deserialize_value(kg, data+offs, header.len-offs);kg = (ceps::ast::Unit_rep::sc_t)ntohl(kg);offs+=r;
						r = ceps::deserialize_value(s, data+offs, header.len-offs);s = (ceps::ast::Unit_rep::sc_t)ntohl(s);offs+=r;
						r = ceps::deserialize_value(ampere, data+offs, header.len-offs);ampere = (ceps::ast::Unit_rep::sc_t)ntohl(ampere);offs+=r;
						r = ceps::deserialize_value(kelvin, data+offs, header.len-offs);kelvin = (ceps::ast::Unit_rep::sc_t)ntohl(kelvin);offs+=r;
						r = ceps::deserialize_value(mol, data+offs, header.len-offs);mol = (ceps::ast::Unit_rep::sc_t)ntohl(mol);offs+=r;
						r = ceps::deserialize_value(candela, data+offs, header.len-offs);candela = (ceps::ast::Unit_rep::sc_t)ntohl(candela);offs+=r;
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
						r = ceps::deserialize_value(m, data+offs, header.len-offs);m = (ceps::ast::Unit_rep::sc_t)ntohl(m);offs+=r;
						r = ceps::deserialize_value(kg, data+offs, header.len-offs);kg = (ceps::ast::Unit_rep::sc_t)ntohl(kg);offs+=r;
						r = ceps::deserialize_value(s, data+offs, header.len-offs);s = (ceps::ast::Unit_rep::sc_t)ntohl(s);offs+=r;
						r = ceps::deserialize_value(ampere, data+offs, header.len-offs);ampere = (ceps::ast::Unit_rep::sc_t)ntohl(ampere);offs+=r;
						r = ceps::deserialize_value(kelvin, data+offs, header.len-offs);kelvin = (ceps::ast::Unit_rep::sc_t)ntohl(kelvin);offs+=r;
						r = ceps::deserialize_value(mol, data+offs, header.len-offs);mol = (ceps::ast::Unit_rep::sc_t)ntohl(mol);offs+=r;
						r = ceps::deserialize_value(candela, data+offs, header.len-offs);candela = (ceps::ast::Unit_rep::sc_t)ntohl(candela);offs+=r;
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
						return;
					}
				} else break;
			}
			for(auto const & entry: sysstates_table)
			{
				std::cout << entry.first << " = " << *entry.second << std::endl;
			}
			std::cout << "\n";

			delete[] data;
		}
		std::this_thread::sleep_for(std::chrono::microseconds(update_interval));
	}

	close(cfd);
}

int main(int argc,char ** argv)
{
	if (argc <= 1)
	{
        cout <<  "ceps is a tool for writing executable specifications and supports the cepS Ansatz.\n";
        cout << "Usage: " << argv[0] << " [FILE...] [option...]\n";
		cout << "\n";
        cout << "Example:\n " << argv[0] <<" a.ceps b.ceps\n\n";
        cout << "Option --help lists detailed information about available options.\n";
		return EXIT_FAILURE;
	}
	auto result_cmd_line = process_cmd_line(argc,argv);

	if (result_cmd_line.version_flag_set)
        std::cout << "ceps version " VERSION_SM4CEPS_MAJOR << "."<<  VERSION_SM4CEPS_MINOR<<"\n";
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

	State_machine_simulation_core sm_core;
	sm_core.set_fatal_error_handler(fatal);
	sm_core.set_non_fatal_error_handler(warn);
	sm_core.set_log_stream(&std::cout);
	string last_file_processed;
	try{
		Result_process_cmd_line result_cmd_line;
		init_state_machine_simulation(argc,argv,&sm_core,result_cmd_line);
		if (result_cmd_line.print_help){
			using namespace std;
			cout << "Usage: ceps [options] file...\n";
			cout << "Options:\n";
			vector<pair<string,string>> options = {
				{"--create_plugin_project ","Generate cmake build files for a plugin project."},
				{"--dot_gen               ","Generate DOT file for each top level state machine."},
				{"--format FORMAT         ","Set output format (applies to options --pr, --pe). Default is 'ansi'."},
				{"  Supported values are :",""},
				{"    raw                 ", ""},
				{"    ansi                ", ""},
				{"    markdown            ", ""},
				{"    markdown_github     ", ""},
				{"    markdown_jira       ", ""},
				{"    html5               ", ""},
				{"--pr                    ","Print unevaluated spec."},
				{"--pe                    ","Print evaluated spec."},
				{"--quiet                 ","Suppress any output on stdout (applies to tool's messages only)."},
				{"--rip a.b.c.d           ","IP address of remote ceps server."},
				{"--rport SHORT           ","IP port of remote ceps server."},
				{"--server                ","Start in server mode."},
				{"--timeout T             ","ceps runs for at most T seconds."},

			};
			for(auto e: options)
				cout << "   " << e.first <<  e.second << "\n";			
		} else {
			if (result_cmd_line.create_plugin_project){
			{
				ofstream os{"CMakeLists.ceps-plugin.txt"};
				os << 
R"~~(
cmake_minimum_required(VERSION 3.10)
project(INSERT_PROJECT_NAME_HERE)

add_compile_options(-O  -Wall -MD  -std=c++2a  -fPIC -static -Wno-undef )

IF(NOT( DEFINED ENV{CEPSCORE}))
    MESSAGE(FATAL_ERROR "Could not find ceps core (Environment variable CEPSCORE not set).")
ENDIF()

IF(NOT( DEFINED ENV{MACHINES4CEPS}))
    MESSAGE(FATAL_ERROR "Could not find machines4ceps (Environment variable MACHINES4CEPS not set).")
ENDIF()

IF(NOT( DEFINED ENV{LOG4CEPS}))
    MESSAGE(FATAL_ERROR "Could not find log4ceps (Environment variable LOG4CEPS not set).")
ENDIF()

include_directories($ENV{CEPSCORE}/include)
include_directories($ENV{LOG4CEPS}/include)
include_directories($ENV{MACHINES4CEPS})
include_directories($ENV{MACHINES4CEPS}/core/src_gen/logging)
include_directories(include)
include_directories(../include)
#include_directories(include/tests)

link_directories($ENV{CEPSCORE}/bin)

add_library(INSERT_PLUGIN_NAME_HERE SHARED 
           plugin-entrypoint.cpp 
           )

target_link_libraries(INSERT_PLUGIN_NAME_HERE cepscore)					
)~~";
			}

			{
				ofstream os{"rebuild.ceps-plugin.sh"};
				os << R"(
#!/bin/bash

mkdir bin 2>/dev/null

cd bin 
rm CMakeFiles -rf 2>/dev/null  
rm cmake_install.cmake -f 2>/dev/null
rm CMakeCache.txt -f 2>/dev/null
rm Makefile -f 2>/dev/null
rm lib* -f 2>/dev/null

CEPSCORE=../ceps/core MACHINES4CEPS=../machines4ceps LOG4CEPS=../log4ceps cmake .. && make -B
cd ..					
				)";
			}
			{
				ofstream os{"plugin-entrypoint.cpp"};
				os << R"~~(
#include <stdlib.h>
#include <iostream>
#include <ctype.h>
#include <chrono>
#include <sstream>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <queue>
#include <unordered_map>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <stdlib.h>
#include <map>
#include <algorithm>
#include <future>
#include <netinet/sctp.h> 

#include "ceps_ast.hh"
#include "core/include/state_machine_simulation_core.hpp"

namespace cepsplugin{
    static Ism4ceps_plugin_interface* plugin_master = nullptr;
    static const std::string version_info = "INSERT_NAME_HERE v0.1";
    static constexpr bool print_debug_info{true};
    ceps::ast::node_t plugin_entrypoint(ceps::ast::node_callparameters_t params);
}

ceps::ast::node_t cepsplugin::plugin_entrypoint(ceps::ast::node_callparameters_t params){
    using namespace std;
    using namespace ceps::ast;
    using namespace ceps::interpreter;

    auto data = get_first_child(params);    
    if (!is<Ast_node_kind::structdef>(data)) return nullptr;
    auto& ceps_struct = *as_struct_ptr(data);
    cout << "cepsplugin::plugin_entrypoint:\n";
    for(auto e : children(ceps_struct)){
        cout <<"\t"<< * e << "\n";
    }
    cout <<"\n\n";
    auto result = mk_struct("result");
    children(*result).push_back(mk_int_node(42));
    return result;
}

extern "C" void init_plugin(IUserdefined_function_registry* smc)
{
  cepsplugin::plugin_master = smc->get_plugin_interface();
  cepsplugin::plugin_master->reg_ceps_phase0plugin("INSERT_NAME_FOR_FUNCTION_HERE", cepsplugin::plugin_entrypoint);
}					
				)~~";
				{
					ofstream os{"run.ceps-plugin.sh"};
					os << R"(
#!/bin/bash

LD_LIBRARY_PATH=$(pwd)/bin:$LD_LIBRARY_PATH ceps \
 $1 \
 --pluginlibINSERT_PLUGIN_NAME_HERE.so						
					)";
				}
			}

				{
					ofstream os{"ceps-plugin.test.ceps"};
					os << R"~~(INSERT_NAME_FOR_FUNCTION_HERE(
	input{
		123;
		"abc";
		uint32{
			ThisIsAnIdentifier;
		};
		1+1;
	}
);

print ("result = ",root.result.content(),"\n\n");)~~";
				}
			}
			if (result_cmd_line.no_warn) sm_core.set_non_fatal_error_handler(dummy);
			PRINT_DEBUG_INFO = sm_core.print_debug_info_;

			if (result_cmd_line.run_as_monitor) {
				std::cout << "Running as monitor." << std::endl;
				run_as_monitor(result_cmd_line);
				return 0;
			}
			run_state_machine_simulation(&sm_core,result_cmd_line);
		}
	}
	catch (ceps::interpreter::semantic_exception & se)
	{
		std::cout << "***Fatal Error: "<< se.what() << std::endl;
		return EXIT_FAILURE;
	}
	catch (std::runtime_error & re)
	{
		std::cout << "***Fatal Error:" << re.what() << std::endl;
		return EXIT_FAILURE;
	}
	catch (std::invalid_argument & re)
	{
		std::cout << "***Fatal Error:" << re.what() << std::endl;
		return EXIT_FAILURE;
	}

#ifdef _WIN32
	WSACleanup();
#endif

	return EXIT_SUCCESS;
}//main



