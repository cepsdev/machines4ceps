#include "core/include/sm_raw_frame.hpp"
#include "core/include/state_machine_simulation_core.hpp"
#include <sys/types.h>
#include <limits>
#include <cstring>


#include "core/include/base_defs.hpp"


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



size_t fill_raw_chunk(State_machine_simulation_core* smc,
		            std::vector<ceps::ast::Nodebase_ptr> pattern,
		            size_t data_size,
		            char* data,
		            size_t bit_offs,
		            size_t bit_width=sizeof(std::int64_t)*8,
		            bool signed_value = true,
		            bool write_data = true,
		            bool host_byte_order = true
		            );

size_t fill_raw_chunk( State_machine_simulation_core* smc,
		               ceps::ast::Nodebase_ptr p,
		               size_t data_size,
		               char* data,
		               size_t bit_offs,
		               size_t bit_width=sizeof(std::int64_t)*8,
		               bool signed_value = true,
		               bool write_data = true,
		               bool host_byte_order = true) {
	using namespace ceps::ast;
	if (p == nullptr) return 0;
	//if (write_data) std::cout << "*** bit_width=" << bit_width << std::endl;
	if (p->kind() == ceps::ast::Ast_node_kind::structdef){
		auto& st = ceps::ast::as_struct_ref(p);
		auto& nm = ceps::ast::name(st);
		if (nm == "in"){
		  return 0;
		} else if (nm == "out"){
			return fill_raw_chunk(smc,st.children(), data_size,data, bit_offs, bit_width,signed_value,write_data,host_byte_order);
		} else if (nm == "byte" || nm == "int8"){
			return fill_raw_chunk(smc,st.children(),data_size,data,bit_offs,8,true,write_data,host_byte_order);
		} else if (nm == "ubyte" || nm == "uint8"){
			return fill_raw_chunk(smc,st.children(),data_size,data,bit_offs,8,false,write_data,host_byte_order);
		} else if (nm == "ushort" || nm == "uint16"){
			return fill_raw_chunk(smc,st.children(),data_size,data,bit_offs,16,false,write_data,host_byte_order);
		} else if (nm == "short" || nm == "int16"){
			return fill_raw_chunk(smc,st.children(),data_size,data,bit_offs,16,true,write_data,host_byte_order);
		} else if (nm == "uint" || nm == "uint32"){
			return fill_raw_chunk(smc,st.children(),data_size,data,bit_offs,32,false,write_data,host_byte_order);
		} else if (nm == "int" || nm == "int32") {
			return fill_raw_chunk(smc,st.children(),data_size,data,bit_offs,32,true,write_data,host_byte_order);
		} else if (nm == "ulonglong" || nm == "uint64") {
			return fill_raw_chunk(smc,st.children(),data_size,data,bit_offs,64,false,write_data,host_byte_order);
		} else if (nm == "longlong" || nm == "int64") {
			return fill_raw_chunk(smc,st.children(),data_size,data,bit_offs,64,true,write_data,host_byte_order);
		} else if (nm == "uint24"){
			return fill_raw_chunk(smc,st.children(),data_size,data,bit_offs,24,false,write_data,host_byte_order);
		} else if (nm == "int24"){
			return fill_raw_chunk(smc,st.children(),data_size,data,bit_offs,24,true,write_data,host_byte_order);
		}else if (nm == "uint23"){
			return fill_raw_chunk(smc,st.children(),data_size,data,bit_offs,23,false,write_data,host_byte_order);
		} else if (nm == "int23"){
			return fill_raw_chunk(smc,st.children(),data_size,data,bit_offs,23,true,write_data,host_byte_order);
		}else if (nm == "uint22"){
			return fill_raw_chunk(smc,st.children(),data_size,data,bit_offs,22,false,write_data,host_byte_order);
		} else if (nm == "int22"){
			return fill_raw_chunk(smc,st.children(),data_size,data,bit_offs,22,true,write_data,host_byte_order);
		}else if (nm == "uint21"){
			return fill_raw_chunk(smc,st.children(),data_size,data,bit_offs,21,false,write_data,host_byte_order);
		} else if (nm == "int21"){
			return fill_raw_chunk(smc,st.children(),data_size,data,bit_offs,21,true,write_data,host_byte_order);
		}else if (nm == "uint20"){
			return fill_raw_chunk(smc,st.children(),data_size,data,bit_offs,20,false,write_data,host_byte_order);
		} else if (nm == "int20"){
			return fill_raw_chunk(smc,st.children(),data_size,data,bit_offs,20,true,write_data,host_byte_order);
		}else if (nm == "uint19"){
			return fill_raw_chunk(smc,st.children(),data_size,data,bit_offs,19,false,write_data,host_byte_order);
		} else if (nm == "int19"){
			return fill_raw_chunk(smc,st.children(),data_size,data,bit_offs,19,true,write_data,host_byte_order);
		}else if (nm == "uint18"){
			return fill_raw_chunk(smc,st.children(),data_size,data,bit_offs,18,false,write_data,host_byte_order);
		} else if (nm == "int18"){
			return fill_raw_chunk(smc,st.children(),data_size,data,bit_offs,18,true,write_data,host_byte_order);
		}else if (nm == "uint17"){
			return fill_raw_chunk(smc,st.children(),data_size,data,bit_offs,17,false,write_data,host_byte_order);
		} else if (nm == "int17"){
			return fill_raw_chunk(smc,st.children(),data_size,data,bit_offs,17,true,write_data,host_byte_order);
		}else if (nm == "uint15"){
			return fill_raw_chunk(smc,st.children(),data_size,data,bit_offs,15,false,write_data,host_byte_order);
		} else if (nm == "int15"){
			return fill_raw_chunk(smc,st.children(),data_size,data,bit_offs,15,true,write_data,host_byte_order);
		} else if (nm == "uint14"){
			return fill_raw_chunk(smc,st.children(),data_size,data,bit_offs,14,false,write_data,host_byte_order);
		} else if (nm == "int14"){
			return fill_raw_chunk(smc,st.children(),data_size,data,bit_offs,14,true,write_data,host_byte_order);
		}else if (nm == "uint13"){
			return fill_raw_chunk(smc,st.children(),data_size,data,bit_offs,13,false,write_data,host_byte_order);
		} else if (nm == "int13"){
			return fill_raw_chunk(smc,st.children(),data_size,data,bit_offs,13,true,write_data,host_byte_order);
		}else if (nm == "uint12"){
			return fill_raw_chunk(smc,st.children(),data_size,data,bit_offs,12,false,write_data,host_byte_order);
		} else if (nm == "int12"){
			return fill_raw_chunk(smc,st.children(),data_size,data,bit_offs,12,true,write_data,host_byte_order);
		}else if (nm == "uint11"){
			return fill_raw_chunk(smc,st.children(),data_size,data,bit_offs,11,false,write_data,host_byte_order);
		} else if (nm == "int11"){
			return fill_raw_chunk(smc,st.children(),data_size,data,bit_offs,11,true,write_data,host_byte_order);
		} else if (nm == "uint10"){
			return fill_raw_chunk(smc,st.children(),data_size,data,bit_offs,10,false,write_data,host_byte_order);
		} else if (nm == "int10"){
			return fill_raw_chunk(smc,st.children(),data_size,data,bit_offs,10,true,write_data,host_byte_order);
		}else if (nm == "uint9"){
			return fill_raw_chunk(smc,st.children(),data_size,data,bit_offs,9,false,write_data,host_byte_order);
		} else if (nm == "int9"){
			return fill_raw_chunk(smc,st.children(),data_size,data,bit_offs,9,true,write_data,host_byte_order);
		}else if (nm == "uint7"){
			return fill_raw_chunk(smc,st.children(),data_size,data,bit_offs,7,false,write_data,host_byte_order);
		} else if (nm == "int7"){
			return fill_raw_chunk(smc,st.children(),data_size,data,bit_offs,7,true,write_data,host_byte_order);
		} else if (nm == "uint6"){
			return fill_raw_chunk(smc,st.children(),data_size,data,bit_offs,6,false,write_data,host_byte_order);
		} else if (nm == "int6"){
			return fill_raw_chunk(smc,st.children(),data_size,data,bit_offs,6,true,write_data,host_byte_order);
		} else if (nm == "uint5"){
			return fill_raw_chunk(smc,st.children(),data_size,data,bit_offs,5,false,write_data,host_byte_order);
		} else if (nm == "int5"){
			return fill_raw_chunk(smc,st.children(),data_size,data,bit_offs,8,true,write_data,host_byte_order);
		} else if (nm == "uint4"){
			return fill_raw_chunk(smc,st.children(),data_size,data,bit_offs,4,false,write_data,host_byte_order);
		} else if (nm == "int4"){
			return fill_raw_chunk(smc,st.children(),data_size,data,bit_offs,4,true,write_data,host_byte_order);
		} else if (nm == "uint3"){
			return fill_raw_chunk(smc,st.children(),data_size,data,bit_offs,3,false,write_data,host_byte_order);
		} else if (nm == "int3"){
			return fill_raw_chunk(smc,st.children(),data_size,data,bit_offs,3,true,write_data,host_byte_order);
		} else if (nm == "uint2"){
			return fill_raw_chunk(smc,st.children(),data_size,data,bit_offs,2,false,write_data,host_byte_order);
		} else if (nm == "int2"){
			return fill_raw_chunk(smc,st.children(),data_size,data,bit_offs,2,true,write_data,host_byte_order);
		} else if (nm == "bit"){ //std::cout << "------\n";
			return fill_raw_chunk(smc,st.children(),data_size,data,bit_offs,1,true,write_data,host_byte_order);
		} else {
			return fill_raw_chunk(smc,st.children(), data_size,data, bit_offs, bit_width,signed_value,write_data,host_byte_order);
		}
	}
	else if (p->kind() == ceps::ast::Ast_node_kind::identifier){
		auto& ident = ceps::ast::as_id_ref(p);
		if (ceps::ast::name(ident) == "any") return bit_width;
		else if (ceps::ast::name(ident) == "__current_frame_size") {
			auto temp_node = ceps::ast::Int(data_size,ceps::ast::all_zero_unit(), nullptr, nullptr, nullptr);
			return fill_raw_chunk(smc,&temp_node,data_size, data, bit_offs, bit_width,signed_value,write_data,host_byte_order);
		} else smc->fatal_(-1,std::string("Raw frame:  Unknown identifier '")+ceps::ast::name(ident)+"'");
	}else if (p->kind() == ceps::ast::Ast_node_kind::int_literal){
		if (write_data){
			std::uint64_t v = (std::uint64_t)value(as_int_ref(p));
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
				   std:uint64_t w=1;
				   for(size_t i = 0; i < bit_width;++i) w |= (w << 1);
				   v &= w;
			   }
			}
			int bits_written = 0;
			if (bit_offs % 8) {
				unsigned short o = bit_offs % 8;
				unsigned short d = 8 - o ; // d is the number of bits left in the byte to write
				unsigned char w = *(data + bit_offs/8);
				if (d > bit_width) d = bit_width;
				unsigned char c1=1;for(int i = 0; i < d;++i)c1 |= (c1 << 1);
				unsigned char c2=1;for(int i = 0; i < o-1;++i)c2 |= (c2 << 1);

				w = (w & c2) | ( ((unsigned char)v & c1) << o );
				*( (unsigned char*)( data + bit_offs/8)) = w;
				bits_written = d;bit_offs+=d;
			}
			v = v >> bits_written;

			//INVARIANT: bit_offs points to a byte address
			if (bits_written < bit_width){
				for(int i = 0; i < (bit_width-bits_written)/8;++i,bit_offs+=8,bits_written+=8){
					*(data+bit_offs/8) = ((unsigned char)v & 0xFF);
				    v = v >> 8;
				}
			}
			//INVARIANT: bit_offs points to a byte address
			//INVARIANT: bit_width - bits_written < 8
			if (bits_written < bit_width){
				if (bit_width-bits_written  == 1){
					unsigned char & target = *( (unsigned char*)(data+ bit_offs/8) );
					if (v & 1)  target |= 1;
					else target &= 0xFE;
				} else
				{
					unsigned char & target = *( (unsigned char*)(data+ bit_offs/8) );
					unsigned char c = 1;
					for(int i = 0; i < bit_width-bits_written-1; ++i ) c |= c << 1;
					target = c & v;
				}
			}
		}
		return bit_width;
	}
	else if (p->kind() == ceps::ast::Ast_node_kind::float_literal) {
		 smc->fatal_(-1,"Floating point numbers not supported in raw frames.");
	}
	else if (p->kind() == ceps::ast::Ast_node_kind::string_literal){
		 std::string s = ceps::ast::value(ceps::ast::as_string_ref(p));
		 int corr = 0;
		 if (bit_offs % 8) corr = 8 - (bit_offs % 8);
		 bit_offs += corr;
		 if (write_data) memcpy((data+bit_offs/8),s.c_str(),s.length());
		 return s.length()*8+corr;
	}
	else {std::stringstream ss; ss << *p; smc->fatal_(-1,"Serialization of raw frame: Illformed expression:"+ss.str());}
	return 0;
}

