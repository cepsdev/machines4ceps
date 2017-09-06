#include "common.h"
#pragma comment(lib, "ws2_32.lib")


std::map<std::string, net::can::can_info::BAUD_RATE>str_br2id{
	{ "1M",net::can::can_info::BAUD_1M},
	{ "800K",net::can::can_info::BAUD_800K},
	{ "500K",net::can::can_info::BAUD_500K},
	{ "250K",net::can::can_info::BAUD_250K},
	{ "125K",net::can::can_info::BAUD_125K},
	{ "100K",net::can::can_info::BAUD_100K},
	{ "95K",net::can::can_info::BAUD_95K},
	{ "83K",net::can::can_info::BAUD_83K},
	{ "50K",net::can::can_info::BAUD_50K},
	{ "47K",net::can::can_info::BAUD_47K},
	{ "33K",net::can::can_info::BAUD_33K},
	{ "20K",net::can::can_info::BAUD_20K},
	{ "10K",net::can::can_info::BAUD_10K},
	{ "5K",net::can::can_info::BAUD_5K},
	{ "1000K",net::can::can_info::BAUD_1M },
	{ "1000000",net::can::can_info::BAUD_1M },
	{ "800000",net::can::can_info::BAUD_800K },
	{ "500000",net::can::can_info::BAUD_500K },
	{ "250000",net::can::can_info::BAUD_250K },
	{ "125000",net::can::can_info::BAUD_125K },
	{ "100000",net::can::can_info::BAUD_100K },
	{ "95000",net::can::can_info::BAUD_95K },
	{ "83000",net::can::can_info::BAUD_83K },
	{ "50000",net::can::can_info::BAUD_50K },
	{ "47000",net::can::can_info::BAUD_47K },
	{ "33000",net::can::can_info::BAUD_33K },
	{ "20000",net::can::can_info::BAUD_20K },
	{ "10000",net::can::can_info::BAUD_10K },
	{ "5000",net::can::can_info::BAUD_5K }
};


void fatal(std::string msg) {
	std::cerr << "***Fatal Error: " << msg << std::endl;
	exit(1);
}

void warn(std::string msg, bool terminate) {
	std::cerr << "***Warning: " << msg << (terminate ? " Program exited.":"") << std::endl;
	if(terminate)exit(0);
}


static HINSTANCE pcan_dll = nullptr;
namespace pcan_api {
	CAN_GetValue_t getvalue{};
	CAN_Initialize_t initialize{};
	CAN_InitializeFD_t initialize_fd{};
	CAN_Uninitialize_t uninitialize{};
	CAN_Reset_t reset{};
	CAN_Read_t read{};
	CAN_ReadFD_t read_fd{};
	CAN_Write_t write{};
	CAN_WriteFD_t write_fd{};
	CAN_FilterMessages_t filtermessages{};
	CAN_SetValue_t setvalue{};
	CAN_GetErrorText_t geterrortext{};

}


template <typename T, typename E> void getprocaddr(HINSTANCE hdll, E e,const char * sz, T& f ) {
	f = (T) GetProcAddress(hdll, sz);
	if (f == T{}) e(sz);
}

static void init_pcan_dll() {
	pcan_dll = LoadLibrary("PCANBasic");
	if (pcan_dll == nullptr) warn("No PEAK driver found: LoadLibrary('PCANBAsic') failed.",false);
	if (pcan_dll == nullptr) return;
	auto e = [](const char* sz) {fatal("Incompatible PEAK driver: GetProcAddress(\"" + std::string{sz}+"\") failed."); };
	getprocaddr(pcan_dll, e,"CAN_GetValue", pcan_api::getvalue);
	getprocaddr(pcan_dll, e, "CAN_Initialize", pcan_api::initialize);
	getprocaddr(pcan_dll, e, "CAN_InitializeFD", pcan_api::initialize_fd);
	getprocaddr(pcan_dll, e, "CAN_Uninitialize", pcan_api::uninitialize);
	getprocaddr(pcan_dll, e, "CAN_Reset", pcan_api::reset);
	getprocaddr(pcan_dll, e, "CAN_Read", pcan_api::read);
	getprocaddr(pcan_dll, e, "CAN_ReadFD", pcan_api::read_fd);
	getprocaddr(pcan_dll, e, "CAN_Write", pcan_api::write);
	getprocaddr(pcan_dll, e, "CAN_WriteFD", pcan_api::write_fd);
	getprocaddr(pcan_dll, e, "CAN_FilterMessages", pcan_api::filtermessages);
	getprocaddr(pcan_dll, e, "CAN_SetValue", pcan_api::setvalue);
	getprocaddr(pcan_dll, e, "CAN_GetErrorText", pcan_api::geterrortext);
}

