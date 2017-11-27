#include "cepscloud_streaming_common.h"
#include "cepscloud_streaming_endpoint_ws_api.h"
#include "vcanstreams.h"
#include "vcan_standard_ctrls.h"

#pragma comment(lib, "ws2_32.lib")

std::ofstream* of = nullptr;
bool run_as_service = false;


void log(std::string m) {
	//if (!of) of = new std::ofstream{ "d:\\temp\\log.txt" };
	//*of << m << std::endl;
}

void fatal(std::string msg) {
	log("[FATAL] " + msg);
	std::cerr << "***Fatal Error: " << msg << std::endl;
	exit(1);
}

void warn(std::string msg, bool terminate) {
	log("[WARNING] " + msg);
	std::cerr << "***Warning: " << msg << (terminate ? " Program exited.":"") << std::endl;
	if(terminate)exit(0);
}

static std::string usage = R"(
cangateway  [CEPS_CORE] [EXPRESSION...]

 SIMULATION_CORE
  host name and port running a cepS core.
  
  Example:
   foo.de:8186

 EXPRESSION
  There are two classes of expressions: mapping expressions and settings.

   - Mapping expressions. This class of expressions takes one of the following forms:
    
    LOCAL CAN ENDPOINT "->" REMOTE CAN 
    LOCAL CAN ENDPOINT "<-" REMOTE CAN
    REMOTE CAN "->" LOCAL CAN ENDPOINT
    REMOTE CAN "<-" LOCAL CAN ENDPOINT
     
    Example:
     can_out "->" PCAN-USB-1
     This example defines a downstream - can data flows from the remote site 
     to a locally installed PCAN USB device.
    Example:
     can_out "<-" PCAN-USB-1
     This example defines an upstream - can data flows from the 
     locally installed PCAN USB device to the remote site.

   - Settings:
    -h HOSTNAME:PORT

    -b BITRATE
    BITRATE can be one of the following:
       1M,800K,500K,250K,125K,100K,95K,
       83K,50K,47K,33K,20K,10K,5K
    Example:
     -b 100K can_out "->" PCAN-USB-1 -b 1M can_out "->" PCAN-USB-2 can_out "->" PCAN-USB-3
     This example defines three downstreams, the first of which with a local can channel with 
     a bitrate of 100K. The remaining two can channels share the same bitrate of 1M.

 )";



using handle_mapping_return_t = std::tuple<bool, bool, ceps::cloud::Stream_Mapping>;
handle_mapping_return_t handle_mapping(std::string a,std::string b,bool stream_to_b, net::can::can_info::BAUD_RATE br, std::vector<std::pair<ceps::cloud::Remote_Interface, std::string>> remote_out,
	                                                                                 std::vector<std::pair<ceps::cloud::Remote_Interface, std::string>> remote_in) {
	auto ll = false;
	auto rl = false;

	ll = net::can::get_local_endpoint(a) != ceps::cloud::Local_Interface{};
	rl = net::can::get_local_endpoint(b) != ceps::cloud::Local_Interface{};
	if (!rl && !ll) fatal("[USER ERROR] Mapping '"+a+"' "+(stream_to_b?"->":"<-") + " '"+b+"' contains no local endpoint.");
	auto down_stream = rl && stream_to_b || ll && !stream_to_b;
	auto remote_endpoints = (down_stream?&remote_out:&remote_in);
	std::pair<ceps::cloud::Remote_Interface, std::string> remote_endpoint;
	for (auto e : *remote_endpoints) if (e.first == (ll ? b : a)) {
		remote_endpoint = e; break;
	}
	if (remote_endpoint == std::pair<ceps::cloud::Remote_Interface, std::string>{}) fatal("[USER ERROR] Mapping '" + a + "' " + (stream_to_b ? "->" : "<-") + " '" + b + 
		"' contains no compatible remote endpoint. Rerun with option -ar or -a to print a list of available remote endpoints.");

	auto info = net::can::get_local_endpoint_info(ll ? a : b);
	info.br = br;
	net::can::set_local_endpoint_info((ll ? a : b),info);
	return std::make_tuple(true,down_stream,std::make_pair( (ll?a:b), (ll?b:a)));
}

static ceps::cloud::Simulation_Core get_core_by_name_or_short_name(std::string s) {
 for (auto e : ceps::cloud::global_directory) {
	 if (e.name == s || e.short_name == s) return e.sim_core;
 }
 return ceps::cloud::Simulation_Core{};
}