size_t fill_raw_chunk(State_machine_simulation_core* smc,
		            std::vector<ceps::ast::Nodebase_ptr> pattern,
		            size_t data_size,
		            char* data,
		            size_t bit_offs,
		            size_t bit_width,
		            bool signed_value,
		            bool write_data,
		            bool host_byte_order
		            ){
	size_t r=0;
	for(auto p : pattern){
		auto rr = fill_raw_chunk(smc,p,data_size,data,bit_offs,bit_width,signed_value,write_data,host_byte_order);
		r+=rr;
		bit_offs += rr;
	}
	return r;
}

size_t compute_size(State_machine_simulation_core* smc,std::vector<ceps::ast::Nodebase_ptr> pattern) {
	size_t bits = fill_raw_chunk(smc,
            pattern,
            0,
            nullptr,
            0,
            0,true,false,true);
	return bits / 8 + ( (bits % 8) ? 1 : 0);
}

size_t Podframe_generator::compute_size_of_msg(State_machine_simulation_core* smc,
		                                       std::vector<std::string> params,bool& failed)
{

	auto data_format = spec_["data"];

	failed = false;

	return compute_size(smc, data_format.nodes() );
}

extern ceps::ast::Nodebase_ptr eval_locked_ceps_expr(State_machine_simulation_core* smc,
		 State_machine* containing_smp,
		 ceps::ast::Nodebase_ptr node,
		 ceps::ast::Nodebase_ptr root_node);




