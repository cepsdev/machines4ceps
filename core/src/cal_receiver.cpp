#include "core/include/cal_receiver.hpp"
#include "core/include/base_defs.hpp"
#include <thread>
#include <mutex>
#include <chrono>
#include <condition_variable>

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


extern std::vector<std::thread*> comm_threads;


#ifdef USE_KMW_MULTIBUS

namespace kmw {
	constexpr auto MIN_CAN_FRAME_SIZE = 2;
	constexpr auto MAX_CAN_FRAME_PAYLOAD = 8;
	extern bool kmw_multibus_initialized;
	extern bool canbus_reader_exist;
    #include "Multibus.h"
}
bool kmw::canbus_reader_exist = false;

void comm_receiver_kmw_multibus(int id,
	int bus_id,
	State_machine_simulation_core* smc);
#else
void comm_receiver_socket_can(int id,
	std::string bus_id,
	State_machine_simulation_core* smc,
	bool extended_can_id);
namespace sockcan{
	constexpr auto MIN_CAN_FRAME_SIZE = 2;
	constexpr auto MAX_CAN_FRAME_PAYLOAD = 8;
	extern std::map<std::string,int> interfaces_to_sockets;
	extern std::mutex interfaces_to_sockets_m;
}
#endif

bool State_machine_simulation_core::handle_userdefined_receiver_definition(std::string call_name,
	ceps::ast::Nodeset const & ns)
{
	DEBUG_FUNC_PROLOGUE
	if (call_name == "canbus") {
#ifdef USE_KMW_MULTIBUS
			if (kmw::canbus_reader_exist) {
				fatal_(-1,"Receiver definition: CAN-CALs implemented by KMW Multibus allow only one receiver.");
			}
			if (!kmw::kmw_multibus_initialized) {
				kmw::CanStart();
				DEBUG << "[KMW_MULTIBUS_INITIALIZED]\n";
			}
			std::string channel_id;
			int can_bus = 0;
			if (ns["id"].size() != 1 || ns["id"].nodes()[0]->kind() != ceps::ast::Ast_node_kind::identifier)
				fatal_(-1, "A CAN(KMW MULTIBUS) CAL receiver definition requires an id.");
			channel_id = ceps::ast::name(ceps::ast::as_id_ref(ns["id"].nodes()[0]));
			auto bus_id_ = ns["transport"]["canbus"]["bus_id"];
			if (bus_id_.nodes().empty())
				DEBUG << "[KMW_MULTIBUS_RECEIVER_DEFINITION][No bus specified, default assumed (0)]\n";
			else {
				try {
					can_bus = bus_id_.as_int();
				}
				catch (...) {
					fatal_(-1, "CAN(KMW MULTIBUS) CAL receiver definition: bus_id must be an integer value.");
				}
				using namespace ceps::ast;
				DEBUG << "[PROCESSING_UNCONDITIONED_RECEIVER]" << "[CAL=" << call_name << "]" << "[bus=" << can_bus << "]" << "\n";
				auto handlers = ns[all{ "on_msg" }];
				if (handlers.empty()) fatal_(-1, "on_msg required in receiver definition.");
				int dispatcher_id = -1;
				auto& ctxt = allocate_dispatcher_thread_ctxt(dispatcher_id);
				DEBUG << "[PROCESSING_UNCONDITIONED_RECEIVER (CAL="<< call_name << "][dispatcher_id=" << dispatcher_id << "]\n";
				for (auto const & handler_ : handlers) {
					auto const & handler = handler_["on_msg"];
					auto frame_id_ = handler["frame_id"];
					auto handler_func_ = handler["handler"];

					if (frame_id_.size() != 1 || frame_id_.nodes()[0]->kind() != ceps::ast::Ast_node_kind::identifier)
						fatal_(-1, "Receiver definition illformed: frame_id not an identifier / wrong number of arguments.");
					if (handler_func_.size() != 1 || handler_func_.nodes()[0]->kind() != ceps::ast::Ast_node_kind::identifier)
						fatal_(-1, "Receiver definition illformed: handler not an identifier / wrong number of arguments.");

					auto frame_id = ceps::ast::name(ceps::ast::as_id_ref(frame_id_.nodes()[0]));
					auto handler_id = ceps::ast::name(ceps::ast::as_id_ref(handler_func_.nodes()[0]));
					auto it_frame = frame_generators().find(frame_id);
					if (it_frame == frame_generators().end()) fatal_(-1, "Receiver definition: on_msg : frame_id unknown.");
					auto it_func = global_funcs().find(handler_id);
					if (it_func == global_funcs().end()) fatal_(-1, "Receiver definition: on_msg : function unknown.");
					ctxt.handler.push_back(std::make_pair(it_frame->second, it_func->second));
				}
				kmw::canbus_reader_exist = true;
				if (start_comm_threads()){
				 comm_threads.push_back(new std::thread{ comm_receiver_kmw_multibus,
					dispatcher_id,
					can_bus, this });

				 running_as_node() = true;
				}
			}
			return true;
#else
			std::string channel_id;
			std::string can_bus = "can0";
			if (ns["id"].size() != 1 || ns["id"].nodes()[0]->kind() != ceps::ast::Ast_node_kind::identifier)
				fatal_(-1, "A CAN(SOCKET CAN) CAL receiver definition requires an id.");
			channel_id = ceps::ast::name(ceps::ast::as_id_ref(ns["id"].nodes()[0]));
			auto bus_id_ = ns["transport"]["canbus"]["bus_id"];

			bool extended= false;
			auto candef = ns["transport"]["canbus"];
			for(auto p : candef.nodes())
			{
				if (p->kind() != ceps::ast::Ast_node_kind::identifier) continue;
				if (ceps::ast::name(ceps::ast::as_id_ref(p))!="extended") continue;
				extended =true;
				break;
			}
			if (bus_id_.nodes().empty())
				DEBUG << "[SOCKET_CAN_RECEIVER_DEFINITION][No bus specified, default assumed (can0)]\n";
			else {
				if (bus_id_.nodes()[0]->kind() == ceps::ast::Ast_node_kind::int_literal)
					can_bus = std::string("can")+std::to_string(ceps::ast::value(ceps::ast::as_int_ref(bus_id_.nodes()[0])));
				else if (bus_id_.nodes()[0]->kind() == ceps::ast::Ast_node_kind::string_literal)
					can_bus = ceps::ast::value(ceps::ast::as_string_ref(bus_id_.nodes()[0]));
				else
					fatal_(-1, "CAN-CAL receiver definition: bus_id must be an integer or string.");

				using namespace ceps::ast;
				DEBUG << "[PROCESSING_UNCONDITIONED_RECEIVER]" << "[CAL=" << call_name << "]" << "[bus=" << can_bus << "]" << "\n";
				auto handlers = ns[all{ "on_msg" }];
				//if (handlers.empty()) fatal_(-1, "on_msg required in receiver definition.");
				int dispatcher_id = -1;
				auto ctxt = allocate_dispatcher_thread_ctxt(dispatcher_id);
				ctxt->id_=channel_id;
                if(extended) ctxt->info()="CANX";
                else ctxt->info()="CAN";

				DEBUG << "[PROCESSING_UNCONDITIONED_RECEIVER (CAL="<< call_name << "][dispatcher_id=" << dispatcher_id << "]\n";
				for (auto const & handler_ : handlers) {
					auto const & handler = handler_["on_msg"];
					auto frame_id_ = handler["frame_id"];
					auto handler_func_ = handler["handler"];

					if (frame_id_.size() != 1 || frame_id_.nodes()[0]->kind() != ceps::ast::Ast_node_kind::identifier)
						fatal_(-1, "Receiver definition illformed: frame_id not an identifier / wrong number of arguments.");
					if (handler_func_.size() != 1 || handler_func_.nodes()[0]->kind() != ceps::ast::Ast_node_kind::identifier)
						fatal_(-1, "Receiver definition illformed: handler not an identifier / wrong number of arguments.");

					auto frame_id = ceps::ast::name(ceps::ast::as_id_ref(frame_id_.nodes()[0]));
					auto handler_id = ceps::ast::name(ceps::ast::as_id_ref(handler_func_.nodes()[0]));
					auto it_frame = frame_generators().find(frame_id);
					if (it_frame == frame_generators().end()) fatal_(-1, "Receiver definition: on_msg : frame_id unknown.");
					auto it_func = global_funcs().find(handler_id);
					if (it_func == global_funcs().end()) fatal_(-1, "Receiver definition: on_msg : function unknown.");
					ctxt->handler.push_back(std::make_pair(it_frame->second, it_func->second));
				}
				if (start_comm_threads()){
				 comm_threads.push_back(new std::thread{ comm_receiver_socket_can,
					dispatcher_id,
					can_bus, this, extended });

				 running_as_node() = true;
				}
			}
			return true;
#endif
	}
	return false;
}


