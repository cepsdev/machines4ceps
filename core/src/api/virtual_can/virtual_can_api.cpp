#include "core/include/api/virtual_can/virtual_can_api.hpp"
#include "core/include/state_machine_simulation_core.hpp"

#include <sys/types.h>
#include <limits>
#include <cstring>
#include <atomic>
#include "core/include/base_defs.hpp"


extern void comm_receiver_socket_can(int id,
    std::string bus_id,
    State_machine_simulation_core* smc,
    std::unordered_map<int,std::uint32_t> frame2id,
    std::string channel_id,
    bool extended_can_id,
    int sock);
/*
 * virtual_can sockets
*/


static std::pair<bool,std::string> get_virtual_can_attribute_content(std::string attr, std::vector<std::pair<std::string,std::string>> const & http_header){
 for(auto const & v : http_header){
     if (v.first == attr)
         return {true,v.second};
 }
 return {false,{}};
}


static std::tuple<bool,std::string,std::vector<std::pair<std::string,std::string>>> read_virtual_can_request(int sck,std::string& unconsumed_data){
 using header_t = std::vector<std::pair<std::string,std::string>>;
 std::tuple<bool,std::string,header_t> r;

 constexpr auto buf_size = 4096;
 char buf[buf_size];
 std::string buffer = unconsumed_data;
 std::string eom = "\r\n\r\n";
 std::size_t eom_pos = 0;

 unconsumed_data.clear();
 bool req_complete = false;
 ssize_t readbytes = 0;
 ssize_t buf_pos = 0;

 for(; (readbytes=recv(sck,buf,buf_size-1,0)) > 0;){
  buf[readbytes] = 0;
  for(buf_pos = 0; buf_pos < readbytes; ++buf_pos){
   if (buf[buf_pos] == eom[eom_pos])++eom_pos;else eom_pos = 0;
   if (eom_pos == eom.length()){
    req_complete = true;
	if (buf_pos+1 < readbytes) unconsumed_data = buf+buf_pos+1;
	buf[buf_pos+1] = 0;
	break;
   }
  }
  buffer.append(buf);
  if(req_complete) break;
 }
 if (req_complete) {
  header_t header;
  std::string first_line;
  size_t line_start = 0;
  for(size_t i = 0; i < buffer.length();++i){
    if (i+1 < buffer.length() && buffer[i] == '\r' && buffer[i+1] == '\n' ){
		if (line_start == 0) first_line = buffer.substr(line_start,i);
		else if (line_start != i){
		 std::string attribute;
		 std::string content;
		 std::size_t j = line_start;
		 for(;j < i && buffer[j]==' ';++j);
		 auto attr_start = j;
		 for(;j < i && buffer[j]!=':';++j);
		 attribute = buffer.substr(attr_start,j-attr_start);
         ++j; //INVARIANT: buffer[j] == ':' || j == i
		 for(;j < i && buffer[j]==' ' ;++j);
		 auto cont_start = j;
         auto cont_end = i - 1;
		 for(;buffer[cont_end] == ' ';--cont_end);
         if ( cont_start <= cont_end) content = buffer.substr(cont_start, cont_end - cont_start + 1);
         header.push_back(std::make_pair(attribute,content));
		}
		line_start = i + 2;++i;
	}
  }
  return std::make_tuple(true,first_line,header);
 }
 return std::make_tuple(false,std::string{},header_t{});
}

ceps::ast::Nodeset& Virtual_can_interface::hub_directory(){
    reset_directory_of_known_simcores_ = true;
    return this->hub_directory_;
}

directory_of_known_simcores Virtual_can_interface::fetch_known_simcores_thread_safe(){
 std::lock_guard<std::mutex> lg(known_simcores_mutex_);
 sync_known_simcores_with_static_hublist();
 return known_simcores_;
}

void Virtual_can_interface::sync_known_simcores_with_static_hublist(){
    if (reset_directory_of_known_simcores_){
        for (auto simcore_ : hub_directory()[ceps::ast::all{"simcore"}]){
            auto simcore = simcore_["simcore"];
            auto name = simcore["name"].as_str();
            auto short_name = simcore["short"].as_str();
            auto host_name = simcore["host_name"].as_str();
            auto port = simcore["port"].as_str();
            directory_of_known_simcores::simcore_info info;
            info.host_name = host_name;
            info.name = name;
            info.short_name = short_name;
            info.port = port;
            known_simcores_.entries.push_back(info);
        }
        reset_directory_of_known_simcores_ = false;
    }
}

