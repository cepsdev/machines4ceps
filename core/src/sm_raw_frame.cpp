#include "core/include/sm_raw_frame.hpp"
#include "core/include/state_machine_simulation_core.hpp"
#include <sys/types.h>
#include <limits>
#include <cstring>


#include "core/include/base_defs.hpp"


ceps::ast::Nodebase_ptr ceps_interface_eval_func_callback(std::string const & id, ceps::ast::Call_parameters* params, void* context);
ceps::ast::Nodebase_ptr ceps_interface_binop_resolver( ceps::ast::Binary_operator_ptr binop,
	 	 	 	  	  	  	  	  	  	  	  	  	  	  	  ceps::ast::Nodebase_ptr lhs ,
	 	 	 	  	  	  	  	  	  	  	  	  	  	  	  ceps::ast::Nodebase_ptr rhs,
	 	 	 	  	  	  	  	  	  	  	  	  	  	  	  void* cxt,ceps::ast::Nodebase_ptr parent_node);

bool read_func_call_values(State_machine_simulation_core* smc,	ceps::ast::Nodebase_ptr root_node,
							std::string & func_name,
							std::vector<ceps::ast::Nodebase_ptr>& args);

bool Podframe_generator::readfrom_spec(ceps::ast::Nodeset const & spec)
{
 spec_ = spec;
 return true;
}

size_t compute_size(State_machine_simulation_core* smc,ceps::ast::Nodebase_ptr p,std::vector<std::string> const & params){
 using namespace ceps::ast;
 if (p == nullptr) return 0;
 if (p->kind() == ceps::ast::Ast_node_kind::int_literal) return sizeof(std::int64_t);
 if (p->kind() == ceps::ast::Ast_node_kind::float_literal) return sizeof(double);
 if (p->kind() == ceps::ast::Ast_node_kind::string_literal)
	 return ceps::ast::value(ceps::ast::as_string_ref(p)).length();
 if (p->kind() == ceps::ast::Ast_node_kind::symbol){
	 ceps::ast::Symbol& sym = ceps::ast::as_symbol_ref(p);
	 if (ceps::ast::kind(sym) == "Systemstate")
	 {
		 //std::cerr << "!!!!!!!!!!" << std::endl;
		 std::lock_guard<std::recursive_mutex>g(smc->states_mutex());
		 auto it = smc->global_systemstates().find( ceps::ast::name(sym));
		 if (it == smc->global_systemstates().end()) return 0;
		 if (it->second->kind() == ceps::ast::Ast_node_kind::int_literal)
			 return sizeof(std::int64_t);
		 else return 0;
	 }
 }
 if (p->kind() == ceps::ast::Ast_node_kind::identifier){
	 ceps::ast::Identifier& id = ceps::ast::as_id_ref(p);
	 std::string id_name = ceps::ast::name(id);
	 for(auto x:params) if (x==id_name) return sizeof(std::int64_t);
	 return 0;
 }
 if (p->kind() == ceps::ast::Ast_node_kind::func_call)
 {
	 std::string func_name;	std::vector<ceps::ast::Nodebase_ptr> args;
	 read_func_call_values(smc,p,func_name,args);
	 if (func_name == "uint" || func_name == "int")
	 {
		 if (args.size() == 0 || args.size() > 2)
			 smc->fatal_(-1, "uint: uint takes one or two arguments: uint(VALUE) or uint(1-64,VALUE)");
		 if (args.size() == 1) return sizeof(std::int32_t);

		 if (args[0]->kind() != ceps::ast::Ast_node_kind::int_literal)
			 smc->fatal_(-1, "uint: expected an integral scalar as first argument.");
		 return(value(as_int_ref(args[0])) / 8);
	 }
 }
 return 0;
}

size_t compute_size(State_machine_simulation_core* smc,std::vector<ceps::ast::Nodebase_ptr> pattern) {
	size_t acc = 0;
	std::vector<std::string> dummy;
	for(auto p : pattern){

		acc += compute_size(smc,p,dummy);
	}
	return acc;
}