#ifdef KMW_MULTIBUS_API
static HINSTANCE kmw_multibus_dll = nullptr;
static void init_kmw_multibus_dll() {
	kmw_multibus_dll = LoadLibrary("Multibus");
	if (kmw_multibus_dll == nullptr) warn("No KMW Multibus library found: LoadLibrary('Multibus') failed.",false);
	if (kmw_multibus_dll == nullptr) return;
	auto e = [](const char* sz) {fatal("Incompatible KMW Multibus library: GetProcAddress(\"" + std::string{ sz }+"\") failed."); };
	getprocaddr(kmw_multibus_dll, e, "CanStart", kmw_api::canstart);
	getprocaddr(kmw_multibus_dll, e, "CanOpen", kmw_api::canopen);
	getprocaddr(kmw_multibus_dll, e, "CanWrite", kmw_api::canwrite);
	getprocaddr(kmw_multibus_dll, e, "CanRead", kmw_api::canread);
	getprocaddr(kmw_multibus_dll, e, "CanInstall", kmw_api::caninstall);
}
#endif

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

std::pair<bool, ceps::cloud::Sim_Directory::entry> ceps::cloud::get_direntry(Simulation_Core s) {
 for (auto e : ceps::cloud::global_directory) {
	 if (e.sim_core == s) return std::make_pair(true,e);
 }
 return std::make_pair(false, ceps::cloud::Sim_Directory::entry{});
}

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

std::map<int, std::string> pcan_errcode2text = {
	{ PCAN_ERROR_XMTFULL, "Transmit buffer in CAN controller is full" },
	{ PCAN_ERROR_OVERRUN, "CAN controller was read too late" },
	{ PCAN_ERROR_BUSLIGHT, "Bus error: an error counter reached the 'light' limit" },
	{ PCAN_ERROR_BUSHEAVY, "Bus error: an error counter reached the 'heavy' limit" },
	{ PCAN_ERROR_BUSWARNING, "Bus error: an error counter reached the 'warning' limit" },
	{ PCAN_ERROR_BUSPASSIVE, "Bus error: the CAN controller is error passive" },
	{ PCAN_ERROR_BUSOFF, "Bus error: the CAN controller is in bus-off state" },
	{ PCAN_ERROR_QRCVEMPTY, "Receive queue is empty" },
	{ PCAN_ERROR_QOVERRUN, "Receive queue was read too late" },
	{ PCAN_ERROR_QXMTFULL, "Transmit queue is full" },
	{ PCAN_ERROR_REGTEST, " Test of the CAN controller hardware registers failed (no hardware found)" },
	{ PCAN_ERROR_NODRIVER, "Driver not loaded" },
	{ PCAN_ERROR_HWINUSE, "Hardware already in use by a Net" },
	{ PCAN_ERROR_NETINUSE, "A Client is already connected to the Net" },
	{ PCAN_ERROR_ILLHW, "Hardware handle is invalid" },
	{ PCAN_ERROR_ILLNET, "Net handle is invalid" },
	{ PCAN_ERROR_ILLCLIENT, "Client handle is invalid" },
	{ PCAN_ERROR_RESOURCE, "Resource (FIFO, Client, timeout) cannot be created" },
	{ PCAN_ERROR_ILLPARAMTYPE, "Invalid parameter" },
	{ PCAN_ERROR_ILLPARAMVAL, "Invalid parameter value" },
	{ PCAN_ERROR_UNKNOWN, "Unknown error" },
	{ PCAN_ERROR_ILLDATA, "Invalid data, function, or action" },
	{ PCAN_ERROR_CAUTION, "An operation was successfully carried out, however, irregularities were registered" },
	{ PCAN_ERROR_INITIALIZE, "Channel is not initialized [Value was changed from 0x40000 to 0x4000000]" },
	{ PCAN_ERROR_ILLOPERATION, "Invalid operation [Value was changed from 0x80000 to 0x8000000]" }
};

