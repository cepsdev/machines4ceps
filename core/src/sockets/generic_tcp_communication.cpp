/*
Copyright 2021 Tomas Prerovsky (cepsdev@hotmail.com).

Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include "core/include/state_machine_simulation_core.hpp"
#include "core/include/base_defs.hpp"
#include "core/include/websocket.hpp"

std::string base64_encoded_sha1(std::string s);

void comm_sender_generic_tcp_out_thread(
 State_machine_simulation_core::frame_queue_t* frames,
 State_machine_simulation_core* smc,
 std::string ip,
 std::string port,
 std::string som,
 std::string eof,
 std::string sock_name,
 bool reuse_socket,
 bool reg_socket)
{
 DEBUG_FUNC_PROLOGUE2
 int cfd = -1;
 struct addrinfo hints;
 struct addrinfo *result, *rp;
 auto q = frames;
 bool conn_established = false;
 bool io_err = false;
 bool socket_owner = !reuse_socket;

 char* frame = nullptr;
 size_t frame_size = 0;
 bool pop_frame = true;
 State_machine_simulation_core::dispatcher_thread_ctxt_t* ctxt = nullptr;
 State_machine_simulation_core::frame_queue_elem_t frame_info;

 for(;;)
 {
  if (io_err && socket_owner){
   if(reg_socket){
	 std::lock_guard<std::recursive_mutex>g(smc->get_reg_sock_mtx());
	 smc->get_reg_socks()[sock_name] = std::tuple<int,bool>{-1,false};
	}
	close(cfd);
	io_err = false;
  }
  rp = nullptr;result = nullptr;

  if (pop_frame) {
   q->wait_and_pop(frame_info);
   frame_size = std::get<1>(frame_info);
   frame = (decltype(frame))std::get<1>(std::get<0>(frame_info));

  }
  auto options = std::get<0>(std::get<0>(frame_info));


  pop_frame = false;
  if (!conn_established)
  {
   if (reuse_socket){
   for(;;){
    {
	 std::lock_guard<std::recursive_mutex>g(smc->get_reg_sock_mtx());
	 auto it = smc->get_reg_socks().find(sock_name);
	 if (it != smc->get_reg_socks().end() && std::get<1>(it->second) && (!io_err || cfd != std::get<0>(it->second) )){
	  cfd = std::get<0>(it->second);
	  conn_established = true;io_err = false;
	  break;
	 }
	}
    std::this_thread::sleep_for(std::chrono::microseconds(1000));continue;
   }//for
  } else {
   for(;rp == nullptr;)
   {
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_canonname = NULL;    hints.ai_addr = NULL;    hints.ai_next = NULL;
    hints.ai_family = AF_INET;    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(ip.c_str(), port.c_str(), &hints, &result) != 0){
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
    std::this_thread::sleep_for(std::chrono::microseconds(1000000));continue;
   }
  }//for
  conn_established = true;
  if(reg_socket){
   std::lock_guard<std::recursive_mutex>g(smc->get_reg_sock_mtx());
   smc->get_reg_socks()[sock_name] = std::tuple<int,bool>{cfd,true};
  }
  }
  if (reuse_socket && sock_name.length() && ctxt == nullptr){
   ctxt = smc->get_dispatcher_thread_ctxt(sock_name);
  }
 }

  auto len = frame_size;
  int wr = 0;

  if (som.size())
   if ( (wr = write(cfd, som.c_str(),eof.length() )) != (int)som.length())
   {
    io_err=true;
    conn_established=false;
    continue;
   }
  if (len && frame){
   if (ctxt->websocket_server()){
	   
	bool fin = true;
	bool ext1_len = len >= 126 && len <= 65535;
	bool ext2_len = len > 65535;

    std::uint16_t header = 0;
    if (fin) header |= 0x80;
    if(!ext1_len && !ext2_len) header |= len << 8;
    else if (ext1_len) header |= 126 << 8;
    else header |= 127 << 8;
    if (options & Rawframe_generator::IS_BINARY) header |= 2;
    else if ( (options & Rawframe_generator::IS_ASCII) || (options & Rawframe_generator::IS_JSON)) header |= 1;
    if( (wr = write(cfd, &header,sizeof header )) != sizeof header){io_err=true;conn_established=false;continue;}
    if (ext1_len)
    {
     std::uint16_t v = len;v = htons(v);
     if( (wr = write(cfd, &v,sizeof v )) != sizeof v){io_err=true;conn_established=false;continue;}
    }
    if (ext2_len)
    {
     std::uint64_t v = len;v = htobe64(v);
     if( (wr = write(cfd, &v,sizeof v )) != sizeof v){io_err=true;conn_established=false;continue;}
    }
    if ( (wr = write(cfd, frame,len )) != (int)len){io_err=true;conn_established=false;continue;}
   } else {
    if ( (wr = write(cfd, frame,len )) != (int)len)
	{
	 io_err=true;
	 conn_established=false;
	 continue;
	}
   }
  }

  if (eof.size())
   if ( (wr = write(cfd, eof.c_str(),eof.length() )) != (int)eof.length())
   {
	io_err=true;
	conn_established=false;
	continue;
   }
   if (frame != nullptr) {delete[] frame;frame=nullptr;}
   pop_frame = true;
 }
 if (conn_established && socket_owner){
  if(reg_socket){
   std::lock_guard<std::recursive_mutex>g(smc->get_reg_sock_mtx());
   smc->get_reg_socks()[sock_name] = std::tuple<int,bool>{-1,false};
  }
  close(cfd);
 }
}



static void comm_act_as_websocket_server(State_machine_simulation_core::dispatcher_thread_ctxt_t * ctx,
		State_machine_simulation_core* smc,sockaddr_storage claddr, int sck,std::string ev_id,std::string sock_name,bool reg_sock,bool reuse_sock){
 auto cleanup_f = [smc,reg_sock,sck,sock_name](){
  if (reg_sock){
	 	std::lock_guard<std::recursive_mutex> g(smc->get_reg_sock_mtx());
	 	smc->get_reg_socks()[sock_name] = std::tuple<int,bool>{-1,false};
  }
  close(sck);
 };
 cleanup<decltype(cleanup_f)> cl{cleanup_f};

 std::string unconsumed_data;
 auto rhr = ceps::http::read_http_request(sck,unconsumed_data);
 if (!std::get<0>(rhr)) return;
 auto const & attrs = std::get<2>(rhr);
 if (!ceps::http::field_with_content("Upgrade","websocket",attrs) || !ceps::http::field_with_content("Connection","Upgrade",attrs)) return;
 auto r = ceps::http::get_http_attribute_content("Sec-WebSocket-Key",attrs);
 if (!r.first)return;
 auto phrase = r.second+"258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
 auto hash = base64_encoded_sha1(phrase);
 std::stringstream response;
 response
  << "HTTP/1.1 101 Switching Protocols\r\n"
  << "Upgrade: websocket\r\n"
  << "Connection: Upgrade\r\n"
  << "Sec-WebSocket-Accept: "
  << hash <<"\r\n\r\n";
 auto byteswritten = write(sck, response.str().c_str(),response.str().length());
 if (byteswritten == (ssize_t)response.str().length()){
  if (reg_sock){
	std::lock_guard<std::recursive_mutex> g(smc->get_reg_sock_mtx());
	smc->get_reg_socks()[sock_name] = std::tuple<int,bool>{sck,true};
  }
  for(;;){
   auto frm = ceps::websocket::read_websocket_frame(sck);
   if (!frm.first) break;
   std::vector<unsigned char> payload = std::move(frm.second.payload);
   while (!frm.second.fin){
    frm = ceps::websocket::read_websocket_frame(sck);
    if (!frm.first) break;
    payload.reserve(payload.size()+frm.second.payload.size());
    payload.insert(payload.end(),frm.second.payload.begin(),frm.second.payload.end());
   }
   if (!frm.first) break;
   if(frm.second.opcode == 1) {
    std::string s; s.resize(payload.size());for(size_t j = 0; j < payload.size();++j)s[j] = payload[j];//std::cout << s << std::endl;
    State_machine_simulation_core::event_t ev(ev_id);
    ev.already_sent_to_out_queues_ = true;
    ev.payload_.push_back(new ceps::ast::String(s));
 	smc->main_event_queue().push(ev);
   }
  }//for
 }
}

void comm_generic_tcp_in_thread_fn(
 int id,
 Rawframe_generator* gen,
 std::string ev_id,
 std::vector<std::string> params,
 State_machine_simulation_core* smc,
 sockaddr_storage claddr,
 int sck,
 std::string som,
 std::string eof,std::string sock_name,bool reg_sock,bool reuse_sock){

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
 if (id >= 0 && ev_id.length() == 0){ // This is a generic fixed size input stream with a handler context
  auto ctxt = smc->get_dispatcher_thread_ctxt(id);
  std::uint32_t size = 0;
  char* buffer = nullptr;
  for(;!smc->shutdown();){
   std::uint32_t new_size = 0;
   auto r = recv(sck,(char*)&new_size,sizeof(new_size),0);
   if (r != sizeof(new_size)) break;
   new_size = ntohl(new_size);
   if (new_size > size){if (buffer) delete[] buffer; buffer = new char[size = new_size];}
   if(buffer == nullptr) {smc->fatal_(-1,"Couldn't allocate memory.");}
   r = recv(sck,buffer,size,0);
   DEBUG << "[comm_generic_tcp_in_thread_fn][DATA_READ][SIZE="<<new_size<<"]"<< "\n";
   std::vector<std::string> v1;
   std::vector<ceps::ast::Nodebase_ptr> v2;
   for(auto handler_info: ctxt->handler){
    auto match = handler_info.first->read_msg(buffer,new_size,smc,v1,v2);
	if (!match) continue;
	DEBUG << "[comm_generic_tcp_in_thread_fn][MATCH_FOUND]\n";
	smc->execute_action_seq(nullptr,handler_info.second);
   }
  }
  close(sck);if (buffer) delete[] buffer;
  return;
 } else if (id >= 0){
  auto ctxt = smc->get_dispatcher_thread_ctxt(id);
  if (ctxt->websocket_server()) comm_act_as_websocket_server(ctxt,smc,claddr,sck,ev_id,sock_name,reg_sock,reuse_sock);
  return;
 }

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
   std::string temp(buffer_);
   if (temp == eof) {
   }
   std::string::size_type r=0;
   bool buffer_processed = false;
   do{
    auto new_r = temp.find(eof,r);
	buffer_processed = new_r == std::string::npos;
	*buffer << temp.substr(r,new_r-r );
	if (new_r != std::string::npos){
	 DEBUG << "[comm_generic_tcp_in_thread_fn][DATA_CHUNK_READ][SIZE="<<buffer->str().length() <<"]\n";
	 r = new_r+eof.length();
     bool decode_result = false;
	 std::vector<ceps::ast::Nodebase_ptr> payload;
	 {
	  std::lock_guard<std::recursive_mutex>g(smc->states_mutex());
	  decode_result = gen->read_msg((char*)buffer->str().c_str() +som.length(),
       buffer->str().length()-som.length(),
	   smc,
	   params,payload);
	  if (decode_result) DEBUG << "[comm_generic_tcp_in_thread_fn][DATA_SUCCESSFULLY_DECODED]\n";
	  else DEBUG << "[comm_generic_tcp_in_thread_fn][FAILED_TO_DECODE_DATA]\n";
	 }
     if (decode_result){
	  if (gen->frame_ctxt != nullptr){
	   auto clone = gen->frame_ctxt->clone();
	   State_machine_simulation_core::event_t ev;
	   ev.id_ = "@@framecontext";
	   ev.frmctxt_ = clone;
	   smc->enqueue_event(ev);
     }
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
		char *data = nullptr;
		try
		{
			DEBUG << "[comm_generic_tcp_in_thread_fn][READING_DATA]\n";
			data = new char[frame_size];
			if (data == nullptr){DEBUG << "[ERROR_comm_generic_tcp_in_thread_fn][ALLOC_FAILED]\n";close(sck);return;}
			ssize_t already_read = 0;
			ssize_t n = 0;
			for(; (already_read < (ssize_t)frame_size) && (n = recv(sck,data+already_read,(ssize_t)frame_size-already_read,0)) > 0;already_read+=n);

			if(already_read < (ssize_t)frame_size){DEBUG << "[ERROR_comm_generic_tcp_in_thread_fn][READ_FAILED]\n";close(sck);return;}
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
			     std::string port,std::string som,std::string eof,std::string sock_name,bool reg_sock,bool reuse_sock,
			     void (*handler_fn) (int,Rawframe_generator*,std::string,std::vector<std::string> ,State_machine_simulation_core* , sockaddr_storage,int,std::string,std::string,std::string,bool,bool))
{
 std::vector<std::thread*> client_handler_threads;
 socklen_t addrlen = sizeof(struct sockaddr_storage);
 struct sockaddr_storage claddr = {0};
 int cfd = -1;

 if (reuse_sock){
  for(;;){
   {
	std::lock_guard<std::recursive_mutex>g(smc->get_reg_sock_mtx());
	auto it = smc->get_reg_socks().find(sock_name);
	if (it != smc->get_reg_socks().end() && std::get<1>(it->second)){
	 cfd = std::get<0>(it->second);
	 new std::thread(*handler_fn,id,gen,ev_id,params,smc,claddr,cfd,som,eof,sock_name,reg_sock, reuse_sock);
	 return;
	}
   }
   std::this_thread::sleep_for(std::chrono::microseconds(1000));continue;
  }//for
 }
 struct addrinfo hints = {0};
 struct addrinfo* result, * rp;
 int lfd = -1;

 hints.ai_canonname = nullptr;
 hints.ai_addr = nullptr;
 hints.ai_next = nullptr;
 hints.ai_socktype = SOCK_STREAM;
 hints.ai_family = AF_INET;
 hints.ai_flags = AI_PASSIVE;// | AI_NUMERICSERV;

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
  cfd = accept(lfd, (struct sockaddr*) &claddr, &addrlen);
  if (reg_sock){
   std::lock_guard<std::recursive_mutex> g(smc->get_reg_sock_mtx());
   smc->get_reg_socks()[sock_name] = std::tuple<int,bool>{cfd,false};
 }
 if (cfd == -1){
  continue;
 }
 if (handler_fn)
  client_handler_threads.push_back(new std::thread(*handler_fn,id,gen,ev_id,params,smc,claddr,cfd,som,eof,sock_name,reg_sock,reuse_sock));
 else close(cfd);
 }
 for(auto tp: client_handler_threads) tp->join();
}