class Execute_knownsimcores;
static std::string make_vcan_config_str(ceps::ast::Nodeset ns, Execute_knownsimcores* p);

class Execute_knownsimcores : public sm4ceps_plugin_int::Executioncontext {
    int sck;
    std::string id_token;
    std::unique_ptr<std::stringstream> response;
    Virtual_can_interface* vcan_int;
    directory_of_known_simcores sim_cores;

 public:
    Execute_knownsimcores() = default;
    Execute_knownsimcores(int s,std::string idt,std::unique_ptr<std::stringstream>&& ss,Virtual_can_interface* vi )
        : sck{s},id_token{idt}, response{std::move(ss)}, vcan_int{vi} {}
    void  run(State_machine_simulation_core* ctxt){

      if(id_token.length()){
          auto ns = ctxt->current_universe()[ceps::ast::all{"__vcan_config_streams"}];
          //std::cout << id_token << std::endl;
          //std::cout << ns << std::endl;
          sim_cores = vcan_int->fetch_known_simcores_thread_safe();

          for(auto it = ns.nodes().rbegin();it!=ns.nodes().rend();++it){
              auto entry = ceps::ast::Nodeset(*it)["__vcan_config_streams"];
              auto ep_ = entry["ep"];
              auto ep = ep_.as_str();
              if (ep != id_token) continue;
              *response << "config: "<< make_vcan_config_str(entry,this) << "\r\n";
              break;
          }
      }
      *response <<"\r\n";
      send(sck,response->str().c_str(),response->str().length(),0);
    }

    std::string get_full_sim_name(std::string n){
        for(auto nn : sim_cores.entries){
            if (n != nn.name) continue;
            return n + "@" + nn.host_name + ":" + nn.port;
        }
        return n;
    }
};

static std::string make_vcan_config_str(ceps::ast::Nodeset ns, Execute_knownsimcores* p){
    std::stringstream ss;
    auto downstreams = ns["downstreams"];
    auto upstreams = ns["upstreams"];
    auto routes = ns["routes"];

    ss << "CONFIG_STREAMS\n";

    if (downstreams.nodes().size()){
        for (auto entry_:downstreams[ceps::ast::all{"entry"}]){
            auto entry = entry_["entry"];
            ss << "CONFIG_DOWN_STREAM\nENTRY\n";
            ss << p->get_full_sim_name(entry["sim"].as_str()) << "\n";
            ss << entry["channel"].as_str() << "\n";
            ss << entry["local_channel"].as_str() << "\n";
        }
    }
    if (upstreams.nodes().size()){
        for (auto entry_:upstreams[ceps::ast::all{"entry"}]){
            auto entry = entry_["entry"];
            ss << "CONFIG_UP_STREAM\nENTRY\n";
            ss << p->get_full_sim_name(entry["sim"].as_str()) << "\n";
            ss << entry["channel"].as_str() << "\n";
            ss << entry["local_channel"].as_str() << "\n";
        }
    }

    return ss.str();
}