std::map<int, int>info_br2pcan_br{
	{net::can::can_info::BAUD_1M, PCAN_BAUD_1M },
	{ net::can::can_info::BAUD_800K, PCAN_BAUD_800K },
	{ net::can::can_info::BAUD_500K, PCAN_BAUD_500K },
	{ net::can::can_info::BAUD_250K, PCAN_BAUD_250K },
	{ net::can::can_info::BAUD_125K, PCAN_BAUD_125K},
	{ net::can::can_info::BAUD_100K, PCAN_BAUD_100K },
	{ net::can::can_info::BAUD_95K, PCAN_BAUD_95K },
	{ net::can::can_info::BAUD_83K, PCAN_BAUD_83K },
	{ net::can::can_info::BAUD_50K, PCAN_BAUD_50K },
	{ net::can::can_info::BAUD_47K, PCAN_BAUD_47K },
	{ net::can::can_info::BAUD_33K, PCAN_BAUD_33K },
	{ net::can::can_info::BAUD_20K, PCAN_BAUD_20K },
	{ net::can::can_info::BAUD_10K, PCAN_BAUD_10K },
	{ net::can::can_info::BAUD_5K, PCAN_BAUD_5K }
};

__declspec(align(8)) struct can_frame {
	std::uint32_t can_id;  /* 32 bit CAN_ID + EFF/RTR/ERR flags */
	std::uint8_t    can_dlc; /* frame payload length in byte (0 .. CAN_MAX_DLEN) */
	std::uint8_t    __pad;   /* padding */
	std::uint8_t    __res0;  /* reserved / padding */
	std::uint8_t    __res1;  /* reserved / padding */
	std::uint8_t    data[8];
};

void upstream_ctrl(
	ceps::cloud::Simulation_Core sim_core,
	ceps::cloud::Upstream_Mapping um) {
	INIT_SYS_ERR_HANDLING
	auto remote_sck = -1;
	constexpr auto CAN_RTR_FLAG = 0x40000000U;
	CLEANUP([&]() {
		if (remote_sck != -1)closesocket(remote_sck);
	});
	auto ext_can = false;
	for (auto e : ceps::cloud::info_in_channels[sim_core]) {
		if (e.first == um.second) {
			ext_can = e.second == "CANX";
			break;
		}
	}
	auto info = net::can::get_local_endpoint_info(um.first);
	auto baudrate = info_br2pcan_br[info.br];
	auto channel = net::can::get_local_endpoint_handle(um.first);
	try {
		if (channel == -1) throw net::exceptions::err_can{ "Couldn't acquire channel handle for '" + um.first + "'." };
		auto rinit = pcan_api::initialize(channel, baudrate, 0, 0, 0);
		if (rinit != PCAN_ERROR_OK && rinit != PCAN_ERROR_INITIALIZE) {
			throw net::exceptions::err_can{ "Initialization failed, endpoint is '" + um.first + "', errormessage: " + pcan_errcode2text[rinit] + "." };
		}

		remote_sck = net::inet::establish_inet_stream_connect(sim_core.first, sim_core.second);
		if (remote_sck == -1) { THROW_ERR_INET }
		{
			std::stringstream cmd;
			cmd << "HTTP/1.1 100\r\n";
			cmd << "cmd: subscribe_in_channel\r\n";
			cmd << "in_channel: " << um.second << "\r\n\r\n";

			auto r = send(remote_sck, cmd.str().c_str(), cmd.str().length(), 0);STORE_SYS_ERR;
			if (r != cmd.str().length()) {
				THROW_ERR_INET;
			}
			auto read_ev = CreateEvent(	NULL,FALSE,FALSE,NULL); STORE_SYS_ERR;
			if (read_ev == NULL) {
				fatal("[INTERNAL ERROR] CreateEvent failed.");
			}
			{
				auto r = pcan_api::setvalue(channel, PCAN_RECEIVE_EVENT, &read_ev, sizeof(read_ev));
				if (r != PCAN_ERROR_OK) {
					throw net::exceptions::err_can{ "Setting of event object failed, endpoint is '" + um.first + "', errormessage: " + pcan_errcode2text[r] + "." };
				}
			}

			for (;;) {
				auto wr = WaitForSingleObject(read_ev, INFINITE); STORE_SYS_ERR;
				if (wr == WAIT_FAILED) fatal("[INTERNAL ERROR] WaitForSingleObject failed.");
				DWORD rr = PCAN_ERROR_OK;
				do {
					TPCANMsg can_message{ 0 };

					auto rr = pcan_api::read(channel, &can_message, NULL);
					if (rr != PCAN_ERROR_OK && rr != PCAN_ERROR_BUSLIGHT && rr != PCAN_ERROR_BUSHEAVY) {
						if (can_message.ID == 0) break;
					}
					if (rr != PCAN_ERROR_QRCVEMPTY) {
						can_frame frame = { 0 };
						frame.can_id = can_message.ID;
						if (can_message.MSGTYPE & PCAN_MESSAGE_RTR) frame.can_id |= CAN_RTR_FLAG;
						frame.can_dlc = can_message.LEN;
						if (frame.can_dlc) memcpy(frame.data, can_message.DATA, frame.can_dlc);
						auto r = send(remote_sck, (char*)&frame, sizeof(frame), 0); STORE_SYS_ERR; if (r != sizeof(frame)) THROW_ERR_INET;
					}
				} while (rr != PCAN_ERROR_QRCVEMPTY);
			}
		}
	}
	catch (net::exceptions::err_inet const & e) {
		fatal(std::string{ "[" + um.second + "->" + um.first + "][NETWORK ERROR] " }+e.what());
	}
	catch (ceps::cloud::exceptions::err_vcan_api const & e) {
		fatal(std::string{ "[" + um.second + "->" + um.first + "][VCAN_API ERROR] " }+e.what());
	}
	catch (net::exceptions::err_can const & e) {
		fatal(std::string{ "[" + um.second + "->" + um.first + "][CAN ERROR] " }+e.what());
	}
}

