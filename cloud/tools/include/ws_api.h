#pragma once

#include <string> 
#include <vector>
#include <map>
#include <algorithm>
#include <set> 
class State_machine_simulation_core;

class Websocket_interface {
	void dispatcher();
	void handler(int sck);
	std::string port_;
	std::string directory_server_name_;
	std::string directory_server_port_;
	std::string current_configuration;
	std::thread* dispatcher_thread_ = nullptr;
	std::mutex handler_threads_status_mutex_;
	using thread_status_type = std::tuple<std::thread*, bool, int>;
	std::vector< thread_status_type > handler_threads_status_;
	void handle_config_cmd(std::string const &);
public:
	Websocket_interface(std::string directory_server_name, std::string directory_server_port, std::string port = {}) :directory_server_name_{ directory_server_name}, 
		directory_server_port_{ directory_server_port },  port_ { port } {}
	std::thread* start();
};