void Virtual_can_interface::handler(int sck){

 bool close_sck = true;
 auto cleanup_f = [this,sck,&close_sck](){
     using namespace std;
     lock_guard<std::mutex> lg(handler_threads_status_mutex_);
     for(auto& s : handler_threads_status_)
        if (get<1>(s) && get<0>(s) && get<2>(s)==sck)
          {get<1>(s)=false;get<2>(s)=-1;if(close_sck)close(sck);}

 };
 cleanup<decltype(cleanup_f)> cl{cleanup_f};

 std::string hostname;
 {
  char buffer[HOST_NAME_MAX+1] ={0};
  if (0 == gethostname(buffer,HOST_NAME_MAX)){
      hostname = buffer;
  }
 }

 std::string unconsumed_data;
 for(;;){
    auto rhr = read_virtual_can_request(sck,unconsumed_data);
    if (!std::get<0>(rhr)) break;

    auto const & attrs = std::get<2>(rhr);
    auto cmd_ = get_virtual_can_attribute_content("cmd",attrs);
    if (!cmd_.first) return;
    auto const & cmd = cmd_.second;
    std::stringstream response;
    if (cmd == "get_out_channels"){
        auto v = smc_->get_out_channels();
        response << "HTTP/1.1 100\r\n"<< "out_channels:";
        for(auto ch:v)
            response << " "<<std::get<0>(ch);
        response << "\r\n"<<"types:";
        for(auto ch:v)
            response << " "<<std::get<2>(ch);
        response << "\r\n";
        response <<"\r\n";
    } else if (cmd == "get_in_channels") {
        auto v = smc_->dispatcher_thread_ctxts();
        response << "HTTP/1.1 100\r\n"<< "in_channels:";
        for(auto ch:v)
           response <<" "<<ch->id();
        response << "\r\n"<<"types:";
        for(auto ch:v)
            response <<" "<<ch->info();
        response << "\r\n";
        response <<"\r\n";
    } else if (cmd == "subscribe_out_channel") {
        auto out_channel_ = get_virtual_can_attribute_content("out_channel",attrs);
        if (!out_channel_.first) return;
        auto const & out_channel_name = out_channel_.second;
        auto out_channel = smc_->get_out_channel(out_channel_name);
        if(std::get<0>(out_channel) == nullptr) return;
        auto frame_queue = std::get<0>(out_channel);
        frame_queue->push(std::make_tuple(std::make_tuple(0,nullptr),0,0,sck));
        close_sck = false;
        return;
    } else if (cmd == "subscribe_in_channel") {
        auto in_channel_ = get_virtual_can_attribute_content("in_channel",attrs);
        if (!in_channel_.first) return;
        auto const & in_channel_name = in_channel_.second;
        auto dispatcher_ctxt = smc_->get_dispatcher_thread_ctxt(in_channel_name);
        if (dispatcher_ctxt == nullptr) return;
        comm_receiver_socket_can(dispatcher_ctxt->handle(),
            "",
            smc_,
            smc_->channel_frame_to_id[in_channel_name],
            in_channel_name,
            dispatcher_ctxt->can_extended(),
            sck);
    } else if (cmd == "get_known_sim_cores") {
        std::string id_token;
        auto id_token_ = get_virtual_can_attribute_content("id-token",attrs);
        if (id_token_.first) id_token = id_token_.second;
        std::lock_guard<std::mutex> lg(known_simcores_mutex_);
        sync_known_simcores_with_static_hublist();
        std::unique_ptr<std::stringstream> response{new std::stringstream{} };

        *response << "HTTP/1.1 100\r\n"<< "known_sim_cores:";
        for (auto simcore : known_simcores_.entries){
            *response << "$";
            *response << simcore.name << "\t";
            *response << simcore.short_name << "\t";
            if (simcore.host_name != hostname && simcore.host_name != "127.0.0.1") *response << simcore.host_name;
            *response << ":";
            *response << simcore.port;
            *response << "$";
        }
        *response << "\r\n";

        State_machine_simulation_core::event_t ev;
        auto exec = new Execute_knownsimcores{sck,id_token,std::move(response),this};
        ev.exec = exec;
        smc_->enqueue_event(ev);
        continue;
    } else if (cmd == "register_sim_core") {
        auto sim_name = get_virtual_can_attribute_content("name",attrs);
        if (!sim_name.first) return;
        auto sim_short_name = get_virtual_can_attribute_content("short_name",attrs);
        if (!sim_short_name.first) return;
        auto sim_host_name = get_virtual_can_attribute_content("host_name",attrs);
        if (!sim_host_name.first) return;
        auto sim_port = get_virtual_can_attribute_content("port",attrs);
        if (!sim_port.first) return;
        auto ws_api_port = get_virtual_can_attribute_content("ws_api_port",attrs);

        bool listed = false;
        for (auto & e:known_simcores_.entries){
            if(e.host_name == sim_host_name.second && e.port == sim_port.second){
                e.name = sim_name.second;
                e.short_name = sim_short_name.second;
                break;
                listed = true;
            }
        }
        if(!listed){
            directory_of_known_simcores::simcore_info info;
            info.host_name = sim_host_name.second;
            info.name = sim_name.second;
            info.short_name = sim_short_name.second;
            info.port = sim_port.second;
            if(ws_api_port.first)info.ws_api_port = ws_api_port.second;
            known_simcores_.entries.push_back(info);
        }
        response << "HTTP/1.1 100\r\n\r\n";
    }  else if (cmd == "register_stream_endpoint") {
        std::lock_guard<std::mutex> lk(smc_->vcan_wsapi_mutex());
        auto host = get_virtual_can_attribute_content("host",attrs);
        if (!host.first) return;
        auto port = get_virtual_can_attribute_content("port",attrs);
        if (!port.first) return;
        std::size_t j = 0;
        for(;j != smc_->streaming_endpoints_registered_via_vcan_api().size();++j){
            if (smc_->streaming_endpoints_registered_via_vcan_api()[j].first == host.second) break;
        }
        if (j == smc_->streaming_endpoints_registered_via_vcan_api().size())
            smc_->streaming_endpoints_registered_via_vcan_api().push_back(std::make_pair(host.second,port.second));
        else smc_->streaming_endpoints_registered_via_vcan_api()[j].second = port.second;
        response << "HTTP/1.1 100\r\n\r\n";
    } else if (cmd == "get_known_stream_endpoints") {
        std::lock_guard<std::mutex> lk(smc_->vcan_wsapi_mutex());
        response << "HTTP/1.1 100\r\n"<< "known_stream_endpoints:";
        for (auto s_endpoint : smc_->streaming_endpoints_registered_via_vcan_api()){
            response << "$";
            if (s_endpoint.first != hostname && s_endpoint.first != "127.0.0.1") response << s_endpoint.first;
            response << ":";
            response << s_endpoint.second;
            response << "$";
        }
        response << "\r\n";
        response <<"\r\n";
    } else return;
    auto r = send(sck,response.str().c_str(),response.str().length(),0);
    if (r != (ssize_t)response.str().length()) return;
 }
}