#ifdef KMW_MULTIBUS_API
static std::vector<int> kmw_queue2sock;
static std::vector<bool> kmw_remote_inf_is_canx;
#endif

static void canbus_read_callback(unsigned char queue) {
	constexpr auto CAN_RTR_FLAG = 0x40000000U;
	CanEvent event = { 0 };
	kmw_api::canread(queue, &event);
	if (!event.messageValid) return;
	auto remote_sck = kmw_queue2sock[queue];
	can_frame frame = { 0 };
	frame.can_id = event.message.id;
	frame.can_dlc = event.message.length;
	if (event.message.rtr) frame.can_id |= CAN_RTR_FLAG;
	if (frame.can_dlc) memcpy(frame.data, event.message.data, frame.can_dlc);
	auto r = send(remote_sck, (char*)&frame, sizeof(frame), 0); 
	if (r != sizeof(frame)) fatal("send failed.");
}

static void canbus_flush_callback(unsigned char queue, bool flushed) {
}

void upstream_ctrl_kmw(
	ceps::cloud::Simulation_Core sim_core,
	ceps::cloud::Upstream_Mapping um) {
	INIT_SYS_ERR_HANDLING
		auto remote_sck = -1;
	constexpr auto CAN_RTR_FLAG = 0x40000000U;
	CLEANUP([&]() {
		if (remote_sck != -1)closesocket(remote_sck);
	});
	auto ext_can = false;
	for (auto e : ceps::cloud::info_in_channels[sim_core]) {
		if (e.first == um.second) {
			ext_can = e.second == "CANX";
			break;
		}
	}
	auto info = net::can::get_local_endpoint_info(um.first);
	auto baudrate = info_br2pcan_br[info.br];
	auto channel = net::can::get_local_endpoint_handle(um.first);
	try {
		if (channel == -1) throw net::exceptions::err_can{ "Couldn't acquire channel handle for '" + um.first + "'." };
		auto canbus_queue = kmw_api::canopen(channel);

		remote_sck = net::inet::establish_inet_stream_connect(sim_core.first, sim_core.second);
		if (remote_sck == -1) { THROW_ERR_INET }
		{
			std::stringstream cmd;
			cmd << "HTTP/1.1 100\r\n";
			cmd << "cmd: subscribe_in_channel\r\n";
			cmd << "in_channel: " << um.second << "\r\n\r\n";

			auto r = send(remote_sck, cmd.str().c_str(), cmd.str().length(), 0); STORE_SYS_ERR;
			if (r != cmd.str().length()) {
				THROW_ERR_INET;
			}
			kmw_queue2sock[canbus_queue] = remote_sck;
			kmw_remote_inf_is_canx[canbus_queue] = ext_can;
		    kmw_api::caninstall(canbus_queue, canbus_read_callback, canbus_flush_callback);
			for (; ;) {
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
			}
		}
	}
	catch (net::exceptions::err_inet const & e) {
		fatal(std::string{ "[" + um.second + "->" + um.first + "][NETWORK ERROR] " }+e.what());
	}
	catch (ceps::cloud::exceptions::err_vcan_api const & e) {
		fatal(std::string{ "[" + um.second + "->" + um.first + "][VCAN_API ERROR] " }+e.what());
	}
	catch (net::exceptions::err_can const & e) {
		fatal(std::string{ "[" + um.second + "->" + um.first + "][CAN ERROR] " }+e.what());
	}
}