void* Podframe_generator::gen_msg(State_machine_simulation_core* smc,size_t& data_size){

	if (smc == nullptr) return nullptr;
	auto THIS = smc;
	DEBUG_FUNC_PROLOGUE2;
	auto data_format = spec_["data"];
	if (data_format.nodes().empty()) {
		std::string id;
		auto t = spec_["id"];
		if (t.nodes().size() != 0 && t.nodes()[0]->kind() == ceps::ast::Ast_node_kind::identifier)
			id = ceps::ast::name(ceps::ast::as_id_ref(t.nodes()[0]));
		smc->fatal_(-1,"Frame '"+id+"' doesn't contain a data section.");
	}

	ceps_interface_eval_func_callback_ctxt_t ctxt;
	ctxt.active_smp = nullptr;
	ctxt.smc  = smc;

	ceps::ast::Nodebase_ptr frame_pattern = nullptr;
	ceps::ast::Scope scope;
	scope.children() = data_format.nodes();scope.owns_children() = false;
	frame_pattern = eval_locked_ceps_expr(smc,nullptr,&scope,nullptr);
	scope.children().clear();

	if (frame_pattern == nullptr) return nullptr;
	DEBUG << "[Podframe_generator::gen_msg][pattern]" << *frame_pattern << "\n\n";
	auto chunk_size = compute_size(smc,ceps::ast::nlf_ptr(frame_pattern)->children());
	data_size = chunk_size;
	DEBUG << "[Podframe_generator::gen_msg][CHUNK_SIZE="<<chunk_size<<"]\n";
	char* data = new char[chunk_size];
	bzero(data,chunk_size);
	fill_raw_chunk( smc,ceps::ast::nlf_ptr(frame_pattern)->children(),chunk_size, data,0);
	for(size_t offs = 0; offs < chunk_size;++offs)
		DEBUG <<"[Podframe_generator::gen_msg][CHUNK_BYTE_"<< offs << "="<< ((std::uint32_t) *( (unsigned char*)data+offs)) << "]\n";

	return data;
}


//////////////////////////////////////////////////////////////////
////////////////////////////////////// READ RAW MESSAGE //////////
//////////////////////////////////////////////////////////////////
int read_raw_chunk(State_machine_simulation_core* smc,
		            std::vector<ceps::ast::Nodebase_ptr> pattern,
		            size_t data_size,
		            char* data,
		            size_t bit_offs,
		            size_t bit_width=sizeof(std::int64_t)*8,
		            bool signed_value = true,
		            bool read_data = true,
		            bool host_byte_order = true
		            );






