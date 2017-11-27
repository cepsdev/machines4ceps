#include "mainwindow.h"
#include "cepscloud_streaming_endpoint_ws_api.h"
#include "vcan_standard_ctrls.h"
#include <QApplication>
#include <QDebug>

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

ceps::cloud::Simulation_Core directory_server;
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


 if (directory_server != ceps::cloud::Simulation_Core{}) {
         bool fetching_entries_successful = false;
         for (; !fetching_entries_successful;) {
             try {
                 auto dir = ceps::cloud::fetch_directory_entries(directory_server);
                 for (auto e : dir.entries) {
                     ceps::cloud::sim_cores.insert(e.sim_core);
                     ceps::cloud::global_directory.push_back(e);
                 }
                 fetching_entries_successful = true;
             }
             catch (net::exceptions::err_inet & e) {
                 warn(std::string{ "[INET ERROR] " }+
                      "(" + directory_server.first + ":" + directory_server.second + ") " + e.what(),false);
                 std::this_thread::sleep_for(std::chrono::seconds(1));
             }
             catch (ceps::cloud::exceptions::err_vcan_api const & e) {
                 warn(std::string{ "[VCAN_API ERROR] " }+e.what(),false);
                 std::this_thread::sleep_for(std::chrono::seconds(1));
             }
         }
     }

 std::vector<std::pair<ceps::cloud::Simulation_Core, std::future<ceps::cloud::vcan_api::fetch_channels_return_t>>> handles;
 for(auto s : ceps::cloud::sim_cores)
  handles.push_back(std::make_pair(s,std::async(std::launch::async, ceps::cloud::vcan_api::fetch_channels, s)));

 for (auto & handle : handles) {
             auto remote_channels = handle.second.get();
             ceps::cloud::info_out_channels[handle.first] = remote_channels.first;
             ceps::cloud::info_in_channels[handle.first] = remote_channels.second;
 }

 ws_api =  new Websocket_interface{ directory_server.first,directory_server.second };
 auto t = ws_api->start();
}

int main(int argc, char *argv[])
{
    setup_and_run_cepscloud_streaming_endpoint(SIMBOX_HOST,SIMBOX_HOST_PORT,argc,argv);

    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
}