ceps::cloud::mappingsex_t ceps::cloud::parse_cmdline_and_extract_mappings(int argc,
	char* argv[],
	info_out_channels_t const & out_channels,
	info_in_channels_t const & in_channels)

{
	mappingsex_t rv;
	Simulation_Core sim_core;

	net::can::can_info::BAUD_RATE br = net::can::can_info::BAUD_1M;
	for (int j = 0; j != argc; ++j) {
		std::string token = argv[j];
		if (token == "->" || token == "<-") {
			if (j == 0 || j + 1 == argc) throw exceptions::err_vcan_api("[USER ERROR] Illformed mapping definition.");
			auto it1 = out_channels.find(sim_core);
			auto it2 = in_channels.find(sim_core);
			if (it1 == out_channels.end() || it2 == in_channels.end()) throw exceptions::err_vcan_api("[INTERNAL ERROR] out_channels/in_channels not properly initialized.");
			std::string next_token = argv[j + 1];
			if (next_token == "-h" || next_token == "-s" || next_token == "--sim") {
				Simulation_Core sim_core2;
				if (next_token != "-h") {
					std::string n = argv[j + 2];
					auto sim = get_core_by_name_or_short_name(n);
					if (sim == Simulation_Core{}) throw exceptions::err_vcan_api("[USER ERROR] Unknown simulation '" + n + "'.");
					sim_core2 = sim;
				} else sim_core2 = ceps::cloud::cmdline_read_remote_host(argv[j + 2]);
				if (sim_core == Simulation_Core{} || sim_core2 == Simulation_Core{})  throw exceptions::err_vcan_api("[USER ERROR] Illformed route definition (remote -> remote mapping).");
				std::pair<Simulation_Core, std::string> re1{sim_core,argv[j - 1] };
				std::pair<Simulation_Core, std::string> re2{sim_core2,argv[j + 3] };
				decltype(re1)* left;
				decltype(re2)* right;
				if (token == "->") { left = &re1; right = &re2; } else { left = &re2; right = &re1; }
				//std::cout << left->second << "@" << left->first.first << ":" << left->first.second << "\n";
				//std::cout << right->second << "@" << right->first.first << ":" << right->first.second << "\n";
				rv.routes.push_back(ceps::cloud::Route{*left,*right});
				j += 2;
			} else {
				auto r = handle_mapping(argv[j - 1], argv[j + 1], token == "->", br, it1->second, it2->second);
				if (!std::get<0>(r)) throw exceptions::err_vcan_api("[USER ERROR] Illformed mapping definition.");
				if (std::get<1>(r)) rv.remote_to_local_mappings.push_back(std::make_pair(std::get<2>(r), sim_core));
				else rv.local_to_remote_mappings.push_back(std::make_pair(std::get<2>(r), sim_core));
				++j;
			}
		}
		else if (token == "-h") {
			if (j + 1 == argc) throw exceptions::err_vcan_api("[USER ERROR] Trailing -h. Expected a host address.");
			sim_core = ceps::cloud::cmdline_read_remote_host(argv[++j]);
			if (sim_core == Simulation_Core{}) throw exceptions::err_vcan_api("[USER ERROR] Erroneous host name:'" + std::string{ argv[j] }+"'.");

		} else if (token == "-b") {
			if (j + 1 == argc) throw exceptions::err_vcan_api("[USER ERROR] Trailing -b. Expected a baudrate definition.");
			auto it = str_br2id.find(argv[j + 1]);
			if (it == str_br2id.end()) throw exceptions::err_vcan_api("[USER ERROR] Unknown parameter value for baudrate: '" + std::string{ argv[j + 1] } +"'.");
			br = it->second;
			++j;
		}
		else if (token == "-s" || token == "--sim") {
			if (j + 1 == argc) throw exceptions::err_vcan_api("[USER ERROR] Trailing -s. Expected a simulation name.");
			std::string n = argv[j + 1];
			auto sim = get_core_by_name_or_short_name(n);
			if (sim == Simulation_Core{}) throw exceptions::err_vcan_api("[USER ERROR] Unknown simulation '" + n +"'.");
			sim_core = sim;
			++j;
		}
	}
	return rv;
}

void r2r_ctrl(
	ceps::cloud::Route um);



ceps::cloud::Simulation_Core directory_server;
void WINAPI service_main(DWORD argc, LPTSTR []);