void r2r_ctrl(
	ceps::cloud::Route um) {
	INIT_SYS_ERR_HANDLING;
	auto remote_sck_from = -1;
	auto remote_sck_to = -1;

	constexpr auto CAN_RTR_FLAG = 0x40000000U;
	CLEANUP([&]() {
		if (remote_sck_from != -1) closesocket(remote_sck_from);
		if (remote_sck_to != -1) closesocket(remote_sck_to);
	});
	auto ext_can = false;
	for (auto e : ceps::cloud::info_in_channels[um.to.first]) {
		if (e.first == um.to.second) {
			ext_can = e.second == "CANX";
			break;
		}
	}
	try {

		remote_sck_from = net::inet::establish_inet_stream_connect(um.from.first.first, um.from.first.second);
		if (remote_sck_from == -1) { THROW_ERR_INET; }
		{
			std::stringstream cmd;
			cmd << "HTTP/1.1 100\r\n";
			cmd << "cmd: subscribe_out_channel\r\n";
			cmd << "out_channel: " << um.from.second << "\r\n\r\n";

			auto r = send(remote_sck_from, cmd.str().c_str(), cmd.str().length(), 0); STORE_SYS_ERR;
			if (r != cmd.str().length()) {
				THROW_ERR_INET;
			}
		}
		remote_sck_to = net::inet::establish_inet_stream_connect(um.to.first.first, um.to.first.second);
		if (remote_sck_to == -1) { THROW_ERR_INET; }
		{
			std::stringstream cmd;
			cmd << "HTTP/1.1 100\r\n";
			cmd << "cmd: subscribe_in_channel\r\n";
			cmd << "in_channel: " << um.to.second << "\r\n\r\n";

			auto r = send(remote_sck_to, cmd.str().c_str(), cmd.str().length(), 0); STORE_SYS_ERR;
			if (r != cmd.str().length()) {
				THROW_ERR_INET;
			}
		}

		can_frame frame = { 0 };
		std::uint32_t l = 0;
		int r;
		for (;;) {
			r = recv(remote_sck_from, (char*)&l, sizeof(l), 0);
			if (r != sizeof(l)) THROW_ERR_INET;
			l = ntohl(l);
			if(sizeof(frame) != l) THROW_ERR_INET;
			r = recv(remote_sck_from, (char*)&frame, l, 0);
			if (r != l) THROW_ERR_INET;
			r = send(remote_sck_to, (char*)&frame, sizeof(frame), 0); STORE_SYS_ERR; if (r != sizeof(frame)) THROW_ERR_INET;
		}
	}
	catch (net::exceptions::err_inet const & e) {
		fatal(std::string{ "[NETWORK ERROR] " }+e.what());
	}
	catch (ceps::cloud::exceptions::err_vcan_api const & e) {
		fatal(std::string{ "[VCAN_API ERROR] " }+e.what());
	}
	catch (net::exceptions::err_can const & e) {
		fatal(std::string{ "[CAN ERROR] " }+e.what());
	}
}


