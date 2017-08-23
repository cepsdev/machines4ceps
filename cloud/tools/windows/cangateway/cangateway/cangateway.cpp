#include "common.h"
#pragma comment(lib, "ws2_32.lib")


void fatal(std::string msg) {
	std::cerr << "***Fatal Error: " << msg << std::endl;
	exit(-1);
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
  Address of host running a cepS core.
 EXPRESSION
  CAN(LOCAL) -> CAN(REMOTE) 
  CAN(LOCAL) <- CAN(REMOTE) 
 )";



int main(int argc, char* argv[])
{
	bool wsa_startup_successful = false;
	CLEANUP([&](){if (wsa_startup_successful) WSACleanup(); })
    
	init_pcan_dll();

	auto v = ceps::cloud::check_available_ifs();
	ceps::cloud::sys_info_available_ifs = ceps::misc::sort_and_remove_duplicates(v);
	if (v.size() == 0) fatal("No CAN Devices found.");
	if (argc == 1) {
		std::cout << usage << std::endl;
		exit(1);
	}
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
	
	auto handle = std::async(std::launch::async, ceps::cloud::fetch_out_channels, ceps::cloud::current_core);
	try {
		auto remote_out_channels = handle.get();
		//for (auto e : remote_out_channels) std::cout << e.first << " (" << e.second << ")" << "\n";
		ceps::cloud::info_out_channels[ceps::cloud::current_core] = remote_out_channels;
	}
	catch (net::exceptions::err_inet const & e) {
		fatal(std::string{ "[NETWORK ERROR] " }+e.what());
	}
	catch (ceps::cloud::exceptions::err_vcan_api const & e) {
		fatal(std::string{ "[VCAN_API ERROR] " }+e.what());
	}
    return 0;
}

