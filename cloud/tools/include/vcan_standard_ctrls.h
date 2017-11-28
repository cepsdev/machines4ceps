#pragma once

#include "cepscloud_streaming_common.h"
#include "vcanstreams.h"

#ifdef PCAN_API
namespace pcan_api {
	void downstream_ctrl(
		ceps::cloud::Simulation_Core sim_core,
		ceps::cloud::Downstream_Mapping dm,
		ceps::cloud::downstream_hook_t);
	void upstream_ctrl(
		ceps::cloud::Simulation_Core sim_core,
		ceps::cloud::Upstream_Mapping um);
}
#endif

#ifdef KMW_MULTIBUS_API
namespace kmw_api {
	void downstream_ctrl(
		ceps::cloud::Simulation_Core sim_core,
		ceps::cloud::Downstream_Mapping dm,
		ceps::cloud::downstream_hook_t);
	void upstream_ctrl(
		ceps::cloud::Simulation_Core sim_core,
		ceps::cloud::Upstream_Mapping um);
}
#endif

void setup_shared_libs();
void setup_stream_ctrls(std::vector<std::string> local_ifs);
extern std::map<std::string, net::can::can_info::BAUD_RATE>str_br2id;
extern ceps::cloud::Ctrlregistry global_ctrlregistry;
