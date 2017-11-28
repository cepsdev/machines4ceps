#include "vcanstreams.h"
using namespace std;
using namespace ceps::cloud;

void ceps::cloud::Ctrlregistry::reg_down_stream_ctrl(Streamtype t, Local_Interface i, downstream_ctrl_t f, bool overwrite_existing_entry) {
	auto & cont = downstream_ctrls;
	if (overwrite_existing_entry) {
		if (f == nullptr) cont.erase(make_pair(t, i)); else cont[make_pair(t, i)] = f;
	}
	else {
		auto it = cont.find(make_pair(t, i));
		if (it != cont.end()) return;
		it->second = f;
	}
}
void ceps::cloud::Ctrlregistry::reg_up_stream_ctrl(Streamtype t, Local_Interface i, upstream_ctrl_t f, bool overwrite_existing_entry) {
	auto & cont = upstream_ctrls;
	if (overwrite_existing_entry) {
		if (f == nullptr) cont.erase(make_pair(t, i)); else cont[make_pair(t, i)] = f;
	}
	else {
		auto it = cont.find(make_pair(t, i));
		if (it != cont.end()) return;
		it->second = f;
	}
}
void ceps::cloud::Ctrlregistry::reg_route_ctrl(Streamtype t, route_ctrl_t f, bool overwrite_existing_entry) {
	auto & cont = route_ctrls;
	if (overwrite_existing_entry) {
		if (f == nullptr) cont.erase(t); else cont[t] = f;
	}
	else {
		auto it = cont.find(t);
		if (it != cont.end()) return;
		it->second = f;
	}
}

downstream_ctrl_t ceps::cloud::Ctrlregistry::get_down_stream_ctrl(Streamtype t , Local_Interface i) {
	auto it = downstream_ctrls.find(make_pair(t,i));
	if (it == downstream_ctrls.end()) return nullptr;
	return it->second;
}

upstream_ctrl_t ceps::cloud::Ctrlregistry::get_up_stream_ctrl(Streamtype t, Local_Interface i) {
	auto it = upstream_ctrls.find(make_pair(t, i));
	if (it == upstream_ctrls.end()) return nullptr;
	return it->second;
}
route_ctrl_t ceps::cloud::Ctrlregistry::get_route_ctrl(Streamtype t) {
	auto it = route_ctrls.find(t);
	if (it == route_ctrls.end()) return nullptr;
	return it->second;
}


void ceps::cloud::Ctrlregistry::reg_down_stream_hook(Streamtype t, Local_Interface i, downstream_hook_t f, bool overwrite_existing_entry) {
	auto & cont = downstream_hooks;
	if (overwrite_existing_entry) {
		if (f == nullptr) cont.erase(make_pair(t, i)); else cont[make_pair(t, i)] = f;
	}
	else {
		auto it = cont.find(make_pair(t, i));
		if (it != cont.end()) return;
		it->second = f;
	}
}

downstream_hook_t ceps::cloud::Ctrlregistry::get_down_stream_hook(Streamtype t, Local_Interface i) {
	auto it = downstream_hooks.find(make_pair(t, i));
	if (it == downstream_hooks.end()) return nullptr;
	return it->second;
}