int main(int argc, char* argv[])
{
	setup_shared_libs();

	auto display_name = [](std::pair<ceps::cloud::Simulation_Core, std::string> p) -> std::string {
		return p.second + "@" + p.first.first + ":" + p.first.second;
	};
	bool wsa_startup_successful = false;
	bool run_as_server = true;
	 
	CLEANUP([&](){if (wsa_startup_successful) WSACleanup(); })

	auto v = ceps::cloud::check_available_ifs();
	ceps::cloud::sys_info_available_ifs = ceps::misc::sort_and_remove_duplicates(v);
	setup_stream_ctrls(ceps::cloud::sys_info_available_ifs);

	//handle information parameters which requires only local data
	if (argc == 1 && !run_as_service) {
		std::cout << usage << std::endl;
		exit(1);
	}

	if (argc > 1) for (int i = 1; i != argc; ++i) {
		std::string token = argv[i];
		if (token == "--run_as_console_app") {
			run_as_server = false; run_as_service = false;
		}
	}

	if (argc > 1) for (int i = 1; i != argc; ++i) {
		std::string token = argv[i];
		if (token == "--run_as_server") run_as_server = true;
	}

	if (v.size() == 0) warn("No CAN Devices found.",false);

	{
		WORD wVersionRequested;
		WSADATA wsaData;
		int err;
		wVersionRequested = MAKEWORD(2, 2);
		err = WSAStartup(wVersionRequested, &wsaData);
		if (err != 0)
			fatal("WSAStartup failed with error: " + std::to_string(err));
	}

	if (argc > 1) for (int i = 1; i != argc; ++i) {
		std::string token = argv[i];
        if (token == "--print_available_local_endpoints" || token == "-a" || token == "-al") {
			std::cout << "Available local communication endpoints:\n";
			for (auto e : ceps::cloud::sys_info_available_ifs) std::cout << "\t" << e << "\n";
		}
		else if (token == "-h") {
			if (i + 1 == argc) fatal("[USER ERROR] Trailing -h. Expected a host address.");
			auto sim_core = ceps::cloud::cmdline_read_remote_host(argv[++i]);
			if (sim_core == ceps::cloud::Simulation_Core{}) fatal("[USER ERROR] Erroneous host name:'" + std::string{ argv[i] }+"'.");
			ceps::cloud::sim_cores.insert(sim_core);
		}
		else if (token == "-directory" || token == "--directory" || token == "-d") {
			directory_server = ceps::cloud::cmdline_read_remote_host(argv[++i]);
			if (directory_server == ceps::cloud::Simulation_Core{}) fatal("[USER ERROR] Erroneous host name:'" + std::string{ argv[i] }+"'.");
		}
		else if (token == "--print_known_streaming_endpoints") {
			auto l = ceps::cloud::fetch_streaming_endpoints(directory_server);
			if (l.size() == 0)
				std::cout << "No known streaming endpoints.\n";
			else {
				std::cout << "Streaming endpoints:\n";
				for (auto & e : l) {
					std::cout << "\t" << e.first << ":" << e.second << "\n";
				}
			}
		}
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
				if (!run_as_server)
					fatal(std::string{ "[INET ERROR] " }+"(" + directory_server.first + ":" + directory_server.second + ") " + e.what());
				else std::this_thread::sleep_for(std::chrono::seconds(1));
			}
			catch (ceps::cloud::exceptions::err_vcan_api const & e) {
				if (!run_as_server)
					fatal(std::string{ "[VCAN_API ERROR] " }+e.what());
				else std::this_thread::sleep_for(std::chrono::seconds(1));
			}
		}
	}

	if (argc > 1) for (int i = 1; i != argc; ++i) {
		std::string token = argv[i];
		if (token == "--print_known_hosts") {
			std::cout << "Known hosts:\n";
			for (auto s : ceps::cloud::sim_cores) {
				std::cout <<"\t" << s.first << ":" << s.second << " aka " << ceps::cloud::display_name_with_details(s) << std::endl;
			}
		}
		else if (token == "--print_config_data") {
			std::cout << "Configuration Data fetched from directoy service:\n";
			std::cout << "\n" << initial_config << "\n\n";
		}
	}

	log("[DEBUG][prepare to fetch channel information]");
	std::vector<std::pair<ceps::cloud::Simulation_Core, std::future<ceps::cloud::vcan_api::fetch_channels_return_t>>> handles;
	for(auto s : ceps::cloud::sim_cores)
		handles.push_back(std::make_pair(s,std::async(std::launch::async, ceps::cloud::vcan_api::fetch_channels, s)));

	try {
		for (auto & handle : handles) {
			auto remote_channels = handle.second.get();
			ceps::cloud::info_out_channels[handle.first] = remote_channels.first;
			ceps::cloud::info_in_channels[handle.first] = remote_channels.second;
		}
		log("[DEBUG][done fetching channel information]");
		//handle information parameters which requires remote data
		if (argc > 1) for (int i = 2; i != argc; ++i) {
			std::string token = argv[i];
			if (token == "--print_available_remote_endpoints" || token == "-a" || token == "-ar") {
				if (ceps::cloud::sim_cores.size()) std::cout << "Available remote endpoints (name,direction,type):\n";
				else  std::cout << "No remote communication endpoints available.\n";
				for (auto s : ceps::cloud::sim_cores) {
					std::cout <<" "<< ceps::cloud::display_name_with_details(s) << ":\n";
					for (auto e : ceps::cloud::info_out_channels[s]) std::cout << "\t" << e.first << ", out , " << e.second << "\n";
					for (auto e : ceps::cloud::info_in_channels[s]) std::cout << "\t" << e.first << ", in , " << e.second << "\n";
				}
			}
		}

		log("[DEBUG][before ceps::cloud::parse_cmdline_and_extract_mappings()]");
		auto mappings = ceps::cloud::parse_cmdline_and_extract_mappings(argc,argv, ceps::cloud::info_out_channels, ceps::cloud::info_in_channels);
		log("[DEBUG][after ceps::cloud::parse_cmdline_and_extract_mappings()]");

		//handle information parameters which requires parsed mapping data
		if (argc > 1) for (int i = 2; i != argc; ++i) {
			std::string token = argv[i];
			if (token == "--print_mappings" || token == "-m" ) {
				if (mappings.remote_to_local_mappings.size()) {
					std::cout << "Downstream mappings (remote endpoint -> local endpoint):\n";
					for (auto e : mappings.remote_to_local_mappings) std::cout << "\t" << ceps::cloud::down_stream(e).second 
						<< "@" << ceps::cloud::display_name(ceps::cloud::sim_core(e))
						<< " -> " << ceps::cloud::down_stream(e).first << "\n";
				}
				if (mappings.local_to_remote_mappings.size()) {
					std::cout << "Upstream mappings (local endpoint -> remote endpoint):\n";
					for (auto e : mappings.local_to_remote_mappings) std::cout << "\t" << ceps::cloud::up_stream(e).first << " -> " << ceps::cloud::up_stream(e).second 
						<< "@" << ceps::cloud::display_name(ceps::cloud::sim_core(e)) << "\n";
				}
				if (mappings.routes.size()) {
					std::cout << "Routes (remote endpoint -> remote endpoint):\n";
					for (auto e : mappings.routes) std::cout << "\t" << display_name(e.from) << " -> " << display_name(e.to) << "\n";
				}

				if (mappings.empty()) std::cout << "No mappings defined at all.\n";
			}
		}
		if (mappings.empty() && !run_as_server) warn(std::string{ "No mappings defined." },true);
		//Ready to run
		log("[DEBUG][READY TO START WS API]");
		if (run_as_server) {
			if (run_as_service) {
				SERVICE_TABLE_ENTRY dispatch_table[] = {
					{"Streaming Service Client by cepS Technologies (www.ceps.technology).",service_main},
					{NULL,NULL}
				};
				StartServiceCtrlDispatcher(dispatch_table);
			}
			else {
				Websocket_interface ws_api{ directory_server.first,directory_server.second };
				auto t = ws_api.start();
				if (t != nullptr) t->join();
				else return 1;
			}
			return 0;
		}

		std::vector<std::thread*> downstream_threads;
		std::vector<std::thread*> upstream_threads;
		std::vector<std::thread*> route_threads;
		for (auto e : mappings.remote_to_local_mappings) {
			auto dwn_ctrl = global_ctrlregistry.get_down_stream_ctrl(ceps::cloud::get_down_stream_type(e), e.first.first);
			if (dwn_ctrl == nullptr) fatal(std::string{ "[INTERNAL ERROR] No downstreamcontrol registered for " }+e.first.first+"/"+ceps::cloud::get_down_stream_type(e));
			downstream_threads.push_back(new std::thread{ dwn_ctrl,ceps::cloud::sim_core(e),ceps::cloud::down_stream(e) });
		}
		
		for (auto e : mappings.local_to_remote_mappings) {
			auto up_ctrl = global_ctrlregistry.get_up_stream_ctrl(ceps::cloud::get_up_stream_type(e), e.first.first);
			if (up_ctrl == nullptr) fatal(std::string{ "[INTERNAL ERROR] No upstreamcontrol registered for " }+e.first.first + "/" + ceps::cloud::get_up_stream_type(e));
			upstream_threads.push_back(new std::thread{ up_ctrl,ceps::cloud::sim_core(e),ceps::cloud::up_stream(e) });
		}

		for (auto e : mappings.routes)
			route_threads.push_back(new std::thread{ r2r_ctrl, e });
		for (auto t : downstream_threads) t->join();
		for (auto t : upstream_threads) t->join();
		for (auto t : route_threads) t->join();
	}
	catch (net::exceptions::err_inet const & e) {
		fatal(std::string{ "[NETWORK ERROR] " }+e.what());
		log(std::string{ "[NETWORK ERROR] " }+e.what());
	}
	catch (ceps::cloud::exceptions::err_vcan_api const & e) {
		fatal(std::string{ "[VCAN_API ERROR] " }+e.what());
		log(std::string{ "[VCAN_API ERROR] " }+e.what());
	}
	catch (net::exceptions::err_can const & e) {
		fatal(std::string{ "[CAN_API ERROR] " }+e.what());
		log(std::string{ "[CAN_API ERROR] " }+e.what());
	}
    return 0;
}