int read_raw_chunk( State_machine_simulation_core* smc,
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
			return read_raw_chunk(smc,st.children(), data_size,data, bit_offs, bit_width,signed_value,read_data,host_byte_order);
		} else if (nm == "byte" || nm == "int8"){
			return read_raw_chunk(smc,st.children(),data_size,data,bit_offs,8,true,read_data,host_byte_order);
		} else if (nm == "ubyte" || nm == "uint8"){
			return read_raw_chunk(smc,st.children(),data_size,data,bit_offs,8,false,read_data,host_byte_order);
		} else if (nm == "ushort" || nm == "uint16"){
			return read_raw_chunk(smc,st.children(),data_size,data,bit_offs,16,false,read_data,host_byte_order);
		} else if (nm == "short" || nm == "int16"){
			return read_raw_chunk(smc,st.children(),data_size,data,bit_offs,16,true,read_data,host_byte_order);
		} else if (nm == "uint" || nm == "uint32"){
			return read_raw_chunk(smc,st.children(),data_size,data,bit_offs,32,false,read_data,host_byte_order);
		} else if (nm == "int" || nm == "int32") {
			return read_raw_chunk(smc,st.children(),data_size,data,bit_offs,32,true,read_data,host_byte_order);
		} else if (nm == "ulonglong" || nm == "uint64") {
			return read_raw_chunk(smc,st.children(),data_size,data,bit_offs,64,false,read_data,host_byte_order);
		} else if (nm == "longlong" || nm == "int64") {
			return read_raw_chunk(smc,st.children(),data_size,data,bit_offs,64,true,read_data,host_byte_order);
		} else if (nm == "uint24"){
			return read_raw_chunk(smc,st.children(),data_size,data,bit_offs,24,false,read_data,host_byte_order);
		} else if (nm == "int24"){
			return read_raw_chunk(smc,st.children(),data_size,data,bit_offs,24,true,read_data,host_byte_order);
		}else if (nm == "uint23"){
			return read_raw_chunk(smc,st.children(),data_size,data,bit_offs,23,false,read_data,host_byte_order);
		} else if (nm == "int23"){
			return read_raw_chunk(smc,st.children(),data_size,data,bit_offs,23,true,read_data,host_byte_order);
		}else if (nm == "uint22"){
			return read_raw_chunk(smc,st.children(),data_size,data,bit_offs,22,false,read_data,host_byte_order);
		} else if (nm == "int22"){
			return read_raw_chunk(smc,st.children(),data_size,data,bit_offs,22,true,read_data,host_byte_order);
		}else if (nm == "uint21"){
			return read_raw_chunk(smc,st.children(),data_size,data,bit_offs,21,false,read_data,host_byte_order);
		} else if (nm == "int21"){
			return read_raw_chunk(smc,st.children(),data_size,data,bit_offs,21,true,read_data,host_byte_order);
		}else if (nm == "uint20"){
			return read_raw_chunk(smc,st.children(),data_size,data,bit_offs,20,false,read_data,host_byte_order);
		} else if (nm == "int20"){
			return read_raw_chunk(smc,st.children(),data_size,data,bit_offs,20,true,read_data,host_byte_order);
		}else if (nm == "uint19"){
			return read_raw_chunk(smc,st.children(),data_size,data,bit_offs,19,false,read_data,host_byte_order);
		} else if (nm == "int19"){
			return read_raw_chunk(smc,st.children(),data_size,data,bit_offs,19,true,read_data,host_byte_order);
		}else if (nm == "uint18"){
			return read_raw_chunk(smc,st.children(),data_size,data,bit_offs,18,false,read_data,host_byte_order);
		} else if (nm == "int18"){
			return read_raw_chunk(smc,st.children(),data_size,data,bit_offs,18,true,read_data,host_byte_order);
		}else if (nm == "uint17"){
			return read_raw_chunk(smc,st.children(),data_size,data,bit_offs,17,false,read_data,host_byte_order);
		} else if (nm == "int17"){
			return read_raw_chunk(smc,st.children(),data_size,data,bit_offs,17,true,read_data,host_byte_order);
		}else if (nm == "uint15"){
			return read_raw_chunk(smc,st.children(),data_size,data,bit_offs,15,false,read_data,host_byte_order);
		} else if (nm == "int15"){
			return read_raw_chunk(smc,st.children(),data_size,data,bit_offs,15,true,read_data,host_byte_order);
		} else if (nm == "uint14"){
			return read_raw_chunk(smc,st.children(),data_size,data,bit_offs,14,false,read_data,host_byte_order);
		} else if (nm == "int14"){
			return read_raw_chunk(smc,st.children(),data_size,data,bit_offs,14,true,read_data,host_byte_order);
		}else if (nm == "uint13"){
			return read_raw_chunk(smc,st.children(),data_size,data,bit_offs,13,false,read_data,host_byte_order);
		} else if (nm == "int13"){
			return read_raw_chunk(smc,st.children(),data_size,data,bit_offs,13,true,read_data,host_byte_order);
		}else if (nm == "uint12"){
			return read_raw_chunk(smc,st.children(),data_size,data,bit_offs,12,false,read_data,host_byte_order);
		} else if (nm == "int12"){
			return read_raw_chunk(smc,st.children(),data_size,data,bit_offs,12,true,read_data,host_byte_order);
		}else if (nm == "uint11"){
			return read_raw_chunk(smc,st.children(),data_size,data,bit_offs,11,false,read_data,host_byte_order);
		} else if (nm == "int11"){
			return read_raw_chunk(smc,st.children(),data_size,data,bit_offs,11,true,read_data,host_byte_order);
		} else if (nm == "uint10"){
			return read_raw_chunk(smc,st.children(),data_size,data,bit_offs,10,false,read_data,host_byte_order);
		} else if (nm == "int10"){
			return read_raw_chunk(smc,st.children(),data_size,data,bit_offs,10,true,read_data,host_byte_order);
		}else if (nm == "uint9"){
			return read_raw_chunk(smc,st.children(),data_size,data,bit_offs,9,false,read_data,host_byte_order);
		} else if (nm == "int9"){
			return read_raw_chunk(smc,st.children(),data_size,data,bit_offs,9,true,read_data,host_byte_order);
		}else if (nm == "uint7"){
			return read_raw_chunk(smc,st.children(),data_size,data,bit_offs,7,false,read_data,host_byte_order);
		} else if (nm == "int7"){
			return read_raw_chunk(smc,st.children(),data_size,data,bit_offs,7,true,read_data,host_byte_order);
		} else if (nm == "uint6"){
			return read_raw_chunk(smc,st.children(),data_size,data,bit_offs,6,false,read_data,host_byte_order);
		} else if (nm == "int6"){
			return read_raw_chunk(smc,st.children(),data_size,data,bit_offs,6,true,read_data,host_byte_order);
		} else if (nm == "uint5"){
			return read_raw_chunk(smc,st.children(),data_size,data,bit_offs,5,false,read_data,host_byte_order);
		} else if (nm == "int5"){
			return read_raw_chunk(smc,st.children(),data_size,data,bit_offs,8,true,read_data,host_byte_order);
		} else if (nm == "uint4"){
			return read_raw_chunk(smc,st.children(),data_size,data,bit_offs,4,false,read_data,host_byte_order);
		} else if (nm == "int4"){
			return read_raw_chunk(smc,st.children(),data_size,data,bit_offs,4,true,read_data,host_byte_order);
		} else if (nm == "uint3"){
			return read_raw_chunk(smc,st.children(),data_size,data,bit_offs,3,false,read_data,host_byte_order);
		} else if (nm == "int3"){
			return read_raw_chunk(smc,st.children(),data_size,data,bit_offs,3,true,read_data,host_byte_order);
		} else if (nm == "uint2"){
			return read_raw_chunk(smc,st.children(),data_size,data,bit_offs,2,false,read_data,host_byte_order);
		} else if (nm == "int2"){
			return read_raw_chunk(smc,st.children(),data_size,data,bit_offs,2,true,read_data,host_byte_order);
		} else if (nm == "bit"){ //std::cout << "------\n";
			return read_raw_chunk(smc,st.children(),data_size,data,bit_offs,1,true,read_data,host_byte_order);
		} else {
			return read_raw_chunk(smc,st.children(), data_size,data, bit_offs, bit_width,signed_value,read_data,host_byte_order);
		}
	}
	else if (p->kind() == ceps::ast::Ast_node_kind::identifier){
		auto& ident = ceps::ast::as_id_ref(p);
		if (ceps::ast::name(ident) == "any") return bit_width;
		else if (ceps::ast::name(ident) == "__current_frame_size") {
			auto temp_node = ceps::ast::Int(data_size,ceps::ast::all_zero_unit(), nullptr, nullptr, nullptr);
			return read_raw_chunk(smc,&temp_node,data_size, data, bit_offs, bit_width,signed_value,read_data,host_byte_order);
		} else smc->fatal_(-1,std::string("Raw frame:  Unknown identifier '")+ceps::ast::name(ident)+"'");
	}else if (p->kind() == ceps::ast::Ast_node_kind::int_literal
			|| (p->kind() == ceps::ast::Ast_node_kind::symbol && ceps::ast::kind(ceps::ast::as_symbol_ref(p) ) == "Systemstate"  ) ){
		if (read_data){
			auto orig_bit_offs = bit_offs;
			std::uint64_t v = 0;

			int bits_read = 0;
			if (bit_offs % 8) {
				unsigned short o = bit_offs % 8;
				unsigned short d = 8 - o ; // d is the number of bits left in the byte to read
				v = *( ( (unsigned char*) data) + bit_offs/8);
				v = v >> o;
				if (d > bit_width) {
				 unsigned char c1=1;for(int i = 1; i < bit_width;++i)c1 |= (c1 << 1);
				 v &= c1;
				 d = bit_width;
				}
				bits_read = d;bit_offs+=d;
			}

			//INVARIANT: bit_offs points to a byte address
			int bits_read_after_byte_boundary=0;
			if (bits_read< bit_width){
				std::uint64_t v_temp = 0;
				for(int i = 0; i < (bit_width-bits_read)/8;++i,bit_offs+=8,bits_read+=8,bits_read_after_byte_boundary+=8){
					v_temp |=  ((std::uint64_t) *( ( (unsigned char*) data) +bit_offs/8)) << bits_read;
				}
				v |= v_temp;
			}
			//INVARIANT: bit_offs points to a byte address
			//INVARIANT: bit_width - bits_written < 8
			if (bits_read < bit_width){
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
			    else if (v != ceps::ast::value(ceps::ast::as_int_ref(p))) return -2;
			} else {
				std::lock_guard<std::recursive_mutex>g(smc->states_mutex());
				std::string s;
				auto w = smc->get_global_states()[s = ceps::ast::name(ceps::ast::as_symbol_ref(p))];
				if (w == nullptr || w->kind() != ceps::ast::Ast_node_kind::int_literal)
				 smc->get_global_states()[s] =
						new ceps::ast::Int( v ,ceps::ast::all_zero_unit(),nullptr,nullptr,nullptr);
				else
					ceps::ast::value(ceps::ast::as_int_ref(smc->get_global_states()[s])) = v;
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










int read_raw_chunk(State_machine_simulation_core* smc,
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
		auto rr = read_raw_chunk(smc,p,data_size,data,bit_offs,bit_width,signed_value,read_data,host_byte_order);
		if (rr < 0) return rr;
		r+=rr;
		bit_offs += rr;
	}
	return r;
}


bool Podframe_generator::read_msg(char* data,size_t size,
		                          State_machine_simulation_core* smc,
		                          std::vector<std::string> params,
		                          std::vector<ceps::ast::Nodebase_ptr>& payload)
{
	if (data == nullptr || size == 0) return true;

	auto data_format = spec_["data"];
	size_t offs = 0;


	if (data_format.nodes().empty()) {
		std::string id;
		auto t = spec_["id"];
		if (t.nodes().size() != 0 && t.nodes()[0]->kind() == ceps::ast::Ast_node_kind::identifier)
			id = ceps::ast::name(ceps::ast::as_id_ref(t.nodes()[0]));
		smc->fatal_(-1,"Frame '"+id+"' doesn't contain a data section.");
	}

	ceps_interface_eval_func_callback_ctxt_t ctxt;
	ctxt.active_smp = nullptr;
	ctxt.smc  = smc;


	if (0 > read_raw_chunk( smc,data_format.nodes(),size, data,0)) return false;

	return true;
}





void comm_sender_generic_tcp_out_thread(threadsafe_queue< std::pair<char*,size_t>, std::queue<std::pair<char*,size_t> >>* frames,
			     State_machine_simulation_core* smc,
			     std::string ip,
			     std::string port,
			     std::string som,
			     std::string eof,
			     std::string sock_name,
			     bool reuse_socket,
			     bool reg_socket)
{
	auto THIS = smc;
	DEBUG_FUNC_PROLOGUE2
	int cfd;
	struct addrinfo hints;
	struct addrinfo *result, *rp;
	auto q = frames;
	bool conn_established = false;
	char* frame = nullptr;
	size_t frame_size = 0;
	bool pop_frame = true;
	for(;;)
	{
		rp = nullptr;result = nullptr;

		DEBUG << "[comm_sender_generic_tcp_out_thread][WAIT_FOR_FRAME][pop_frame="<<pop_frame <<"]\n";
		std::pair<char*,size_t> frame_info;

		if (pop_frame) {q->wait_and_pop(frame_info);frame_size = frame_info.second;frame= frame_info.first;}
		pop_frame = false;

		DEBUG << "[comm_sender_generic_tcp_out_thread][FETCHED_FRAME]\n";
		if (!conn_established)
		{
			if (reuse_socket){
				DEBUG << "[comm_sender_generic_tcp_out_thread][REUSE_SOCK("<< sock_name <<")]\n";
				for(;;){
					{
						std::lock_guard<std::recursive_mutex>g(smc->get_reg_sock_mtx());
						auto it = smc->get_reg_socks().find(sock_name);
						if (it != smc->get_reg_socks().end()){
							cfd = it->second;
							conn_established = true;
							break;
						}
					}
					std::this_thread::sleep_for(std::chrono::microseconds(1000));continue;
				}
			}
			else { DEBUG << "[comm_sender_generic_tcp_out_thread][CONNECTING@"<< ip << ":" << port << "]\n";
			for(;rp == nullptr;)
			{

			memset(&hints, 0, sizeof(struct addrinfo));
			hints.ai_canonname = NULL;
			hints.ai_addr = NULL;
			hints.ai_next = NULL;
			hints.ai_family = AF_INET;

			hints.ai_socktype = SOCK_STREAM;
			hints.ai_flags = AI_NUMERICSERV;
			if (getaddrinfo(ip.c_str(), port.c_str(), &hints, &result) != 0){
				DEBUG << "[comm_sender_generic_tcp_out_thread][FAILED_TO_CONNECT@"<< ip << ":" << port << "]\n";
				std::this_thread::sleep_for(std::chrono::microseconds(1000000));continue;
			}

			 for (rp = result; rp != NULL; rp = rp->ai_next) {
			  cfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
			  if (cfd == -1)	continue;
			  if (connect(cfd, rp->ai_addr, rp->ai_addrlen) != -1)break;
			  close(cfd);
			 }
			 if (result != nullptr) freeaddrinfo(result);
			 if (rp == nullptr) {
				 DEBUG << "[comm_sender_generic_tcp_out_thread][FAILED_TO_CONNECT@"<< ip << ":" << port << "]\n";
				 std::this_thread::sleep_for(std::chrono::microseconds(1000000));continue;
			 }
			}
			conn_established = true;
			}
		}

		if (!reuse_socket)DEBUG << "[comm_sender_generic_tcp_out_thread][SENDING_FRAME@"<< ip << ":" << port << "]\n";
		else DEBUG << "[comm_sender_generic_tcp_out_thread][SENDING_FRAME@"<< sock_name << "]\n";

		if(reg_socket){
			std::lock_guard<std::recursive_mutex>g(smc->get_reg_sock_mtx());
			smc->get_reg_socks()[sock_name] = cfd;
		}


		auto len = frame_size;
		int wr = 0;

		if (som.size() == 0 && eof.size() == 0){
			auto l = htonl(len);
			if ( write(cfd, &l,sizeof(l)) != sizeof(l)){
				close(cfd);	conn_established=false;DEBUG << "[comm_sender_generic_tcp_out_thread][write of data length failed]\n";continue;
			}
		}
		if (som.size())
		 if ( (wr = write(cfd, som.c_str(),eof.length() )) != som.length())
		 {
			close(cfd);
			conn_established=false;
			DEBUG << "[comm_sender_generic_tcp_out_thread][Partial/failed write]\n";
			continue;
		}
		if (len && frame) if ( (wr = write(cfd, frame,len )) != len)
		{
			close(cfd);
			conn_established=false;
			DEBUG << "[comm_sender_generic_tcp_out_thread][Partial/failed write]\n";
			continue;
		}
		if (eof.size())
			if ( (wr = write(cfd, eof.c_str(),eof.length() )) != eof.length())
			{
				close(cfd);
				conn_established=false;
				DEBUG << "[comm_sender_generic_tcp_out_thread][Partial/failed write]\n";
				continue;
			}
		DEBUG << "[comm_sender_generic_tcp_out_thread][FRAME_WRITTEN][("<< frame_size<< " bytes)]\n";
		if (frame != nullptr) {delete[] frame;frame=nullptr;}
		pop_frame = true;

	}
	if (conn_established)close(cfd);
}


void comm_generic_tcp_in_thread_fn(int id,
		 Rawframe_generator* gen,
		 std::string ev_id,
		 std::vector<std::string> params,
		 State_machine_simulation_core* smc,
		 sockaddr_storage claddr,int sck,std::string som,std::string eof)
{
	auto THIS = smc;
	DEBUG_FUNC_PROLOGUE2
	char host[1024] = {0};
	char service[1024] = {0};
	char addrstr[2048] = {0};
	socklen_t addrlen = sizeof(struct sockaddr_storage);

	if (getnameinfo((struct sockaddr*)&claddr,addrlen,host,1024,service,1024,0) == 0)
		snprintf(addrstr,2048,"[host=%s, service=%s]",host, service);
	else
		snprintf(addrstr,2048,"[host=?,service=?]");
	DEBUG << "[comm_generic_tcp_in_thread_fn][CONN_ESTABLISHED]" << addrstr << "\n";

	if (id >= 0){ // This is a generic fixed size input stream with a handler context

		auto ctxt = smc->get_dispatcher_thread_ctxt(id);
		std::uint32_t size = 0;
		char* buffer = nullptr;
		for(;!smc->shutdown();){
			std::uint32_t new_size = 0;
			auto r = recv(sck,(char*)&new_size,sizeof(new_size),0);
			if (r != sizeof(new_size)) break;
			new_size = ntohl(new_size);
			if (new_size > size){if (buffer) delete[] buffer; buffer = new char[size = new_size];}
			if(buffer == nullptr) {smc->fatal_(-1,"Couldn't allocate memory.");}
			r = recv(sck,buffer,size,0);
			DEBUG << "[comm_generic_tcp_in_thread_fn][DATA_READ][SIZE="<<new_size<<"]"<< "\n";
			std::vector<std::string> v1;
			std::vector<ceps::ast::Nodebase_ptr> v2;
			for(auto handler_info: ctxt.handler){
				auto match = handler_info.first->read_msg(buffer,new_size,smc,v1,v2);
				if (!match) continue;
				DEBUG << "[comm_generic_tcp_in_thread_fn][MATCH_FOUND]\n";
				smc->execute_action_seq(nullptr,handler_info.second);
			}
		}
		close(sck);if (buffer) delete[] buffer;
		return;
	}

	size_t frame_size = 0;
	if (eof.length() == 0){
	 bool failed_size_computation = true;
	 for(;failed_size_computation;)
	 {
		frame_size = gen->compute_size_of_msg(smc,params,failed_size_computation);
		if (failed_size_computation) std::this_thread::sleep_for(std::chrono::microseconds(1));
	 }

	 DEBUG << "[comm_generic_tcp_in_thread_fn][FRAME_SIZE_AVAILABLE]" << addrstr << "\n";
	} else DEBUG << "[comm_generic_tcp_in_thread_fn][EOF_MODE_DETECTED]\n";


	if (eof.length())
	{

		std::string last_suffix;
		for(;!smc->shutdown();){
			char buffer_[4096];
			std::stringstream* buffer = new std::stringstream;
			auto n = 0;
			DEBUG << "[comm_generic_tcp_in_thread_fn][READ...]\n";
			for(; (n = recv(sck,buffer_,4095,0)) > 0;)
			{
				DEBUG << "[comm_generic_tcp_in_thread_fn][DATA_RECEIVED][SIZE="<<n <<"]\n";
				buffer_[n] = 0;

				//DEBUG << "[comm_generic_tcp_in_thread_fn][DATA_RECEIVED][VALUE='"<<buffer_ <<"']\n";

				std::string temp(buffer_);
				if (temp == eof) {

				}
				std::string::size_type r=0;
				bool buffer_processed = false;
				do{
					auto new_r = temp.find(eof,r);
					//std::cout << new_r << std::endl;
					buffer_processed = new_r == std::string::npos;
					*buffer << temp.substr(r,new_r-r );
					if (new_r != std::string::npos){

						DEBUG << "[comm_generic_tcp_in_thread_fn][DATA_CHUNK_READ][SIZE="<<buffer->str().length() <<"]\n";
						r = new_r+eof.length();

					    bool decode_result = false;
					    std::vector<ceps::ast::Nodebase_ptr> payload;
					    {
					    	std::lock_guard<std::recursive_mutex>g(smc->states_mutex());
					    	decode_result = gen->read_msg((char*)buffer->str().c_str() +som.length(),
					    								  buffer->str().length()-som.length(),
					    								  smc,
					    								  params,payload);
					    	if (decode_result) DEBUG << "[comm_generic_tcp_in_thread_fn][DATA_SUCCESSFULLY_DECODED]\n";
					    	else DEBUG << "[comm_generic_tcp_in_thread_fn][FAILED_TO_DECODE_DATA]\n";
					    }

					    if (decode_result){
					    	State_machine_simulation_core::event_t ev(ev_id);
					    	ev.already_sent_to_out_queues_ = true;
					    	if (payload.size())
					    		ev.payload_ = payload;
					    	smc->main_event_queue().push(ev);
					    }

						delete buffer;
						buffer =  new std::stringstream;
					}
				} while (!buffer_processed && r < temp.length());
			}
			delete buffer;
			if (n<=0) break;
		}
	} else

	for(;!smc->shutdown() && frame_size;)
	{
		char *data;
		try
		{
			DEBUG << "[comm_generic_tcp_in_thread_fn][READING_DATA]\n";
			data = new char[frame_size];
			if (data == nullptr){DEBUG << "[ERROR_comm_generic_tcp_in_thread_fn][ALLOC_FAILED]\n";close(sck);return;}
			ssize_t already_read = 0;
			ssize_t n = 0;
			for(; (already_read < frame_size) && (n = recv(sck,data+already_read,frame_size-already_read,0)) > 0;already_read+=n);

			if(already_read < frame_size){DEBUG << "[ERROR_comm_generic_tcp_in_thread_fn][READ_FAILED]\n";close(sck);return;}
		    DEBUG << "[comm_generic_tcp_in_thread_fn][DATA_SUCCESSFULLY_READ]\n";


		    bool decode_result = false;
		    std::vector<ceps::ast::Nodebase_ptr> payload;
		    {
		    	std::lock_guard<std::recursive_mutex>g(smc->states_mutex());
		    	decode_result = gen->read_msg(data,frame_size,smc,params,payload);
		    	if (decode_result) DEBUG << "[comm_generic_tcp_in_thread_fn][DATA_SUCCESSFULLY_DECODED]\n";
		    	else DEBUG << "[comm_generic_tcp_in_thread_fn][FAILED_TO_DECODE_DATA]\n";
		    }

		    if (decode_result){
		    	State_machine_simulation_core::event_t ev(ev_id);
		    	ev.already_sent_to_out_queues_ = true;
		    	if (payload.size())
		    		ev.payload_ = payload;
		    	smc->main_event_queue().push(ev);
		    }




		} catch(...){}

		if (data) delete[] data;
	}
	close(sck);
}



void comm_generic_tcp_in_dispatcher_thread(int id,
				 Rawframe_generator* gen,
				 std::string ev_id,
				 std::vector<std::string> params,
			     State_machine_simulation_core* smc,
			     std::string ip,
			     std::string port,std::string som,std::string eof,std::string sock_name,bool reg_sock,bool reuse_sock,
			     void (*handler_fn) (int,Rawframe_generator*,std::string,std::vector<std::string> ,State_machine_simulation_core* , sockaddr_storage,int,std::string,std::string))
{
	auto THIS = smc;
	DEBUG_FUNC_PROLOGUE2
	std::vector<std::thread*> client_handler_threads;

	socklen_t addrlen = sizeof(struct sockaddr_storage);
	struct sockaddr_storage claddr = {0};
	int cfd = -1;

	if (reuse_sock){
		DEBUG << "[comm_generic_tcp_in_dispatcher_thread][REUSE_SOCK("<< sock_name <<")]\n";
		for(;;){
			{
				std::lock_guard<std::recursive_mutex>g(smc->get_reg_sock_mtx());
				auto it = smc->get_reg_socks().find(sock_name);
				if (it != smc->get_reg_socks().end()){
					cfd = it->second;
					new std::thread(*handler_fn,id,gen,ev_id,params,smc,claddr,cfd,som,eof);
					return;
				}
			}
			std::this_thread::sleep_for(std::chrono::microseconds(1000));continue;
		}
	}


	struct addrinfo hints;
	struct addrinfo* result, * rp;
	int lfd;

	memset(&hints,0,sizeof(struct addrinfo));
	hints.ai_canonname = nullptr;
	hints.ai_addr = nullptr;
	hints.ai_next = nullptr;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_INET;
    hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;

    if (getaddrinfo(nullptr,port.c_str(),&hints,&result) != 0)
    	smc->fatal_(-1,"getaddrinfo failed");

    int optval=1;

	for(rp=result;rp;rp=rp->ai_next)
	{
		lfd = socket(rp->ai_family,rp->ai_socktype,rp->ai_protocol);
		if(lfd == -1) continue;
		if (setsockopt(lfd,SOL_SOCKET,SO_REUSEADDR,(char*)&optval,sizeof(optval))) smc->fatal_(-1,"setsockopt");
		if (bind(lfd,rp->ai_addr,rp->ai_addrlen) == 0) break;
		close(lfd);
	}
	if (!rp) smc->fatal_(-1,"comm_dispatcher_thread:Could not bind socket to any address.port="+port);

	if (listen(lfd,5)==-1)smc->fatal_(-1,"listen");

	freeaddrinfo(result);

	for(;!smc->shutdown();)
	{
		cfd = accept(lfd, (struct sockaddr*) &claddr, &addrlen);
		if (reg_sock){
			std::lock_guard<std::recursive_mutex> g(smc->get_reg_sock_mtx());
			smc->get_reg_socks()[sock_name] = cfd;
			DEBUG << "[SOCKET_REGISTERED ("<< sock_name <<")]\n";
		}
		if (cfd == -1){
			DEBUG << "[ERROR_COMM_DISPATCHER][ACCEPT_FAILED]\n";continue;
		}
		if (handler_fn)
			client_handler_threads.push_back(new std::thread(*handler_fn,id,gen,ev_id,params,smc,claddr,cfd,som,eof));
		else close(cfd);
	}
	for(auto tp: client_handler_threads) tp->join();
}







