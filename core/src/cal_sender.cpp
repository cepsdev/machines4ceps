#include "core/include/cal_sender.hpp"
#include "core/include/base_defs.hpp"
#include <thread>
#include <mutex>
extern std::vector<std::thread*> comm_threads;

#ifdef __linux
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <net/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#endif


#ifdef USE_KMW_MULTIBUS

namespace kmw {
	constexpr auto MIN_CAN_FRAME_SIZE = 2;
	constexpr auto MAX_CAN_FRAME_PAYLOAD = 8;
	extern bool kmw_multibus_initialized;
    #include "Multibus.h"
	
	void map_can_frame(CanMessage* out, char* frame,int frame_size) {
		if (frame_size < MIN_CAN_FRAME_SIZE) return;
		if (frame == nullptr) return;
		out->id = *((std::uint16_t*)frame) & 0x7FF;
		out->rtr = (*((std::uint16_t*)frame) & 0x800) >> 11;
		out->length = std::min( ((*((std::uint16_t*)frame) & 0xF000) >> 12) - 2, MAX_CAN_FRAME_PAYLOAD);
		memcpy(out->data, frame + 2, out->length);		
	}
}

void comm_sender_kmw_multibus(threadsafe_queue< std::pair<char*, size_t>, std::queue<std::pair<char*, size_t> >>* channel,
	int can_bus, State_machine_simulation_core* smp);

bool kmw::kmw_multibus_initialized = false;
#else

namespace sockcan{
	constexpr auto MIN_CAN_FRAME_SIZE = 2;
	constexpr auto MAX_CAN_FRAME_PAYLOAD = 8;

	extern std::map<std::string,int> interfaces_to_sockets;
	extern std::mutex interfaces_to_sockets_m;


	void map_can_frame(struct can_frame* out, char* frame,int frame_size, int header_size) {
		if (frame_size < MIN_CAN_FRAME_SIZE) return;
		if (frame == nullptr) return;

		//std::cout << "!!!!!!!!!!!!!!!!!!!"<< header_size << std::endl;
		if (header_size == 16){
		  out->can_id = (*((std::uint16_t*)frame) & 0x7FF) | ((*((std::uint16_t*)frame) & 0x800) >> 11);
		  out->can_dlc = std::min( ((*((std::uint16_t*)frame) & 0xF000) >> 12) - 2, MAX_CAN_FRAME_PAYLOAD);
		  memcpy(out->data, frame + 2, out->can_dlc);
		}
		else {
		 out->can_id = *((std::uint32_t*)frame) ;
		 out->can_dlc = *((std::uint8_t*) (frame+4)) - 5;
		 memcpy(out->data, frame + 5, out->can_dlc);
		}
	}
}

std::map<std::string,int> sockcan::interfaces_to_sockets;
std::mutex sockcan::interfaces_to_sockets_m;

void comm_sender_socket_can(threadsafe_queue< std::tuple<char*, size_t,size_t>, std::queue<std::tuple<char*, size_t,size_t> >>* channel,
	std::string can_bus, State_machine_simulation_core* smp);
#endif	

bool State_machine_simulation_core::handle_userdefined_sender_definition(std::string call_name,
	ceps::ast::Nodeset const & ns)
{
	DEBUG_FUNC_PROLOGUE

	if (call_name == "canbus") {
#ifdef USE_KMW_MULTIBUS

		if (!kmw::kmw_multibus_initialized) {
			kmw::CanStart();
			DEBUG << "[KMW_MULTIBUS_INITIALIZED]\n";
		}
		std::string channel_id;
		int can_bus = 0;
		if (ns["id"].size() != 1 || ns["id"].nodes()[0]->kind() != ceps::ast::Ast_node_kind::identifier)
			fatal_(-1,"A CAN(KMW MULTIBUS) CAL sender definition requires an id.");
		channel_id = ceps::ast::name(ceps::ast::as_id_ref(ns["id"].nodes()[0]));
		auto bus_id_ = ns["transport"]["canbus"]["bus_id"];
		if (bus_id_.nodes().empty())
			DEBUG << "[KMW_MULTIBUS_SENDER_DEFINITION][No bus specified, default assumed (0)]\n";
		else {
			try {
				can_bus = bus_id_.as_int();
			}
			catch (...) {
				fatal_(-1, "CAN(KMW MULTIBUS) CAL sender definition: bus_id must be an integer value.");
			}

			auto channel = new threadsafe_queue< std::pair<char*, size_t>, std::queue<std::pair<char*, size_t> >>;
			this->set_out_channel(channel_id, channel);
			running_as_node() = true;

			comm_threads.push_back(
				new std::thread{ comm_sender_kmw_multibus,
				channel,
				can_bus,
				this});
		}
		return true;
#else
		if (ns["id"].size() != 1 || ns["id"].nodes()[0]->kind() != ceps::ast::Ast_node_kind::identifier)
			fatal_(-1,"A CAN(Socket CAN) CAL sender definition requires an id.");
		auto channel_id = ceps::ast::name(ceps::ast::as_id_ref(ns["id"].nodes()[0]));
		auto bus_id_ = ns["transport"]["canbus"]["bus_id"];
		std::string can_bus = "can0";
		if (bus_id_.nodes().empty())
			DEBUG << "[CAN_SENDER_DEFINITION][No bus specified, default assumed (can0)]\n";
		else {
			if (bus_id_.nodes()[0]->kind() == ceps::ast::Ast_node_kind::int_literal)
				can_bus = std::string("can")+std::to_string(ceps::ast::value(ceps::ast::as_int_ref(bus_id_.nodes()[0])));
			else if (bus_id_.nodes()[0]->kind() == ceps::ast::Ast_node_kind::string_literal)
				can_bus = ceps::ast::value(ceps::ast::as_string_ref(bus_id_.nodes()[0]));
			else
				fatal_(-1, "CAN-CAL sender definition: bus_id must be an integer or string.");
		}

		auto channel = new threadsafe_queue< std::tuple<char*, size_t,size_t>, std::queue<std::tuple<char*, size_t,size_t> >>;
		this->set_out_channel(channel_id, channel);
		running_as_node() = true;

		comm_threads.push_back(
			new std::thread{ comm_sender_socket_can,
			channel,
			can_bus,
			this});


		return true;
#endif
	}

	return false;
}




