#include "core/include/sm_raw_frame.hpp"
#include "core/include/state_machine_simulation_core.hpp"
#include <sys/types.h>
#include <limits>
#include <cstring>


#ifdef __gnu_linux__
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <endian.h>
#else
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#pragma comment(lib, "Ws2_32.lib")
static inline int write(SOCKET s, const void* buf, int len, int flags = 0) { return send(s, (char*)buf, len, flags); }
static inline int close(SOCKET s) { return closesocket(s); }
typedef std::int64_t ssize_t;
#include <intrin.h>


static inline std::uint64_t be64toh(std::uint64_t i) { return _byteswap_uint64(i); }
static inline std::uint64_t htobe64(std::uint64_t i) { return _byteswap_uint64(i); }
static inline std::uint32_t htobe32(std::uint32_t i) { return htonl(i); }
static inline  void bzero(void *s, size_t n) { memset(s, 0, n); }

#endif
#endif

#ifdef __GNUC__
#define __funcname__ __PRETTY_FUNCTION__
#else
#define __funcname__ __FUNCSIG__
#endif
#define DEBUG_FUNC_PROLOGUE 	Debuglogger debuglog(__funcname__,this,this->print_debug_info_);
#define DEBUG (debuglog << "[DEBUG]", debuglog)
#define ERRORLOG (debuglog << "[ERROR]", debuglog)
#define DEBUG_FUNC_PROLOGUE2 	State_machine_simulation_core::Debuglogger debuglog(__funcname__,THIS,THIS->print_debug_info_);



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

size_t compute_size(State_machine_simulation_core* smc,ceps::ast::Nodebase_ptr p,std::vector<std::string> const & params){
 using namespace ceps::ast;
 if (p == nullptr) return 0;
 if (p->kind() == ceps::ast::Ast_node_kind::int_literal) return sizeof(std::int64_t);
 if (p->kind() == ceps::ast::Ast_node_kind::float_literal) return sizeof(double);
 if (p->kind() == ceps::ast::Ast_node_kind::string_literal)
	 return ceps::ast::value(ceps::ast::as_string_ref(p)).length();
 if (p->kind() == ceps::ast::Ast_node_kind::symbol){
	 ceps::ast::Symbol& sym = ceps::ast::as_symbol_ref(p);
	 if (ceps::ast::kind(sym) == "Systemstate")
	 {
		 //std::cerr << "!!!!!!!!!!" << std::endl;
		 std::lock_guard<std::recursive_mutex>g(smc->states_mutex());
		 auto it = smc->global_systemstates().find( ceps::ast::name(sym));
		 if (it == smc->global_systemstates().end()) return 0;
		 if (it->second->kind() == ceps::ast::Ast_node_kind::int_literal)
			 return sizeof(std::int64_t);
		 else return 0;
	 }
 }
 if (p->kind() == ceps::ast::Ast_node_kind::identifier){
	 ceps::ast::Identifier& id = ceps::ast::as_id_ref(p);
	 std::string id_name = ceps::ast::name(id);
	 for(auto x:params) if (x==id_name) return sizeof(std::int64_t);
	 return 0;
 }
 if (p->kind() == ceps::ast::Ast_node_kind::func_call)
 {
	 std::string func_name;	std::vector<ceps::ast::Nodebase_ptr> args;
	 read_func_call_values(smc,p,func_name,args);
	 if (func_name == "uint" || func_name == "int")
	 {
		 if (args.size() == 0 || args.size() > 2)
			 smc->fatal_(-1, "uint: uint takes one or two arguments: uint(VALUE) or uint(1-64,VALUE)");
		 if (args.size() == 1) return sizeof(std::int32_t);

		 if (args[0]->kind() != ceps::ast::Ast_node_kind::int_literal)
			 smc->fatal_(-1, "uint: expected an integral scalar as first argument.");
		 return(value(as_int_ref(args[0])) / 8);
	 }
 }
 return 0;
}

