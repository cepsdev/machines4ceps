/*
Copyright 2021 Tomas Prerovsky (cepsdev@hotmail.com).

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


#include "core/include/sm_raw_frame.hpp"
#include "core/include/state_machine_simulation_core.hpp"

#include <sys/types.h>
#include <limits>
#include <cstring>

#include "core/include/base_defs.hpp"
#include "core/include/websocket.hpp"

const int Rawframe_generator::IS_ASCII = 1;
const int Rawframe_generator::IS_UTF8 = 2;
const int Rawframe_generator::IS_BINARY = 4;
const int Rawframe_generator::IS_JSON = 8;

extern ceps::ast::Nodebase_ptr eval_locked_ceps_expr(State_machine_simulation_core* smc,
		 State_machine* containing_smp,
		 ceps::ast::Nodebase_ptr node,
		 ceps::ast::Nodebase_ptr root_node);

ceps::ast::Nodebase_ptr ceps_interface_eval_func_callback(std::string const & id, ceps::ast::Call_parameters* params, void* context);
ceps::ast::Nodebase_ptr ceps_interface_binop_resolver( ceps::ast::Binary_operator_ptr binop,
	 	 	 	  	  	  	  	  	  	  	  	  	  	  	  ceps::ast::Nodebase_ptr lhs ,
	 	 	 	  	  	  	  	  	  	  	  	  	  	  	  ceps::ast::Nodebase_ptr rhs,
	 	 	 	  	  	  	  	  	  	  	  	  	  	  	  void* cxt,ceps::ast::Nodebase_ptr parent_node);

bool read_func_call_values(State_machine_simulation_core* smc,	ceps::ast::Nodebase_ptr root_node,
							std::string & func_name,
							std::vector<ceps::ast::Nodebase_ptr>& args);

bool Podframe_generator::readfrom_spec(ceps::ast::Nodeset const & spec)
{
 spec_ = spec;
 return true;
}

size_t Podframe_generator::header_length() {
	return header_length_;
}


static std::map<std::string,std::pair<unsigned short, bool>> typename2descr=
{
		{"bit",{1,true}},{"byte",{8,true}},
		{"int1",{1,true}},{"int2",{2,true}},{"int3",{3,true}},{"int4",{4,true}},{"int5",{5,true}},{"int6",{6,true}},{"int7",{7,true}},{"int8",{8,true}},
		{"int9",{9,true}},{"int10",{10,true}},{"int11",{11,true}},{"int12",{12,true}},{"int13",{13,true}},{"int14",{14,true}},{"int15",{15,true}},{"int16",{16,true}},
		{"int17",{17,true}},{"int18",{18,true}},{"int19",{19,true}},{"int20",{20,true}},{"int21",{21,true}},{"int22",{22,true}},{"int23",{23,true}},{"int24",{24,true}},
		{"int25",{25,true}},{"int26",{26,true}},{"int27",{27,true}},{"int28",{28,true}},{"int29",{29,true}},{"int30",{30,true}},{"int31",{31,true}},{"int32",{32,true}},
		{"int33",{33,true}},{"int34",{34,true}},{"int35",{35,true}},{"int36",{36,true}},{"int37",{37,true}},{"int38",{38,true}},{"int39",{39,true}},{"int40",{40,true}},
		{"int41",{41,true}},{"int42",{42,true}},{"int43",{43,true}},{"int44",{44,true}},{"int45",{45,true}},{"int46",{46,true}},{"int47",{47,true}},{"int48",{48,true}},
		{"int49",{49,true}},{"int50",{50,true}},{"int51",{51,true}},{"int52",{52,true}},{"int53",{53,true}},{"int54",{54,true}},{"int55",{55,true}},{"int56",{56,true}},
		{"int57",{57,true}},{"int58",{58,true}},{"int59",{59,true}},{"int60",{60,true}},{"int61",{61,true}},{"int62",{62,true}},{"int63",{63,true}},{"int64",{64,true}},

		{"uint1",{1,false}},{"uint2",{2,false}},{"uint3",{3,false}},{"uint4",{4,false}},{"uint5",{5,false}},{"uint6",{6,false}},{"uint7",{7,false}},{"uint8",{8,false}},
		{"uint9",{9,false}},{"uint10",{10,false}},{"uint11",{11,false}},{"uint12",{12,false}},{"uint13",{13,false}},{"uint14",{14,false}},{"uint15",{15,false}},{"uint16",{16,false}},
		{"uint17",{17,false}},{"uint18",{18,false}},{"uint19",{19,false}},{"uint20",{20,false}},{"uint21",{21,false}},{"uint22",{22,false}},{"uint23",{23,false}},{"uint24",{24,false}},
		{"uint25",{25,false}},{"uint26",{26,false}},{"uint27",{27,false}},{"uint28",{28,false}},{"uint29",{29,false}},{"uint30",{30,false}},{"uint31",{31,false}},{"uint32",{32,false}},
		{"uint33",{33,false}},{"uint34",{34,false}},{"uint35",{35,false}},{"uint36",{36,false}},{"uint37",{37,false}},{"uint38",{38,false}},{"uint39",{39,false}},{"uint40",{40,false}},
		{"uint41",{41,false}},{"uint42",{42,false}},{"uint43",{43,false}},{"uint44",{44,false}},{"uint45",{45,false}},{"uint46",{46,false}},{"uint47",{47,false}},{"uint48",{48,false}},
		{"uint49",{49,false}},{"uint50",{50,false}},{"uint51",{51,false}},{"uint52",{52,false}},{"uint53",{53,false}},{"uint54",{54,false}},{"uint55",{55,false}},{"uint56",{56,false}},
		{"uint57",{57,false}},{"uint58",{58,false}},{"uint59",{59,false}},{"uint60",{60,false}},{"uint61",{61,false}},{"uint62",{62,false}},{"uint63",{63,false}},{"uint64",{64,false}}


};

size_t fill_raw_chunk(std::map<std::string /*systemstate*/,
 std::map< int, ceps::ast::Nodebase_ptr> > const & encoding,
 size_t & header_length, State_machine_simulation_core* smc,
 std::vector<ceps::ast::Nodebase_ptr> pattern,
 size_t data_size,
 char* data,
 size_t bit_offs,
 std::uint32_t* info,
 size_t bit_width=sizeof(std::int64_t)*8,
 bool signed_value = true,
 bool write_data = true,
 bool host_byte_order = true
);

