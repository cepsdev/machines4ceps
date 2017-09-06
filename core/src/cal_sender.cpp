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


        bool map_can_frame(struct can_frame* out, char* frame,int frame_size, int header_size) {
                if (frame_size < MIN_CAN_FRAME_SIZE) return false;
                if (frame == nullptr) return false;

		if (header_size == 16){
		  out->can_id = (*((std::uint16_t*)frame) & 0x7FF) | ((*((std::uint16_t*)frame) & 0x800) >> 11);
		  out->can_dlc = std::min( ((*((std::uint16_t*)frame) & 0xF000) >> 12) - 2, MAX_CAN_FRAME_PAYLOAD);
                  if(out->can_dlc > 8 || out->can_dlc < 0)
                   return false;
		  memcpy(out->data, frame + 2, out->can_dlc);
		}
		else {
                 out->can_id = *((std::uint32_t*)frame) ;
		 out->can_dlc = *((std::uint8_t*) (frame+4)) - 5;
                 if(out->can_dlc > 8 || out->can_dlc < 0)
                   return false;
		 memcpy(out->data, frame + 5, out->can_dlc);
		}
		return true;
	}
}

std::map<std::string,int> sockcan::interfaces_to_sockets;
std::mutex sockcan::interfaces_to_sockets_m;

void comm_sender_socket_can(State_machine_simulation_core::frame_queue_t* channel,
        std::string can_bus, State_machine_simulation_core* smp,std::unordered_map<int,std::uint32_t>,bool);
#endif	

static bool is_assignment(ceps::ast::Nodebase_ptr p){
	if (p == nullptr) return false;
	if (p->kind() != ceps::ast::Ast_node_kind::binary_operator) return false;
	if (ceps::ast::op(ceps::ast::as_binop_ref(p)) != '=') return false;
	return true;
}

static bool is_symbol(ceps::ast::Nodebase_ptr p, std::string& name, std::string& kind){
	if (p == nullptr) return false;
	if (p->kind() != ceps::ast::Ast_node_kind::symbol) return false;
	name = ceps::ast::name(ceps::ast::as_symbol_ref(p));
	kind = ceps::ast::kind(ceps::ast::as_symbol_ref(p));
	return true;
}

static bool get_one_and_only_symbol(ceps::ast::Nodebase_ptr p, std::string& name, std::string& kind){
	if (p == nullptr) return false;
	if (p->kind() == ceps::ast::Ast_node_kind::symbol)
	{
	 name = ceps::ast::name(ceps::ast::as_symbol_ref(p));
	 kind = ceps::ast::kind(ceps::ast::as_symbol_ref(p));
	 return true;
	}

	if (p->kind() == ceps::ast::Ast_node_kind::int_literal || p->kind() == ceps::ast::Ast_node_kind::float_literal || p->kind() == ceps::ast::Ast_node_kind::string_literal)
		return false;

	if (p->kind() == ceps::ast::Ast_node_kind::binary_operator) {
      auto & oper = ceps::ast::as_binop_ref(p);
      std::string name1;
      std::string kind1;
      std::string name2;
      std::string kind2;
      bool r1 = get_one_and_only_symbol(oper.left(),name1,kind1);
      bool r2 = get_one_and_only_symbol(oper.right(),name2,kind2);
      if (!(r1 || r2)) return false;
      if (! (r1 && r2) ) {
    	  if (r1) {name=name1;kind=kind1;return true;}
    	  else {name=name2;kind=kind2;return true;}
      }
      if (name1 != name2 || kind1 != kind2 ) return false;
      name = name1; kind = kind1;
      return true;
	}

	return false;
}

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
			if (!start_comm_threads()){
			 running_as_node() = true;

			 comm_threads.push_back(
				new std::thread{ comm_sender_kmw_multibus,
				channel,
				can_bus,
				this});
			}
		}
		return true;
