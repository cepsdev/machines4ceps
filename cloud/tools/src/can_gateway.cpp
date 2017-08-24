#include "common.h"
#include <sstream>
using namespace ceps;
using namespace ceps::cloud;
using namespace ceps::misc;

std::vector<std::string> ceps::cloud::sys_info_available_ifs;
std::map<Simulation_Core,std::vector< Downstream_Mapping > > ceps::cloud::mappings_downstream;
std::map<Simulation_Core,std::vector< Upstream_Mapping > > ceps::cloud::mappings_upstream;
std::map<Simulation_Core, std::shared_ptr<ctrl_thread_info>   > ceps::cloud::ctrl_threads;
std::map<Simulation_Core,std::vector< std::pair<Remote_Interface,Remote_Interface_Type> > > ceps::cloud::info_out_channels;
std::map<Simulation_Core,std::vector< std::pair<Remote_Interface,Remote_Interface_Type> > > ceps::cloud::info_in_channels;

downstream_threads_t ceps::cloud::downstream_threads;
Simulation_Core ceps::cloud::current_core;
std::mutex ceps::cloud::global_mutex;

#ifdef PCAN_API
std::vector< std::pair<unsigned int, std::string> > pcan_api::all_channels  =
{
	{ PCAN_ISABUS1 , "PCAN-ISA-1" },{ PCAN_ISABUS2 , "PCAN-ISA-2" },{ PCAN_ISABUS3 , "PCAN-ISA-3" },{ PCAN_ISABUS4 , "PCAN-ISA-4" },
	{ PCAN_ISABUS5 , "PCAN-ISA-5" },{ PCAN_ISABUS6 , "PCAN-ISA-6" },{ PCAN_ISABUS7 , "PCAN-ISA-7" },{ PCAN_ISABUS8 , "PCAN-ISA-8" },
	{ PCAN_DNGBUS1 , "PCAN-DNG-1" },
	{ PCAN_PCIBUS1 , "PCAN_PCI-1" }  ,{ PCAN_PCIBUS2 , "PCAN_PCI-2" } ,{ PCAN_PCIBUS3 , "PCAN_PCI-3" } ,{ PCAN_PCIBUS4 , "PCAN_PCI-4" },
	{ PCAN_PCIBUS5 , "PCAN_PCI-5" } ,{ PCAN_PCIBUS6 , "PCAN_PCI-6" } ,{ PCAN_PCIBUS7 , "PCAN_PCI-7" } ,{ PCAN_PCIBUS8 , "PCAN_PCI-8" },
	{ PCAN_PCIBUS9 , "PCAN_PCI-9" } ,{ PCAN_PCIBUS10 , "PCAN_PCI-10" } ,{ PCAN_PCIBUS11 , "PCAN_PCI-11" } ,{ PCAN_PCIBUS12 , "PCAN_PCI-12" },
	{ PCAN_PCIBUS13 , "PCAN_PCI-13" } ,{ PCAN_PCIBUS14 , "PCAN_PCI-14" } ,{ PCAN_PCIBUS15 , "PCAN_PCI-15" } ,{ PCAN_PCIBUS16 , "PCAN_PCI-16" },
	{ PCAN_USBBUS1,"PCAN-USB-1" },{ PCAN_USBBUS2,"PCAN-USB-2" },{ PCAN_USBBUS3,"PCAN-USB-3" },{ PCAN_USBBUS4,"PCAN-USB-4" },
	{ PCAN_USBBUS5,"PCAN-USB-5" },{ PCAN_USBBUS6,"PCAN-USB-6" },{ PCAN_USBBUS7,"PCAN-USB-7" },{ PCAN_USBBUS8,"PCAN-USB-8" } ,
	{ PCAN_USBBUS9,"PCAN-USB-9" },{ PCAN_USBBUS10,"PCAN-USB-10" },{ PCAN_USBBUS11,"PCAN-USB-11" },{ PCAN_USBBUS12,"PCAN-USB-12" },
	{ PCAN_USBBUS13,"PCAN-USB-13" },{ PCAN_USBBUS14,"PCAN-USB-14" },{ PCAN_USBBUS15,"PCAN-USB-15" },{ PCAN_USBBUS16,"PCAN-USB-16" }
};