size_t fill_raw_chunk(
						std::map<std::string /*systemstate*/,
 						std::map< int, ceps::ast::Nodebase_ptr> > const & encoding,
 						size_t & header_length, State_machine_simulation_core* smc,
 						ceps::ast::Nodebase_ptr p,
 						size_t data_size,
 						char* data,
 						size_t bit_offs,
 						std::uint32_t* info,
 						size_t bit_width=sizeof(std::int64_t)*8,
 						bool signed_value = true,
 						bool write_data = true,
 						bool host_byte_order = true) 
{
	using namespace ceps::ast;
	if (p == nullptr) return 0;

	if (p->kind() == ceps::ast::Ast_node_kind::structdef){
	 auto& st = ceps::ast::as_struct_ref(p);
	 auto& nm = ceps::ast::name(st);
	 if (nm == "in"){
	  return 0;
	 } else if (nm == "out"){
	 return fill_raw_chunk(encoding,header_length,smc,st.children(), data_size,data, bit_offs,info, bit_width,signed_value,write_data,host_byte_order);
	}
	auto r = typename2descr.find(nm);
	 if (r != typename2descr.end())
	  return fill_raw_chunk(encoding,header_length,smc,st.children(),data_size,data,bit_offs,info,r->second.first,r->second.second,write_data,host_byte_order);
	 return fill_raw_chunk(encoding,header_length,smc,st.children(), data_size,data, bit_offs,info, bit_width,signed_value,write_data,host_byte_order);
	}
	else if (p->kind() == ceps::ast::Ast_node_kind::identifier){
		auto& ident = ceps::ast::as_id_ref(p);
		if (ceps::ast::name(ident) == "any") return bit_width;
		else if (ceps::ast::name(ident) == "__current_frame_size") {
			auto temp_node = ceps::ast::Int(data_size,ceps::ast::all_zero_unit(), nullptr, nullptr, nullptr);
			return fill_raw_chunk(encoding,header_length,smc,&temp_node,data_size, data, bit_offs,info, bit_width,signed_value,write_data,host_byte_order);
		} else smc->fatal_(-1,std::string("Raw frame:  Unknown identifier '")+ceps::ast::name(ident)+"'");
	}else if (p->kind() == ceps::ast::Ast_node_kind::int_literal || p->kind() == ceps::ast::Ast_node_kind::float_literal){
		if (write_data){
			if (info) *info |= Rawframe_generator::IS_BINARY;
			std::uint64_t v;
			if (p->kind() == ceps::ast::Ast_node_kind::int_literal) v = (std::uint64_t)value(as_int_ref(p));
			else v = (std::uint64_t)value(as_double_ref(p));

			if (bit_width < 64){
			   if (bit_width == 1){ v =  (v ? 1 : 0) ;  }
			   else if (bit_width == 4) v &= 0xFLL;
			   else if (bit_width == 8) v &= 0xFFLL;
			   else if (bit_width == 12) v &= 0xFFFLL;
			   else if (bit_width == 16) v &= 0xFFFFLL;
			   else if (bit_width == 20) v &= 0xFFFFFLL;
			   else if (bit_width == 24) v &= 0xFFFFFFLL;
			   else if (bit_width == 28) v &= 0xFFFFFFFLL;
			   else if (bit_width == 32) v &= 0xFFFFFFFFLL;
			   else{
				   std::uint64_t w=1;
				   for(size_t i = 0; i < bit_width;++i) w |= (w << 1);
				   v &= w;
			   }
			}
			int bits_written = 0;
			if (bit_offs % 8) {
				unsigned short o = bit_offs % 8;
				unsigned short d = 8 - o ; // d is the number of bits left in the byte to write
				unsigned char w = *(data + bit_offs/8);
				if (d > bit_width) d = (unsigned short) bit_width;
				unsigned char c1=1;for(int i = 0; i < d;++i)c1 |= (c1 << 1);
				unsigned char c2=1;for(int i = 0; i < o-1;++i)c2 |= (c2 << 1);

				w = (w & c2) | ( ((unsigned char)v & c1) << o );
				*( (unsigned char*)( data + bit_offs/8)) = w;
				bits_written = d;bit_offs+=d;
			}
			if (bits_written) v = v >> bits_written;

			//INVARIANT: bit_offs points to a byte address
			if (bits_written < (int) bit_width){
				for(; bit_width-bits_written>=8;bit_offs+=8,bits_written+=8){
					*( (unsigned char*)data+bit_offs/8) = (unsigned char) v;
				    v = v >> 8;
				}
			}
			//INVARIANT: bit_offs points to a byte address
			//INVARIANT: bit_width - bits_written < 8
			if (bits_written < (int)bit_width){
				if ((int)bit_width-bits_written  == 1){
					unsigned char & target = *( (unsigned char*)(data+ bit_offs/8) );
					if (v & 1)  target |= 1;
					else target &= 0xFE;
				} else
				{
					unsigned char & target = *( (unsigned char*)(data+ bit_offs/8) );
					unsigned char c = 1;
					for(int i = 0; i < (int)bit_width-bits_written-1; ++i ) c |= c << 1;
					target = c & v;
				}
			}
		}
		return bit_width;
	} else if (p->kind() == ceps::ast::Ast_node_kind::string_literal){
		 if (info) *info |= Rawframe_generator::IS_ASCII;
		 std::string s = ceps::ast::value(ceps::ast::as_string_ref(p));
		 int corr = 0;
		 if (bit_offs % 8) corr = 8 - (bit_offs % 8);
		 bit_offs += corr;
		 if (write_data) memcpy((data+bit_offs/8),s.c_str(),s.length());
		 return s.length()*8+corr;
	} else if (p->kind() == ceps::ast::Ast_node_kind::symbol && "Systemstate" == ceps::ast::kind(ceps::ast::as_symbol_ref(p)) ) {
		if(write_data){
		 auto & state = ceps::ast::as_symbol_ref(p);
		 auto it = encoding.find(ceps::ast::name(state));
		 if (it == encoding.end()){
			auto r = eval_locked_ceps_expr(smc,nullptr,&state,nullptr);
			if (r){ 
				return fill_raw_chunk(encoding,header_length,smc,r,data_size, data, bit_offs,info, bit_width,signed_value,write_data,host_byte_order);
			}
		 } else {
			 auto func_it = it->second.find(bit_width*(signed_value?-1:1));
			 if (func_it == it->second.end()){
				 auto r = eval_locked_ceps_expr(smc,nullptr,&state,nullptr);
				 if (r){ 
					 return fill_raw_chunk(encoding,header_length,smc,r,data_size, data, bit_offs,info, bit_width,signed_value,write_data,host_byte_order);
				 }
			 } else {
				 auto func = func_it->second;
				 auto r = eval_locked_ceps_expr(smc,nullptr,func,nullptr);
				 if (r){ 
					 return fill_raw_chunk(encoding,header_length,smc,r,data_size, data, bit_offs,info, bit_width,signed_value,write_data,host_byte_order);
				 }
			 }
		 }
		}
		return bit_width;
	}
	else {std::stringstream ss; ss << *p; smc->fatal_(-1,"Serialization of raw frame: Illformed expression:"+ss.str());}
	return 0;
}

