
/*
Copyright 2022 Tomas Prerovsky (cepsdev@hotmail.com).

Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/


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


void comm_receiver_socket_can(int id,
	std::string bus_id,
	State_machine_simulation_core* smc,
    std::unordered_map<int,std::uint32_t> frame2id,
    std::string channel_id,
    bool extended_can_id,
    int sock);
namespace sockcan{
	constexpr auto MIN_CAN_FRAME_SIZE = 2;
	constexpr auto MAX_CAN_FRAME_PAYLOAD = 8;
	extern std::map<std::string,int> interfaces_to_sockets;
	extern std::mutex interfaces_to_sockets_m;
}

bool State_machine_simulation_core::handle_userdefined_receiver_definition(std::string call_name,
	ceps::ast::Nodeset const & ns)
{
	DEBUG_FUNC_PROLOGUE
	if (call_name == "canbus") {
			std::string channel_id;
            std::string can_bus;
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
            {
                if(!bus_id_.nodes().empty()){
                 if (bus_id_.nodes()[0]->kind() == ceps::ast::Ast_node_kind::int_literal)
					can_bus = std::string("can")+std::to_string(ceps::ast::value(ceps::ast::as_int_ref(bus_id_.nodes()[0])));
                 else if (bus_id_.nodes()[0]->kind() == ceps::ast::Ast_node_kind::string_literal)
					can_bus = ceps::ast::value(ceps::ast::as_string_ref(bus_id_.nodes()[0]));
                 else
					fatal_(-1, "CAN-CAL receiver definition: bus_id must be an integer or string.");
                }

				using namespace ceps::ast;
				auto handlers = ns[all{ "on_msg" }];
				int dispatcher_id = -1;
				auto ctxt = allocate_dispatcher_thread_ctxt(dispatcher_id);
				ctxt->id_=channel_id;
                ctxt->handle() = dispatcher_id;
                ctxt->can_extended() = extended;
                if(extended) ctxt->info()="CANX";
                else ctxt->info()="CAN";
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


				if (start_comm_threads()){
                 if(can_bus.length()) comm_threads.push_back(new std::thread{ comm_receiver_socket_can,
					dispatcher_id,
                    can_bus, this, channel_frame_to_id[channel_id],channel_id, extended , -1});
				 running_as_node() = true;
				}
			}
			return true;
	}
	return false;
}

class Read_CAN_frame : public sm4ceps_plugin_int::Executioncontext {
    std::string channel_id;
    int frame_id;
    char* data;
    int len;
 public:
    Read_CAN_frame() = default;
    Read_CAN_frame(std::string ch_id, char * d,int l,int frid):channel_id{ch_id},frame_id{frid},data{d},len{l}
    {

    }
    void run(State_machine_simulation_core* ctxt){
        std::string frame_name;
        for(auto e : ctxt->channel_frame_name_to_id[channel_id]) if (e.second == frame_id) {frame_name = e.first;break;}
        if (!frame_name.length()) return;
        auto it_frame = ctxt->frame_generators().find(frame_name);
        if (it_frame == ctxt->frame_generators().end()) return;
        std::vector<std::string> params;
        std::vector<ceps::ast::Nodebase_ptr> payload;
        it_frame->second->read_msg(data,len,
                                    ctxt,
                                    params,
                                    payload);
    }
};


constexpr int CAN_MSG_SIZE = 13;
//#define DEBUG std::cout
void comm_receiver_socket_can(int id,
	std::string can_bus,
	State_machine_simulation_core* smc,
    std::unordered_map<int,std::uint32_t> frame2id,
    std::string channel_id,
    bool extended_can_id,
    int sock){
	int s = 0;
    /*
     In the case can_bus is the empty string this communication
     endpoint is virtual and the id parameter holds a valid TCP Stream socket.
     A virtual in channel reads can_frame messages from this socket.
    */

    if(can_bus.length()){
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
    } else s = sock;

	auto current_smc = smc;
	auto in_ctxt = smc->get_dispatcher_thread_ctxt(id);

	in_ctxt->wait_for_start_request();

	int ctr=1;
	for (;;++ctr)
	{
		struct can_frame can_message{0};
		auto r = recv(s, &can_message, sizeof(struct can_frame),0);
		if (r <= 0)
            continue;


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
            for(auto c : frame2id){
                if ((int)can_message.can_id == c.second){
                    //We found a matching frame
                    char* buf = new char[can_message.can_dlc];
                    memcpy(buf,can_message.data, can_message.can_dlc);
                    auto e = new Read_CAN_frame(channel_id,buf,can_message.can_dlc,c.first);
                    State_machine_simulation_core::event_t ev;
                    ev.exec = e;
                    current_smc->enqueue_event(ev);
                    continue;
                }
            }
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

