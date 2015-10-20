#include "core/include/sm_comm_naive_msg_prot.hpp"

#include <sys/types.h>
#include <limits>
#include <cstring>

#include "core/include/state_machine_simulation_core.hpp"
#include "core/include/base_defs.hpp"

static  auto comm_dispatcher_thread_started_flag = false;
static std::map<uint32_t, void (State_machine_simulation_core::*)(nmp_header,char*) > recv_handler;
static std::map<uint32_t, void (*)(nmp_header&,char**) > send_handler;


void register_dispatcher_handler(uint32_t id,void (State_machine_simulation_core::*fn)(nmp_header,char*))
{
	recv_handler[id] = fn;
}

bool comm_dispatcher_thread_started()
{
	return comm_dispatcher_thread_started_flag;
}

template<typename T> struct clear_on_exit{
	T& p_;
	clear_on_exit(T& p):p_(p){}
	~clear_on_exit() {p_ = T{};}
};


void nmp_consumer_thread_fn(int id,State_machine_simulation_core* smc,sockaddr_storage claddr,int sck)
{
	DEBUG_FUNC_PROLOGUE
	char host[1024] = {0};
	char service[1024] = {0};
	char addrstr[2048] = {0};
	socklen_t addrlen = sizeof(struct sockaddr_storage);

	if (getnameinfo((struct sockaddr*)&claddr,addrlen,host,1024,service,1024,0) == 0)
		snprintf(addrstr,2048,"[host=%s, service=%s]",host, service);
	else
		snprintf(addrstr,2048,"[host=?,service=?]");
	DEBUG << "[CLIENT_HANDLER_THREAD][CONN_ESTABLISHED]" << addrstr << "\n";

	for(;!smc->shutdown();)
	{
		nmp_header header;
		auto r = recv(sck,reinterpret_cast<char*>(&header),sizeof(nmp_header),0);
		if (r <= 0)
		{
			DEBUG << "[CLIENT_HANDLER_THREAD][READ_FAILED=>TERMINATE]\n";closesocket(sck);return;
		}

		header.id = ntohl(header.id);
		header.len = ntohl(header.len);
		DEBUG << "[CLIENT_HANDLER_THREAD][READ_MSG][ID="<< header.id <<"][DATA_LEN="<<header.len<<"]\n";
		char * data = nullptr;
		try
		{
			DEBUG << "[CLIENT_HANDLER_THREAD][READING_DATA]\n";
			if (header.len)
			{
				data = new char[header.len];
				if (data == nullptr){DEBUG << "[ERROR_CLIENT_HANDLER_THREAD][ALLOC_FAILED]\n";closesocket(sck);return;}
				size_t already_read = 0;
				size_t n = 0;
				for(; (already_read < header.len) && (n = recv(sck,data+already_read,header.len-already_read,0)) > 0;already_read+=n);

				if(already_read < header.len){DEBUG << "[ERROR_CLIENT_HANDLER_THREAD][READ_FAILED]\n";closesocket(sck);return;}
			}
		    DEBUG << "[CLIENT_HANDLER_THREAD][DATA_SUCCESSFULLY_READ]\n";
			if (recv_handler.size()) DEBUG << "[CLIENT_HANDLER_THREAD][THERE_ARE_CALLBACKS_REGISTERED]\n";
			auto it = recv_handler.find(header.id);
			if (it != recv_handler.end())
			{
				DEBUG << "[CLIENT_HANDLER_THREAD][HANDLER_FOUND]\n";
				(smc->*it->second) (header,data);
			}
		} catch(...){}

		if (data) delete[] data;
	}
	closesocket(sck);
}