size_t fill_raw_chunk(std::map<std::string /*systemstate*/,
 std::map< int, ceps::ast::Nodebase_ptr> > const & encoding,
 size_t& header_length,State_machine_simulation_core* smc,
 std::vector<ceps::ast::Nodebase_ptr> pattern,
 size_t data_size,
 char* data,
 size_t bit_offs,
 std::uint32_t* info,
 size_t bit_width,
 bool signed_value,
 bool write_data,
 bool host_byte_order){


 size_t r=0;
 for(auto p : pattern){
  bool header = false;
  if (p->kind() == ceps::ast::Ast_node_kind::structdef && !write_data){
   auto& st = ceps::ast::as_struct_ref(p);
   auto& nm = ceps::ast::name(st);
   if (nm == "header") {header = true;}
  }
  auto rr = fill_raw_chunk(encoding,header_length,smc,p,data_size,data,bit_offs,info,bit_width,signed_value,write_data,host_byte_order);
  if (header) {header_length = rr; }
  r+=rr;
  bit_offs += rr;
 }
 return r;
}

size_t compute_size(size_t& header_length,State_machine_simulation_core* smc,std::vector<ceps::ast::Nodebase_ptr> pattern) {
	size_t bits = fill_raw_chunk({},header_length,smc,
            pattern,
            0,
            nullptr,
            0,
			nullptr,
            0,
			true,false,true);
	return (bits + 7) / 8;
}

