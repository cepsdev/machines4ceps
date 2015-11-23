#include "core/include/sm_xml_frame.hpp"
#include "core/include/state_machine_simulation_core.hpp"
#include <sys/types.h>
#include <limits>
#include <cstring>
#include "pugixml.hpp"

#include "core/include/base_defs.hpp"


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
		cond = eval_locked_ceps_expr(smc,nullptr,ifelse->children()[0],n);

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
			auto rhs = eval_locked_ceps_expr(smc,nullptr,node.right(),data);
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
		ceps::ast::Nodebase_ptr r = nullptr;
		r  = eval_locked_ceps_expr(smc,nullptr,data,nullptr);
		if (r != nullptr)
			make_xml_fragment(ss,smc,r);
	} else {
		//std::cout << "IN:" << *data << std::endl;
		auto r = eval_locked_ceps_expr(smc,nullptr,data,nullptr);
		//std::cout << "OUT:" << *r << std::endl;
		if (r != nullptr) {
			if (r->kind() != ceps::ast::Ast_node_kind::int_literal &&
					r->kind() != ceps::ast::Ast_node_kind::string_literal &&
					r->kind() != ceps::ast::Ast_node_kind::float_literal &&
					r->kind() != ceps::ast::Ast_node_kind::ifelse &&
					r->kind() != ceps::ast::Ast_node_kind::binary_operator &&
					r->kind() != ceps::ast::Ast_node_kind::func_call
					) {std::stringstream ss; ss << *r; smc->fatal_(-1,"Illformed xml-data section:"+ss.str()+"\n");}
			make_xml_fragment(ss,smc,r,inside_attr);
		}
	}
}

void make_xml_fragment(std::stringstream& ss,State_machine_simulation_core* smc,std::vector<ceps::ast::Nodebase_ptr> data)
{
	for(auto e : data)
		make_xml_fragment(ss,smc,e);
}

void* Xmlframe_generator::gen_msg(State_machine_simulation_core* smc,size_t& data_size){
	if (smc == nullptr) return nullptr;
	DEBUG_FUNC_PROLOGUE2;
	auto data_format = spec_["data"];
	if (data_format.nodes().empty()) return nullptr;

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

static bool eval_query(std::vector<ceps::ast::Nodebase_ptr>& nodes,State_machine_simulation_core* smc){

	for(auto p : nodes){
		using namespace ceps::ast;
		if (p == nullptr) return false;


		if (p->kind() == ceps::ast::Ast_node_kind::ifelse)
		{

			auto ifelse = ceps::ast::as_ifelse_ptr(p);
			auto cond =  eval_locked_ceps_expr(smc,nullptr,ifelse->children()[0],ifelse);

			if (cond->kind() != ceps::ast::Ast_node_kind::int_literal &&  cond->kind() != ceps::ast::Ast_node_kind::float_literal){
				std::stringstream ss; ss << *cond;
				smc->fatal_(-1,"Expression in conditional should evaluate to int or double. Read:" + ss.str());
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
			if (branch_to_take == nullptr) continue;
			if (branch_to_take->kind() != ceps::ast::Ast_node_kind::structdef && branch_to_take->kind() != ceps::ast::Ast_node_kind::scope){
				std::vector<ceps::ast::Nodebase_ptr> v = {branch_to_take};
				eval_query(v,smc);
			} else eval_query(ceps::ast::nlf_ptr(branch_to_take)->children(),smc);
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
		 eval_locked_ceps_expr(smc,nullptr,p,nullptr);
		}
	}
	return true;
}

bool Xmlframe_generator::read_msg(char* xml_data,size_t size,
		                          State_machine_simulation_core* smc,
		                          std::vector<std::string> params,
		                          std::vector<ceps::ast::Nodebase_ptr>& payload)
{
	//std::cout << xml_data << std::endl;
	//Read xml_data, store in symbol table
	{
		pugi::xml_document* xml_doc = new pugi::xml_document();
		auto r = xml_doc->load_buffer(xml_data,size);

		if (!r){
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
	if (query.size() == 0){
		std::cout << spec_ <<std::endl;
		smc->warn_(-1,"No Query defined (tcp_generic_in)");
	}
	return eval_query(query.nodes(),smc);
}