std::vector< std::pair<std::string, net::can::can_info> > pcan_api::all_channels_info = {
	{ "PCAN-ISA-1",net::can::can_info{} },{ "PCAN-ISA-2",net::can::can_info{} },{ "PCAN-ISA-3",net::can::can_info{} },{ "PCAN-ISA-4",net::can::can_info{} },
	{  "PCAN-ISA-5",net::can::can_info{} },{  "PCAN-ISA-6",net::can::can_info{} },{ "PCAN-ISA-7",net::can::can_info{} },{"PCAN-ISA-8",net::can::can_info{} },
	{ "PCAN-DNG-1",net::can::can_info{} },
	{ "PCAN_PCI-1",net::can::can_info{} }  ,{ "PCAN_PCI-2",net::can::can_info{} } ,{ "PCAN_PCI-3" ,net::can::can_info{} } ,{ "PCAN_PCI-4" ,net::can::can_info{} },
	{ "PCAN_PCI-5",net::can::can_info{} } ,{ "PCAN_PCI-6",net::can::can_info{} } ,{ "PCAN_PCI-7",net::can::can_info{} } ,{ "PCAN_PCI-8",net::can::can_info{} },
	{ "PCAN_PCI-9",net::can::can_info{} } ,{ "PCAN_PCI-10",net::can::can_info{} } ,{ "PCAN_PCI-11",net::can::can_info{} } ,{ "PCAN_PCI-12" ,net::can::can_info{} },
	{ "PCAN_PCI-13" ,net::can::can_info{} } ,{ "PCAN_PCI-14",net::can::can_info{} } ,{ "PCAN_PCI-15" ,net::can::can_info{} } ,{ "PCAN_PCI-16",net::can::can_info{} },
	{ "PCAN-USB-1",net::can::can_info{} },{ "PCAN-USB-2",net::can::can_info{} },{ "PCAN-USB-3",net::can::can_info{} },{ "PCAN-USB-4",net::can::can_info{} },
	{ "PCAN-USB-5",net::can::can_info{} },{ "PCAN-USB-6",net::can::can_info{} },{ "PCAN-USB-7",net::can::can_info{} },{ "PCAN-USB-8",net::can::can_info{} } ,
	{ "PCAN-USB-9" ,net::can::can_info{} },{ "PCAN-USB-10",net::can::can_info{} },{ "PCAN-USB-11" ,net::can::can_info{} },{ "PCAN-USB-12",net::can::can_info{} },
	{ "PCAN-USB-13",net::can::can_info{} },{ "PCAN-USB-14",net::can::can_info{} },{ "PCAN-USB-15",net::can::can_info{} },{ "PCAN-USB-16" ,net::can::can_info{} }
};
#endif

net::can::can_info net::can::get_local_endpoint_info(std::string endpoint) {
#ifdef PCAN_API
	for (auto e : pcan_api::all_channels_info) {
		if (e.first == endpoint) return e.second;
	}
#endif
	throw net::exceptions::err_can("Unknown local endpoint '"+endpoint+"'");
}

void set_local_endpoint_info(std::string endpoint, net::can::can_info info) {
#ifdef PCAN_API
#ifdef PCAN_API
	for (auto& e : pcan_api::all_channels_info) {
		if (e.first == endpoint) {
			e.second = info; return;
		}
	}
#endif
	throw net::exceptions::err_can("Unknown local endpoint '" + endpoint + "'");
#endif
}

ceps::cloud::Local_Interface net::can::get_local_endpoint(std::string s) {
 #ifdef PCAN_API
	for (auto e : pcan_api::all_channels) {
		if (e.second == s)
			return ceps::cloud::Local_Interface{e.second};
	}
#endif
	return ceps::cloud::Local_Interface{};
}

int net::can::get_local_endpoint_handle(std::string s) {
#ifdef PCAN_API
	for (auto e : pcan_api::all_channels) {
		if (e.second == s)
			return e.first;
	}
#endif
	return -1;
}