size_t compute_size(State_machine_simulation_core* smc,std::vector<ceps::ast::Nodebase_ptr> pattern) {
	size_t acc = 0;
	std::vector<std::string> dummy;
	for(auto p : pattern){

		acc += compute_size(smc,p,dummy);
	}
	return acc;
}

size_t fill_raw_chunk(State_machine_simulation_core* smc,ceps::ast::Nodebase_ptr p,char* data,bool host_byte_order = false) {
	using namespace ceps::ast;
	if (p == nullptr) return 0;
	if (p->kind() == ceps::ast::Ast_node_kind::int_literal){
		if (!host_byte_order ) *((std::int64_t*)data) = htobe64(value(as_int_ref(p)));
		else  *((std::int64_t*)data) = value(as_int_ref(p));
		return sizeof(std::int64_t);
	}
	 if (p->kind() == ceps::ast::Ast_node_kind::float_literal) {
		 if (!host_byte_order ) *((double*)data) = htobe64((std::uint64_t)value(as_double_ref(p)));
		 else *((double*)data) = (std::uint64_t)value(as_double_ref(p));
		 return sizeof(double);
	 }
	 if (p->kind() == ceps::ast::Ast_node_kind::string_literal){
		 std::string s = ceps::ast::value(ceps::ast::as_string_ref(p));
		 memcpy(data,s.c_str(),s.length());
		 return s.length();
	 }
	 if (p->kind() == ceps::ast::Ast_node_kind::func_call)
	 {
		 std::string func_name;	std::vector<ceps::ast::Nodebase_ptr> args;
		 read_func_call_values(smc,p,func_name,args);
		 if (func_name == "uint" || func_name == "int")
		 {
			 if (args.size() == 0 || args.size() > 2)
				 smc->fatal_(-1, "uint: uint takes one or two arguments: uint(VALUE) or uint(1-64,VALUE)");
			 if (args.size() == 1) return sizeof(std::int32_t);

			 if (args[0]->kind() != ceps::ast::Ast_node_kind::int_literal)
				 smc->fatal_(-1, "uint: expected an integral scalar as first argument.");
			 if (args[1]->kind() != ceps::ast::Ast_node_kind::int_literal)
				 smc->fatal_(-1, "uint: expected an integral value as second argument.");

			 auto v = value(as_int_ref(args[1]));
			 size_t l = value(as_int_ref(args[0])) / 8;
			 if (l == 8) memcpy(data, (char*)&v,1);
			 else if (l == 16) memcpy(data, (char*)&v,2);
			 else if (l == 24) memcpy(data, (char*)&v,3);
			 else if (l == 32) {auto vv = htobe32(v); memcpy(data, (char*)&vv,4);}
			 else if (l == 40) memcpy(data, (char*)&v,5);
			 else if (l == 48) memcpy(data, (char*)&v,6);
			 else if (l == 56) memcpy(data, (char*)&v,7);
			 else memcpy(data, (char*)&v,6);

			 return l;
		 }
	 }
	 return 0;
}

void fill_raw_chunk(State_machine_simulation_core* smc,std::vector<ceps::ast::Nodebase_ptr> pattern,char* data) {
	size_t offs = 0;
	for(auto p : pattern){
		offs += fill_raw_chunk(smc,p,data+offs);
	}
}

size_t Podframe_generator::compute_size_of_msg(State_machine_simulation_core* smc,
		                                       std::vector<std::string> params,bool& failed)
{
	size_t acc = 0;
	failed = true;
	auto data_format = spec_["data"];

	for(auto p : data_format.nodes()){
		//std::cerr << *p << std::endl;
		auto t =  compute_size(smc,p,params);
		if (t == 0) return 0;
		acc += t;
	}

	failed = false;

	return acc;
}