size_t fill_raw_chunk(State_machine_simulation_core* smc,ceps::ast::Nodebase_ptr p,char* data,bool host_byte_order = false) {
	using namespace ceps::ast;
	if (p == nullptr) return 0;
	if (p->kind() == ceps::ast::Ast_node_kind::int_literal){
		if (!host_byte_order ) *((std::int64_t*)data) = htobe64(value(as_int_ref(p)));
		else  *((std::int64_t*)data) = value(as_int_ref(p));
		return sizeof(std::int64_t);
	}
	 if (p->kind() == ceps::ast::Ast_node_kind::float_literal) {
		 if (!host_byte_order ) *((double*)data) = htobe64((std::uint64_t)value(as_double_ref(p)));
		 else *((double*)data) = (std::uint64_t)value(as_double_ref(p));
		 return sizeof(double);
	 }
	 if (p->kind() == ceps::ast::Ast_node_kind::string_literal){
		 std::string s = ceps::ast::value(ceps::ast::as_string_ref(p));
		 memcpy(data,s.c_str(),s.length());
		 return s.length();
	 }
	 if (p->kind() == ceps::ast::Ast_node_kind::func_call)
	 {
		 std::string func_name;	std::vector<ceps::ast::Nodebase_ptr> args;
		 read_func_call_values(smc,p,func_name,args);
		 if (func_name == "uint" || func_name == "int")
		 {
			 if (args.size() == 0 || args.size() > 2)
				 smc->fatal_(-1, "uint: uint takes one or two arguments: uint(VALUE) or uint(1-64,VALUE)");
			 if (args.size() == 1) return sizeof(std::int32_t);

			 if (args[0]->kind() != ceps::ast::Ast_node_kind::int_literal)
				 smc->fatal_(-1, "uint: expected an integral scalar as first argument.");
			 if (args[1]->kind() != ceps::ast::Ast_node_kind::int_literal)
				 smc->fatal_(-1, "uint: expected an integral value as second argument.");

			 auto v = value(as_int_ref(args[1]));
			 size_t l = value(as_int_ref(args[0])) / 8;
			 if (l == 8) memcpy(data, (char*)&v,1);
			 else if (l == 16) memcpy(data, (char*)&v,2);
			 else if (l == 24) memcpy(data, (char*)&v,3);
			 else if (l == 32) {auto vv = htobe32(v); memcpy(data, (char*)&vv,4);}
			 else if (l == 40) memcpy(data, (char*)&v,5);
			 else if (l == 48) memcpy(data, (char*)&v,6);
			 else if (l == 56) memcpy(data, (char*)&v,7);
			 else memcpy(data, (char*)&v,6);

			 return l;
		 }
	 }
	 return 0;
}

void fill_raw_chunk(State_machine_simulation_core* smc,std::vector<ceps::ast::Nodebase_ptr> pattern,char* data) {
	size_t offs = 0;
	for(auto p : pattern){
		offs += fill_raw_chunk(smc,p,data+offs);
	}
}

size_t Podframe_generator::compute_size_of_msg(State_machine_simulation_core* smc,
		                                       std::vector<std::string> params,bool& failed)
{
	size_t acc = 0;
	failed = true;
	auto data_format = spec_["data"];

	for(auto p : data_format.nodes()){
		//std::cerr << *p << std::endl;
		auto t =  compute_size(smc,p,params);
		if (t == 0) return 0;
		acc += t;
	}

	failed = false;

	return acc;
}

void* Podframe_generator::gen_msg(State_machine_simulation_core* smc,size_t& data_size){

	if (smc == nullptr) return nullptr;
	auto THIS = smc;
	DEBUG_FUNC_PROLOGUE2;
	auto data_format = spec_["data"];
	if (data_format.nodes().empty()) return nullptr;

	ceps_interface_eval_func_callback_ctxt_t ctxt;
	ctxt.active_smp = nullptr;
	ctxt.smc  = smc;

	ceps::ast::Nodebase_ptr frame_pattern = nullptr;
	ceps::ast::Scope scope;
	scope.children() = data_format.nodes();
	{
		std::lock_guard<std::recursive_mutex>g(smc->states_mutex());


		smc->ceps_env_current().interpreter_env().symbol_mapping()["Systemstate"] = &smc->global_systemstates();
		smc->ceps_env_current().interpreter_env().set_func_callback(ceps_interface_eval_func_callback,&ctxt);
		smc->ceps_env_current().interpreter_env().set_binop_resolver(ceps_interface_binop_resolver,this);
		frame_pattern = ceps::interpreter::evaluate(&scope,
				smc->ceps_env_current().get_global_symboltable(),
				smc->ceps_env_current().interpreter_env(),nullptr	);
		smc->ceps_env_current().interpreter_env().set_func_callback(nullptr,nullptr);
		smc->ceps_env_current().interpreter_env().set_binop_resolver(nullptr,nullptr);
	}

	if (frame_pattern == nullptr) return nullptr;
	auto chunk_size = compute_size(smc,ceps::ast::nlf_ptr(frame_pattern)->children());
	DEBUG << "[Podframe_generator::gen_msg][CHUNK_SIZE="<<chunk_size<<"]\n";
	char* data = new char[chunk_size];
	bzero(data,chunk_size);
	fill_raw_chunk( smc,ceps::ast::nlf_ptr(frame_pattern)->children(), data);
	for(size_t offs = 0; offs < chunk_size;++offs)
		DEBUG <<"[Podframe_generator::gen_msg][CHUNK_BYTE_"<< offs << "="<< ((std::uint32_t) *( (unsigned char*)data+offs)) << "]\n";
	data_size = chunk_size;
	return data;
}