size_t Podframe_generator::compute_size_of_msg(State_machine_simulation_core* smc,
		                                       std::vector<std::string> params,bool& failed)
{

	auto data_format = spec_["data"];

	failed = false;

	return compute_size(header_length_,smc, data_format.nodes() );
}






Rawframe_generator::gen_msg_return_t Podframe_generator::gen_msg(State_machine_simulation_core* smc,
		                          size_t& data_size,
								  std::map<std::string /*systemstate*/,
								  std::map< int, ceps::ast::Nodebase_ptr> > const & encoding){

 if (smc == nullptr) return gen_msg_return_t{0,nullptr};
 auto data_format = spec_["data"];
 if (data_format.nodes().empty()) {
  std::string id;
  auto t = spec_["id"];
  if (t.nodes().size() != 0 && t.nodes()[0]->kind() == ceps::ast::Ast_node_kind::identifier)
	id = ceps::ast::name(ceps::ast::as_id_ref(t.nodes()[0]));
  return gen_msg_return_t{0,nullptr};
 }

 ceps::ast::Nodebase_ptr frame_pattern = nullptr;
 ceps::ast::Scope scope;
 scope.children() = data_format.nodes();scope.owns_children() = false;
 frame_pattern = eval_locked_ceps_expr(smc,nullptr,&scope,nullptr);

 if (frame_pattern == nullptr) return gen_msg_return_t{0,nullptr};
 auto chunk_size = compute_size(header_length_,smc,ceps::ast::nlf_ptr(frame_pattern)->children());
 data_size = chunk_size;
 char* data = new char[chunk_size];
 bzero(data,chunk_size);
 std::uint32_t info = 0;
 fill_raw_chunk(encoding,header_length_, smc,ceps::ast::nlf_ptr(frame_pattern)->children(),chunk_size, data,0,&info);
 
 scope.children().clear();
 if (info & Rawframe_generator::IS_BINARY) info = Rawframe_generator::IS_BINARY;
 return gen_msg_return_t{info,(void*)data};
}


//////////////////////////////////////////////////////////////////
////////////////////////////////////// READ RAW MESSAGE //////////
//////////////////////////////////////////////////////////////////
int read_raw_chunk(size_t& header_length,State_machine_simulation_core* smc,
		            std::vector<ceps::ast::Nodebase_ptr> pattern,
		            size_t data_size,
		            char* data,
		            size_t bit_offs,
		            size_t bit_width=sizeof(std::int64_t)*8,
		            bool signed_value = true,
		            bool read_data = true,
		            bool host_byte_order = true
		            );

