#include "cepscloud_streaming_common.h"
#include "vcanstreams.h"
#include "vcan_standard_ctrls.h"

extern void fatal(std::string msg);
extern void warn(std::string msg, bool terminate);
extern std::map<std::string, net::can::can_info::BAUD_RATE>str_br2id;
extern std::map<int, int>info_br2pcan_br;
extern std::map<int, std::string> pcan_errcode2text;
extern void log(std::string m);


ceps::cloud::Ctrlregistry global_ctrlregistry;

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

std::map<std::string, net::can::can_info::BAUD_RATE>str_br2id{
	{ "1M",net::can::can_info::BAUD_1M },
	{ "800K",net::can::can_info::BAUD_800K },
	{ "500K",net::can::can_info::BAUD_500K },
	{ "250K",net::can::can_info::BAUD_250K },
	{ "125K",net::can::can_info::BAUD_125K },
	{ "100K",net::can::can_info::BAUD_100K },
	{ "95K",net::can::can_info::BAUD_95K },
	{ "83K",net::can::can_info::BAUD_83K },
	{ "50K",net::can::can_info::BAUD_50K },
	{ "47K",net::can::can_info::BAUD_47K },
	{ "33K",net::can::can_info::BAUD_33K },
	{ "20K",net::can::can_info::BAUD_20K },
	{ "10K",net::can::can_info::BAUD_10K },
	{ "5K",net::can::can_info::BAUD_5K },
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


#ifdef PCAN_API

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
	{ net::can::can_info::BAUD_1M, PCAN_BAUD_1M },
	{ net::can::can_info::BAUD_800K, PCAN_BAUD_800K },
	{ net::can::can_info::BAUD_500K, PCAN_BAUD_500K },
	{ net::can::can_info::BAUD_250K, PCAN_BAUD_250K },
	{ net::can::can_info::BAUD_125K, PCAN_BAUD_125K },
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


void pcan_api::downstream_ctrl(
	ceps::cloud::Simulation_Core sim_core,
	ceps::cloud::Downstream_Mapping dm,
	ceps::cloud::downstream_hook_t hook) {
	INIT_SYS_ERR_HANDLING;
    log("pcan_api::downstream_ctrl()");
	auto cur_token = get_current_token();
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
	bool init_pcan = true;
	TPCANStatus rinit;
	for (;;) {
		try {
			if (channel == -1) throw net::exceptions::err_can{ "Couldn't acquire channel handle for '" + dm.first + "'." };
			if (init_pcan) {
				init_pcan = false;
				rinit = pcan_api::initialize(channel, baudrate, 0, 0, 0);
				if (rinit != PCAN_ERROR_OK && rinit != PCAN_ERROR_INITIALIZE) {
					throw net::exceptions::err_can{ "Initialization failed, endpoint is '" + dm.first + "', errormessage: " + pcan_errcode2text[rinit] + "." };
				}
			}
			if (remote_sck == -1)remote_sck = net::inet::establish_inet_stream_connect(sim_core.first, sim_core.second);
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
				TPCANMsg can_message{ 0 };
				char buffer[32];
				r = recv(remote_sck, buffer, l, 0);
				if (r != l) THROW_ERR_INET;
				if (hook) hook( *((net::can::can_frame*)buffer) );
				std::uint32_t can_id = *((::uint32_t*)buffer);
				std::uint8_t len = *(((::uint8_t*)buffer) + 4);
				can_message.MSGTYPE = PCAN_MESSAGE_STANDARD;
				if (can_id & CAN_RTR_FLAG) { can_message.MSGTYPE |= PCAN_MESSAGE_RTR; can_id &= ~CAN_RTR_FLAG; }
				can_message.ID = can_id;
				can_message.LEN = len;

				if (ext_can) can_message.MSGTYPE |= PCAN_MESSAGE_EXTENDED;
				memcpy(can_message.DATA, buffer + 8, len);
				if (cur_token != get_current_token()) break;
				auto wr = pcan_api::write(channel, &can_message);
				if (wr != PCAN_ERROR_OK) {
					throw net::exceptions::err_can{ "Write failed, endpoint is '" + dm.first + "', errormessage: " + pcan_errcode2text[rinit] + "." };
				}
			}
		}
		catch (net::exceptions::err_inet const & e) {
			warn(std::string{ "[" + dm.second + "->" + dm.first + "][NETWORK ERROR] " }+e.what(), false);
			closesocket(remote_sck); remote_sck = -1;
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
		catch (ceps::cloud::exceptions::err_vcan_api const & e) {
			warn(std::string{ "[" + dm.second + "->" + dm.first + "][VCAN_API ERROR] " }+e.what(), false);
			closesocket(remote_sck); remote_sck = -1;
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
		catch (net::exceptions::err_can const & e) {
			init_pcan = true;
			warn(std::string{ "[" + dm.second + "->" + dm.first + "][CAN ERROR] " }+e.what(), false);
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
		if (cur_token != get_current_token()) break;
	}
}

void pcan_api::upstream_ctrl(
	ceps::cloud::Simulation_Core sim_core,
	ceps::cloud::Upstream_Mapping um) {
	INIT_SYS_ERR_HANDLING
		auto cur_token = get_current_token();
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

			auto r = send(remote_sck, cmd.str().c_str(), cmd.str().length(), 0); STORE_SYS_ERR;
			if (r != cmd.str().length()) {
				THROW_ERR_INET;
			}
			auto read_ev = CreateEvent(NULL, FALSE, FALSE, NULL); STORE_SYS_ERR;
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
					if (cur_token != get_current_token()) break;
					if (rr != PCAN_ERROR_QRCVEMPTY) {
						net::can::can_frame frame = { 0 };
						frame.can_id = can_message.ID;
						if (can_message.MSGTYPE & PCAN_MESSAGE_RTR) frame.can_id |= CAN_RTR_FLAG;
						frame.can_dlc = can_message.LEN;
						if (frame.can_dlc) memcpy(frame.data, can_message.DATA, frame.can_dlc);
						auto r = send(remote_sck, (char*)&frame, sizeof(frame), 0); STORE_SYS_ERR; if (r != sizeof(frame)) THROW_ERR_INET;
					}
				} while (rr != PCAN_ERROR_QRCVEMPTY);
				if (cur_token != get_current_token()) break;
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

#endif

#ifdef KMW_MULTIBUS_API

void kmw_api::downstream_ctrl(
	ceps::cloud::Simulation_Core sim_core,
	ceps::cloud::Downstream_Mapping dm,
	ceps::cloud::downstream_hook_t) {
	INIT_SYS_ERR_HANDLING;
	log("[downstream_ctrl_multibus()] Enter");

	auto cur_token = get_current_token();
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
		log("[downstream_ctrl_multibus()] Leave");
		if (remote_sck != -1)closesocket(remote_sck);
	});
	auto info = net::can::get_local_endpoint_info(dm.first);
	auto baudrate = info_br2pcan_br[info.br];
	auto channel = net::can::get_local_endpoint_handle(dm.first);

	unsigned char multibus_queue;
	bool init_multibus_queue = true;
	for (;;) {
		try {
			if (channel == -1) throw net::exceptions::err_can{ "Couldn't acquire channel handle for '" + dm.first + "'." };
			if (init_multibus_queue) {
				multibus_queue = kmw_api::canopen(channel);
				log("[downstream_ctrl_multibus()] multibus_queue=" + std::to_string((int)multibus_queue));
				init_multibus_queue = false;
			}

			if (remote_sck == -1) remote_sck = net::inet::establish_inet_stream_connect(sim_core.first, sim_core.second);
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
				if (l != sizeof(net::can::can_frame)) THROW_ERR_INET;

				CanMessage can_message{ 0 };
				net::can::can_frame frame = { 0 };
				r = recv(remote_sck, (char*)&frame, l, 0);
				if (r != l) THROW_ERR_INET;
				if (ext_can) can_message.format = CanFormat::CanFormatExtended;
				else can_message.format = CanFormat::CanFormatStandard;
				if (ext_can) can_message.id = frame.can_id  & ~CAN_EFF_FLAG;
				else can_message.id = frame.can_id;

				can_message.length = frame.can_dlc;
				memcpy(can_message.data, frame.data, frame.can_dlc);
				if (cur_token != get_current_token()) break;
				//log("[DEBUG] can_message.length=" + std::to_string((int)can_message.length));
				//log("[DEBUG] can_message.id=" + std::to_string(can_message.id));

				auto rw = kmw_api::canwrite(
					multibus_queue,
					&can_message);
			}
		}
		catch (net::exceptions::err_inet const & e) {
			closesocket(remote_sck); remote_sck = -1;
			warn(std::string{ "[" + dm.second + "->" + dm.first + "][NETWORK ERROR] " }+e.what(), false);
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
		catch (ceps::cloud::exceptions::err_vcan_api const & e) {
			closesocket(remote_sck); remote_sck = -1;
			warn(std::string{ "[" + dm.second + "->" + dm.first + "][VCAN_API ERROR] " }+e.what(), false);
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
		catch (net::exceptions::err_can const & e) {
			init_multibus_queue = true;
			warn(std::string{ "[" + dm.second + "->" + dm.first + "][CAN ERROR] " }+e.what(), false);
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}

		if (cur_token != get_current_token()) break;
	}//for(;;)

}


static std::vector<int> kmw_queue2sock;
static std::vector<bool> kmw_remote_inf_is_canx;

static void canbus_read_callback(unsigned char queue) {
	constexpr auto CAN_RTR_FLAG = 0x40000000U;
	CanEvent event = { 0 };
	kmw_api::canread(queue, &event);
	if (!event.messageValid) return;
	auto remote_sck = kmw_queue2sock[queue];
	net::can::can_frame frame = { 0 };
	frame.can_id = event.message.id;
	frame.can_dlc = event.message.length;
	if (event.message.rtr) frame.can_id |= CAN_RTR_FLAG;
	if (frame.can_dlc) memcpy(frame.data, event.message.data, frame.can_dlc);
	auto r = send(remote_sck, (char*)&frame, sizeof(frame), 0);
	if (r != sizeof(frame)) fatal("send failed.");
}

static void canbus_flush_callback(unsigned char queue, bool flushed) {
}

void kmw_api::upstream_ctrl(
	ceps::cloud::Simulation_Core sim_core,
	ceps::cloud::Upstream_Mapping um) {
	INIT_SYS_ERR_HANDLING
		auto cur_token = get_current_token();
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
				if (cur_token != get_current_token()) {
					kmw_api::caninstall(canbus_queue, nullptr, nullptr);
					break;
				}
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


#endif


template <typename T, typename E> void getprocaddr(HINSTANCE hdll, E e, const char * sz, T& f) {
	f = (T)GetProcAddress(hdll, sz);
	if (f == T{}) e(sz);
}

#ifdef PCAN_API

static HINSTANCE pcan_dll = nullptr;

static void init_pcan_dll() {
    pcan_dll = LoadLibraryA("PCANBasic");
	if (pcan_dll == nullptr) warn("No PEAK driver found: LoadLibrary('PCANBAsic') failed.", false);
	if (pcan_dll == nullptr) return;
	auto e = [](const char* sz) {fatal("Incompatible PEAK driver: GetProcAddress(\"" + std::string{ sz }+"\") failed."); };
	getprocaddr(pcan_dll, e, "CAN_GetValue", pcan_api::getvalue);
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
#endif

#ifdef KMW_MULTIBUS_API
HINSTANCE kmw_multibus_dll = nullptr;
static void init_kmw_multibus_dll() {
    kmw_multibus_dll = LoadLibraryA("Multibus");
	if (kmw_multibus_dll == nullptr) {
		auto err = GetLastError();
		warn("No KMW Multibus library found: LoadLibrary('Multibus') failed. (" + std::to_string(err) + ")", false);
	}
	if (kmw_multibus_dll == nullptr) return;
	auto e = [](const char* sz) {fatal("Incompatible KMW Multibus library: GetProcAddress(\"" + std::string{ sz }+"\") failed."); };
	getprocaddr(kmw_multibus_dll, e, "CanStart", kmw_api::canstart);
	getprocaddr(kmw_multibus_dll, e, "CanOpen", kmw_api::canopen);
	getprocaddr(kmw_multibus_dll, e, "CanWrite", kmw_api::canwrite);
	getprocaddr(kmw_multibus_dll, e, "CanRead", kmw_api::canread);
	getprocaddr(kmw_multibus_dll, e, "CanInstall", kmw_api::caninstall);
}
#endif


void setup_shared_libs() {
#ifdef PCAN_API
	init_pcan_dll();
#endif
#ifdef KMW_MULTIBUS_API
	kmw_queue2sock.resize(256);
	kmw_remote_inf_is_canx.resize(256);
	init_kmw_multibus_dll();
	if (kmw_api::canstart) {
		kmw_api::canstart();
	}
#endif
}

void setup_stream_ctrls(std::vector<std::string> local_ifs) {
#ifdef PCAN_API
	for (auto const & e : local_ifs) {
		if (!pcan_api::is_pcan(e)) continue;
		global_ctrlregistry.reg_down_stream_ctrl("CAN", e, pcan_api::downstream_ctrl);
		global_ctrlregistry.reg_down_stream_ctrl("CANX", e, pcan_api::downstream_ctrl);

		global_ctrlregistry.reg_up_stream_ctrl("CAN", e, pcan_api::upstream_ctrl);
		global_ctrlregistry.reg_up_stream_ctrl("CANX", e, pcan_api::upstream_ctrl);
	}
#endif

#ifdef KMW_MULTIBUS_API
	for (auto const & e : local_ifs) {
		if (!kmw_api::is_kmw(e)) continue;
		global_ctrlregistry.reg_down_stream_ctrl("CAN", e, kmw_api::downstream_ctrl);
		global_ctrlregistry.reg_down_stream_ctrl("CANX", e, kmw_api::downstream_ctrl);
		
		global_ctrlregistry.reg_up_stream_ctrl("CAN", e, kmw_api::upstream_ctrl);
		global_ctrlregistry.reg_up_stream_ctrl("CANX", e, kmw_api::upstream_ctrl);		
	}
#endif
}


void r2r_ctrl(
	ceps::cloud::Route um) {
	INIT_SYS_ERR_HANDLING;
	auto cur_token = get_current_token();
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

		net::can::can_frame frame = { 0 };
		std::uint32_t l = 0;
		int r;
		for (;;) {
			r = recv(remote_sck_from, (char*)&l, sizeof(l), 0);
			if (r != sizeof(l)) THROW_ERR_INET;
			l = ntohl(l);
			if (sizeof(frame) != l) THROW_ERR_INET;
			r = recv(remote_sck_from, (char*)&frame, l, 0);
			if (r != l) THROW_ERR_INET;
			if (cur_token != get_current_token()) break;
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