SERVICE_STATUS global_service_status;
SERVICE_STATUS_HANDLE global_handle_service_set_status;
auto global_service_update_time = 1000;

void WINAPI server_ctrl_handler(DWORD dwControl);

void update_status(int new_status, int check) {
	log("[update_status()][new_status="+std::to_string(new_status)+"][check=" + std::to_string(check) + "]");
	if (check < 0) ++global_service_status.dwCheckPoint;
	else global_service_status.dwCheckPoint = check;
	if (new_status >= 0) global_service_status.dwCurrentState = new_status;
	if (!SetServiceStatus(global_handle_service_set_status, &global_service_status)) {
		global_service_status.dwCurrentState = SERVICE_STOPPED;
		global_service_status.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
		global_service_status.dwServiceSpecificExitCode = 2;
		SetServiceStatus(global_handle_service_set_status, &global_service_status);
	}
}

void WINAPI service_main(DWORD argc, LPTSTR argv[]) {
	log("[service_main()]");
	global_service_status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	global_service_status.dwCurrentState = SERVICE_START_PENDING;
	global_service_status.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN | SERVICE_ACCEPT_PAUSE_CONTINUE;
	global_service_status.dwWin32ExitCode = NO_ERROR;
	global_service_status.dwServiceSpecificExitCode = 0;
	global_service_status.dwCheckPoint = 0;
	global_service_status.dwWaitHint = 2 * global_service_update_time;
	//"Streaming Service Client by cepS Technologies (www.ceps.technology)."
	global_handle_service_set_status = RegisterServiceCtrlHandler("simple_service", server_ctrl_handler);
	
	if (global_handle_service_set_status == 0) {
		log("[global_handle_service_set_status == 0]");
		warn("RegisterServiceCtrlHandler() failed.",false);
		global_service_status.dwCurrentState = SERVICE_STOPPED;
		global_service_status.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
		global_service_status.dwServiceSpecificExitCode = 1;
		update_status(SERVICE_STOPPED,-1);
		return;
	}
	SetServiceStatus(global_handle_service_set_status,&global_service_status);
	log("[STARTING WS_API]");
	Websocket_interface ws_api{ directory_server.first,directory_server.second };
	auto t = ws_api.start();
	update_status(SERVICE_RUNNING, 0);
	log("[START WS_API]");
	if (t != nullptr) t->join();
	
	update_status(SERVICE_STOPPED, 0);
}

void WINAPI server_ctrl_handler(DWORD dwControl) {
	log("[server_ctrl_handler()][dwControl="+std::to_string(dwControl)+"]");
	switch (dwControl) {
	case SERVICE_CONTROL_SHUTDOWN:
	case SERVICE_CONTROL_STOP:
		//update_status(SERVICE_STOP_PENDING, -1);
		update_status(SERVICE_STOPPED, 0);
		exit(0);
		break;
	case SERVICE_CONTROL_PAUSE:
		break;
	case SERVICE_CONTROL_CONTINUE:
		break;
	case SERVICE_CONTROL_INTERROGATE:
		break;
	default: break;
	}
	update_status(-1, -1);
}