#ifdef USE_KMW_MULTIBUS
void comm_sender_kmw_multibus(threadsafe_queue< std::pair<char*, size_t>, std::queue<std::pair<char*, size_t> >>* frames,
	int can_bus, State_machine_simulation_core* smc) {
	kmw::CanMessage can_message{ 0 };
	auto multibus_queue = kmw::CanOpen((unsigned char)can_bus);
	auto THIS = smc;
	DEBUG_FUNC_PROLOGUE2

	char* frame = nullptr;
	size_t frame_size = 0;
	bool pop_frame = true;
	for (;;)
	{
		DEBUG << "[comm_sender_kmw_multibus][WAIT_FOR_FRAME][pop_frame=" << pop_frame << "]\n";
		std::pair<char*, size_t> frame_info;

		if (pop_frame) { 
			frames->wait_and_pop(frame_info); frame_size = frame_info.second; frame = frame_info.first; 
		}
		pop_frame = false;

		DEBUG << "[comm_sender_kmw_multibus][FETCHED_FRAME]\n";

		auto len = frame_size;
		int wr = 0;

		if (len >= kmw::MIN_CAN_FRAME_SIZE && frame) {
			map_can_frame(&can_message, frame, frame_size);
			auto r = kmw::CanWrite(
				multibus_queue,
				&can_message);
			DEBUG << "[comm_sender_kmw_multibus][FRAME_WRITTEN][(" << frame_size << " bytes)][CanWrite Return ("<<(int)r <<")]\n";
		}
		if (frame != nullptr) { delete[] frame; frame = nullptr; }
		pop_frame = true;
	}//for(;;)
}
#else
//#define DEBUG std::cout
void comm_sender_socket_can(threadsafe_queue< std::tuple<char*, size_t,size_t>, std::queue<std::tuple<char*, size_t,size_t> >>* frames,
	std::string can_bus, State_machine_simulation_core* smc) {
	int s = 0;
	//auto THIS = smc;
	DEBUG_FUNC_PROLOGUE2
	char* frame = nullptr;
	size_t frame_size = 0;
	size_t header_size = 0;
	bool pop_frame = true;

	{
		//extern std::map<std::string,int> interfaces_to_sockets;
		std::lock_guard<std::mutex> lock(sockcan::interfaces_to_sockets_m);

		auto it = sockcan::interfaces_to_sockets.find(can_bus);
		if (it != sockcan::interfaces_to_sockets.end()){
			s = it->second;
		} else {
			struct sockaddr_can addr;
			struct ifreq ifr;
			if((s = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
				smc->fatal_(-1,"comm_sender_socket_can:Error while opening socket");
			}

			strcpy(ifr.ifr_name, can_bus.c_str());
			ioctl(s, SIOCGIFINDEX, &ifr);

			addr.can_family  = AF_CAN;
			addr.can_ifindex = ifr.ifr_ifindex;

			if(bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
				smc->fatal_(-1,"comm_sender_socket_can:Error in socket bind");
			}
			sockcan::interfaces_to_sockets[can_bus] = s;
		}
	}


	for (;;)
	{
		DEBUG << "[comm_sender_socket_can][WAIT_FOR_FRAME][pop_frame=" << pop_frame << "]\n";
		std::tuple<char*, size_t,size_t> frame_info;

		if (pop_frame) {
			frames->wait_and_pop(frame_info);
			frame_size = std::get<1>(frame_info);
			frame = std::get<0>(frame_info);
			header_size = std::get<2>(frame_info);
		}
		pop_frame = false;
		DEBUG << "[comm_sender_socket_can][FETCHED_FRAME]\n";
		auto len = frame_size;
		//int wr = 0;
		struct can_frame can_message{0};
		if (len >= sockcan::MIN_CAN_FRAME_SIZE && frame) {
			sockcan::map_can_frame(&can_message, frame, frame_size,header_size);
			auto r = write(s, &can_message, sizeof(struct can_frame));
			DEBUG << "[comm_sender_socket_can][r = write(s, &can_message, sizeof(struct can_frame));][r=" << r << "]\n";
		}
		if (frame != nullptr) { delete[] frame; frame = nullptr; }
		pop_frame = true;
	}//for(;;)
}
#endif


