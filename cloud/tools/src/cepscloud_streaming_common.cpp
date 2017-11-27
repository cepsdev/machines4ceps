#include "cepscloud_streaming_common.h"


std::string ceps::cloud::get_down_stream_type(ceps::cloud::Downstream_Mapping_ex dm) {
	for (auto e : ceps::cloud::info_out_channels[ceps::cloud::sim_core(dm)]) {
		if (e.first == ceps::cloud::down_stream(dm).second)
			return e.second;	
	}
	return {};
}

std::string ceps::cloud::get_up_stream_type(ceps::cloud::Upstream_Mapping_ex um) {
	for (auto e : ceps::cloud::info_in_channels[ceps::cloud::sim_core(um)]) {
		if (e.first == ceps::cloud::up_stream(um).second)
			return e.second;
	}
	return {};
}


std::string ceps::cloud::get_down_stream_type(Simulation_Core sim_core, std::string channel) {
	for (auto e : ceps::cloud::info_out_channels[sim_core]) {
		if (e.first == channel)
			return e.second;
	}
	return {};
}

std::string ceps::cloud::get_up_stream_type(Simulation_Core sim_core, std::string channel) {
	for (auto e : ceps::cloud::info_in_channels[sim_core]) {
		if (e.first == channel)
			return e.second;
	}
	return {};
}