std::vector<std::string> ceps::cloud::check_available_ifs(){
	std::vector<std::string> r;
#ifdef	UNIX_API
    struct ifaddrs *addrs,*tmp;
    getifaddrs(&addrs);
    tmp = addrs;

    while (tmp)
    {
       if (tmp->ifa_name)
            r.push_back(tmp->ifa_name);
        tmp = tmp->ifa_next;
    }
    freeifaddrs(addrs);
#endif
#ifdef WIN_API
 #ifdef PCAN_API
	for (auto channel : pcan_api::all_channels) {
		unsigned int v{};
		auto rc = pcan_api::getvalue(channel.first, PCAN_CHANNEL_CONDITION, &v, sizeof(v));
		if (rc != PCAN_ERROR_OK) continue;
		if (v != PCAN_CHANNEL_AVAILABLE) continue;
		r.push_back(channel.second);
	}
 #endif	
#endif
return r;
}

int net::inet::establish_inet_stream_connect(std::string remote, std::string port) {
	INIT_SYS_ERR_HANDLING

	int cfd = -1;
	struct addrinfo hints;
	struct addrinfo *result, *rp;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	if ((syscall_result = getaddrinfo(remote.c_str(), port.c_str(), &hints, &result)) != 0) {
		throw net::exceptions::err_getaddrinfo(gai_strerror(syscall_result));
	}

	for (rp = result; rp != NULL; rp = rp->ai_next) {
		cfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (cfd == -1) continue;
		if (connect(cfd, rp->ai_addr, rp->ai_addrlen) != -1) break;

		if (rp->ai_next == NULL) {
			STORE_SYS_ERR
				close(cfd); cfd = -1;
			if (result != nullptr) freeaddrinfo(result);
			THROW_ERR_INET
		}
		syscall_result = errno;
		close(cfd); cfd = -1;
	}
	return cfd;
}



ceps::cloud::Simulation_Core ceps::cloud::cmdline_read_remote_host(int argc, char* argv[]) {
	ceps::cloud::Simulation_Core r;
	std::string s = argv[0];
	std::regex word_regex("([0-9a-zA-Z.-]+):([0-9]+)");
	std::smatch m;
	if (std::regex_match(s, m, word_regex)) {
		if (m.size() == 3) {
			r.first = m[1].str();
			r.second = m[2].str();
		}
	}
	return r;
}



std::pair<bool,std::string> ceps::cloud::get_virtual_can_attribute_content(std::string attr, std::vector<std::pair<std::string,std::string>> const & http_header){
 for(auto const & v : http_header){
     if (v.first == attr)
         return {true,v.second};
 }
 return {false,{}};
}


std::tuple<bool,std::string,std::vector<std::pair<std::string,std::string>>> ceps::cloud::read_virtual_can_msg(int sck,std::string& unconsumed_data){
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
         ++j;
         for(;j < i && buffer[j]==' ' ;++j);
         auto cont_start = j;
         auto cont_end = i - 1;
         for(;buffer[cont_end] == ' ';--cont_end);
         content = buffer.substr(cont_start, cont_end - cont_start + 1);
         header.push_back(std::make_pair(attribute,content));
        }
        line_start = i + 2;++i;
    }
  }
  return std::make_tuple(true,first_line,header);
 }

 return std::make_tuple(false,std::string{},header_t{});
}


std::tuple<bool, std::string, std::vector<std::pair<std::string, std::string>>> ceps::cloud::vcan_api::send_cmd(int sock, std::string command) {
	INIT_SYS_ERR_HANDLING
		std::stringstream cmd;
	cmd << "HTTP/1.1 100\r\n";
	cmd << "cmd: " + command + "\r\n\r\n";
	auto r = send(sock, cmd.str().c_str(), cmd.str().length(), 0);
	if (r != cmd.str().length()) {
		THROW_ERR_INET
	}
	std::string unconsumed_data;
	return ceps::cloud::read_virtual_can_msg(sock, unconsumed_data);
}

