#pragma once

#include "cepscloud_streaming_common.h"
#include <algorithm>
#include <map>


namespace ceps {
	namespace cloud {
		using namespace std;
		using downstream_hook_t = void(*)(net::can::can_frame frame);
		using upstream_ctrl_t = void(*)(Simulation_Core , Upstream_Mapping );
		using downstream_ctrl_t = void(*)(Simulation_Core , Downstream_Mapping, downstream_hook_t);
		using route_ctrl_t = void(*)(Route);

		

		class Ctrlregistry {
			map<pair<Streamtype, Local_Interface>, downstream_hook_t> downstream_hooks;
			map<pair<Streamtype, Local_Interface>, downstream_ctrl_t> downstream_ctrls;
			map<pair<Streamtype, Local_Interface>, upstream_ctrl_t> upstream_ctrls;
			map<Streamtype, route_ctrl_t> route_ctrls;
		public:
			void reg_down_stream_ctrl(Streamtype,Local_Interface, downstream_ctrl_t,bool overwrite_existing_entry = true);
			void reg_down_stream_hook(Streamtype, Local_Interface, downstream_hook_t, bool overwrite_existing_entry = true);
			void reg_up_stream_ctrl(Streamtype,Local_Interface, upstream_ctrl_t, bool overwrite_existing_entry = true);
			void reg_route_ctrl(Streamtype, route_ctrl_t, bool overwrite_existing_entry = true);

			downstream_ctrl_t get_down_stream_ctrl(Streamtype, Local_Interface);
			downstream_hook_t get_down_stream_hook(Streamtype, Local_Interface);
			upstream_ctrl_t get_up_stream_ctrl(Streamtype, Local_Interface);
			route_ctrl_t get_route_ctrl(Streamtype);
		};
	}
}