void serialize_system_state_block(State_machine_simulation_core* smc,
		                          char** data,
		                          size_t& buffer_size)
{
	auto THIS = smc;
	DEBUG_FUNC_PROLOGUE2
	size_t data_size = 0;

	smc->lock_global_states();
	auto& states = smc->get_global_states();

	size_t states_to_write = 0;

	/*Compute necessary space*/
	for(auto elem:states)
	{
		auto p = elem.second;
		if (p->kind() != ceps::ast::Ast_node_kind::int_literal && 
			p->kind() != ceps::ast::Ast_node_kind::float_literal  && 
			p->kind() != ceps::ast::Ast_node_kind::string_literal)
		continue;
		
		++states_to_write;

		data_size += ceps::serialize_value( elem.first,
											nullptr,
											std::numeric_limits<size_t>::max(),
											false,ceps::nothrow_exception_policy());;

		if (p->kind() == ceps::ast::Ast_node_kind::int_literal){
		 int d;
		 auto w = ceps::serialize_value(d,nullptr,std::numeric_limits<size_t>::max(),false,ceps::nothrow_exception_policy());
		 data_size += w;
		 ceps::ast::Unit_rep::sc_t u;
		 data_size += 7* ceps::serialize_value(u,nullptr,std::numeric_limits<size_t>::max(),false,ceps::nothrow_exception_policy());

		} else if (p->kind() == ceps::ast::Ast_node_kind::float_literal){
		 double d;
		 data_size += ceps::serialize_value(d,nullptr,std::numeric_limits<size_t>::max(),false,ceps::nothrow_exception_policy());
		 ceps::ast::Unit_rep::sc_t u;
		 data_size += 7* ceps::serialize_value(u,nullptr,std::numeric_limits<size_t>::max(),false,ceps::nothrow_exception_policy());
		} else if (p->kind() == ceps::ast::Ast_node_kind::string_literal){
		 std::string d;
		 data_size += ceps::serialize_value(d,nullptr,std::numeric_limits<size_t>::max(),false,ceps::nothrow_exception_policy());
		}
	}
	buffer_size = data_size + /*payload signatures*/states_to_write*sizeof(int)*2 + /*nmp header*/ 2*sizeof(int);

	*data = new char[buffer_size];
	if (*data == nullptr) {buffer_size=0;return;}
	((nmp_header*) (*data))->id = htonl(NMP_SYSTEMSTATES);
	((nmp_header*) (*data))->len = htonl(buffer_size-/*nmp header*/ 2*sizeof(int));

	size_t offs = sizeof(nmp_header);

	if (states_to_write) for(auto elem:states)
	{
		auto p = elem.second;
		if (p->kind() != ceps::ast::Ast_node_kind::int_literal && p->kind() != ceps::ast::Ast_node_kind::float_literal  && p->kind() != ceps::ast::Ast_node_kind::string_literal)
			continue;

		*((uint32_t*) (*data+offs))= htonl(NMP_PAYLOAD_SYSTEMSTATE);offs+=sizeof(uint32_t);
		offs+=ceps::serialize_value(elem.first,*data+offs,std::numeric_limits<size_t>::max(),true,ceps::nothrow_exception_policy());;

		//strncpy(*data,elem.first.c_str(),elem.first.length());offs+=elem.first.length();

		if (p->kind() == ceps::ast::Ast_node_kind::int_literal){
		 DEBUG << "[serialize_flat_payload][SERIALIZE_INT]["<< ceps::ast::value(ceps::ast::as_int_ref(p)) <<"]\n";
		 int d = htonl(ceps::ast::value(ceps::ast::as_int_ref(p)));
		 * ((uint32_t*) (*data+offs)) = htonl(NMP_PAYLOAD_INT);
		 offs += sizeof(int);
		 auto written = ceps::serialize_value(d,*data+offs,std::numeric_limits<size_t>::max(),true,ceps::nothrow_exception_policy());
		 offs += written;
		 auto unit = ceps::ast::unit(ceps::ast::as_int_ref(p));
		 written = ceps::serialize_value(unit.m,*data+offs,std::numeric_limits<size_t>::max(),true,ceps::nothrow_exception_policy());
		 offs += written;
		 written = ceps::serialize_value(unit.kg,*data+offs,std::numeric_limits<size_t>::max(),true,ceps::nothrow_exception_policy());
		 offs += written;
		 written = ceps::serialize_value(unit.s,*data+offs,std::numeric_limits<size_t>::max(),true,ceps::nothrow_exception_policy());
		 offs += written;
		 written = ceps::serialize_value(unit.ampere,*data+offs,std::numeric_limits<size_t>::max(),true,ceps::nothrow_exception_policy());
		 offs += written;
		 written = ceps::serialize_value(unit.kelvin,*data+offs,std::numeric_limits<size_t>::max(),true,ceps::nothrow_exception_policy());
		 offs += written;
		 written = ceps::serialize_value(unit.mol,*data+offs,std::numeric_limits<size_t>::max(),true,ceps::nothrow_exception_policy());
		 offs += written;
		 written = ceps::serialize_value(unit.candela,*data+offs,std::numeric_limits<size_t>::max(),true,ceps::nothrow_exception_policy());
		 offs += written;
		} else if (p->kind() == ceps::ast::Ast_node_kind::float_literal){
		 double d = ceps::ast::value(ceps::ast::as_double_ref(p));
		 * ((int*)*data+offs) = htonl(NMP_PAYLOAD_DOUBLE);
		 offs += sizeof(int);
		 auto written = ceps::serialize_value(d,*data+offs,std::numeric_limits<size_t>::max(),true,ceps::nothrow_exception_policy());
		 offs += written;
		 auto unit = ceps::ast::unit(ceps::ast::as_int_ref(p));
		 written = ceps::serialize_value(unit.m,*data+offs,std::numeric_limits<size_t>::max(),true,ceps::nothrow_exception_policy());
		 offs += written;
		 written = ceps::serialize_value(unit.kg,*data+offs,std::numeric_limits<size_t>::max(),true,ceps::nothrow_exception_policy());
		 offs += written;
		 written = ceps::serialize_value(unit.s,*data+offs,std::numeric_limits<size_t>::max(),true,ceps::nothrow_exception_policy());
		 offs += written;
		 written = ceps::serialize_value(unit.ampere,*data+offs,std::numeric_limits<size_t>::max(),true,ceps::nothrow_exception_policy());
		 offs += written;
		 written = ceps::serialize_value(unit.kelvin,*data+offs,std::numeric_limits<size_t>::max(),true,ceps::nothrow_exception_policy());
		 offs += written;
		 written = ceps::serialize_value(unit.mol,*data+offs,std::numeric_limits<size_t>::max(),true,ceps::nothrow_exception_policy());
		 offs += written;
		 written = ceps::serialize_value(unit.candela,*data+offs,std::numeric_limits<size_t>::max(),true,ceps::nothrow_exception_policy());
		 offs += written;
		} else if (p->kind() == ceps::ast::Ast_node_kind::string_literal){
		 *((int*)*data+offs) = htonl(NMP_PAYLOAD_STRING);
		 offs += sizeof(int);
		 std::string d = ceps::ast::value(ceps::ast::as_string_ref(p));;
		 auto written = ceps::serialize_value(d,*data+offs,std::numeric_limits<size_t>::max(),true,ceps::nothrow_exception_policy());
		 offs += written;
		} else {

		}
	}


	smc->unlock_global_states();
}


