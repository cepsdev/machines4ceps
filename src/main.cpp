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

/*
Execution model



*/

#include <signal.h>
#include <sys/types.h>
#include <limits>
#include <cstring>
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
static inline int write(SOCKET s, const void* buf, int len, int flags=0) { return send(s, (char*) buf, len, flags); }
static inline int close(SOCKET s) { return closesocket(s); }

#endif
#endif






#include "core/include/state_machine.hpp"
#include "core/include/state_machine_simulation_core.hpp"
#include "main.hpp"


#define VERSION_TST2DB_MAJOR "0"
#define VERSION_TST2DB_MINOR "5.1"

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





void fatal(int code, std::string const & msg )
{
	stringstream ss;
	if (State_machine_simulation_core::ERR_FILE_OPEN_FAILED == code)
		ss << "Couldn't open file '" << msg <<"'.";
	else if (State_machine_simulation_core::ERR_CEPS_PARSER == code)
		ss << "A parser exception occured in file '" << msg << "'.";
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




auto INDENT = 0;

string indent()
{
 string t;
 for(int i = 0; i < INDENT; ++i)
  t.append(" ");
 return t;
}

std::string replace_dot_in_string(const std::string & s,const std::string & r)
{
	std::string t;
	for(size_t i = 0; i < s.length();++i)
	{
		char b[2] = {0};
		b[0] = s[i];
		if (b[0]=='.') t.append(r);
		else t.append(b);
	}
	return t;
}

std::string get_id(State_machine* m)
{
	std::string r;
	if (m == nullptr) return "";
	if (m->parent_ == nullptr) return m->id_;

	std::vector<State_machine*> v;
	for(State_machine* t = m;t;t= t->parent_)v.push_back(t);
	for(int i = v.size()-1; i >= 0;--i)
	{
		if (i > 0)
		 r.append(v[i]->id_+"."); else  r.append(v[i]->id_);
	}
	return r;
}


std::string gen_plantuml_action_event_seq(std::set< State_machine::Transition::Event>  const & evs,std::vector<State_machine::Transition::Action> const & ac)
{

	if (evs.size() == 0 && ac.size()==0) return "";
	if (evs.size() == 0 && ac.size()!=0) return " : / " +ac[0].id();
	if (evs.size() > 0 && ac.size()==0) return " : " +evs.cbegin()->id();
	return " : " +evs.cbegin()->id() + " / " +ac[0].id();
}

void write_plantuml_rep(ostream& out_stream,ceps::Ceps_Environment& ceps_env,
		      ceps::ast::Nodeset& universe,
		      Result_process_cmd_line& cmd_line)
{

  int e_m = 0;
  vector<State_machine> sms;
  for(auto & v : State_machine::statemachines)
  {
  sms.push_back(*v.second); if (v.second == entry_machine) e_m = v.second->order_;
  }
  sort(sms.begin(),sms.end());


  out_stream << "@startuml\n";
  ++INDENT;
  for(auto & v : sms)
  {

    auto& sm = v;
    if (sm.order_ != e_m)
      {
        out_stream <<  indent() << "state " << sm.id() << "{\n";
        ++INDENT;
      }
    for(auto const & t : sm.transitions())
    {
      //


      out_stream << indent() <<
       t.from() << " -right-> " << t.to() <<gen_plantuml_action_event_seq(t.events(),t.actions()) << "\n";
    }
    for(size_t thread = 0; thread < sm.threads().size();++thread)
    {
      for(auto const & t : sm.threads()[thread])
      {
      //


      out_stream << indent() <<
       t.from() << " -right-> " << t.to() << gen_plantuml_action_event_seq(t.events(),t.actions()) << "\n";
      }
      if (thread + 1 < sm.threads().size()) out_stream << "--\n";
    }
    if (sm.order_ != e_m) {--INDENT; out_stream <<  indent() << "}\n";}
  }
  --INDENT;
  out_stream << "@enduml\n";
}



void write_plantuml_rep(ostream& out_stream, State_machine & sm,ceps::Ceps_Environment& ceps_env,
		      ceps::ast::Nodeset& universe,
		      Result_process_cmd_line& cmd_line,
		      std::map<State_machine*,std::set<State_machine*>>  & refered_by)
{


  INDENT = 0;
  out_stream << "@startuml\n";
  ++INDENT;

  std::set<State_machine*> referenced_sm;

  for(auto const & t : sm.transitions()) if (t.to().is_sm_) referenced_sm.insert(t.to().smp_);

  for(auto smp : referenced_sm)
  {
    std::string t = get_id(smp);
    out_stream << indent() << "state " << t << std::endl;
  }

  for(auto smp : refered_by[&sm])
  {
	  std::string sm_id = get_id(smp);
	  for (auto const & t: smp->transitions())
	  {
		  if (t.to().smp_ != &sm) continue;


		  if (t.from().is_initial())
			  out_stream << indent() <<
			 	    		  sm_id<< " --> " << sm.id() << gen_plantuml_action_event_seq(t.events(),t.actions()) << "\n";
		  else
	       out_stream << indent() <<
	    		  sm_id<<"." << t.from() << " --> " << sm.id() << gen_plantuml_action_event_seq(t.events(),t.actions()) << "\n";

	  }
	  //out_stream << indent() <<  t <<" -> "<< sm.id() << std::endl;
  }

  out_stream <<  indent() << "state " << sm.id() << "{\n";
  ++INDENT;



  for(auto const & t : sm.transitions())
    {
      //


      out_stream << indent() <<
       t.from() << " -right-> " << t.to() <<gen_plantuml_action_event_seq(t.events(),t.actions()) << "\n";
   }

  for(size_t thread = 0; thread < sm.threads().size();++thread)
    {
      for(auto const & t : sm.threads()[thread])
      {
      //

      out_stream << indent() <<
       t.from() << " -right-> " << t.to() << gen_plantuml_action_event_seq(t.events(),t.actions()) << "\n";
      }
      if (thread + 1 < sm.threads().size()) out_stream << "--\n";
    }
  --INDENT; out_stream <<  indent() << "}\n";

  --INDENT;
  out_stream << "@enduml\n";
}


void compute_refered_by(std::map<State_machine*,std::set<State_machine*>>&  m)
{
	for(auto & v : State_machine::statemachines)
	{
		for(auto & t : v.second->transitions())
		{
			if (!t.to().is_sm_) continue;
			m[t.to().smp_].insert(v.second);
		}
	}
}

void write_doxygen_doc(std::string const & out_dir,
		      ceps::Ceps_Environment& ceps_env,
		      ceps::ast::Nodeset& universe,
		      Result_process_cmd_line& cmd_line)
{
	//compute refered by

	std::map<State_machine*,std::set<State_machine*>> refered_by;
	compute_refered_by(refered_by);

	for(auto & v : State_machine::statemachines)
	{
		{std::ofstream out(out_dir + "sm_"+v.first+".txt");
		out << "/*! \\page " << "Statemachine_" <<replace_dot_in_string(v.first,"_")<<std::endl;
		out << "* \\section sm"<< replace_dot_in_string(v.first,"_")<<" State machine " << v.first <<"" <<  std::endl;
		out << "* \\image html sm_"<<replace_dot_in_string(v.first,"_")<<".png"<< std::endl;

		if (v.second->children().size())
		{
			out << "* \\section subs Contains the following (sub-) state machines\n";
			for(auto sp: v.second->children())
			{
				auto t = get_id(sp);
			 	out << "* \\ref Statemachine_"<< replace_dot_in_string(t,"_") << std::endl;
			}

		}
		if(v.second->parent_ != nullptr)
		{
			auto parent  = v.second->parent_;
			auto t = get_id(parent);
			out << "* \\section par Parent is\n\\ref Statemachine_" << replace_dot_in_string(t,"_") << std::endl;
		}
		//out << "* \\section ref_by Referenced by following state machines\n";
		std::vector<State_machine*> references;
		for(auto & t : v.second->transitions())
		{
			if (t.to().is_sm_) references.push_back(t.to().smp_);
		}
		if (references.size() > 0) {
			 out << "* \\section refs References the following state machines\n";
			 for(auto sp: references)
			 {
				 auto t = get_id(sp);
				 out << "* \\ref Statemachine_"<< replace_dot_in_string(t,"_") << std::endl;
			 }
		}
		if (refered_by[v.second].size())
		{
			out << "* \\section ref_by Referenced by the following state machines\n";
			auto& m = refered_by[v.second];
			for(auto sp: m)
			 {
				 auto t = get_id(sp);
				 out << "* \\ref Statemachine_"<< replace_dot_in_string(t,"_") << std::endl;
			 }
		}
		out << "*/\n";
		std::ofstream outumplrep(out_dir + "sm_"+replace_dot_in_string(v.first,"_")+".plantuml");
		write_plantuml_rep(outumplrep, *v.second, ceps_env,universe, cmd_line,refered_by);}


		std::string cmd = "java -jar plantuml.jar "+out_dir + "sm_"+replace_dot_in_string(v.first,"_")+".plantuml";
		std::system(cmd.c_str());
	}
}

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

		//std:cout << "***Table of Systemstates***\n";

		std::this_thread::sleep_for(std::chrono::microseconds(update_interval));
	}

	close(cfd);
}