#ifdef USE_KMW_MULTIBUS

static State_machine_simulation_core::dispatcher_thread_ctxt_t kmw_multibus_in_ctxt;
constexpr int CAN_MSG_SIZE = 10;
static std::uint8_t can_msg[CAN_MSG_SIZE];
static State_machine_simulation_core* kmw_multibus_current_smc;

void canbus_read_callback(unsigned char queue) {
	kmw::CanEvent event = { 0 };
	kmw::CanRead(queue,&event);
	if (!event.messageValid) return;
	bzero(can_msg, CAN_MSG_SIZE);
	*((std::uint16_t*)can_msg) = (std::uint16_t)event.message.id;
	if (event.message.rtr) *((std::uint16_t*)can_msg) |= 0x800;
	*(can_msg + 1) |= (event.message.length+2) << 4;
	memcpy(can_msg + 2, event.message.data, event.message.length);
	std::vector<std::string> v1;
	std::vector<ceps::ast::Nodebase_ptr> v2;
	for (auto handler_info : kmw_multibus_in_ctxt.handler) {
		auto match = handler_info.first->read_msg((char*)can_msg, event.message.length + 2, kmw_multibus_current_smc, v1, v2);
		if (!match) continue;		
		kmw_multibus_current_smc->execute_action_seq(nullptr, handler_info.second);
		return;
	}
}