void* Podframe_generator::gen_msg(State_machine_simulation_core* smc,size_t& data_size){

	if (smc == nullptr) return nullptr;
	auto THIS = smc;
	DEBUG_FUNC_PROLOGUE2;
	auto data_format = spec_["data"];
	if (data_format.nodes().empty()) return nullptr;

	ceps_interface_eval_func_callback_ctxt_t ctxt;
	ctxt.active_smp = nullptr;
	ctxt.smc  = smc;

	ceps::ast::Nodebase_ptr frame_pattern = nullptr;
	ceps::ast::Scope scope;
	scope.children() = data_format.nodes();
	{
		std::lock_guard<std::recursive_mutex>g(smc->states_mutex());


		smc->ceps_env_current().interpreter_env().symbol_mapping()["Systemstate"] = &smc->global_systemstates();
		smc->ceps_env_current().interpreter_env().set_func_callback(ceps_interface_eval_func_callback,&ctxt);
		smc->ceps_env_current().interpreter_env().set_binop_resolver(ceps_interface_binop_resolver,this);
		frame_pattern = ceps::interpreter::evaluate(&scope,
				smc->ceps_env_current().get_global_symboltable(),
				smc->ceps_env_current().interpreter_env(),nullptr	);
		smc->ceps_env_current().interpreter_env().set_func_callback(nullptr,nullptr);
		smc->ceps_env_current().interpreter_env().set_binop_resolver(nullptr,nullptr);
	}

	if (frame_pattern == nullptr) return nullptr;
	auto chunk_size = compute_size(smc,ceps::ast::nlf_ptr(frame_pattern)->children());
	DEBUG << "[Podframe_generator::gen_msg][CHUNK_SIZE="<<chunk_size<<"]\n";
	char* data = new char[chunk_size];
	bzero(data,chunk_size);
	fill_raw_chunk( smc,ceps::ast::nlf_ptr(frame_pattern)->children(), data);
	for(size_t offs = 0; offs < chunk_size;++offs)
		DEBUG <<"[Podframe_generator::gen_msg][CHUNK_BYTE_"<< offs << "="<< ((std::uint32_t) *( (unsigned char*)data+offs)) << "]\n";
	data_size = chunk_size;
	return data;
}

bool Podframe_generator::read_msg(char* data,size_t size,
		                          State_machine_simulation_core* smc,
		                          std::vector<std::string> params,
		                          std::vector<ceps::ast::Nodebase_ptr>& payload)
{
	std::map<std::string,int> p2i;
	for(size_t i = 0;i < params.size(); ++i) {p2i[params[i]] = i;payload.push_back(nullptr);}
	auto data_format = spec_["data"];
	size_t offs = 0;

	for(auto p : data_format.nodes()){
		using namespace ceps::ast;
		if (p == nullptr) return false;

		if (p->kind() == ceps::ast::Ast_node_kind::int_literal){
			offs += sizeof(std::int64_t);
		}
		else if (p->kind() == ceps::ast::Ast_node_kind::float_literal) {
			 offs += sizeof(double);
	    }
		else  if (p->kind() == ceps::ast::Ast_node_kind::string_literal){
			 std::string s = ceps::ast::value(ceps::ast::as_string_ref(p));
			 offs += s.length();
		} else if (p->kind() == ceps::ast::Ast_node_kind::symbol && kind(as_symbol_ref(p)) == "Systemstate")
		{
			std::string state_id = name(as_symbol_ref(p));
			auto it = smc->get_global_states().find(state_id);
			if (it == smc->get_global_states().end()) return false;
			auto state_value = it->second;
			if (state_value == nullptr) return 0;
			if (state_value->kind() == ceps::ast::Ast_node_kind::int_literal){
				Int& v = as_int_ref(state_value);
				value(v) = be64toh( *((std::int64_t*)(data+offs)) );
				offs += sizeof(std::int64_t);
			} else return false;
		} else if (p->kind() == ceps::ast::Ast_node_kind::identifier)
		{
			std::string id = name(as_id_ref(p));
			if (p2i.find(id) == p2i.end()) return false;
			auto idx = p2i[id];
			payload[idx] = new Int(be64toh( *((std::int64_t*)(data+offs)) ), ceps::ast::all_zero_unit(),nullptr,nullptr,nullptr );
			offs += sizeof(std::int64_t);
		}
	}

	return true;
}