bool Podframe_generator::read_msg(char* data,size_t size,
		                          State_machine_simulation_core* smc,
		                          std::vector<std::string> params,
		                          std::vector<ceps::ast::Nodebase_ptr>& payload)
{
	std::map<std::string,int> p2i;
	for(size_t i = 0;i < params.size(); ++i) {p2i[params[i]] = i;payload.push_back(nullptr);}
	auto data_format = spec_["data"];
	size_t offs = 0;

	for(auto p : data_format.nodes()){
		using namespace ceps::ast;
		if (p == nullptr) return false;

		if (p->kind() == ceps::ast::Ast_node_kind::int_literal){
			offs += sizeof(std::int64_t);
		}
		else if (p->kind() == ceps::ast::Ast_node_kind::float_literal) {
			 offs += sizeof(double);
	    }
		else  if (p->kind() == ceps::ast::Ast_node_kind::string_literal){
			 std::string s = ceps::ast::value(ceps::ast::as_string_ref(p));
			 offs += s.length();
		} else if (p->kind() == ceps::ast::Ast_node_kind::symbol && kind(as_symbol_ref(p)) == "Systemstate")
		{
			std::string state_id = name(as_symbol_ref(p));
			auto it = smc->get_global_states().find(state_id);
			if (it == smc->get_global_states().end()) return false;
			auto state_value = it->second;
			if (state_value == nullptr) return 0;
			if (state_value->kind() == ceps::ast::Ast_node_kind::int_literal){
				Int& v = as_int_ref(state_value);
				value(v) = be64toh( *((std::int64_t*)(data+offs)) );
				offs += sizeof(std::int64_t);
			} else return false;
		} else if (p->kind() == ceps::ast::Ast_node_kind::identifier)
		{
			std::string id = name(as_id_ref(p));
			if (p2i.find(id) == p2i.end()) return false;
			auto idx = p2i[id];
			payload[idx] = new Int(be64toh( *((std::int64_t*)(data+offs)) ), ceps::ast::all_zero_unit(),nullptr,nullptr,nullptr );
			offs += sizeof(std::int64_t);
		}
	}

	return true;
}





void comm_sender_generic_tcp_out_thread(threadsafe_queue< std::pair<char*,size_t>, std::queue<std::pair<char*,size_t> >>* frames,
			     State_machine_simulation_core* smc,
			     std::string ip,
			     std::string port,
			     std::string eof)
{
	auto THIS = smc;
	DEBUG_FUNC_PROLOGUE2
	int cfd;
	struct addrinfo hints;
	struct addrinfo *result, *rp;
	auto q = frames;
	bool conn_established = false;
	char* frame = nullptr;
	size_t frame_size = 0;
	bool pop_frame = true;
	for(;;)
	{
		rp = nullptr;result = nullptr;

		DEBUG << "[comm_sender_generic_tcp_out_thread][WAIT_FOR_FRAME][pop_frame="<<pop_frame <<"]\n";
		std::pair<char*,size_t> frame_info;

		if (pop_frame) {q->wait_and_pop(frame_info);frame_size = frame_info.second;frame= frame_info.first;}
		pop_frame = false;

		DEBUG << "[comm_sender_generic_tcp_out_thread][FETCHED_FRAME]\n";
		if (!conn_established)
		{
			DEBUG << "[comm_sender_generic_tcp_out_thread][CONNECTING@"<< ip << ":" << port << "]\n";
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
				DEBUG << "[comm_sender_generic_tcp_out_thread][FAILED_TO_CONNECT@"<< ip << ":" << port << "]\n";
				std::this_thread::sleep_for(std::chrono::microseconds(1000000));continue;
			}

			 for (rp = result; rp != NULL; rp = rp->ai_next) {
			  cfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
			  if (cfd == -1)	continue;
			  if (connect(cfd, rp->ai_addr, rp->ai_addrlen) != -1)break;
			  close(cfd);
			 }
			 if (result != nullptr) freeaddrinfo(result);
			 if (rp == nullptr) {
				 DEBUG << "[comm_sender_generic_tcp_out_thread][FAILED_TO_CONNECT@"<< ip << ":" << port << "]\n";
				 std::this_thread::sleep_for(std::chrono::microseconds(1000000));continue;
			 }
			}
			conn_established = true;
		}

		DEBUG << "[comm_sender_generic_tcp_out_thread][SENDING_FRAME@"<< ip << ":" << port << "]\n";


		auto len = frame_size;
		int wr = 0;

		//std::cout << "-++---------------------------\n";
		//std::cout << eof << std::endl;


		if (len && frame) if ( (wr = write(cfd, frame,len )) != len)
		{
			close(cfd);
			conn_established=false;
			DEBUG << "[comm_sender_generic_tcp_out_thread][Partial/failed write]\n";
			continue;
		}
		if (eof.size())
			if ( (wr = write(cfd, eof.c_str(),eof.length() )) != eof.length())
			{
				close(cfd);
				conn_established=false;
				DEBUG << "[comm_sender_generic_tcp_out_thread][Partial/failed write]\n";
				continue;
			}
		DEBUG << "[comm_sender_generic_tcp_out_thread][FRAME_WRITTEN][("<< frame_size<< " bytes)]\n";
		if (frame != nullptr) {delete[] frame;frame=nullptr;}
		pop_frame = true;

	}
	if (conn_established)close(cfd);
}