int read_raw_chunk(size_t& header_length, State_machine_simulation_core* smc,
		               ceps::ast::Nodebase_ptr p,
		               size_t data_size,
		               char* data,
		               size_t bit_offs,
		               size_t bit_width=sizeof(std::int64_t)*8,
		               bool signed_value = true,
		               bool read_data = true,
		               bool host_byte_order = true) {
	using namespace ceps::ast;
	if (p == nullptr) return 0;
	//if (write_data) std::cout << "*** bit_width=" << bit_width << std::endl;
	if (p->kind() == ceps::ast::Ast_node_kind::structdef){
		auto& st = ceps::ast::as_struct_ref(p);
		auto& nm = ceps::ast::name(st);
		if (nm == "out"){
		  return 0;
		} else if (nm == "in"){
			return read_raw_chunk(header_length,smc,st.children(), data_size,data, bit_offs, bit_width,signed_value,read_data,host_byte_order);
		} else if (nm == "byte" || nm == "int8"){
			return read_raw_chunk(header_length,smc,st.children(),data_size,data,bit_offs,8,true,read_data,host_byte_order);
		} else if (nm == "ubyte" || nm == "uint8"){
			return read_raw_chunk(header_length,smc,st.children(),data_size,data,bit_offs,8,false,read_data,host_byte_order);
		} else if (nm == "ushort" || nm == "uint16"){
			return read_raw_chunk(header_length,smc,st.children(),data_size,data,bit_offs,16,false,read_data,host_byte_order);
		} else if (nm == "short" || nm == "int16"){
			return read_raw_chunk(header_length,smc,st.children(),data_size,data,bit_offs,16,true,read_data,host_byte_order);
		} else if (nm == "uint" || nm == "uint32"){
			return read_raw_chunk(header_length,smc,st.children(),data_size,data,bit_offs,32,false,read_data,host_byte_order);
		} else if (nm == "int" || nm == "int32") {
			return read_raw_chunk(header_length,smc,st.children(),data_size,data,bit_offs,32,true,read_data,host_byte_order);
		} else if (nm == "ulonglong" || nm == "uint64") {
			return read_raw_chunk(header_length,smc,st.children(),data_size,data,bit_offs,64,false,read_data,host_byte_order);
		} else if (nm == "longlong" || nm == "int64") {
			return read_raw_chunk(header_length,smc,st.children(),data_size,data,bit_offs,64,true,read_data,host_byte_order);
		}

		else if (nm == "uint25"){
			return read_raw_chunk(header_length,smc,st.children(),data_size,data,bit_offs,25,false,read_data,host_byte_order);
		}		else if (nm == "uint26"){
			return read_raw_chunk(header_length,smc,st.children(),data_size,data,bit_offs,26,false,read_data,host_byte_order);
		}		else if (nm == "uint27"){
			return read_raw_chunk(header_length,smc,st.children(),data_size,data,bit_offs,27,false,read_data,host_byte_order);
		}		else if (nm == "uint28"){
			return read_raw_chunk(header_length,smc,st.children(),data_size,data,bit_offs,28,false,read_data,host_byte_order);
		}		else if (nm == "uint29"){
			return read_raw_chunk(header_length,smc,st.children(),data_size,data,bit_offs,29,false,read_data,host_byte_order);
		}		else if (nm == "uint30"){
			return read_raw_chunk(header_length,smc,st.children(),data_size,data,bit_offs,30,false,read_data,host_byte_order);
		}		else if (nm == "uint31"){
			return read_raw_chunk(header_length,smc,st.children(),data_size,data,bit_offs,31,false,read_data,host_byte_order);
		} 		else if (nm == "int25"){
			return read_raw_chunk(header_length,smc,st.children(),data_size,data,bit_offs,25,true,read_data,host_byte_order);
		}		else if (nm == "int26"){
			return read_raw_chunk(header_length,smc,st.children(),data_size,data,bit_offs,26,true,read_data,host_byte_order);
		}		else if (nm == "int27"){
			return read_raw_chunk(header_length,smc,st.children(),data_size,data,bit_offs,27,true,read_data,host_byte_order);
		}		else if (nm == "int28"){
			return read_raw_chunk(header_length,smc,st.children(),data_size,data,bit_offs,28,true,read_data,host_byte_order);
		}		else if (nm == "int29"){
			return read_raw_chunk(header_length,smc,st.children(),data_size,data,bit_offs,29,true,read_data,host_byte_order);
		}		else if (nm == "int30"){
			return read_raw_chunk(header_length,smc,st.children(),data_size,data,bit_offs,30,true,read_data,host_byte_order);
		}		else if (nm == "uint31"){
			return read_raw_chunk(header_length,smc,st.children(),data_size,data,bit_offs,31,true,read_data,host_byte_order);
		}







		else if (nm == "uint24"){
			return read_raw_chunk(header_length,smc,st.children(),data_size,data,bit_offs,24,false,read_data,host_byte_order);
		} else if (nm == "int24"){
			return read_raw_chunk(header_length,smc,st.children(),data_size,data,bit_offs,24,true,read_data,host_byte_order);
		}else if (nm == "uint23"){
			return read_raw_chunk(header_length,smc,st.children(),data_size,data,bit_offs,23,false,read_data,host_byte_order);
		} else if (nm == "int23"){
			return read_raw_chunk(header_length,smc,st.children(),data_size,data,bit_offs,23,true,read_data,host_byte_order);
		}else if (nm == "uint22"){
			return read_raw_chunk(header_length,smc,st.children(),data_size,data,bit_offs,22,false,read_data,host_byte_order);
		} else if (nm == "int22"){
			return read_raw_chunk(header_length,smc,st.children(),data_size,data,bit_offs,22,true,read_data,host_byte_order);
		}else if (nm == "uint21"){
			return read_raw_chunk(header_length,smc,st.children(),data_size,data,bit_offs,21,false,read_data,host_byte_order);
		} else if (nm == "int21"){
			return read_raw_chunk(header_length,smc,st.children(),data_size,data,bit_offs,21,true,read_data,host_byte_order);
		}else if (nm == "uint20"){
			return read_raw_chunk(header_length,smc,st.children(),data_size,data,bit_offs,20,false,read_data,host_byte_order);
		} else if (nm == "int20"){
			return read_raw_chunk(header_length,smc,st.children(),data_size,data,bit_offs,20,true,read_data,host_byte_order);
		}else if (nm == "uint19"){
			return read_raw_chunk(header_length,smc,st.children(),data_size,data,bit_offs,19,false,read_data,host_byte_order);
		} else if (nm == "int19"){
			return read_raw_chunk(header_length,smc,st.children(),data_size,data,bit_offs,19,true,read_data,host_byte_order);
		}else if (nm == "uint18"){
			return read_raw_chunk(header_length,smc,st.children(),data_size,data,bit_offs,18,false,read_data,host_byte_order);
		} else if (nm == "int18"){
			return read_raw_chunk(header_length,smc,st.children(),data_size,data,bit_offs,18,true,read_data,host_byte_order);
		}else if (nm == "uint17"){
			return read_raw_chunk(header_length,smc,st.children(),data_size,data,bit_offs,17,false,read_data,host_byte_order);
		} else if (nm == "int17"){
			return read_raw_chunk(header_length,smc,st.children(),data_size,data,bit_offs,17,true,read_data,host_byte_order);
		}else if (nm == "uint15"){
			return read_raw_chunk(header_length,smc,st.children(),data_size,data,bit_offs,15,false,read_data,host_byte_order);
		} else if (nm == "int15"){
			return read_raw_chunk(header_length,smc,st.children(),data_size,data,bit_offs,15,true,read_data,host_byte_order);
		} else if (nm == "uint14"){
			return read_raw_chunk(header_length,smc,st.children(),data_size,data,bit_offs,14,false,read_data,host_byte_order);
		} else if (nm == "int14"){
			return read_raw_chunk(header_length,smc,st.children(),data_size,data,bit_offs,14,true,read_data,host_byte_order);
		}else if (nm == "uint13"){
			return read_raw_chunk(header_length,smc,st.children(),data_size,data,bit_offs,13,false,read_data,host_byte_order);
		} else if (nm == "int13"){
			return read_raw_chunk(header_length,smc,st.children(),data_size,data,bit_offs,13,true,read_data,host_byte_order);
		}else if (nm == "uint12"){
			return read_raw_chunk(header_length,smc,st.children(),data_size,data,bit_offs,12,false,read_data,host_byte_order);
		} else if (nm == "int12"){
			return read_raw_chunk(header_length,smc,st.children(),data_size,data,bit_offs,12,true,read_data,host_byte_order);
		}else if (nm == "uint11"){
			return read_raw_chunk(header_length,smc,st.children(),data_size,data,bit_offs,11,false,read_data,host_byte_order);
		} else if (nm == "int11"){
			return read_raw_chunk(header_length,smc,st.children(),data_size,data,bit_offs,11,true,read_data,host_byte_order);
		} else if (nm == "uint10"){
			return read_raw_chunk(header_length,smc,st.children(),data_size,data,bit_offs,10,false,read_data,host_byte_order);
		} else if (nm == "int10"){
			return read_raw_chunk(header_length,smc,st.children(),data_size,data,bit_offs,10,true,read_data,host_byte_order);
		}else if (nm == "uint9"){
			return read_raw_chunk(header_length,smc,st.children(),data_size,data,bit_offs,9,false,read_data,host_byte_order);
		} else if (nm == "int9"){
			return read_raw_chunk(header_length,smc,st.children(),data_size,data,bit_offs,9,true,read_data,host_byte_order);
		}else if (nm == "uint7"){
			return read_raw_chunk(header_length,smc,st.children(),data_size,data,bit_offs,7,false,read_data,host_byte_order);
		} else if (nm == "int7"){
			return read_raw_chunk(header_length,smc,st.children(),data_size,data,bit_offs,7,true,read_data,host_byte_order);
		} else if (nm == "uint6"){
			return read_raw_chunk(header_length,smc,st.children(),data_size,data,bit_offs,6,false,read_data,host_byte_order);
		} else if (nm == "int6"){
			return read_raw_chunk(header_length,smc,st.children(),data_size,data,bit_offs,6,true,read_data,host_byte_order);
		} else if (nm == "uint5"){
			return read_raw_chunk(header_length,smc,st.children(),data_size,data,bit_offs,5,false,read_data,host_byte_order);
		} else if (nm == "int5"){
			return read_raw_chunk(header_length,smc,st.children(),data_size,data,bit_offs,8,true,read_data,host_byte_order);
		} else if (nm == "uint4"){
			return read_raw_chunk(header_length,smc,st.children(),data_size,data,bit_offs,4,false,read_data,host_byte_order);
		} else if (nm == "int4"){
			return read_raw_chunk(header_length,smc,st.children(),data_size,data,bit_offs,4,true,read_data,host_byte_order);
		} else if (nm == "uint3"){
			return read_raw_chunk(header_length,smc,st.children(),data_size,data,bit_offs,3,false,read_data,host_byte_order);
		} else if (nm == "int3"){
			return read_raw_chunk(header_length,smc,st.children(),data_size,data,bit_offs,3,true,read_data,host_byte_order);
		} else if (nm == "uint2"){
			return read_raw_chunk(header_length,smc,st.children(),data_size,data,bit_offs,2,false,read_data,host_byte_order);
		} else if (nm == "int2"){
			return read_raw_chunk(header_length,smc,st.children(),data_size,data,bit_offs,2,true,read_data,host_byte_order);
		} else if (nm == "bit"){ //std::cout << "------\n";
			return read_raw_chunk(header_length,smc,st.children(),data_size,data,bit_offs,1,true,read_data,host_byte_order);
		} else if (nm == "header"){ //std::cout << "------\n";
			return header_length = read_raw_chunk(header_length,smc,st.children(),data_size,data,bit_offs,1,true,read_data,host_byte_order);
		} else {
			return read_raw_chunk(header_length,smc,st.children(), data_size,data, bit_offs, bit_width,signed_value,read_data,host_byte_order);
		}
	}
	else if (p->kind() == ceps::ast::Ast_node_kind::identifier){
		auto& ident = ceps::ast::as_id_ref(p);
		if (ceps::ast::name(ident) == "any") return bit_width;
		else if (ceps::ast::name(ident) == "__current_frame_size") {
			auto temp_node = ceps::ast::Int(data_size,ceps::ast::all_zero_unit(), nullptr, nullptr, nullptr);
			return read_raw_chunk(header_length,smc,&temp_node,data_size, data, bit_offs, bit_width,signed_value,read_data,host_byte_order);
		} else smc->fatal_(-1,std::string("Raw frame:  Unknown identifier '")+ceps::ast::name(ident)+"'");
	}else if (p->kind() == ceps::ast::Ast_node_kind::int_literal
			|| node_isrw_state(p)/*(p->kind() == ceps::ast::Ast_node_kind::symbol && ceps::ast::kind(ceps::ast::as_symbol_ref(p) ) == "Systemstate"  )*/ ){
		if (read_data){
			std::uint64_t v = 0;

			int bits_read = 0;
			if (bit_offs % 8) {
				unsigned short o = bit_offs % 8;
				unsigned short d = 8 - o ; // d is the number of bits left in the byte to read
				v = *( ( (unsigned char*) data) + bit_offs/8);
				v = v >> o;
				if (d > bit_width) {
				 unsigned char c1=1;for(int i = 1; i < (int)bit_width;++i)c1 |= (c1 << 1);
				 v &= c1;
				 d = (unsigned short) bit_width;
				}
				bits_read = d;bit_offs+=d;
			}

			//INVARIANT: bit_offs points to a byte address
			int bits_read_after_byte_boundary=0;
			if (bits_read< (int)bit_width){
				std::uint64_t v_temp = 0;
				for(; bit_width-bits_read>=8;bit_offs+=8,bits_read+=8,bits_read_after_byte_boundary+=8){
					v_temp |=  ((std::uint64_t) *( ( (unsigned char*) data) +bit_offs/8)) << bits_read;
				}
				v |= v_temp;
			}
			//INVARIANT: bit_offs points to a byte address
			//INVARIANT: bit_width - bits_written < 8
			if (bits_read < (int)bit_width){
 			 std::uint64_t target = *( (unsigned char*)(data+ bit_offs/8) );
 			 int bit_left = bit_width-bits_read;
 			 std::uint64_t ch=1;
 			 for(int i = 1; i < bit_left;++i ) ch |= ch << 1;
 			 target &= ch;
 			 target = target << bits_read_after_byte_boundary;
 			 v |= target;
			}
			//std::cout << "bit offset:"<< orig_bit_offs <<"; bit_width:"<< bit_width<<"; " << v << "; " << ceps::ast::value(ceps::ast::as_int_ref(p)) << std::endl;
			if (p->kind() == ceps::ast::Ast_node_kind::int_literal)
			{
				if (bit_width == 1)
				{
				 if (ceps::ast::value(ceps::ast::as_int_ref(p)) != 0){
					if (v == 0) return -2;
				 } else if (v!=0) return -2;
			    }
			    else if ((std::int64_t)v != ceps::ast::value(ceps::ast::as_int_ref(p))) return -2;
			} else {
				std::lock_guard<std::recursive_mutex>g(smc->states_mutex());
				std::string s;
				auto w = smc->get_global_states()[s = ceps::ast::name(ceps::ast::as_symbol_ref(p))];
				smc->global_systemstates_prev()[s] = w;
                if (signed_value){
                    if (bit_width == 8) v = (std::int8_t)v;
                    else if (bit_width == 16) v = (std::int16_t)v;
                    else if (bit_width == 32) v = (std::int32_t)v;
                    else {
                        std::int64_t u = -1;
                        std::uint64_t uu = u;
                        uu = uu << bit_width;
                        v = ((std::uint64_t)v) | uu;
                    }
                }


				if (w == nullptr || w->kind() != ceps::ast::Ast_node_kind::int_literal) 
					smc->get_global_states()[s] =
						new ceps::ast::Int((std::int64_t)v, ceps::ast::all_zero_unit(), nullptr, nullptr, nullptr);
				else {
					auto old_value = ceps::ast::value(ceps::ast::as_int_ref(smc->get_global_states()[s]));
					if (old_value != (std::int64_t)v) smc->global_systemstates_prev()[s] = nullptr;
					ceps::ast::value(ceps::ast::as_int_ref(smc->get_global_states()[s])) = (std::int64_t)v;
				}
			}
		}
		return bit_width;
	}
	else if (p->kind() == ceps::ast::Ast_node_kind::float_literal) {
		 smc->fatal_(-1,"Floating point numbers not supported in raw frames.");
	}
	else if (p->kind() == ceps::ast::Ast_node_kind::string_literal){
		 smc->fatal_(-1,"Strings not supported in raw frames.");
	}
	else {std::stringstream ss; ss << *p; smc->fatal_(-1,"Reading of raw frame: Illformed expression:"+ss.str());}
	return 0;
}