int main(int argc,char ** argv)
{
	if (argc <= 1)
	{
		cout <<  "\nsm4cepssim is a UML2 state machine simulator and part of sm4ceps.\n";
		cout << "Usage: " << argv[0] << " FILE [FILE...] [-i] [-oPATH] [--debug]\n";
		cout << "\n";
		cout << "Example:\n " << argv[0] <<" a.ceps b.ceps .\n";
		return EXIT_FAILURE;
	}


	auto result_cmd_line = process_cmd_line(argc,argv);

	if (result_cmd_line.version_flag_set)
	{
			#ifdef __GNUC__
			 		std::cout << "\n"
							  << "VERSION " VERSION_TST2DB_MAJOR << "."<<  VERSION_TST2DB_MINOR   <<" (" __DATE__ << ") BUILT WITH GCC "<< "" __VERSION__ ""<< " on GNU/LINUX "
			 #ifdef __LP64__
							  << "64BIT"
			 #else
							  << "32BIT"
			 #endif
							  << "\n(C) BY THE AUTHORS OF sm4ceps \n" << std::endl;
			#else
				#ifdef _MSC_FULL_VER
					std::cout << "\n"
						<< "VERSION " VERSION_TST2DB_MAJOR << "." << VERSION_TST2DB_MINOR << " (" __DATE__ << ") BUILT WITH MS VISUAL C++ " <<  _MSC_FULL_VER  << " on Windows "
					#ifdef _WIN64
						<< "64BIT"
					#else
						<< "32BIT"
					#endif
						<< "\n(C) BY THE AUTHORS OF sm4ceps\n" << std::endl;
				#endif
			#endif
	}

	// Do sanity checks

	for(std::string const & f : result_cmd_line.definition_file_rel_paths)
	 if (!std::ifstream{f})
	 {
		 std::cerr << "\n***Error: Couldn't open file '" << f << "' " << std::endl << std::endl;
		 return EXIT_FAILURE;
	 }

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
	sm_core.set_log_stream(&std::cerr);



	string last_file_processed;

	if (global_out_path.length())
	{
		gen_doc = true;
		doc_out_dir = global_out_path;
		DUMP_PLANTUML_REP_TO_COUT = false;
	}

	try{

		Result_process_cmd_line result_cmd_line;
		init_state_machine_simulation(argc,argv,&sm_core,result_cmd_line);

		if (result_cmd_line.run_as_monitor) {
			std::cout << "Running as monitor." << std::endl;
			run_as_monitor(result_cmd_line);
			return 0;
		}
		run_state_machine_simulation(&sm_core,result_cmd_line);


		if (DUMP_PLANTUML_REP_TO_COUT) write_plantuml_rep(cout,sm_core.ceps_env_current(), sm_core.current_universe(), result_cmd_line);
		if (gen_doc)
		{
			std::cerr << "***Writing doxygen conformant documents to: '"<< doc_out_dir << "'" << std::endl;
			write_doxygen_doc(doc_out_dir,sm_core.ceps_env_current(), sm_core.current_universe(), result_cmd_line);
		}
	}
	catch (ceps::interpreter::semantic_exception & se)
	{
		std::cerr << "***Error while evaluating file '" + last_file_processed + "':"<< se.what() << std::endl;
		return EXIT_FAILURE;
	}
	catch (std::runtime_error & re)
	{
		std::cerr << "***Error while evaluating file '" + last_file_processed + "':" << re.what() << std::endl;
		return EXIT_FAILURE;
	}

#ifdef _WIN32
	WSACleanup();
#endif

	return EXIT_SUCCESS;
}//main