void Virtual_can_interface::dispatcher(){
 socklen_t addrlen = sizeof(struct sockaddr_storage);
 struct sockaddr_storage claddr = {0};
 int cfd = -1;
 struct addrinfo hints = {0};
 struct addrinfo* result, * rp;
 int lfd = -1;

 hints.ai_canonname = nullptr;
 hints.ai_addr = nullptr;
 hints.ai_next = nullptr;
 hints.ai_socktype = SOCK_STREAM;
 hints.ai_family = AF_INET;
 hints.ai_flags = AI_PASSIVE;// | AI_NUMERICSERV;

 if (getaddrinfo(nullptr,port_.c_str(),&hints,&result) != 0)
   smc_->fatal_(-1,"getaddrinfo failed");
 int optval=1;
 for(rp=result;rp;rp=rp->ai_next)
 {
  lfd = socket(rp->ai_family,rp->ai_socktype,rp->ai_protocol);
  if(lfd == -1) continue;
  if (setsockopt(lfd,SOL_SOCKET,SO_REUSEADDR,(char*)&optval,sizeof(optval))) smc_->fatal_(-1,"setsockopt");
  if (bind(lfd,rp->ai_addr,rp->ai_addrlen) == 0) break;
  close(lfd);
 }
 if (!rp) smc_->fatal_(-1,"Virtual_can_interface::dispatcher(): Could not bind socket to any address.port="+port_);
 if (listen(lfd,128)==-1)smc_->fatal_(-1,"listen");
 freeaddrinfo(result);

 for(;!smc_->shutdown();)
 {
  cfd = accept(lfd, (struct sockaddr*) &claddr, &addrlen);
  if (cfd == -1)  continue;
  {
    using namespace std;
    lock_guard<std::mutex> lg(handler_threads_status_mutex_);
    thread_status_type* pp = nullptr;
    for(auto& s : handler_threads_status_) if (!get<1>(s) && get<0>(s)) {
        get<0>(s)->join();delete get<0>(s);get<0>(s)=nullptr;if(!pp) pp = &s;
    }//for
    auto tp = new thread(&Virtual_can_interface::handler,this,cfd);
    if (pp) {*pp = thread_status_type{tp,true,cfd};} else handler_threads_status_.push_back(thread_status_type{tp,true,cfd});
  }
 }
}

void Virtual_can_interface::start(){
    dispatcher_thread_ = new std::thread(&Virtual_can_interface::dispatcher,this);
}