int read_raw_chunk(size_t& header_length,State_machine_simulation_core* smc,
		            std::vector<ceps::ast::Nodebase_ptr> pattern,
		            size_t data_size,
		            char* data,
		            size_t bit_offs,
		            size_t bit_width,
		            bool signed_value,
		            bool read_data,
		            bool host_byte_order
		            ){
	size_t r=0;
	for(auto p : pattern){
		auto rr = read_raw_chunk(header_length,smc,p,data_size,data,bit_offs,bit_width,signed_value,read_data,host_byte_order);
		if (rr < 0) return rr;
		r+=rr;
		bit_offs += rr;
	}
	return r;
}


bool Podframe_generator::read_msg(char* data,size_t size,
		                          State_machine_simulation_core* smc,
                                  std::vector<std::string> ,
                                  std::vector<ceps::ast::Nodebase_ptr>& )
{
	if (data == nullptr || size == 0) return true;

	auto data_format = spec_["data"];
	if (data_format.nodes().empty()) {
		std::string id;
		auto t = spec_["id"];
		if (t.nodes().size() != 0 && t.nodes()[0]->kind() == ceps::ast::Ast_node_kind::identifier)
			id = ceps::ast::name(ceps::ast::as_id_ref(t.nodes()[0]));
		smc->fatal_(-1,"Frame '"+id+"' doesn't contain a data section.");
	}
	if (0 > read_raw_chunk(header_length_, smc,data_format.nodes(),size, data,0)) return false;
	return true;
}