void nmp_monitoring_thread_fn(int id,State_machine_simulation_core* smc,sockaddr_storage claddr,int sck)
{
	DEBUG_FUNC_PROLOGUE
	char host[1024] = {0};
	char service[1024] = {0};
	char addrstr[2048] = {0};
	socklen_t addrlen = sizeof(struct sockaddr_storage);

	if (getnameinfo((struct sockaddr*)&claddr,addrlen,host,1024,service,1024,0) == 0)
		snprintf(addrstr,2048,"[host=%s, service=%s]",host, service);
	else
		snprintf(addrstr,2048,"[host=?,service=?]");
	DEBUG << "[MONITORING_THREAD][CONN_ESTABLISHED]" << addrstr << "\n";

	for(;!smc->shutdown();)
	{
		nmp_header header;
		auto r = recv(sck,(char*)&header,sizeof(nmp_header),0);
		if (r <= 0)
		{
			DEBUG << "[MONITORING_THREAD][READ_FAILED=>TERMINATE]\n";closesocket(sck);return;
		}

		header.id = ntohl(header.id);
		header.len = ntohl(header.len);

		if (header.id == NMP_REQ_SYSTEMSTATES)
		{
			DEBUG << "[MONITORING_THREAD][NMP_REQ_SYSTEMSTATES]\n";
			char * data;
			size_t buffer_size;
			serialize_system_state_block(smc,&data,buffer_size);
			DEBUG << "[MONITORING_THREAD][PAYLOAD_SIZE="<< buffer_size <<"]\n";
			int wr;
			if ( ( wr = write(sck, data,buffer_size )) !=	 buffer_size )
			{
				closesocket(sck);delete[] data;return;
			}
			delete[] data;
		} else if (header.id == NMP_EVAL_CEPS_EXPRESSION) {
			DEBUG << "[MONITORING_THREAD][NMP_EVAL_CEPS_EXPRESSION]\n";




			char * data;
			size_t buffer_size;
			serialize_system_state_block(smc,&data,buffer_size);
			DEBUG << "[MONITORING_THREAD][PAYLOAD_SIZE="<< buffer_size <<"]\n";
			int wr;
			if ( ( wr = write(sck, data,buffer_size )) !=	 buffer_size )
			{
				closesocket(sck);delete[] data;return;
			}
			delete[] data;
		} else {
			header.id = ntohl(NMP_ERR);
			header.len = ntohl(0);
			if ( write(sck, (char*)&header,sizeof(header) ) !=	 sizeof(header) )
			{
				closesocket(sck);return;
			}
		}
	}
	close(sck);
}


