/*
Copyright 2014,2015,2016,2017,2018,2019,2020,2021 Tomas Prerovsky (cepsdev@hotmail.com).

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



#define _CRT_SECURE_NO_WARNINGS

#include "core/include/base_defs.hpp"
#include "core/include/state_machine_simulation_core.hpp"

#ifdef __gnu_linux__

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <dlfcn.h>

#else

#endif
#include <sys/types.h>
#include <limits>
#include <cstring>
#include <cassert>
#include <algorithm>
#include <unordered_set>

bool sm4ceps::valid(State_machine_simulation_core::signal_generator_handle const & i){
	return i >= 0;
}

State_machine_simulation_core::signal_generator_handle State_machine_simulation_core::add_sig_gen(std::string id,sm4ceps::datasources::Signalgenerator const & sig){
 for(std::size_t i = 0; i != sig_generators_.size();++i){
	auto& e = sig_generators_[i];
	if (e.first != id) continue;
	e.second = sig;
	return i;
 }
 sig_generators_.push_back(std::make_pair(id,sig));
 return sig_generators_.size() - 1;
}

State_machine_simulation_core::signal_generator_handle State_machine_simulation_core::find_sig_gen(std::string id){
 for(std::size_t i = 0; i != sig_generators_.size();++i){
	auto const & e = sig_generators_[i];
	if (e.first != id) continue;
	return i;
 }
 return -1;
}

sm4ceps::datasources::Signalgenerator* State_machine_simulation_core::sig_gen(State_machine_simulation_core::signal_generator_handle h){
 if (!sm4ceps::valid(h)) return nullptr;
 return &sig_generators_[h].second;
}


void State_machine_simulation_core::build_signal_structures(Result_process_cmd_line const& result_cmd_line){
 using namespace ceps::ast;
 auto& ns = current_universe();
 for(auto e : ns.nodes()){
	 if (e->kind() != Ast_node_kind::structdef || name(as_struct_ref(e)) != "signal" ) continue;
	 std::string id;
	 for (auto f : as_struct_ref(e).children()){
	  if (f->kind() != Ast_node_kind::binary_operator) continue;
	  auto& bop = as_binop_ref(f);
	  if (op(bop) != '=') continue;
	  if (bop.left()->kind() != Ast_node_kind::identifier || bop.right()->kind() != Ast_node_kind::identifier) continue;
	  if (name(as_id_ref(bop.left())) != "id") continue;
	  id = name(as_id_ref(bop.right()));
	 }

	 sm4ceps::datasources::Signalgenerator& sig = *sig_gen(add_sig_gen(id,sm4ceps::datasources::Signalgenerator{}));

	 for (auto f : as_struct_ref(e).children()){
	   if (f->kind() != Ast_node_kind::binary_operator) continue;
	   auto& bop = as_binop_ref(f);
	   if (op(bop) != '=') continue;
	   if (bop.left()->kind() != Ast_node_kind::identifier || bop.right()->kind() != Ast_node_kind::float_literal) continue;
	   if (name(as_id_ref(bop.left())) != "delta_t") continue;
	   sig.delta() = value(as_double_ref(bop.right()));
	 }

	 for (auto f : as_struct_ref(e).children()){
		if (f->kind() != Ast_node_kind::structdef || name(as_struct_ref(f)) != "values" ) continue;
		auto& values =  as_struct_ref(f).children();
		for(auto v : values){
         if (v->kind() == Ast_node_kind::int_literal) sig.values().push_back(value(as_int_ref(v)));
         else if (v->kind() == Ast_node_kind::float_literal) sig.values().push_back(value(as_double_ref(v)));
		}
	 }

 }

 if (result_cmd_line.print_signal_generators){
	 std::stringstream out;
	 out << "Registered Signal Generators:\n";

	 for(std::size_t i = 0; i!= sig_generators_.size();++i){
	  auto& sig = sig_generators_[i].second;
      out << " '" << sig_generators_[i].first << "', id = " << i << ", delta_t = " << sig.delta()
          << "\n";
      out << " values =";
      for(auto v : sig.values()) out << " "<< v;
      out << "\n";
	 }

	 if(live_logger()) live_logger_out()->log_console(out.str()); else std::cout << out.str();
 }
}




