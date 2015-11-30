#include "core/include/cal_sender.hpp"
#include "core/include/base_defs.hpp"
#include <thread>
#include <mutex>
extern std::vector<std::thread*> comm_threads;


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

#endif	

bool State_machine_simulation_core::handle_userdefined_sender_definition(std::string call_name,
	ceps::ast::Nodeset const & ns)
{
	DEBUG_FUNC_PROLOGUE

#ifdef USE_KMW_MULTIBUS
	if (call_name == "canbus") {
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
	}
#endif

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
#endif