void comm_dispatcher_thread(int id,
			     State_machine_simulation_core* smc,
			     std::string ip,
			     std::string port,
			     void (*handler_fn) (int,State_machine_simulation_core* , sockaddr_storage,int))
{
	clear_on_exit<bool> cl(comm_dispatcher_thread_started_flag);
	std::vector<std::thread*> client_handler_threads;
	DEBUG_FUNC_PROLOGUE

	struct addrinfo hints;
	struct addrinfo* result, * rp;
	int lfd;


	memset(&hints,0,sizeof(hints));
	hints.ai_canonname = nullptr;
	hints.ai_addr = nullptr;
	hints.ai_next = nullptr;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_INET;

    hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;
	//hints.ai_flags = AI_PASSIVE;
	hints.ai_protocol = IPPROTO_TCP;
	
    if (getaddrinfo(nullptr,port.c_str(),&hints,&result) != 0)
    	smc->fatal_(-1,"getaddrinfo failed");

    int optval=1;

	for(rp=result;rp;rp=rp->ai_next)
	{
		lfd = socket(rp->ai_family,rp->ai_socktype,rp->ai_protocol);
		if(lfd == -1) continue;
		if (setsockopt(lfd,SOL_SOCKET,SO_REUSEADDR,(char*)&optval,sizeof(optval))) smc->fatal_(-1,"setsockopt");
		if (bind(lfd,rp->ai_addr,rp->ai_addrlen) == 0) break;
		close(lfd);
	}
	if (rp == nullptr) smc->fatal_(-1,"comm_dispatcher_thread:Could not bind socket to any address.port="+port);

	if (listen(lfd,5)==-1)smc->fatal_(-1,"listen");

	freeaddrinfo(result);

	for(;!smc->shutdown();)
	{
		comm_dispatcher_thread_started_flag = true;
		socklen_t addrlen = sizeof(struct sockaddr_storage);
		struct sockaddr_storage claddr;
		INFO << "[***COMM_DISPATCHER][WAITING_FOR_INCOMING_CONNECTIONS] " << port <<"\n";
		INFO << "[***COMM_DISPATCHER][WAITING_FOR_INCOMING_CONNECTIONS] " << port << "\n";
		INFO << "[***COMM_DISPATCHER][WAITING_FOR_INCOMING_CONNECTIONS] " << port << "\n";
		INFO << "[***COMM_DISPATCHER][WAITING_FOR_INCOMING_CONNECTIONS] " << port << "\n";
		int cfd = accept(lfd, (struct sockaddr*) &claddr, &addrlen);
		if (cfd == -1){
			DEBUG << "[ERROR_COMM_DISPATCHER][ACCEPT_FAILED]\n";continue;
		}
		client_handler_threads.push_back(new std::thread(*handler_fn,id,smc,claddr,cfd));
	}
	for(auto tp: client_handler_threads) tp->join();
}