std::vector<std::pair<ceps::cloud::Remote_Interface, std::string>> ceps::cloud::vcan_api::fetch_out_channels(ceps::cloud::Simulation_Core sim_core) {
	INIT_SYS_ERR_HANDLING
		int cfd = -1;
	CLEANUP([&]() {if (cfd != -1) closesocket(cfd); })

	std::vector<std::pair<Remote_Interface, Remote_Interface_Type>> rv;
	cfd = net::inet::establish_inet_stream_connect(sim_core.first, sim_core.second);
	if (cfd == -1) { THROW_ERR_INET }
	{
		auto rhr = send_cmd(cfd, "get_out_channels");
		if (!std::get<0>(rhr))
			throw ceps::cloud::exceptions::err_vcan_api{ "No out channels." };

		auto const & attrs = std::get<2>(rhr);
		auto out_raw = ceps::cloud::get_virtual_can_attribute_content("out_channels", attrs);
		auto types_raw = ceps::cloud::get_virtual_can_attribute_content("types", attrs);
		if (!out_raw.first || !types_raw.first)
			throw ceps::cloud::exceptions::err_vcan_api{ "No out channels and/or invalid types" };

		using namespace std;

		vector<string> out_channels;
		{
			istringstream iss{ out_raw.second };
			copy(istream_iterator<string>(iss),
				istream_iterator<string>(),
				back_inserter(out_channels));
		}
		vector<string> out_channels_types;
		{
			istringstream iss{ types_raw.second };
			copy(istream_iterator<string>(iss),
				istream_iterator<string>(),
				back_inserter(out_channels_types));
		}

		for (size_t i = 0; i != out_channels.size(); ++i) {
			auto const & ch = out_channels[i];
			if (i < out_channels_types.size()) rv.push_back(make_pair(Remote_Interface{ ch }, Remote_Interface_Type{ out_channels_types[i] }));
			else rv.push_back(make_pair(Remote_Interface{ ch }, Remote_Interface_Type{ "?" }));
		}
		return rv;
	}
}



void gateway_fn(Simulation_Core sim_core,
                std::shared_ptr<gateway_thread_info> ctrl,
                int local_sck,
                int extended_can,
                std::string remote_interface){
    //std::cout <<local_sck<<":"<< extended_can << std::endl;
    int remote_sck = -1;
    auto destructor = [&](){
        if(remote_sck!=-1)close(remote_sck);close(local_sck);ctrl->down=true;
    };
    cleanup<decltype(destructor)> trigger_destructor {destructor};
    struct addrinfo hints;
    struct addrinfo *result, *rp;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int syscall_result=0;

    if ((syscall_result = getaddrinfo(sim_core.first.c_str(), sim_core.second.c_str(), &hints, &result)) != 0) {
      return;
    }
    for (rp = result; rp != NULL; rp = rp->ai_next) {
      remote_sck = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
      if (remote_sck == -1) continue;
      if (connect(remote_sck, rp->ai_addr, rp->ai_addrlen) != -1) break;
      syscall_result = errno;
      close(remote_sck);remote_sck=-1;
    }
    if (result != nullptr) freeaddrinfo(result);

    if (rp == nullptr) {
      return;
    }

    {
     //Subscribe
     std::stringstream cmd;
     cmd << "HTTP/1.1 100\r\n";
     cmd << "cmd: subscribe_out_channel\r\n";
     cmd << "out_channel: "<< remote_interface <<"\r\n\r\n";

     auto r = send(remote_sck,cmd.str().c_str(),cmd.str().length(),0);
     if (r != cmd.str().length()) {
         return;
     }
    }
    //std::cout <<local_sck<<":"<< extended_can << std::endl;
    /*for(;!ctrl->shutdown;){
      std::uint32_t l=0;
      auto r = recv(remote_sck,&l,sizeof(l),0);
      if (r != sizeof(l)) return;
      l = ntohl(l);
      struct can_frame can_message{0};
      if ( l != sizeof(can_frame) ) return;
      r = recv(remote_sck,&can_message,sizeof(can_frame),0);
      if (extended_can) can_message.can_id |= CAN_EFF_FLAG;
      r = send(local_sck, &can_message, sizeof(struct can_frame),0);
      if(r!=sizeof(struct can_frame)) return;
    }*/
}

