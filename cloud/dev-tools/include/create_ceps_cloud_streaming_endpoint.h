#ifndef CREATE_CEPS_CLOUD_STREAMING_ENDPOINT_H
#define CREATE_CEPS_CLOUD_STREAMING_ENDPOINT_H
#include <QDebug>
#include "cepscloud_streaming_endpoint_ws_api.h"
#include "vcan_standard_ctrls.h"


void log(std::string m) {
    qDebug() << m.c_str();
}

void fatal(std::string msg) {
    log("[FATAL] " + msg);

    std::cerr << "***Fatal Error: " << msg << std::endl;
    exit(1);
}

void warn(std::string msg, bool terminate) {
    log("***Warning " + msg);
    if(terminate)exit(0);
}

extern ceps::cloud::Simulation_Core directory_server;
Websocket_interface* ws_api{};

void setup_and_run_cepscloud_streaming_endpoint(std::string simbox_host,std::string simbox_host_port,int argc, char *argv[]){
 directory_server = ceps::cloud::Simulation_Core{simbox_host,simbox_host_port};
 setup_shared_libs();
 auto v = ceps::cloud::check_available_ifs();
 ceps::cloud::sys_info_available_ifs = ceps::misc::sort_and_remove_duplicates(v);
 setup_stream_ctrls(ceps::cloud::sys_info_available_ifs);

 {
         WORD wVersionRequested;
         WSADATA wsaData;
         int err;
         wVersionRequested = MAKEWORD(2, 2);
         err = WSAStartup(wVersionRequested, &wsaData);
         if (err != 0)
             fatal("WSAStartup failed with error: " + std::to_string(err));
 }

 if (directory_server == ceps::cloud::Simulation_Core{}) return;
 ceps::cloud::check_and_query_sim_cores(true);

 ws_api =  new Websocket_interface{ directory_server.first,directory_server.second };
 auto t = ws_api->start();
}

#endif // CREATE_CEPS_CLOUD_STREAMING_ENDPOINT_H
