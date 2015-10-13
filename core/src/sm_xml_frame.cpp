#include "core/include/sm_xml_frame.hpp"
#include "core/include/state_machine_simulation_core.hpp"
#include <sys/types.h>
#include <limits>
#include <cstring>
#include "pugixml.hpp"


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

extern ceps::ast::Nodebase_ptr eval_locked_ceps_expr(State_machine_simulation_core* smc,
		 State_machine* containing_smp,
		 ceps::ast::Nodebase_ptr node,
		 ceps::ast::Nodebase_ptr root_node);


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
	} else {
		auto r = eval_locked_ceps_expr(smc,nullptr,data,nullptr);
		if (r != nullptr) make_xml_fragment(ss,smc,r,inside_attr);
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


	std::stringstream ss;
	ss << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
	make_xml_fragment(ss,smc,data_format.nodes());
	DEBUG << "\n\n\n";
	DEBUG << ss.str() << "\n";
	DEBUG << "\n\n\n";

	char* data = new char[ss.str().size()+1];
	memcpy(data,ss.str().c_str(),ss.str().size());data[ss.str().size()] = 0;
	data_size = ss.str().size();
	return data;
}

bool Xmlframe_generator::read_msg(char* xml_data,size_t size,
		                          State_machine_simulation_core* smc,
		                          std::vector<std::string> params,
		                          std::vector<ceps::ast::Nodebase_ptr>& payload)
{
	//Read xml_data, store in symbol table
	{
		pugi::xml_document* xml_doc = new pugi::xml_document();
		auto r = xml_doc->load_buffer(xml_data,size);

		if (!r){
			//std::cout << "\n\n" << xml_data << "\n\n";
			State_machine_simulation_core::event_t ev("runtime_xml_exception");
			ev.already_sent_to_out_queues_ = true;
			ev.payload_.push_back(new ceps::ast::String(r.description(),nullptr,nullptr,nullptr));
			smc->main_event_queue().push(ev);
			return false;
		}
		std::lock_guard<std::recursive_mutex>g(smc->states_mutex());
		auto & symtab = smc->ceps_env_current().get_global_symboltable();
		auto sym = symtab.lookup_global("@@current_xml_doc",false);
		if (sym != nullptr) delete (pugi::xml_document*)sym->payload;
		else sym = symtab.lookup_global("@@current_xml_doc",true);
		if(sym == nullptr) smc->fatal_(-1,"Internal Error #330\n");
		sym->payload = xml_doc;
	}




	auto query = spec_["query"];

	for(auto p : query.nodes()){
		using namespace ceps::ast;
		if (p == nullptr) return false;


		if (p->kind() == ceps::ast::Ast_node_kind::ifelse)
		{

			auto ifelse = ceps::ast::as_ifelse_ptr(p);
			auto cond =  eval_locked_ceps_expr(smc,nullptr,ifelse->children()[0],ifelse);

			if (cond->kind() != ceps::ast::Ast_node_kind::int_literal &&  cond->kind() != ceps::ast::Ast_node_kind::float_literal){
				//std::stringstream ss; ss << *cond;
				//ERRORLOG << "Expression in conditional should evaluate to int or double. Read:" << ss.str();
				return false;
			}
			bool take_left_branch = true;
			if (cond->kind() == ceps::ast::Ast_node_kind::int_literal)
				take_left_branch = ceps::ast::value(ceps::ast::as_int_ref(cond)) != 0;
			else if (cond->kind() == ceps::ast::Ast_node_kind::float_literal)
				take_left_branch = ceps::ast::value(ceps::ast::as_double_ref(cond)) != 0;
			ceps::ast::Nodebase_ptr branch_to_take = nullptr;
			if (take_left_branch && ifelse->children().size() > 1) branch_to_take = ifelse->children()[1];
			else if (!take_left_branch && ifelse->children().size() > 2) branch_to_take = ifelse->children()[2];
			if (branch_to_take == nullptr) return false;
			/*if (branch_to_take->kind() != ceps::ast::Ast_node_kind::structdef && branch_to_take->kind() != ceps::ast::Ast_node_kind::scope)
			 read_msg(xml_data,size,smc,branch_to_take);
			else read_msg(xml_data,size,smc,ceps::ast::nlf_ptr(branch_to_take)->children());*/


		}
		else if ( smc->is_assignment_op(p ) )
		{
			auto & node = ceps::ast::as_binop_ref(p);
			std::string state_id;
			if (smc->is_assignment_to_state(node,state_id))
			{
				auto rhs =  eval_locked_ceps_expr(smc,nullptr,node.right(),p);
				if (rhs == nullptr) return false;

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

		}else{
		 auto r = eval_locked_ceps_expr(smc,nullptr,p,nullptr);
		}
	}

	return true;
}