void canbus_flush_callback(unsigned char queue, bool flushed) {
}


void comm_receiver_kmw_multibus(int id,
	int bus_id,
	State_machine_simulation_core* smc)
{
	auto THIS = smc;
	kmw_multibus_current_smc = smc;
	DEBUG_FUNC_PROLOGUE2
	kmw_multibus_in_ctxt = smc->get_dispatcher_thread_ctxt(id);
	auto canbus_queue = kmw::CanOpen(bus_id);
	kmw::CanInstall(canbus_queue, canbus_read_callback,canbus_flush_callback);

	for (; !smc->shutdown();) {
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}
#else
constexpr int CAN_MSG_SIZE = 13;
//#define DEBUG std::cout
void comm_receiver_socket_can(int id,
	std::string can_bus,
	State_machine_simulation_core* smc,
	bool extended_can_id){
	int s = 0;

	{
	 struct sockaddr_can addr;
	 struct ifreq ifr;
	 if((s = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
		smc->fatal_(-1,"comm_receiver_socket_can:Error while opening socket");
	 }

	 strcpy(ifr.ifr_name, can_bus.c_str());
	 ioctl(s, SIOCGIFINDEX, &ifr);

	 addr.can_family  = AF_CAN;
	 addr.can_ifindex = ifr.ifr_ifindex;

	 if(bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		smc->fatal_(-1,"comm_receiver_socket_can:Error in socket bind");
	 }
	}

	auto current_smc = smc;
	auto in_ctxt = smc->get_dispatcher_thread_ctxt(id);

	in_ctxt->wait_for_start_request();

	int ctr=1;
	for (;;++ctr)
	{
		struct can_frame can_message{0};
		auto r = recv(s, &can_message, sizeof(struct can_frame),0);
		if (r <= 0)
			smc->fatal_(-1,"comm_receiver_socket_can:Read failed.");


		std::uint8_t can_msg[CAN_MSG_SIZE];
		bzero(can_msg, CAN_MSG_SIZE);
		if(!extended_can_id){
		 *((std::uint16_t*)can_msg) = can_message.can_id & 0x7FF;
		 if (can_message.can_id & 0x800) *((std::uint16_t*)can_msg) |= 0x800;
		 *(can_msg + 1) |= (can_message.can_dlc+2) << 4;
		 memcpy(can_msg + 2, can_message.data, can_message.can_dlc);
		} else {
		 can_message.can_id &= 0x1FFFFFFF;
		 *((std::uint32_t*)can_msg) = can_message.can_id;
		 *((std::uint8_t*)(can_msg+sizeof(std::uint32_t)) ) = can_message.can_dlc+5;
		 memcpy(can_msg + sizeof(std::uint32_t) + sizeof(std::uint8_t), can_message.data, can_message.can_dlc);
		}
		std::vector<std::string> v1;
		std::vector<ceps::ast::Nodebase_ptr> v2;

		if (in_ctxt->get_native_handler().size() == 0 && in_ctxt->handler.size() == 0){
			/*
			  event_signature{
               can_frame_received;
               port(id);
               can_frame_id(can_id);
               can_frame_payload(any);
              };*/
			State_machine_simulation_core::event_t ev;
			ev.id_ = "can_frame_received";
			ev.payload_.push_back(new ceps::ast::Identifier(in_ctxt->id_));
			ev.payload_.push_back(new ceps::ast::Int((int)can_message.can_id,ceps::ast::all_zero_unit()));
			std::vector<unsigned char> seq;seq.resize(can_message.can_dlc);
			std::copy(can_message.data,can_message.data+can_message.can_dlc,seq.begin());
			ev.payload_.push_back(new ceps::ast::Byte_array(seq));
			current_smc->enqueue_event(ev);
		} else if (in_ctxt->get_native_handler().size()){
			for(auto fc :  in_ctxt->get_native_handler()){
				if (!fc->match_chunk(can_msg,extended_can_id? can_message.can_dlc + 5 : can_message.can_dlc + 2)) continue;
				fc->read_chunk(can_msg,extended_can_id? can_message.can_dlc + 5 : can_message.can_dlc + 2);
				auto clone = fc->clone();
				State_machine_simulation_core::event_t ev;
				ev.id_ = "@@framecontext";
				ev.frmctxt_ = clone;
				current_smc->enqueue_event(ev);
				break;
			}
		}
		else for (auto handler_info : in_ctxt->handler) {

			/**
			 * Trigger recomputation of message size to get the header length
			 *
			 */

			auto match =
					handler_info.first->read_msg((char*)can_msg, extended_can_id? can_message.can_dlc + 5 : can_message.can_dlc + 2, current_smc, v1, v2);
			if (!match) continue;
			current_smc->execute_action_seq(nullptr, handler_info.second);
			break;
		}
	}//for(;;)
}

#endif