//XML FRAME


bool Xmlframe_generator::readfrom_spec(ceps::ast::Nodeset const & spec)
{
 spec_ = spec;
 return true;
}

size_t Xmlframe_generator::compute_size_of_msg(State_machine_simulation_core* smc,
		                                       std::vector<std::string> params,bool& failed)
{
	return 0;
}

std::string escape_string_xml(std::string const & s)
{
	std::string result;
	for (size_t i = 0; i < s.length(); ++i)
	{
		if (s[i] == '<')
			result += "&lt;";
		else if (s[i] == '&')
			result += "&amp;";
		else
			result += s[i];
	}
	return result;
}

std::string escape_string_xml_attr(std::string const & s)
{
	std::string result;
	for (size_t i = 0; i < s.length(); ++i)
	{
		if (s[i] == '"')
			result += "\\\"";
		else
			result += s[i];
	}
	return result;
}

void make_xml_fragment(std::stringstream& ss,State_machine_simulation_core* smc,ceps::ast::Nodebase_ptr data,bool inside_attr=false);

inline void make_xml_fragment(std::stringstream& ss,State_machine_simulation_core* smc,ceps::ast::String& data,bool inside_attr=false)
{
	if (!inside_attr) ss << escape_string_xml(ceps::ast::value(data));
	else ss << escape_string_xml_attr(ceps::ast::value(data));
}

inline void make_xml_fragment(std::stringstream& ss,State_machine_simulation_core* smc,ceps::ast::Int& data)
{
	ss << ceps::ast::value(data);
}

inline void make_xml_fragment(std::stringstream& ss,State_machine_simulation_core* smc,ceps::ast::Double& data)
{
	ss << ceps::ast::value(data);
}

void make_xml_fragment(std::stringstream& ss,State_machine_simulation_core* smc,ceps::ast::Struct& data)
{
	ss << "<"<<name(data);
	bool children = false;
	for(auto elem:data.children()){
		if (elem->kind() != ceps::ast::Ast_node_kind::structdef) { children=true;continue;}
		if (ceps::ast::name(ceps::ast::as_struct_ref(elem)) != "xml_attr") {children=true;continue;}
		auto& xml_attr = ceps::ast::as_struct_ref(elem);
		if (xml_attr.children().size() < 2) continue;
		if (xml_attr.children()[0]->kind() != ceps::ast::Ast_node_kind::string_literal) continue;
		ss << " " << ceps::ast::value(ceps::ast::as_string_ref(xml_attr.children()[0]))<< " = \"";
		make_xml_fragment(ss,smc,xml_attr.children()[1],true);
		ss << "\" ";
	}
	if (children) {
		ss << ">";
		for(auto elem:data.children()){
			if (elem->kind() == ceps::ast::Ast_node_kind::structdef && ceps::ast::name(ceps::ast::as_struct_ref(elem)) == "xml_attr") continue;
			make_xml_fragment(ss,smc,elem);
		}
		ss << "</"<<name(data)<<">";
	}
	else ss << "/>";
}
extern void define_a_struct(State_machine_simulation_core*,ceps::ast::Struct_ptr sp, std::map<std::string, ceps::ast::Nodebase_ptr> & vars,std::string prefix);
void make_xml_fragment(std::stringstream& ss,State_machine_simulation_core* smc,std::vector<ceps::ast::Nodebase_ptr> data);