void downstream_ctrl(
	ceps::cloud::Simulation_Core sim_core,
	ceps::cloud::Downstream_Mapping dm) {
	INIT_SYS_ERR_HANDLING;
	constexpr auto CAN_RTR_FLAG = 0x40000000U;
	auto ext_can = false;
	for (auto e : ceps::cloud::info_out_channels[sim_core]) {
		if (e.first == dm.second) {
			ext_can = e.second == "CANX";
			break;
		}
	}
	auto remote_sck = -1;
	CLEANUP([&]() {
		if (remote_sck != -1)closesocket(remote_sck); 
	});
	auto info = net::can::get_local_endpoint_info(dm.first);
	auto baudrate = info_br2pcan_br[info.br];
	auto channel = net::can::get_local_endpoint_handle(dm.first);
	try {
		if (channel == -1) throw net::exceptions::err_can{ "Couldn't acquire channel handle for '" + dm.first + "'." };
		auto rinit = pcan_api::initialize(channel, baudrate, 0, 0, 0);
		if (rinit != PCAN_ERROR_OK && rinit != PCAN_ERROR_INITIALIZE) {
			throw net::exceptions::err_can{ "Initialization failed, endpoint is '" + dm.first + "', errormessage: "+pcan_errcode2text[rinit]+"." };
		}
		remote_sck = net::inet::establish_inet_stream_connect(sim_core.first, sim_core.second);
		if (remote_sck == -1) { THROW_ERR_INET }
		{
			std::stringstream cmd;
			cmd << "HTTP/1.1 100\r\n";
			cmd << "cmd: subscribe_out_channel\r\n";
			cmd << "out_channel: " << dm.second << "\r\n\r\n";

			auto r = send(remote_sck, cmd.str().c_str(), cmd.str().length(), 0);
			STORE_SYS_ERR;
			if (r != cmd.str().length()) {
				THROW_ERR_INET;
			}
		}
		for (;;) {
			std::uint32_t l = 0;
			auto r = recv(remote_sck, (char*) &l, sizeof(l), 0);
			if (r != sizeof(l)) THROW_ERR_INET;
			l = ntohl(l);
			TPCANMsg can_message { 0 };
			char buffer[32];
			r = recv(remote_sck, buffer, l, 0);
			if (r != l) THROW_ERR_INET;
			std::uint32_t can_id = *((::uint32_t*)buffer);
			std::uint8_t len = *( ((::uint8_t*)buffer)+4);
			can_message.MSGTYPE = PCAN_MESSAGE_STANDARD;
			if (can_id & CAN_RTR_FLAG) { can_message.MSGTYPE |= PCAN_MESSAGE_RTR; can_id &= ~CAN_RTR_FLAG; }
			can_message.ID = can_id;
			can_message.LEN = len;
			
			if (ext_can) can_message.MSGTYPE |= PCAN_MESSAGE_EXTENDED;
			memcpy(can_message.DATA, buffer + 8, len);
			auto wr = pcan_api::write(channel, &can_message);
			if (wr != PCAN_ERROR_OK) {
				throw net::exceptions::err_can{ "Write failed, endpoint is '" + dm.first + "', errormessage: " + pcan_errcode2text[rinit] + "." };
			}
		}
	}catch(net::exceptions::err_inet const & e) {
		fatal(std::string{ "["+dm.second+"->"+dm.first+"][NETWORK ERROR] " }+e.what());
	}
	catch (ceps::cloud::exceptions::err_vcan_api const & e) {
		fatal(std::string{ "[" + dm.second + "->" + dm.first + "][VCAN_API ERROR] " }+e.what());
	}
	catch (net::exceptions::err_can const & e) {
		fatal(std::string{ "[" + dm.second + "->" + dm.first + "][CAN ERROR] " }+e.what());
	}
}
#ifdef KMW_MULTIBUS_API
void downstream_ctrl_multibus(
	ceps::cloud::Simulation_Core sim_core,
	ceps::cloud::Downstream_Mapping dm) {
	INIT_SYS_ERR_HANDLING;
	constexpr auto CAN_RTR_FLAG = 0x40000000U;
	constexpr auto CAN_EFF_FLAG = 0x80000000U;
	auto ext_can = false;
	for (auto e : ceps::cloud::info_out_channels[sim_core]) {
		if (e.first == dm.second) {
			ext_can = e.second == "CANX";
			break;
		}
	}
	auto remote_sck = -1;
	CLEANUP([&]() {
		if (remote_sck != -1)closesocket(remote_sck);
	});
	auto info = net::can::get_local_endpoint_info(dm.first);
	auto baudrate = info_br2pcan_br[info.br];
	auto channel = net::can::get_local_endpoint_handle(dm.first);
	
	try {
		if (channel == -1) throw net::exceptions::err_can{ "Couldn't acquire channel handle for '" + dm.first + "'." };
		auto multibus_queue = kmw_api::canopen(channel);

		remote_sck = net::inet::establish_inet_stream_connect(sim_core.first, sim_core.second);
		if (remote_sck == -1) { THROW_ERR_INET }
		{
			std::stringstream cmd;
			cmd << "HTTP/1.1 100\r\n";
			cmd << "cmd: subscribe_out_channel\r\n";
			cmd << "out_channel: " << dm.second << "\r\n\r\n";

			auto r = send(remote_sck, cmd.str().c_str(), cmd.str().length(), 0);
			STORE_SYS_ERR;
			if (r != cmd.str().length()) {
				THROW_ERR_INET;
			}
		}
		for (;;) {
			std::uint32_t l = 0;
			
			auto r = recv(remote_sck, (char*)&l, sizeof(l), 0);
			if (r != sizeof(l)) THROW_ERR_INET;
			l = ntohl(l);
			if (l != sizeof(can_frame)) THROW_ERR_INET;

			CanMessage can_message{ 0 };
			can_frame frame = { 0 };
			r = recv(remote_sck, (char*)&frame, l, 0);
			if (r != l) THROW_ERR_INET;
			if (ext_can) can_message.format = CanFormat::CanFormatExtended;
			else can_message.format = CanFormat::CanFormatStandard;
			if(ext_can) can_message.id = frame.can_id | CAN_EFF_FLAG;
			else can_message.id =  frame.can_id;
			can_message.length = frame.can_dlc;
			memcpy(can_message.data, frame.data, frame.can_dlc);
			auto rw = kmw_api::canwrite(
				multibus_queue,
				&can_message);

		}
	}
	catch (net::exceptions::err_inet const & e) {
		fatal(std::string{ "[" + dm.second + "->" + dm.first + "][NETWORK ERROR] " }+e.what());
	}
	catch (ceps::cloud::exceptions::err_vcan_api const & e) {
		fatal(std::string{ "[" + dm.second + "->" + dm.first + "][VCAN_API ERROR] " }+e.what());
	}
	catch (net::exceptions::err_can const & e) {
		fatal(std::string{ "[" + dm.second + "->" + dm.first + "][CAN ERROR] " }+e.what());
	}
}
#endif

