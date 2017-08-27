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
	if (pcan_dll == nullptr) fatal("No PEAK driver found: LoadLibrary('PCANBAsic') failed.");
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

static std::string usage = R"(
cangateway SIMULATION_CORE [EXPRESSION...]

 SIMULATION_CORE
  host name and port running a cepS core.
  
  Example:
   foo.de:8186

 EXPRESSION
  There are essentially two classes of expressions, mapping expressions and settings.
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

using mappings_t = std::pair<std::vector<ceps::cloud::Downstream_Mapping>, std::vector<ceps::cloud::Upstream_Mapping>>;
mappings_t parse_cmdline_and_extract_mappings(int argc,
	char* argv[], 
	std::vector<std::pair<ceps::cloud::Remote_Interface, std::string>> remote_out, 
	std::vector<std::pair<ceps::cloud::Remote_Interface, std::string>> remote_in) 
{
	mappings_t rv;
	net::can::can_info::BAUD_RATE br = net::can::can_info::BAUD_1M;
	for (int j = 0; j != argc; ++j) {
		std::string token = argv[j];
		if (token == "->" || token == "<-") {
			if (j == 0 || j + 1 == argc) fatal("[USER ERROR] Illformed mapping definition.");
			auto r = handle_mapping(argv[j-1], argv[j+1], token == "->",br,remote_out,remote_in);
			if (!std::get<0>(r)) fatal("[USER ERROR] Illformed mapping definition.");
			if (std::get<1>(r)) rv.first.push_back(std::get<2>(r));
			else rv.second.push_back(std::get<2>(r));
			++j;
		}
		else if (token == "-b") {
			if (j + 1 == argc) fatal("[USER ERROR] Trailing -b. Expected a baudrate definition.");
			auto it = str_br2id.find(argv[j+1]);
			if (it == str_br2id.end()) fatal("[USER ERROR] Unknown parameter value for baudrate: '" + std::string{ argv[j + 1] } + "'.");
			br = it->second;
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
	for (auto e : ceps::cloud::info_out_channels[sim_core]) {
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
		if (rinit != PCAN_ERROR_OK) {
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

				/*std::uint32_t l = 0;
				auto r = recv(remote_sck, (char*)&l, sizeof(l), 0);
				if (r != sizeof(l)) THROW_ERR_INET;
				l = ntohl(l);
				char buffer[32];
				r = recv(remote_sck, buffer, l, 0);
				if (r != l) THROW_ERR_INET;
				std::uint32_t can_id = *((::uint32_t*)buffer);
				std::uint8_t len = *(((::uint8_t*)buffer) + 4);
				can_message.MSGTYPE = PCAN_MESSAGE_STANDARD;
				if (can_id & CAN_RTR_FLAG) { can_message.MSGTYPE |= PCAN_MESSAGE_RTR; can_id &= ~CAN_RTR_FLAG; }
				can_message.ID = can_id;
				can_message.LEN = len;

				if (ext_can) can_message.MSGTYPE |= PCAN_MESSAGE_EXTENDED;
				memcpy(can_message.DATA, buffer + 8, len);*/
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
		if (rinit != PCAN_ERROR_OK) {
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

int main(int argc, char* argv[])
{
	bool wsa_startup_successful = false;
	CLEANUP([&](){if (wsa_startup_successful) WSACleanup(); })
    
	init_pcan_dll();

	auto v = ceps::cloud::check_available_ifs();
	ceps::cloud::sys_info_available_ifs = ceps::misc::sort_and_remove_duplicates(v);
    //handle information parameters which requires only local data
	if (argc == 1) {
		std::cout << usage << std::endl;
		exit(1);
	}

	for (int i = 1; i != argc; ++i) {
		std::string token = argv[i];
		if (token == "--print_available_local_endpoints" || token == "-a" || token == "-al") {
			std::cout << "Available local endpoints:\n";
			for (auto e : ceps::cloud::sys_info_available_ifs) std::cout << "\t" << e << "\n";
		}
	}

	if (v.size() == 0) fatal("No CAN Devices found.");
	ceps::cloud::current_core = ceps::cloud::cmdline_read_remote_host(argc-1, argv+1);
	if (ceps::cloud::current_core == ceps::cloud::Simulation_Core{}) fatal("No/Invalid hostname specified.\n A hostname is a a string of the form 'www.foo.com:123'.");

	{
		WORD wVersionRequested;
		WSADATA wsaData;
		int err;
		wVersionRequested = MAKEWORD(2, 2);
		err = WSAStartup(wVersionRequested, &wsaData);
		if (err != 0) 
			fatal("WSAStartup failed with error: "+std::to_string(err));
	}
	
	auto handle = std::async(std::launch::async, ceps::cloud::vcan_api::fetch_channels, ceps::cloud::current_core);
	try {
		auto remote_channels = handle.get();
		ceps::cloud::info_out_channels[ceps::cloud::current_core] = remote_channels.first;
		ceps::cloud::info_in_channels[ceps::cloud::current_core] = remote_channels.second;
		//handle information parameters which requires remote data
		for (int i = 2; i != argc; ++i) {
			std::string token = argv[i];
			if (token == "--print_available_remote_endpoints" || token == "-a" || token == "-ar") {
				std::cout << "Available remote endpoints (name,direction,type):\n";
				for (auto e : remote_channels.first) std::cout << "\t" << e.first <<", out , "<< e.second << "\n";
				for (auto e : remote_channels.second) std::cout << "\t" << e.first << ", in , " << e.second << "\n";
			}
		}
		auto mappings = parse_cmdline_and_extract_mappings(argc,argv, ceps::cloud::info_out_channels[ceps::cloud::current_core], ceps::cloud::info_in_channels[ceps::cloud::current_core]);
		//handle information parameters which requires parsed mapping data
		for (int i = 2; i != argc; ++i) {
			std::string token = argv[i];
			if (token == "--print_mappings" || token == "-m" ) {
				if (mappings.first.size()) {
					std::cout << "Downstream mappings (remote endpoint -> local endpoint):\n";
					for (auto e : mappings.first) std::cout << "\t" << e.second << " -> " << e.first << "\n";
				}
				else if (mappings.second.size()) std::cout << "No downstream mappings defined.\n";
				if (mappings.second.size()) {
					std::cout << "Upstream mappings (local endpoint -> remote endpoint):\n";
					for (auto e : mappings.second) std::cout << "\t" << e.first << " -> " << e.second << "\n";
				}
				else if (mappings.first.size()) std::cout << "No upstream mappings defined.\n";

				if (mappings.first.size() == 0 && mappings.second.size() == 0) std::cout << "No mappings defined at all.\n";

			}
		}
		if (mappings.first.size() == 0 && mappings.second.size() == 0) warn(std::string{ "No downstream/upstream mappings defined." },true);
		//Ready to run
		std::vector<std::thread*> downstream_threads;
		std::vector<std::thread*> upstream_threads;
		for (auto e : mappings.first)
			downstream_threads.push_back(new std::thread{ downstream_ctrl,ceps::cloud::current_core,e });
		for (auto e : mappings.second)
			upstream_threads.push_back(new std::thread{ upstream_ctrl,ceps::cloud::current_core,e });
		for (auto t : downstream_threads) t->join();
		for (auto t : upstream_threads) t->join();
	}
	catch (net::exceptions::err_inet const & e) {
		fatal(std::string{ "[NETWORK ERROR] " }+e.what());
	}
	catch (ceps::cloud::exceptions::err_vcan_api const & e) {
		fatal(std::string{ "[VCAN_API ERROR] " }+e.what());
	}
    return 0;
}

