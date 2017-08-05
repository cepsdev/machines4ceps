#include "mainwindow.h"
#include "common.h"
#include <QApplication>


std::vector<std::string> sys_info_available_ifs;
std::map<Simulation_Core,std::vector< Downstream_Mapping > > mappings_downstream;
std::map<Simulation_Core,std::vector< Upstream_Mapping > > mappings_upstream;
std::map<Simulation_Core, std::shared_ptr<ctrl_thread_info>   > ctrl_threads;
Simulation_Core current_core;

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


void ctrl_thread_fn(std::shared_ptr<ctrl_thread_info> ctrl){
    int cfd;
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    bool conn_established = false;

    for(;!ctrl->shutdown;)
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

}


int main(int argc, char *argv[])
{
    auto v = check_available_ifs();
    sys_info_available_ifs = sort_and_remove_duplicates(v);

    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
}