void comm_generic_tcp_in_thread_fn(int id,
		 Rawframe_generator* gen,
		 std::string ev_id,
		 std::vector<std::string> params,
		 State_machine_simulation_core* smc,
		 sockaddr_storage claddr,int sck,std::string eof)
{
	auto THIS = smc;
	DEBUG_FUNC_PROLOGUE2
	char host[1024] = {0};
	char service[1024] = {0};
	char addrstr[2048] = {0};
	socklen_t addrlen = sizeof(struct sockaddr_storage);

	if (getnameinfo((struct sockaddr*)&claddr,addrlen,host,1024,service,1024,0) == 0)
		snprintf(addrstr,2048,"[host=%s, service=%s]",host, service);
	else
		snprintf(addrstr,2048,"[host=?,service=?]");
	DEBUG << "[comm_generic_tcp_in_thread_fn][CONN_ESTABLISHED]" << addrstr << "\n";


	size_t frame_size = 0;
	if (eof.length() == 0){
	 bool failed_size_computation = true;
	 for(;failed_size_computation;)
	 {
		frame_size = gen->compute_size_of_msg(smc,params,failed_size_computation);
		if (failed_size_computation) std::this_thread::sleep_for(std::chrono::microseconds(1));
	 }

	 DEBUG << "[comm_generic_tcp_in_thread_fn][FRAME_SIZE_AVAILABLE]" << addrstr << "\n";
	} else DEBUG << "[comm_generic_tcp_in_thread_fn][EOF_MODE_DETECTED]\n";


	if (eof.length())
	{
		;
		std::string last_suffix;
		for(;!smc->shutdown();){
			char buffer_[4096];
			std::stringstream* buffer = new std::stringstream;
			auto n = 0;
			DEBUG << "[comm_generic_tcp_in_thread_fn][READ...]\n";
			for(; (n = recv(sck,buffer_,4095,0)) > 0;)
			{
				DEBUG << "[comm_generic_tcp_in_thread_fn][DATA_RECEIVED][SIZE="<<n <<"]\n";
				buffer_[n] = 0;

				//DEBUG << "[comm_generic_tcp_in_thread_fn][DATA_RECEIVED][VALUE='"<<buffer_ <<"']\n";

				std::string temp(buffer_);
				if (temp == eof) {

				}
				std::string::size_type r=0;
				bool buffer_processed = false;
				do{
					auto new_r = temp.find(eof,r);
					//std::cout << new_r << std::endl;
					buffer_processed = new_r == std::string::npos;
					*buffer << temp.substr(r,new_r-r );
					if (new_r != std::string::npos){

						DEBUG << "[comm_generic_tcp_in_thread_fn][DATA_CHUNK_READ][SIZE="<<buffer->str().length() <<"]\n";
						r = new_r+eof.length();

					    bool decode_result = false;
					    std::vector<ceps::ast::Nodebase_ptr> payload;
					    {
					    	std::lock_guard<std::recursive_mutex>g(smc->states_mutex());
					    	decode_result = gen->read_msg((char*)buffer->str().c_str(),
					    								  buffer->str().length(),
					    								  smc,
					    								  params,payload);
					    	if (decode_result) DEBUG << "[comm_generic_tcp_in_thread_fn][DATA_SUCCESSFULLY_DECODED]\n";
					    	else DEBUG << "[comm_generic_tcp_in_thread_fn][FAILED_TO_DECODE_DATA]\n";
					    }

					    if (decode_result){
					    	State_machine_simulation_core::event_t ev(ev_id);
					    	ev.already_sent_to_out_queues_ = true;
					    	if (payload.size())
					    		ev.payload_ = payload;
					    	smc->main_event_queue().push(ev);
					    }

						delete buffer;
						buffer =  new std::stringstream;
					}
				} while (!buffer_processed && r < temp.length());
			}
			delete buffer;
			if (n<=0) break;
		}
	} else

	for(;!smc->shutdown() && frame_size;)
	{
		char *data;
		try
		{
			DEBUG << "[comm_generic_tcp_in_thread_fn][READING_DATA]\n";
			data = new char[frame_size];
			if (data == nullptr){DEBUG << "[ERROR_comm_generic_tcp_in_thread_fn][ALLOC_FAILED]\n";close(sck);return;}
			ssize_t already_read = 0;
			ssize_t n = 0;
			for(; (already_read < frame_size) && (n = recv(sck,data+already_read,frame_size-already_read,0)) > 0;already_read+=n);

			if(already_read < frame_size){DEBUG << "[ERROR_comm_generic_tcp_in_thread_fn][READ_FAILED]\n";close(sck);return;}
		    DEBUG << "[comm_generic_tcp_in_thread_fn][DATA_SUCCESSFULLY_READ]\n";


		    bool decode_result = false;
		    std::vector<ceps::ast::Nodebase_ptr> payload;
		    {
		    	std::lock_guard<std::recursive_mutex>g(smc->states_mutex());
		    	decode_result = gen->read_msg(data,frame_size,smc,params,payload);
		    	if (decode_result) DEBUG << "[comm_generic_tcp_in_thread_fn][DATA_SUCCESSFULLY_DECODED]\n";
		    	else DEBUG << "[comm_generic_tcp_in_thread_fn][FAILED_TO_DECODE_DATA]\n";
		    }

		    if (decode_result){
		    	State_machine_simulation_core::event_t ev(ev_id);
		    	ev.already_sent_to_out_queues_ = true;
		    	if (payload.size())
		    		ev.payload_ = payload;
		    	smc->main_event_queue().push(ev);
		    }




		} catch(...){}

		if (data) delete[] data;
	}
	close(sck);
}