void serialize_flat_payload(State_machine_simulation_core* smc,State_machine_simulation_core::event_t const & ev, std::vector<ceps::ast::Nodebase_ptr> const& in, char** data, size_t& buffer_size)
{

	auto THIS = smc;
	DEBUG_FUNC_PROLOGUE2
	size_t data_size = 0;
	for(auto p:in)
	{
		if (p->kind() == ceps::ast::Ast_node_kind::int_literal){
		 int d;
		 auto w = ceps::serialize_value(d,nullptr,std::numeric_limits<size_t>::max(),false,ceps::nothrow_exception_policy());
		 data_size += w;
		 ceps::ast::Unit_rep::sc_t u;
		 data_size += 7* ceps::serialize_value(u,nullptr,std::numeric_limits<size_t>::max(),false,ceps::nothrow_exception_policy());

		} else if (p->kind() == ceps::ast::Ast_node_kind::float_literal){
		 double d = ceps::ast::value(ceps::ast::as_double_ref(p));
		 data_size += ceps::serialize_value(d,nullptr,std::numeric_limits<size_t>::max(),false,ceps::nothrow_exception_policy());
		 ceps::ast::Unit_rep::sc_t u;
		 data_size += 7* ceps::serialize_value(u,nullptr,std::numeric_limits<size_t>::max(),false,ceps::nothrow_exception_policy());
		} else if (p->kind() == ceps::ast::Ast_node_kind::string_literal){
		 std::string d = ceps::ast::value(ceps::ast::as_string_ref(p));
		 data_size += ceps::serialize_value(d,nullptr,std::numeric_limits<size_t>::max(),false,ceps::nothrow_exception_policy());
		}
	}

	buffer_size = data_size + /*payload signatures*/in.size()*sizeof(int)+
			                  /*nmp header*/ 2*sizeof(int)+
			                  /*event name*/sizeof(int)+ev.id_.length();

	*data = new char[buffer_size];
	if (*data == nullptr) {buffer_size=0;return;}

	size_t offs = 2*sizeof(int);

	* ((int*)(*data+offs)) = htonl(ev.id_.length());
	offs += sizeof(int);
	strncpy(*data+offs,ev.id_.c_str(),ev.id_.length());
	offs += ev.id_.length();

	for(auto p:in)
	{
		if (p->kind() == ceps::ast::Ast_node_kind::int_literal){
		 DEBUG << "[serialize_flat_payload][SERIALIZE_INT]["<< ceps::ast::value(ceps::ast::as_int_ref(p)) <<"]\n";
		 int d = htonl(ceps::ast::value(ceps::ast::as_int_ref(p)));
		 * ((uint32_t*) (*data+offs)) = htonl(NMP_PAYLOAD_INT);
		 offs += sizeof(int);
		 auto written = ceps::serialize_value(d,*data+offs,std::numeric_limits<size_t>::max(),true,ceps::nothrow_exception_policy());
		 offs += written;
		 auto unit = ceps::ast::unit(ceps::ast::as_int_ref(p));
		 written = ceps::serialize_value(unit.m,*data+offs,std::numeric_limits<size_t>::max(),true,ceps::nothrow_exception_policy());
		 offs += written;
		 written = ceps::serialize_value(unit.kg,*data+offs,std::numeric_limits<size_t>::max(),true,ceps::nothrow_exception_policy());
		 offs += written;
		 written = ceps::serialize_value(unit.s,*data+offs,std::numeric_limits<size_t>::max(),true,ceps::nothrow_exception_policy());
		 offs += written;
		 written = ceps::serialize_value(unit.ampere,*data+offs,std::numeric_limits<size_t>::max(),true,ceps::nothrow_exception_policy());
		 offs += written;
		 written = ceps::serialize_value(unit.kelvin,*data+offs,std::numeric_limits<size_t>::max(),true,ceps::nothrow_exception_policy());
		 offs += written;
		 written = ceps::serialize_value(unit.mol,*data+offs,std::numeric_limits<size_t>::max(),true,ceps::nothrow_exception_policy());
		 offs += written;
		 written = ceps::serialize_value(unit.candela,*data+offs,std::numeric_limits<size_t>::max(),true,ceps::nothrow_exception_policy());
		 offs += written;
		} else if (p->kind() == ceps::ast::Ast_node_kind::float_literal){
		 double d = ceps::ast::value(ceps::ast::as_double_ref(p));
		 * ((uint32_t*) (*data+offs)) = htonl(NMP_PAYLOAD_DOUBLE);
		 offs += sizeof(int);
		 auto written = ceps::serialize_value(d,*data+offs,std::numeric_limits<size_t>::max(),true,ceps::nothrow_exception_policy());

		 offs += written;
		 auto unit = ceps::ast::unit(ceps::ast::as_double_ref(p));
		 written = ceps::serialize_value(unit.m,*data+offs,std::numeric_limits<size_t>::max(),true,ceps::nothrow_exception_policy());
		 offs += written;

		 written = ceps::serialize_value(unit.kg,*data+offs,std::numeric_limits<size_t>::max(),true,ceps::nothrow_exception_policy());
		 offs += written;
		 written = ceps::serialize_value(unit.s,*data+offs,std::numeric_limits<size_t>::max(),true,ceps::nothrow_exception_policy());
		 offs += written;
		 written = ceps::serialize_value(unit.ampere,*data+offs,std::numeric_limits<size_t>::max(),true,ceps::nothrow_exception_policy());
		 offs += written;
		 written = ceps::serialize_value(unit.kelvin,*data+offs,std::numeric_limits<size_t>::max(),true,ceps::nothrow_exception_policy());
		 offs += written;
		 written = ceps::serialize_value(unit.mol,*data+offs,std::numeric_limits<size_t>::max(),true,ceps::nothrow_exception_policy());
		 offs += written;

		 written = ceps::serialize_value(unit.candela,*data+offs,std::numeric_limits<size_t>::max(),true,ceps::nothrow_exception_policy());
		 offs += written;
		} else if (p->kind() == ceps::ast::Ast_node_kind::string_literal){
		 * ((uint32_t*) (*data+offs)) = htonl(NMP_PAYLOAD_STRING);
		 offs += sizeof(int);
		 std::string d = ceps::ast::value(ceps::ast::as_string_ref(p));
		 auto written = ceps::serialize_value(d,*data+offs,std::numeric_limits<size_t>::max(),true,ceps::nothrow_exception_policy());
		 offs += written;
		} else {
			smc->fatal_(-1,"Not Implemented.\n");
		}
	}

	((nmp_header*) (*data))->id = htonl(NMP_EVENT_FLAT_PAYLOAD);
	((nmp_header*) (*data))->len = htonl(buffer_size-/*nmp header*/ 2*sizeof(int));

}