int main(int argc, char* argv[])
{
	auto display_name = [](std::pair<ceps::cloud::Simulation_Core, std::string> p) -> std::string {
		return p.second + "@" + p.first.first + ":" + p.first.second;
	};
	bool wsa_startup_successful = false;
	CLEANUP([&](){if (wsa_startup_successful) WSACleanup(); })

#ifdef PCAN_API    
	init_pcan_dll();
#endif

#ifdef KMW_MULTIBUS_API
	kmw_queue2sock.resize(256);
	kmw_remote_inf_is_canx.resize(256);
	init_kmw_multibus_dll();
	if (kmw_api::canstart) kmw_api::canstart();
#endif

	auto v = ceps::cloud::check_available_ifs();
	ceps::cloud::sys_info_available_ifs = ceps::misc::sort_and_remove_duplicates(v);

	//handle information parameters which requires only local data
	if (argc == 1) {
		std::cout << usage << std::endl;
		exit(1);
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


	for (int i = 1; i != argc; ++i) {
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
		else if (token == "-directory") {
			auto sim_core = ceps::cloud::cmdline_read_remote_host(argv[++i]);
			if (sim_core == ceps::cloud::Simulation_Core{}) fatal("[USER ERROR] Erroneous host name:'" + std::string{ argv[i] }+"'.");
			try{
				auto dir = ceps::cloud::fetch_directory_entries(sim_core);
				for (auto e : dir.entries) {
					ceps::cloud::sim_cores.insert(e.sim_core);
					ceps::cloud::global_directory.push_back(e);
				}
			}
			catch (net::exceptions::err_inet & e) {
				throw net::exceptions::err_inet("(" + sim_core.first + ":" + sim_core.second + ") " + e.what());
			}
			catch (ceps::cloud::exceptions::err_vcan_api const & e) {
				fatal(std::string{ "[VCAN_API ERROR] " }+e.what());
			}
		}
	}

	for (int i = 1; i != argc; ++i) {
		std::string token = argv[i];
		if (token == "--print_known_hosts") {
			std::cout << "Known hosts:\n";
			for (auto s : ceps::cloud::sim_cores) {
				std::cout <<"\t" << s.first << ":" << s.second << " aka " << ceps::cloud::display_name_with_details(s) << std::endl;
			}
		}
	}

	
	std::vector<std::pair<ceps::cloud::Simulation_Core, std::future<ceps::cloud::vcan_api::fetch_channels_return_t>>> handles;
	for(auto s : ceps::cloud::sim_cores)
		handles.push_back(std::make_pair(s,std::async(std::launch::async, ceps::cloud::vcan_api::fetch_channels, s)));

	try {
		for (auto & handle : handles) {
			auto remote_channels = handle.second.get();
			ceps::cloud::info_out_channels[handle.first] = remote_channels.first;
			ceps::cloud::info_in_channels[handle.first] = remote_channels.second;
		}
		//handle information parameters which requires remote data
		for (int i = 2; i != argc; ++i) {
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
		auto mappings = ceps::cloud::parse_cmdline_and_extract_mappings(argc,argv, ceps::cloud::info_out_channels, ceps::cloud::info_in_channels);
		//handle information parameters which requires parsed mapping data
		for (int i = 2; i != argc; ++i) {
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
		if (mappings.empty()) warn(std::string{ "No mappings defined." },true);
		//Ready to run
		std::vector<std::thread*> downstream_threads;
		std::vector<std::thread*> upstream_threads;
		std::vector<std::thread*> route_threads;
		for (auto e : mappings.remote_to_local_mappings) {
#ifdef PCAN_API
			if(pcan_api::is_pcan(e.first.first)) downstream_threads.push_back(new std::thread{ downstream_ctrl,ceps::cloud::sim_core(e),ceps::cloud::down_stream(e) });
#endif
#ifdef KMW_MULTIBUS_API
			if (kmw_multibus_dll != nullptr && kmw_api::is_kmw(e.first.first)) downstream_threads.push_back(new std::thread{ downstream_ctrl_multibus,ceps::cloud::sim_core(e),ceps::cloud::down_stream(e) });
#endif
		}
		for (auto e : mappings.local_to_remote_mappings) {
#ifdef PCAN_API
			if (pcan_api::is_pcan(e.first.first))upstream_threads.push_back(new std::thread{ upstream_ctrl,ceps::cloud::sim_core(e),ceps::cloud::up_stream(e) });
#endif
#ifdef KMW_MULTIBUS_API
			if (kmw_multibus_dll!=nullptr && kmw_api::is_kmw(e.first.first))
				upstream_threads.push_back(new std::thread{ upstream_ctrl_kmw,ceps::cloud::sim_core(e),ceps::cloud::up_stream(e) });
#endif

		}
		for (auto e : mappings.routes)
			route_threads.push_back(new std::thread{ r2r_ctrl, e });
		for (auto t : downstream_threads) t->join();
		for (auto t : upstream_threads) t->join();
		for (auto t : route_threads) t->join();
	}
	catch (net::exceptions::err_inet const & e) {
		fatal(std::string{ "[NETWORK ERROR] " }+e.what());
	}
	catch (ceps::cloud::exceptions::err_vcan_api const & e) {
		fatal(std::string{ "[VCAN_API ERROR] " }+e.what());
	}
	catch (net::exceptions::err_can const & e) {
		fatal(std::string{ "[CAN_API ERROR] " }+e.what());
	}
    return 0;
}