void make_xml_fragment(std::stringstream& ss,State_machine_simulation_core* smc,ceps::ast::Nodebase_ptr data,bool inside_attr)
{
	auto THIS = smc;
	DEBUG_FUNC_PROLOGUE2
	if (data == nullptr) return;
	if (data->kind() == ceps::ast::Ast_node_kind::string_literal)
		    make_xml_fragment(ss,smc,ceps::ast::as_string_ref(data),inside_attr);
	else if (data->kind() == ceps::ast::Ast_node_kind::int_literal)
			make_xml_fragment(ss,smc,ceps::ast::as_int_ref(data));
	else if (data->kind() == ceps::ast::Ast_node_kind::float_literal)
			make_xml_fragment(ss,smc,ceps::ast::as_double_ref(data));
	else if (data->kind() == ceps::ast::Ast_node_kind::structdef)
			make_xml_fragment(ss,smc,ceps::ast::as_struct_ref(data));
	else if (data->kind() == ceps::ast::Ast_node_kind::ifelse)
	{
		auto n = data;
		auto ifelse = ceps::ast::as_ifelse_ptr(n);
		ceps::ast::Nodebase_ptr cond = nullptr;
		ceps_interface_eval_func_callback_ctxt_t ctxt;
		ctxt.active_smp = nullptr;
		ctxt.smc  = smc;
		{
			std::lock_guard<std::recursive_mutex>g(smc->states_mutex());

			smc->ceps_env_current().interpreter_env().symbol_mapping()["Systemstate"] = &smc->get_global_states();
			smc->ceps_env_current().interpreter_env().set_func_callback(ceps_interface_eval_func_callback,&ctxt);
			smc->ceps_env_current().interpreter_env().set_binop_resolver(ceps_interface_binop_resolver,smc);
			cond = ceps::interpreter::evaluate(ifelse->children()[0],
					smc->ceps_env_current().get_global_symboltable(),
					smc->ceps_env_current().interpreter_env(),n	);
			smc->ceps_env_current().interpreter_env().set_func_callback(nullptr,nullptr);
			smc->ceps_env_current().interpreter_env().set_binop_resolver(nullptr,nullptr);
		}

		if (cond->kind() != ceps::ast::Ast_node_kind::int_literal &&  cond->kind() != ceps::ast::Ast_node_kind::float_literal){
			std::stringstream ss; ss << *cond;
			ERRORLOG << "Expression in conditional should evaluate to int or double. Read:" << ss.str();
			return;
		}
		bool take_left_branch = true;
		if (cond->kind() == ceps::ast::Ast_node_kind::int_literal) take_left_branch = ceps::ast::value(ceps::ast::as_int_ref(cond)) != 0;
		else if (cond->kind() == ceps::ast::Ast_node_kind::float_literal) take_left_branch = ceps::ast::value(ceps::ast::as_double_ref(cond)) != 0;
		ceps::ast::Nodebase_ptr branch_to_take = nullptr;
		if (take_left_branch && ifelse->children().size() > 1) branch_to_take = ifelse->children()[1];
		else if (!take_left_branch && ifelse->children().size() > 2) branch_to_take = ifelse->children()[2];
		if (branch_to_take == nullptr) return;
		if (branch_to_take->kind() != ceps::ast::Ast_node_kind::structdef && branch_to_take->kind() != ceps::ast::Ast_node_kind::scope)
		 make_xml_fragment(ss,smc,branch_to_take);
		else make_xml_fragment(ss,smc,ceps::ast::nlf_ptr(branch_to_take)->children());
	}
	else if ( smc->is_assignment_op(data) )
	{
		auto & node = ceps::ast::as_binop_ref(data);
		std::string state_id;


		if (smc->is_assignment_to_state(node,state_id))
		{
			ceps::ast::Nodebase_ptr rhs = nullptr;
			ceps_interface_eval_func_callback_ctxt_t ctxt;
			ctxt.active_smp = nullptr;
			ctxt.smc  = smc;
			{
			 std::lock_guard<std::recursive_mutex>g(smc->states_mutex());
			 smc->ceps_env_current().interpreter_env().symbol_mapping()["Systemstate"] = &smc->global_systemstates();
			 smc->ceps_env_current().interpreter_env().set_func_callback(ceps_interface_eval_func_callback,&ctxt);
			 smc->ceps_env_current().interpreter_env().set_binop_resolver(ceps_interface_binop_resolver,smc);
			 rhs = ceps::interpreter::evaluate(node.right(),smc->ceps_env_current().get_global_symboltable(),smc->ceps_env_current().interpreter_env(),data);
			 smc->ceps_env_current().interpreter_env().set_func_callback(nullptr,nullptr);
			 smc->ceps_env_current().interpreter_env().set_binop_resolver(nullptr,nullptr);
			 smc->ceps_env_current().interpreter_env().symbol_mapping().clear();
			}
			if (rhs == nullptr) return;

			if (node.right()->kind() == ceps::ast::Ast_node_kind::identifier)
			{
				std::string id = ceps::ast::name(ceps::ast::as_id_ref(node.right()));
				auto sym = smc->ceps_env_current().get_global_symboltable().lookup(id);
				if (sym != nullptr) {
				  if (sym->payload) {
					  std::lock_guard<std::recursive_mutex>g(smc->states_mutex());
					  smc->get_global_states()[state_id] = (ceps::ast::Nodebase_ptr)sym->payload;
				  }
				} else {
				 auto it = smc->type_definitions().find(id);
				 if (it == smc->type_definitions().end())
						smc->fatal_(-1,id+" is not a type.\n");
				 define_a_struct(smc,ceps::ast::as_struct_ptr(it->second),smc->get_global_states(),name(as_symbol_ref(node.left())) );
				}
			} else 	{ std::lock_guard<std::recursive_mutex>g(smc->states_mutex());smc->get_global_states()[state_id] = rhs;}
		}

	} else if (data->kind() == ceps::ast::Ast_node_kind::func_call) {
		ceps_interface_eval_func_callback_ctxt_t ctxt;
		ctxt.active_smp = nullptr;
		ctxt.smc  = smc;

		ceps::ast::Nodebase_ptr r = nullptr;
		{

		std::lock_guard<std::recursive_mutex>g(smc->states_mutex());


		smc->ceps_env_current().interpreter_env().symbol_mapping()["Systemstate"] = &smc->global_systemstates();
		smc->ceps_env_current().interpreter_env().set_func_callback(ceps_interface_eval_func_callback,&ctxt);
		smc->ceps_env_current().interpreter_env().set_binop_resolver(ceps_interface_binop_resolver,smc);

		r = ceps::interpreter::evaluate(data,
				smc->ceps_env_current().get_global_symboltable(),
				smc->ceps_env_current().interpreter_env(),nullptr	);

		smc->ceps_env_current().interpreter_env().set_func_callback(nullptr,nullptr);
		smc->ceps_env_current().interpreter_env().set_binop_resolver(nullptr,nullptr);
		smc->ceps_env_current().interpreter_env().symbol_mapping().clear();
		}

		if (r != nullptr)
			make_xml_fragment(ss,smc,r);

	}
}