void comm_generic_tcp_in_dispatcher_thread(int id,
				 Rawframe_generator* gen,
				 std::string ev_id,
				 std::vector<std::string> params,
			     State_machine_simulation_core* smc,
			     std::string ip,
			     std::string port,std::string eof,
			     void (*handler_fn) (int,Rawframe_generator*,std::string,std::vector<std::string> ,State_machine_simulation_core* , sockaddr_storage,int,std::string))
{
	auto THIS = smc;
	DEBUG_FUNC_PROLOGUE2
	std::vector<std::thread*> client_handler_threads;


	struct addrinfo hints;
	struct addrinfo* result, * rp;
	int lfd;

	memset(&hints,0,sizeof(struct addrinfo));
	hints.ai_canonname = nullptr;
	hints.ai_addr = nullptr;
	hints.ai_next = nullptr;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_INET;
    hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;

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
	if (!rp) smc->fatal_(-1,"comm_dispatcher_thread:Could not bind socket to any address.port="+port);

	if (listen(lfd,5)==-1)smc->fatal_(-1,"listen");

	freeaddrinfo(result);

	for(;!smc->shutdown();)
	{

		socklen_t addrlen = sizeof(struct sockaddr_storage);
		struct sockaddr_storage claddr;
		int cfd = accept(lfd, (struct sockaddr*) &claddr, &addrlen);
		if (cfd == -1){
			DEBUG << "[ERROR_COMM_DISPATCHER][ACCEPT_FAILED]\n";continue;
		}
		if (handler_fn)
			client_handler_threads.push_back(new std::thread(*handler_fn,id,gen,ev_id,params,smc,claddr,cfd,eof));
		else close(cfd);
	}
	for(auto tp: client_handler_threads) tp->join();
}