#else
		 if (ns["id"].size() != 1 || ns["id"].nodes()[0]->kind() != ceps::ast::Ast_node_kind::identifier)
			fatal_(-1,"A CAN(Socket CAN) CAL sender definition requires an id.");
		 auto channel_id = ceps::ast::name(ceps::ast::as_id_ref(ns["id"].nodes()[0]));
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

         std::string can_bus = "";
		 if (bus_id_.nodes().empty()){
			//check universe for transport definition
			ceps::ast::Nodeset ns = current_universe();
			auto transport_defs = ns[ceps::ast::all{"transport"}];
			bool transport_busid_found = false;
			for(auto transport_ : transport_defs){
			 auto transport = transport_["transport"];
			 auto id = transport["id"];
			 if (id.as_str() != channel_id) continue;
			 auto bus_id_ = transport["canbus"]["bus_id"];
             if (bus_id_.empty()) continue;
             can_bus = bus_id_.as_str();
             transport_busid_found = can_bus.length();
		 }
		 if (!transport_busid_found){
             warn_(-1, "CAN-CAL sender definition '"+channel_id+"': no network interface defined (attribute bus_id).Endpoint is treated as virtual.");
             //return true;
		 }
		}else {
			if (bus_id_.nodes()[0]->kind() == ceps::ast::Ast_node_kind::int_literal)
				can_bus = std::string("can")+std::to_string(ceps::ast::value(ceps::ast::as_int_ref(bus_id_.nodes()[0])));
			else if (bus_id_.nodes()[0]->kind() == ceps::ast::Ast_node_kind::string_literal)
				can_bus = ceps::ast::value(ceps::ast::as_string_ref(bus_id_.nodes()[0]));
			else{
                warn_(-1, "CAN-CAL sender definition: bus_id must be an integer or string. Endpoint is treated as virtual.");
                can_bus = "";
			}
		}

		//Handling of can_id_mapping

		auto can_id_mapping = ns["transport"]["canbus"]["can_id_mapping"];
		if (can_id_mapping.size()){

			for(std::size_t i = 0; i!=can_id_mapping.nodes().size() && i+1!=can_id_mapping.nodes().size();i+=2){
              auto frame_name_ = can_id_mapping.nodes()[i];
              auto frame_id_ = can_id_mapping.nodes()[i+1];
              if (frame_name_->kind() != ceps::ast::Ast_node_kind::identifier ||
                  (frame_id_->kind() != ceps::ast::Ast_node_kind::int_literal && frame_id_->kind() != ceps::ast::Ast_node_kind::string_literal ) )
            	  fatal_(-1,"CAN sender '"+channel_id+"': wrong can id mapping, should be list of identifier/integer pairs.");
              auto frame_name = ceps::ast::name(ceps::ast::as_id_ref(frame_name_));
              channel_frame_name_to_id[channel_id][frame_name] = i / 2;
              if (frame_id_->kind() == ceps::ast::Ast_node_kind::int_literal) channel_frame_to_id[channel_id][i/2] = ceps::ast::value(ceps::ast::as_int_ref(frame_id_));
              else channel_frame_to_id[channel_id][i/2] = std::stol(ceps::ast::value(ceps::ast::as_string_ref(frame_id_)));
			}
		}

		//Handling of encodings

		std::map<std::string /*systemstate*/, std::map< int, ceps::ast::Nodebase_ptr> > encodings;
		auto encodings_ = current_universe()[ceps::ast::all{"encoding"}];
		for (auto e : encodings_){
			auto encoding = e["encoding"];
			auto transport = encoding["transport"].as_str();
			if (transport.length() != 0 && transport != channel_id ) continue;
			for (auto k : encoding.nodes()){
				if (!is_assignment(k)) continue;
				auto & ass = ceps::ast::as_binop_ref(k);
				std::string lhs_name,lhs_kind;
				if (!is_symbol(ass.left(),lhs_name,lhs_kind)) continue;
				bool is_in_encoding = lhs_kind == "Systemstate";
				if (is_in_encoding) continue;
				int bitwidth=1;
				if (lhs_kind == "bool") bitwidth = -1;
				else if (lhs_kind == "uint4_t") bitwidth = 8;
				else if (lhs_kind == "int4_t") bitwidth = -8;
				else if (lhs_kind == "uint8_t") bitwidth = 8;
				else if (lhs_kind == "int8_t") bitwidth = -8;
				else if (lhs_kind == "uint16_t") bitwidth = 16;
				else if (lhs_kind == "int16_t") bitwidth = -16;
				else if (lhs_kind == "uint24_t") bitwidth = 24;
				else if (lhs_kind == "int24_t") bitwidth = -24;
				else if (lhs_kind == "uint32_t") bitwidth = 32;
				else if (lhs_kind == "int32_t") bitwidth = -32;
				else if (lhs_kind == "uint40_t") bitwidth = 40;
				else if (lhs_kind == "int40_t") bitwidth = -40;
				else if (lhs_kind == "uint48_t") bitwidth = 48;
				else if (lhs_kind == "int48_t") bitwidth = -48;
				else if (lhs_kind == "uint56_t") bitwidth = 56;
				else if (lhs_kind == "int56_t") bitwidth = -56;
				else if (lhs_kind == "uint64_t") bitwidth = 64;
				else if (lhs_kind == "int64_t") bitwidth = -64;

				std::string rhs_kind,rhs_name;
				if (!get_one_and_only_symbol(ass.right(),rhs_name,rhs_kind) || rhs_kind != "Systemstate") {
					std::stringstream ss; ss << ass;
					warn_(-1,"Illformed out encoding : "+ss.str());
					continue;
				}
				encodings[rhs_name][bitwidth] = ass.right();
			}
		}

		out_encodings[channel_id] = encodings;

		auto channel = new State_machine_simulation_core::frame_queue_t;
        if (extended) this->set_out_channel(channel_id, channel,"CANX");
        else this->set_out_channel(channel_id, channel,"CAN");

		if (start_comm_threads()){
			running_as_node() = true;

			comm_threads.push_back(
				new std::thread{ comm_sender_socket_can,
				channel,
				can_bus,
                this, channel_frame_to_id[channel_id],extended});
		}


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
                        auto mr = map_can_frame(&can_message, frame, frame_size);
                        if (!mr)
                          smc->fatal_(-1,"CAN Frame incompatible (too long?)");

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
void comm_sender_socket_can(State_machine_simulation_core::frame_queue_t* frames,
        std::string can_bus, State_machine_simulation_core* smc, std::unordered_map<int,std::uint32_t> frame2id, bool extended_can) {
    int s = -1;
	char* frame = nullptr;
	size_t frame_size = 0;
	size_t header_size = 0;
	bool pop_frame = true;
    bool is_virtual = false;

    if (can_bus.length()){
		std::lock_guard<std::mutex> lock(sockcan::interfaces_to_sockets_m);
		auto it = sockcan::interfaces_to_sockets.find(can_bus);
		if (it != sockcan::interfaces_to_sockets.end()){
			s = it->second;
		} else {
			struct sockaddr_can addr;
			struct ifreq ifr;
			if((s = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
				auto te = errno;
				State_machine_simulation_core::event_t ev;
				ev.error_ = new State_machine_simulation_core::error_t{"comm_sender_socket_can() terminated: socket() failed.",te};
				smc->main_event_queue().push(ev);
				return;
			}
			strcpy(ifr.ifr_name, can_bus.c_str());
			ioctl(s, SIOCGIFINDEX, &ifr);
			addr.can_family  = AF_CAN;
			addr.can_ifindex = ifr.ifr_ifindex;

			if(bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
				auto te = errno;
				State_machine_simulation_core::event_t ev;
				ev.error_ = new State_machine_simulation_core::error_t{"comm_sender_socket_can() terminated: bind() failed.",te};
				smc->main_event_queue().push(ev);
				return;
			}
			sockcan::interfaces_to_sockets[can_bus] = s;
		}
    } else is_virtual=true;

    std::vector<int> gateway_sockets;

	for (;;)
	{
		State_machine_simulation_core::frame_queue_elem_t frame_info;

		if (pop_frame) {
			frames->wait_and_pop(frame_info);
            auto new_gtwy_sck = -1;
            if ( 0 <= (new_gtwy_sck = smc->frame_carries_gateway_socket(frame_info))){
                bool already_registered = false;
                for(auto & e : gateway_sockets) if (e == new_gtwy_sck) already_registered = true;
                if (already_registered) continue;
                std::size_t insertion_idx=0;
                for(;insertion_idx!=gateway_sockets.size();++insertion_idx) if (gateway_sockets[insertion_idx] < 0) break;
                if (insertion_idx!=gateway_sockets.size()) gateway_sockets[insertion_idx] = new_gtwy_sck;
                else gateway_sockets.push_back(new_gtwy_sck);
                continue;
            }
			frame_size = std::get<1>(frame_info);
			frame = (decltype(frame))std::get<1>(std::get<0>(frame_info));
			header_size = std::get<2>(frame_info);
		}
		pop_frame = false;
		auto len = frame_size;
        if (len > 8) continue;
        can_frame can_message{0};
		if (header_size == 0 && frame){
			//fetch can id
			auto frame_id = std::get<3>(frame_info);
			auto it = frame2id.find(frame_id);
			bool can_id_found = it != frame2id.end();
			if (!can_id_found){
				State_machine_simulation_core::event_t ev;
				ev.error_ = new State_machine_simulation_core::error_t{"comm_sender_socket_can() terminated: invalid CAN ID.",0};
				smc->main_event_queue().push(ev);
				return;
			}
			can_message.can_id = it->second;
                        if(extended_can) can_message.can_id |= CAN_EFF_FLAG;
			can_message.can_dlc = frame_size;
			memcpy(can_message.data,frame,frame_size);

            if (!is_virtual){
             auto r = write(s, &can_message, sizeof(struct can_frame));
             auto err = errno;
             if (r != sizeof(struct can_frame)){
				State_machine_simulation_core::event_t ev;
                ev.error_ = new State_machine_simulation_core::error_t{"comm_sender_socket_can() terminated: write failed.",err};
				smc->main_event_queue().push(ev);
				return;
             }
            }
            if (gateway_sockets.size())can_message.can_id = it->second;
            for(auto & sck : gateway_sockets){
                auto v = htonl(sizeof (can_frame));
                auto r = send(sck,&v,sizeof(v),MSG_DONTWAIT);
                if (r != sizeof(v) && (r == EAGAIN || r == EWOULDBLOCK)) continue;/*skip potentially blocking sockets*/
                if (r != sizeof(v)) {close(sck);sck=-1; /*kill errorneous connections*/}
                r = send(sck,&can_message,sizeof(can_frame),MSG_DONTWAIT);
                if (r != sizeof(can_frame)) {close(sck);sck=-1; /*necessary even in case of an EAGAIN/EWOULDBLOCK result*/}
            }
		}
		else if (len >= sockcan::MIN_CAN_FRAME_SIZE && frame) {
			sockcan::map_can_frame(&can_message, frame, frame_size,header_size);
			auto r = write(s, &can_message, sizeof(struct can_frame));
			if (r != sizeof(struct can_frame)){
				State_machine_simulation_core::event_t ev;
				ev.error_ = new State_machine_simulation_core::error_t{"comm_sender_socket_can() terminated: write failed.",0};
				smc->main_event_queue().push(ev);
				return;
			}
		}
		if (frame != nullptr) { delete[] frame; frame = nullptr; }
		pop_frame = true;
	}//for(;;)
}
#endif