void make_xml_fragment(std::stringstream& ss,State_machine_simulation_core* smc,std::vector<ceps::ast::Nodebase_ptr> data)
{
	for(auto e : data)
		make_xml_fragment(ss,smc,e);
}

void* Xmlframe_generator::gen_msg(State_machine_simulation_core* smc,size_t& data_size){


	if (smc == nullptr) return nullptr;
	auto THIS = smc;
	DEBUG_FUNC_PROLOGUE2;
	auto data_format = spec_["data"];
	if (data_format.nodes().empty()) return nullptr;

	ceps_interface_eval_func_callback_ctxt_t ctxt;
	ctxt.active_smp = nullptr;
	ctxt.smc  = smc;

	ceps::ast::Nodebase_ptr frame_pattern = nullptr;
	//ceps::ast::Scope scope;
	//scope.children() = data_format.nodes();
	//DEBUG << "[" << __funcname__ << "]" << "[EVAL_DATA(1)]\n";
	//DEBUG << "[" << __funcname__ << "]" << scope << "\n";

	std::stringstream ss;
	ss << "<?xml version=\"1.0\" encoding=\"UTF-8\">";
	make_xml_fragment(ss,smc,data_format.nodes());
	/*{
		std::lock_guard<std::recursive_mutex>g(smc->states_mutex());


		smc->ceps_env_current().interpreter_env().symbol_mapping()["Systemstate"] = &smc->global_systemstates();
		smc->ceps_env_current().interpreter_env().set_func_callback(ceps_interface_eval_func_callback,&ctxt);
		smc->ceps_env_current().interpreter_env().set_binop_resolver(ceps_interface_binop_resolver,this);

		frame_pattern = ceps::interpreter::evaluate(&scope,
				smc->ceps_env_current().get_global_symboltable(),
				smc->ceps_env_current().interpreter_env(),nullptr	);
		smc->ceps_env_current().interpreter_env().set_func_callback(nullptr,nullptr);
		smc->ceps_env_current().interpreter_env().set_binop_resolver(nullptr,nullptr);
	}*/
	//DEBUG << "[" << __funcname__ << "]" << "[EVAL_DATA(2)]\n";
	//DEBUG << "[" << __funcname__ << "]" << *frame_pattern << "\n";

	//scope.children().clear();
	DEBUG << ss.str() << "\n";
	if (frame_pattern == nullptr) return nullptr;
	char* data = nullptr;//new char[chunk_size];
	return data;
}

