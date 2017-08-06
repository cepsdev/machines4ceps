#include "mainwindow.h"
#include "common.h"
#include <QApplication>
#include <QEvent>
#include <sstream>

std::vector<std::string> sys_info_available_ifs;
std::map<Simulation_Core,std::vector< Downstream_Mapping > > mappings_downstream;
std::map<Simulation_Core,std::vector< Upstream_Mapping > > mappings_upstream;
std::map<Simulation_Core, std::shared_ptr<ctrl_thread_info>   > ctrl_threads;
std::map<Simulation_Core,std::vector< std::pair<Remote_Interface,Remote_Interface_Type> > > info_out_channels;
std::map<Simulation_Core,std::vector< std::pair<Remote_Interface,Remote_Interface_Type> > > info_in_channels;
downstream_threads_t downstream_threads;
Simulation_Core current_core;
std::mutex global_mutex;



std::vector<std::string> check_available_ifs(){
    struct ifaddrs *addrs,*tmp;
    std::vector<std::string> r;
    getifaddrs(&addrs);
    tmp = addrs;

    while (tmp)
    {
       if (tmp->ifa_name)
            r.push_back(tmp->ifa_name);
        tmp = tmp->ifa_next;
    }
    freeifaddrs(addrs);
    return r;
}

template<typename F> struct cleanup{
    F f_;
    cleanup(F f):f_(f){}
    ~cleanup(){f_();}
};

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

void ctrl_thread_fn(QCoreApplication* core_app,
                    QMainWindow* main_window,
                    Simulation_Core sim_core,
                    std::shared_ptr<ctrl_thread_info> ctrl){
    int cfd = -1;
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    auto destructor = [&cfd](){if(cfd!=-1)close(cfd);};
    cleanup<decltype(destructor)> trigger_destructor {destructor};

    //Establish Connection

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int syscall_result=0;

    if ((syscall_result = getaddrinfo(sim_core.first.c_str(), sim_core.second.c_str(), &hints, &result)) != 0) {
      auto err = new QCtrlThreadConnectionFailed();
      err->reason() = gai_strerror(syscall_result);
      QCoreApplication::postEvent(main_window, err);
      return;
    }
    for (rp = result; rp != NULL; rp = rp->ai_next) {
      cfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
      if (cfd == -1) continue;
      if (connect(cfd, rp->ai_addr, rp->ai_addrlen) != -1) break;
      syscall_result = errno;
      close(cfd);cfd=-1;
    }
    if (result != nullptr) freeaddrinfo(result);

    if (rp == nullptr) {
      auto err = new QCtrlThreadConnectionFailed();
      err->reason() = strerror(syscall_result);
      QCoreApplication::postEvent(main_window, err);
      return;
    }


    {
     //Fetch outgoing channels
     std::stringstream cmd;
     cmd << "HTTP/1.1 100\r\n";
     cmd << "cmd: get_out_channels\r\n\r\n";
     auto r = send(cfd,cmd.str().c_str(),cmd.str().length(),0);
     if (r != cmd.str().length()) {
         auto err = new QCtrlThreadConnectionFailed();err->reason() = strerror(syscall_result);core_app->postEvent(main_window, err);
         return;
     }

     std::string unconsumed_data;
     auto rhr = read_virtual_can_request(cfd,unconsumed_data);
     if (!std::get<0>(rhr)) {
         auto err = new QCtrlThreadConnectionFailed();err->reason() = "Failed to request out channels";core_app->postEvent(main_window, err);
         return;
     }
     auto const & attrs = std::get<2>(rhr);
     auto out_raw = get_virtual_can_attribute_content("out_channels",attrs);
     auto types_raw = get_virtual_can_attribute_content("types",attrs);
     if (!out_raw.first || !types_raw.first) {
         auto err = new QCtrlThreadConnectionFailed();err->reason() = "No out channels and/or invalid types";core_app->postEvent(main_window, err);
         return;
     }
     using namespace std;

     vector<string> out_channels;
     {
      istringstream iss{out_raw.second};
      copy(istream_iterator<string>(iss),
          istream_iterator<string>(),
          back_inserter(out_channels));
     }
     vector<string> out_channels_types;
     {
      istringstream iss{types_raw.second};
      copy(istream_iterator<string>(iss),
          istream_iterator<string>(),
          back_inserter(out_channels_types));
     }

     std::vector<std::pair<Remote_Interface,Remote_Interface_Type>> rv;
     for(size_t i = 0; i != out_channels.size();++i){
         auto const & ch = out_channels[i];
         if (i < out_channels_types.size()) rv.push_back(make_pair(Remote_Interface{ch},Remote_Interface_Type{out_channels_types[i]}));
         else rv.push_back(make_pair(Remote_Interface{ch},Remote_Interface_Type{"?"}));
     }

     {
      lock_guard<std::mutex> lg(global_mutex);
      info_out_channels[sim_core] = rv;
     }

    }


    auto status = new QCtrlThreadConnectionEstablished();
    core_app->postEvent(main_window, status);
}

void gateway_fn(QMainWindow* main_window,
                Simulation_Core sim_core,
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
    for(;!ctrl->shutdown;){
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
    }
}



int main(int argc, char *argv[])
{
    auto v = check_available_ifs();
    sys_info_available_ifs = sort_and_remove_duplicates(v);

    QApplication a(argc, argv);
    MainWindow w(&a);
    w.show();

    return a.exec();
}