void comm_sender_thread(int id,
			     State_machine_simulation_core* smc,
			     std::string ip,
			     std::string port,
			     bool passive)
{
	DEBUG_FUNC_PROLOGUE
	int cfd;
	struct addrinfo hints;
	struct addrinfo *result, *rp;
	auto q = smc->out_event_queue(id);
	bool conn_established = false;
	State_machine_simulation_core::event_t ev;
	bool pop_ev = true;
	for(;;)
	{
		rp = nullptr;result = nullptr;

		DEBUG << "[comm_sender_thread][WAIT_FOR_EVENT][pop_ev="<<pop_ev <<"]\n";
		if (pop_ev) q->wait_and_pop(ev);
		if (ev.id_.length() >= 1 && ev.id_[0] == '@') continue;
		pop_ev = false;



		DEBUG << "[comm_sender_thread][FETCHED_EVENT]\n";
		if (!conn_established)
		{
			DEBUG << "[comm_sender_thread][CONNECTING]\n";
			for(;rp == nullptr;)
			{

			memset(&hints, 0, sizeof(struct addrinfo));
			hints.ai_canonname = NULL;
			hints.ai_addr = NULL;
			hints.ai_next = NULL;
			hints.ai_family = AF_INET;

			hints.ai_socktype = SOCK_STREAM;
			hints.ai_flags = AI_NUMERICSERV;
			if (getaddrinfo(ip.c_str(), port.c_str(), &hints, &result) != 0){
				DEBUG << "[comm_sender_thread][FAILED_TO_CONNECT:getaddrinfo(ip.c_str(), port.c_str(), &hints, &result) != 0]\n";
				std::this_thread::sleep_for(std::chrono::microseconds(1000));continue;
			}

			if (result == nullptr) {
				DEBUG << "[comm_sender_thread][FAILED_TO_CONNECT:rp == nullptr (A)]\n";
				std::this_thread::sleep_for(std::chrono::microseconds(1000)); continue;
			}
			 for (rp = result; rp != NULL; rp = rp->ai_next) {
			  cfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
			  if (cfd == -1) { DEBUG << "[comm_sender_thread][for (rp = result; rp != NULL; rp = rp->ai_next):cfd == -1]\n"; continue; }
			  if (connect(cfd, rp->ai_addr, rp->ai_addrlen) != -1)break;
			  DEBUG << "[comm_sender_thread][for (rp = result; rp != NULL; rp = rp->ai_next):connect(cfd, rp->ai_addr, rp->ai_addrlen) == -1]\n";
			  close(cfd);
			 }
			 if (result != nullptr) freeaddrinfo(result);
			 if (rp == nullptr) {
				 DEBUG << "[comm_sender_thread][FAILED_TO_CONNECT:rp == nullptr (B)]"<<" " << ip << ":" <<port <<"\n";
				 std::this_thread::sleep_for(std::chrono::microseconds(1000000));continue;
			 }
			}
			conn_established = true;
		}

		DEBUG << "[comm_sender_thread][SEND_EVENT]['"<< ev.id_ << "']\n";

		if (ev.payload_.size() == 0) {

			nmp_header header;
			header.id = htonl(NMP_SIMPLE_EVENT);
			header.len = htonl(ev.id_.length());

			const char * data = ev.id_.c_str();
			auto len = ev.id_.length();
			int wr = 0;

			if ( ( wr = write(cfd, (char*) &header,sizeof(header) )) !=	 sizeof(nmp_header) )
			{
				close(cfd);
				conn_established=false;
				DEBUG << "[comm_sender_thread][Partial/failed write]\n";
				continue;
			}

			if (len && data) if ( (wr = write(cfd, data,len )) != len)
			{
				close(cfd);
				conn_established=false;
				DEBUG << "[comm_sender_thread][Partial/failed write]\n";
				continue;
			}
		} else	{
			DEBUG << "[comm_sender_thread][EVENT_WITH_PAYLOAD][ARGC='"<< ev.payload_.size() << "']\n";
			bool flat_data = true;
			for(auto p : ev.payload_) {
				if (p->kind() == ceps::ast::Ast_node_kind::int_literal)continue;
				if (p->kind() == ceps::ast::Ast_node_kind::float_literal)continue;
				if (p->kind() == ceps::ast::Ast_node_kind::string_literal)continue;
				flat_data = false;break;
			}
			if(flat_data){
				DEBUG << "[comm_sender_thread][FLAT_PAYLOAD]\n";
				int wr = 0;
				size_t buffer_size;
				char* data;

				serialize_flat_payload(smc,ev,ev.payload_, &data,buffer_size);

				DEBUG << "[comm_sender_thread][FLAT_PAYLOAD_SIZE="<< buffer_size <<"]\n";
				if ( ( wr = write(cfd, data,buffer_size )) !=	 buffer_size )
				{
					close(cfd);
					conn_established=false;
					DEBUG << "[comm_sender_thread][Partial/failed write]\n";
					continue;
				}
				delete[] data;
			} else {
				smc->fatal_(-1,"Not Implemented.\n");
			}
		}
		DEBUG << "[comm_sender_thread][EVENT_WRITTEN]['"<< ev.id_ << "']\n";
		pop_ev = true;

	}
	if (conn_established)close(cfd);
}

#ifdef __linux__
bool nmp_send_raw(State_machine_simulation_core* smc,std::string ip, std::string port, uint32_t what, uint32_t len, const char * data)
{
	DEBUG_FUNC_PROLOGUE
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
	if (getaddrinfo(ip.c_str(), port.c_str(), &hints, &result) != 0)
		smc->fatal_(-1,"getaddrinfo");

	for (rp = result; rp != NULL; rp = rp->ai_next) {
	 cfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
	 if (cfd == -1)	continue;
	 if (connect(cfd, rp->ai_addr, rp->ai_addrlen) != -1)break;
 	 close(cfd);
	}
	freeaddrinfo(result);
	if (rp == nullptr) { return false;}


	nmp_header header;
	header.id = htonl(what);
	header.len = htonl(len);

	if (write(cfd, &header,sizeof(header) ) !=	 sizeof(nmp_header))
		smc->fatal_(-1,"Partial/failed write (reqLenStr)");
	if (len && data) if (write(cfd, data,len ) != len)
		smc->fatal_(-1,"Partial/failed write (reqLenStr)");


	close(cfd);
	return true;
}

#endif