bool Xmlframe_generator::read_msg(char* data,size_t size,
		                          State_machine_simulation_core* smc,
		                          std::vector<std::string> params,
		                          std::vector<ceps::ast::Nodebase_ptr>& payload)
{
	return false;
}


void comm_sender_generic_tcp_out_thread(threadsafe_queue< std::pair<char*,size_t>, std::queue<std::pair<char*,size_t> >>* frames,
			     State_machine_simulation_core* smc,
			     std::string ip,
			     std::string port)
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
			DEBUG << "[comm_sender_generic_tcp_out_thread][CONNECTING]\n";
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
				//DEBUG << "[comm_sender_generic_tcp_out_thread][FAILED_TO_CONNECT]\n";
				std::this_thread::sleep_for(std::chrono::microseconds(1000));continue;
			}

			 for (rp = result; rp != NULL; rp = rp->ai_next) {
			  cfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
			  if (cfd == -1)	continue;
			  if (connect(cfd, rp->ai_addr, rp->ai_addrlen) != -1)break;
			  close(cfd);
			 }
			 if (result != nullptr) freeaddrinfo(result);
			 if (rp == nullptr) {
				 //DEBUG << "[comm_sender_generic_tcp_out_thread][FAILED_TO_CONNECT]\n";
				 std::this_thread::sleep_for(std::chrono::microseconds(1000));continue;
			 }
			}
			conn_established = true;
		}

		DEBUG << "[comm_sender_generic_tcp_out_thread][SEND_FRAME]\n";


		auto len = frame_size;
		int wr = 0;

		if (len && frame) if ( (wr = write(cfd, frame,len )) != len)
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
		 sockaddr_storage claddr,int sck)
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


	size_t frame_size = 0;
	bool failed_size_computation = true;
	for(;failed_size_computation;)
	{
		frame_size = gen->compute_size_of_msg(smc,params,failed_size_computation);
		if (failed_size_computation) std::this_thread::sleep_for(std::chrono::microseconds(1));
	}

	DEBUG << "[comm_generic_tcp_in_thread_fn][FRAME_SIZE_AVAILABLE]" << addrstr << "\n";

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
			     std::string port,
			     void (*handler_fn) (int,Rawframe_generator*,std::string,std::vector<std::string> ,State_machine_simulation_core* , sockaddr_storage,int))
{
	auto THIS = smc;
	DEBUG_FUNC_PROLOGUE2
	std::vector<std::thread*> client_handler_threads;


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

		socklen_t addrlen = sizeof(struct sockaddr_storage);
		struct sockaddr_storage claddr;
		int cfd = accept(lfd, (struct sockaddr*) &claddr, &addrlen);
		if (cfd == -1){
			DEBUG << "[ERROR_COMM_DISPATCHER][ACCEPT_FAILED]\n";continue;
		}
		if (handler_fn)
			client_handler_threads.push_back(new std::thread(*handler_fn,id,gen,ev_id,params,smc,claddr,cfd));
		else close(cfd);
	}
	for(auto tp: client_handler_threads) tp->join();
}







