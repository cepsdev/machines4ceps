#define _CRT_SECURE_NO_WARNINGS

#include "core/include/state_machine_simulation_core.hpp"
#include "core/include/base_defs.hpp"
#include "pugixml.hpp"
#include <time.h>
#include <tuple>
#include "core/include/base_defs.hpp"

bool read_func_call_values(State_machine_simulation_core* smc,	ceps::ast::Nodebase_ptr root_node,
							std::string & func_name,
							std::vector<ceps::ast::Nodebase_ptr>& args);

extern void flatten_args(State_machine_simulation_core* smc,ceps::ast::Nodebase_ptr r, std::vector<ceps::ast::Nodebase_ptr>& v, char op_val = ',');
extern std::string to_string(std::vector<ceps::ast::Nodebase_ptr>const& v);
extern std::string to_string(State_machine_simulation_core* smc,ceps::ast::Nodebase_ptr p);

static ceps::ast::Nodebase_ptr handle_make_byte_sequence(State_machine_simulation_core *smc, std::vector<ceps::ast::Nodebase_ptr> args,State_machine* active_smp);
static ceps::ast::Nodebase_ptr handle_send_cmd(State_machine_simulation_core *sm, std::vector<ceps::ast::Nodebase_ptr> args,State_machine* active_smp);
static ceps::ast::Nodebase_ptr handle_breakup_byte_sequence(
		State_machine_simulation_core *smc, std::vector<ceps::ast::Nodebase_ptr> args,State_machine* active_smp, ceps::parser_env::Symboltable & sym_table);


static void flatten_args2(ceps::ast::Nodebase_ptr r, std::vector<ceps::ast::Nodebase_ptr>& v, char op_val = ',')
{
	using namespace ceps::ast;
	if (r == nullptr) return;
	if (r->kind() == ceps::ast::Ast_node_kind::binary_operator && op(as_binop_ref(r)) ==  op_val)
	{
		auto& t = as_binop_ref(r);
		flatten_args2(t.left(),v,op_val);
		flatten_args2(t.right(),v,op_val);
		return;
	}
	v.push_back(r);
}


static bool is_func_call(ceps::ast::Nodebase_ptr p,std::string& fid,std::vector<ceps::ast::Nodebase_ptr>& args){
 if (p->kind() != ceps::ast::Ast_node_kind::func_call) return false;
 ceps::ast::Func_call& func_call = *dynamic_cast<ceps::ast::Func_call*>(p);
 ceps::ast::Identifier& id = *dynamic_cast<ceps::ast::Identifier*>(func_call.children()[0]);
 ceps::ast::Call_parameters& params = *dynamic_cast<ceps::ast::Call_parameters*>(func_call.children()[1]);
 if (params.children().size()) flatten_args2(params.children()[0], args);
 fid = ceps::ast::name(id);
 return true;
}

static bool is_func_call(ceps::ast::Nodebase_ptr p,std::string& fid,std::vector<ceps::ast::Nodebase_ptr>& args, std::size_t number_of_arguments){
	auto r = is_func_call(p,fid,args);
	if (!r) return r;
	return number_of_arguments == args.size();
}

static bool is_id_or_symbol(ceps::ast::Nodebase_ptr p, std::string& n, std::string& k){
	using namespace ceps::ast;
	if (p->kind() == Ast_node_kind::identifier) {n = name(as_id_ref(p));k = ""; return true;}
	if (p->kind() == Ast_node_kind::symbol) {n = name(as_symbol_ref(p));k = kind(as_symbol_ref(p)); return true;}
	return false;
}

static bool is_id(ceps::ast::Nodebase_ptr p, std::string & result, std::string& base_kind){
	using namespace ceps::ast;
	std::string k,l;
	if (p->kind() == Ast_node_kind::binary_operator && op(as_binop_ref(p)) == '.'){
	 if (!is_id_or_symbol(as_binop_ref(p).right(),k,l)) return false;

	 if (!is_id(as_binop_ref(p).left(),result,base_kind)) return false;
	 result = result + "." + k;
	 return true;
	} else if (is_id_or_symbol(p,k,l)){ base_kind = l; result = k; return true; }
	return false;
}



ceps::ast::Nodebase_ptr State_machine_simulation_core::ceps_interface_eval_func(State_machine* active_smp,
																				std::string const & id,
																				ceps::ast::Call_parameters* params,
																				ceps::parser_env::Symboltable & sym_table)
{
	std::vector<ceps::ast::Nodebase_ptr> args;
	if (params != nullptr && params->children().size()) flatten_args(this,params->children()[0], args, ',');
	{
	 auto it = ceps_fns_.find(id);
	 if (it != ceps_fns_.end()){
		 return (this->*it->second)(id,args,active_smp);
	 }
	}
	if (id == "as_id"){
	 if (args.size() != 1 || args[0]->kind() != ceps::ast::Ast_node_kind::string_literal) fatal_(-1,"as_id: illformed argument(s).");
     return new ceps::ast::Identifier(ceps::ast::value(ceps::ast::as_string_ref(args[0])));
	}else if (id == "as_text"){
	 if (args.size() != 1 ) fatal_(-1,"as_text: illformed argument(s).");
	 if (args[0]->kind () == ceps::ast::Ast_node_kind::identifier)
		 return new ceps::ast::String(ceps::ast::name(ceps::ast::as_id_ref(args[0])));
	 else if (args[0]->kind () == ceps::ast::Ast_node_kind::string_literal)
		 return args[0];
	 else if (args[0]->kind () == ceps::ast::Ast_node_kind::int_literal)
		 return new ceps::ast::String(std::to_string(ceps::ast::value(ceps::ast::as_int_ref(args[0]))));
	 fatal_(-1,"as_text: illformed argument(s).");
	}else if (id == "argv")
	{
		auto const & args =  params->children();
		if (args.size() > 0 && args[0]->kind() == ceps::ast::Ast_node_kind::int_literal){
			auto idx = value(as_int_ref(args[0]));
			if (idx == 0)
				return new ceps::ast::String(current_event().id_,nullptr,nullptr,nullptr);
			else if (idx > 0 && idx-1 < (int)current_event().payload_.size() )
				return current_event().payload_[idx-1];
                        fatal_(-1,"Access to argv: Out of bounds.");
		}
	} else if (id == "argc")
	{
		return new ceps::ast::Int( current_event().payload_.size(), ceps::ast::all_zero_unit(), nullptr, nullptr, nullptr);
	} else if (id == "lookup"){
     if (args.size() != 2 || args[0]->kind() != ceps::ast::Ast_node_kind::identifier) fatal_(-1,"lookup(): illformed argument(s).");
     auto const & tid = ceps::ast::name(ceps::ast::as_id_ref(args[0]));
     auto it = lookup_tables().find(tid);
     if (it == lookup_tables().end()) fatal_(-1,"lookup(): table '"+tid+"' not found.");
     return lookup(it->second,args[1]);
	}
	else if (id == "expect")
	{
		if (!(args.size() >= 1 && args[0]->kind() == ceps::ast::Ast_node_kind::int_literal && 0!=ceps::ast::value(ceps::ast::as_int_ref(args[0])) ))
		{
			std::cerr << "***EXPECTATION NOT SATISFIED";
			if (args.size() > 1) std::cerr << ":\n\t";
			for(int i = 1; i <(int) args.size() ;++i){
				std::cerr << to_string(this,args[i]);
			}
			std::cerr << "\n";
			return new ceps::ast::Int(0,ceps::ast::all_zero_unit(),nullptr,nullptr,nullptr);
		}
		return new ceps::ast::Int(1,ceps::ast::all_zero_unit(),nullptr,nullptr,nullptr);
	}
	else 	if(id == "in_state")
	{
	  if(params->children().size() == 0) fatal_(-1,"Function '"+id+"' expects at least one argument");
	  bool found = false;
	  for(auto p : args)
		 {
	  	   if ( !(p->kind() == ceps::ast::Ast_node_kind::identifier) && !(p->kind() == ceps::ast::Ast_node_kind::binary_operator)){
			 std::stringstream ss;
			 ss << *p;
			 fatal_(-1,"Function '"+id+"': illformed argument, expected a qualified id, got: "+ss.str());
			}
			auto state = resolve_state_qualified_id(p,active_smp);
			if(!state.valid())
			{
			 std::stringstream ss;
			 ss << *p;
			 fatal_(-1,"Function '"+id+"': illformed argument, unknown state: "+ss.str());
			}
			found = this->executionloop_context().current_states[state.id_];
			if(found) break;
		}
	  return new ceps::ast::Int( found ? 1 : 0, ceps::ast::all_zero_unit(), nullptr, nullptr, nullptr);
	} else if (id == "stop") {
		for(auto p : args)
		{
		   if ( !(p->kind() == ceps::ast::Ast_node_kind::identifier) && !(p->kind() == ceps::ast::Ast_node_kind::binary_operator)){
				 std::stringstream ss;
				 ss << *p;
				 fatal_(-1,"Function '"+id+"': illformed argument, expected a qualified id, got: "+ss.str());
			}

		   auto state = resolve_state_qualified_id(p,active_smp);
		   if(!state.valid())
		   {
				 std::stringstream ss;
				 ss << *p;
 				 if (conf_ignore_unresolved_state_id_in_directives())
							log() << "****Warning: stop(): Expression doesn't evaluate to an existing state: "<<ss.str();
 				 else fatal_(-1,"Function '"+id+"': illformed argument, unknown state: "+ss.str());
			}
		   remove_states_.insert(state);
		}
		return new ceps::ast::Int( 1, ceps::ast::all_zero_unit(), nullptr, nullptr, nullptr);

	} else if (id == "make_byte_sequence") {
		return handle_make_byte_sequence(this,args,active_smp);
	}else if (id == "breakup_byte_sequence"){
		return handle_breakup_byte_sequence(this,args,active_smp,sym_table);
	} else if (id == "send")
		return handle_send_cmd(this,args,active_smp);
	else if (id == "write_read_frm"){
		if (args.size() == 2
				&& args[0]->kind() == ceps::ast::Ast_node_kind::identifier
				&& args[1]->kind() == ceps::ast::Ast_node_kind::identifier){
			auto out_frame_id = ceps::ast::name(ceps::ast::as_id_ref(args[0]));
			auto in_frame_id = ceps::ast::name(ceps::ast::as_id_ref(args[1]));
			auto it_out_frame_gen = frame_generators().find(out_frame_id);
			if (it_out_frame_gen == frame_generators().end()) fatal_(-1,"'"+out_frame_id+"' is not a frame id.");
			auto it_in_frame_gen = frame_generators().find(in_frame_id);
			if (it_in_frame_gen == frame_generators().end()) fatal_(-1,"'"+in_frame_id+"' is not a frame id.");
			size_t ds;
			char* msg_block = (char*) it_out_frame_gen->second->gen_msg(this,ds,{});
			if (ds == 0 || msg_block == nullptr) fatal_(-1,"Frame-Pattern'"+out_frame_id+"' couldn't create a valid frame.");
			std::vector<std::string> v1;
			std::vector<ceps::ast::Nodebase_ptr> v2;
			bool r = it_in_frame_gen->second->read_msg(msg_block,ds,this,v1,v2);
			delete[] msg_block;
			if (r) return new ceps::ast::Int(1,ceps::ast::all_zero_unit(),nullptr,nullptr,nullptr);
			return new ceps::ast::Int(0,ceps::ast::all_zero_unit(),nullptr,nullptr,nullptr);
		}
	} else if (id == "changed"){

	 if(args.size() == 1 && (args[0]->kind() == ceps::ast::Ast_node_kind::string_literal ||
			 args[0]->kind() == ceps::ast::Ast_node_kind::identifier || args[0]->kind() == ceps::ast::Ast_node_kind::symbol) ){
       std::string state_id;
       if (args[0]->kind() == ceps::ast::Ast_node_kind::identifier) state_id  = ceps::ast::name(ceps::ast::as_id_ref(args[0]));
       else if (args[0]->kind() == ceps::ast::Ast_node_kind::symbol) state_id  = ceps::ast::name(ceps::ast::as_symbol_ref(args[0]));
       else state_id  = ceps::ast::value(ceps::ast::as_string_ref(args[0]));
	   {
		   std::lock_guard<std::recursive_mutex>g(states_mutex());
		   auto it_cur = get_global_states().find(state_id);
		   //std::cout << "FOUND?" << (it_cur != global_systemstates().end()) << std::endl;
           if (it_cur == get_global_states().end()) {
        	   warn_(-1, "changed(): Uninitialized Systemstate/Systemparameter '" + state_id + "' will be set to 0.");
        	   get_global_states()[state_id] = new ceps::ast::Int(0, ceps::ast::all_zero_unit(), nullptr, nullptr, nullptr);
        	   it_cur = get_global_states().find(state_id);
           }

		   auto it_prev = global_systemstates_prev().find(state_id);
		   //std::cout << "HISTORIZED?" << (it_prev != global_systemstates_prev().end()) << std::endl;
		   if (it_prev == global_systemstates_prev().end()) {
			   //it_prev->second = it_cur->second;
			   global_systemstates_prev()[state_id] = it_cur->second;
			   return new ceps::ast::Int(0, ceps::ast::all_zero_unit(), nullptr, nullptr, nullptr);
		   }
		   if (it_cur->second == it_prev->second)
			   return new ceps::ast::Int(0, ceps::ast::all_zero_unit(), nullptr, nullptr, nullptr);
		   if (it_prev->second == nullptr || it_cur->second == nullptr){
			   it_prev->second = it_cur->second;
			   return new ceps::ast::Int(1, ceps::ast::all_zero_unit(), nullptr, nullptr, nullptr);
	       }
           if (it_cur->second->kind() != it_prev->second->kind()){
        	   it_prev->second = it_cur->second;
        	   return new ceps::ast::Int(1,ceps::ast::all_zero_unit(),nullptr,nullptr,nullptr);
           }

           if (it_cur->second->kind() == ceps::ast::Ast_node_kind::int_literal){
        	   bool r = ceps::ast::value(ceps::ast::as_int_ref(it_cur->second)) != ceps::ast::value(ceps::ast::as_int_ref(it_prev->second))
        	           || ceps::ast::unit(ceps::ast::as_int_ref(it_cur->second)) != ceps::ast::unit(ceps::ast::as_int_ref(it_prev->second));
        	   it_prev->second = it_cur->second;
        	   return new ceps::ast::Int(r,ceps::ast::all_zero_unit(),nullptr,nullptr,nullptr);
           }
           else if (it_cur->second->kind() == ceps::ast::Ast_node_kind::float_literal){
        	   bool r = ceps::ast::value(ceps::ast::as_double_ref(it_cur->second)) != ceps::ast::value(ceps::ast::as_double_ref(it_prev->second))
        	            || ceps::ast::unit(ceps::ast::as_double_ref(it_cur->second)) != ceps::ast::unit(ceps::ast::as_double_ref(it_prev->second));
        	   it_prev->second = it_cur->second;
           	   return new ceps::ast::Int(r,ceps::ast::all_zero_unit(),nullptr,nullptr,nullptr);
           } else if (it_cur->second->kind() == ceps::ast::Ast_node_kind::string_literal){
        	   bool r = ceps::ast::value(ceps::ast::as_string_ref(it_cur->second)) != ceps::ast::value(ceps::ast::as_string_ref(it_prev->second));
        	   it_prev->second = it_cur->second;
           	   return new ceps::ast::Int(r,ceps::ast::all_zero_unit(),nullptr,nullptr,nullptr);
           } else {
        	   std::stringstream s1,s2;
        	   s1 << *it_cur->second;
        	   s2 << *it_prev->second;
        	   bool r = s1.str() != s2.str();
        	   it_prev->second = it_cur->second;
           	   return new ceps::ast::Int(r,ceps::ast::all_zero_unit(),nullptr,nullptr,nullptr);
           }
       }
	 }
	 return new ceps::ast::Int(1,ceps::ast::all_zero_unit(),nullptr,nullptr,nullptr);
	} else if (id == "c_lib_localtime_year"){
		 time_t rawtime;
		 struct tm* timeinfo;
		 time(&rawtime);
		 timeinfo = localtime(&rawtime);
		 return new ceps::ast::Int(timeinfo->tm_year+1900,ceps::ast::all_zero_unit(),nullptr,nullptr,nullptr);

	 }else if (id == "c_lib_localtime_month"){
		 time_t rawtime;
		 struct tm* timeinfo;
		 time(&rawtime);
		 timeinfo = localtime(&rawtime);
		 return new ceps::ast::Int(timeinfo->tm_mon+1,ceps::ast::all_zero_unit(),nullptr,nullptr,nullptr);

	 }else if (id == "c_lib_localtime_day"){
		 time_t rawtime;
		 struct tm* timeinfo;
		 time(&rawtime);
		 timeinfo = localtime(&rawtime);
		 return new ceps::ast::Int(timeinfo->tm_mday,ceps::ast::all_zero_unit(),nullptr,nullptr,nullptr);

	 } else if (id == "c_lib_localtime_hour"){
		 time_t rawtime;
		 struct tm* timeinfo;
		 time(&rawtime);
		 timeinfo = localtime(&rawtime);
		 return new ceps::ast::Int(timeinfo->tm_hour,ceps::ast::all_zero_unit(),nullptr,nullptr,nullptr);

	 } else if (id == "c_lib_localtime_minute"){
		 time_t rawtime;
		 struct tm* timeinfo;
		 time(&rawtime);
		 timeinfo = localtime(&rawtime);
		 return new ceps::ast::Int(timeinfo->tm_min,ceps::ast::all_zero_unit(),nullptr,nullptr,nullptr);

	 } else if (id == "c_lib_localtime_second"){
		 time_t rawtime;
		 struct tm* timeinfo;
		 time(&rawtime);
		 timeinfo = localtime(&rawtime);
		 return new ceps::ast::Int(timeinfo->tm_sec,ceps::ast::all_zero_unit(),nullptr,nullptr,nullptr);

	 } else if (id == "xpath"){
		 auto xmldoc = this->ceps_env_current().get_global_symboltable().lookup_global("@@current_xml_doc",false);
		 if (xmldoc == nullptr) fatal_(-1,"xpath: @@current_xml_doc not set.\n");
		 if (args.size() == 0) fatal_(-1,"xpath: missing parameter (xpath expression).\n");
		 if (args[0]->kind() != ceps::ast::Ast_node_kind::string_literal) fatal_(-1,"xpath: illformed parameter, expected a string.\n");
		 pugi::xml_document* xml_doc = (pugi::xml_document*)xmldoc->payload;
		 std::string xpath_expr = ceps::ast::value(ceps::ast::as_string_ref(args[0]));
		 pugi::xpath_node_set r;
		 try{
		  r = xml_doc->select_nodes(xpath_expr.c_str());
		 } catch(pugi::xpath_exception const & e){
			 fatal_(-1,e.what());
		 }
		 pugi::xpath_node_set* ns = new pugi::xpath_node_set(r);
		 return new ceps::ast::User_defined(CEPS_REP_PUGI_XML_NODE_SET,ns,nullptr,nullptr,nullptr);
	 } else if (id == "empty"){
		 if (args.size() == 0) return new ceps::ast::Int(1,ceps::ast::all_zero_unit(),nullptr,nullptr,nullptr);
		 if (args[0]->kind() == ceps::ast::Ast_node_kind::user_defined &&
		 				CEPS_REP_PUGI_XML_NODE_SET == ceps::ast::id(ceps::ast::as_user_defined_ref(args[0]))){
			 auto& udef = ceps::ast::as_user_defined_ref(args[0]);
			 pugi::xpath_node_set* ns = (pugi::xpath_node_set*)ceps::ast::get<1>(udef);
			 if (ns->size() == 0) return new ceps::ast::Int(1,ceps::ast::all_zero_unit(),nullptr,nullptr,nullptr);
		 }
		 return new ceps::ast::Int(0,ceps::ast::all_zero_unit(),nullptr,nullptr,nullptr);
	 }	 else if (id == "as_double"){
		if (args.size() == 0) return new ceps::ast::Double(0,ceps::ast::all_zero_unit(),nullptr,nullptr,nullptr);
		//std::cout << *args[0] << "\n";
		if (args[0]->kind() == ceps::ast::Ast_node_kind::float_literal) return args[0];
		else if (args[0]->kind() == ceps::ast::Ast_node_kind::string_literal)
			return new ceps::ast::Double(std::stod(ceps::ast::value(ceps::ast::as_string_ref(args[0]))),ceps::ast::all_zero_unit(),nullptr,nullptr,nullptr);
		else if (args[0]->kind() == ceps::ast::Ast_node_kind::user_defined &&
				CEPS_REP_PUGI_XML_NODE_SET == ceps::ast::id(ceps::ast::as_user_defined_ref(args[0]))){
			auto& udef = ceps::ast::as_user_defined_ref(args[0]);
			pugi::xpath_node_set* ns = (pugi::xpath_node_set*)ceps::ast::get<1>(udef);
			if (ns->size() == 0)
				fatal_(-1,"as_double: node set is empty.\n");
			return new ceps::ast::Double(ns->first().node().text().as_double(),ceps::ast::all_zero_unit(),nullptr,nullptr,nullptr);
		} else if (args[0]->kind() == ceps::ast::Ast_node_kind::nodeset){
			auto & ns = ceps::ast::as_ast_nodeset_ref(args[0]).children();
			if (ns.size() != 0){
			 if (ns[0]->kind() == ceps::ast::Ast_node_kind::float_literal)
				 return ns[0];
			}
		}
		return new ceps::ast::Double(0,ceps::ast::all_zero_unit(),nullptr,nullptr,nullptr);
	 } else if (id == "as_int"){
			if (args.size() == 0) return new ceps::ast::Int(0,ceps::ast::all_zero_unit(),nullptr,nullptr,nullptr);
			if (args[0]->kind() == ceps::ast::Ast_node_kind::user_defined &&
					CEPS_REP_PUGI_XML_NODE_SET == ceps::ast::id(ceps::ast::as_user_defined_ref(args[0]))){
				auto& udef = ceps::ast::as_user_defined_ref(args[0]);
				pugi::xpath_node_set* ns = (pugi::xpath_node_set*)ceps::ast::get<1>(udef);
				if (ns->size() == 0)
					fatal_(-1,"as_double: node set is empty.\n");
				return new ceps::ast::Int(ns->first().node().text().as_int(),ceps::ast::all_zero_unit(),nullptr,nullptr,nullptr);
			}
			return new ceps::ast::Int(0,ceps::ast::all_zero_unit(),nullptr,nullptr,nullptr);
	} else if (id == "as_string"){
	 		if (args.size() == 0) return new ceps::ast::String("",nullptr,nullptr,nullptr);
	 		if (args[0]->kind() == ceps::ast::Ast_node_kind::user_defined &&
	 				CEPS_REP_PUGI_XML_NODE_SET == ceps::ast::id(ceps::ast::as_user_defined_ref(args[0]))){
	 			auto& udef = ceps::ast::as_user_defined_ref(args[0]);
	 			pugi::xpath_node_set* ns = (pugi::xpath_node_set*)ceps::ast::get<1>(udef);
	 			if (ns->size() == 0)
	 				fatal_(-1,"as_double: node set is empty.\n");
	 			return new ceps::ast::String(ns->first().node().text().get(),nullptr,nullptr,nullptr);
	 		}
	 		return new ceps::ast::String("",nullptr,nullptr,nullptr);
	} else if (id == "sleep"){
		#ifdef _WIN32
		#else
		if(args.size() == 0) sleep(1);
		#endif
	}else if (id == "print"){

		for(auto& n : args)
		{
			if (n->kind() == ceps::ast::Ast_node_kind::int_literal ||
			    n->kind() == ceps::ast::Ast_node_kind::float_literal ||
			    n->kind() == ceps::ast::Ast_node_kind::string_literal
			    )std::cout << to_string(this,n);

		}

	} else if (id == "size") {
		if (args[0]->kind() == ceps::ast::Ast_node_kind::byte_array)
		{
		 return new ceps::ast::Int(ceps::ast::bytes(ceps::ast::as_byte_array_ref(args[0])).size(),ceps::ast::all_zero_unit(),nullptr,nullptr,nullptr);
		}
		fatal_(-1,"args():Wrong arguments.");
	} else if (id == "raw_frame_starts_with") {
		if (args[0]->kind() != ceps::ast::Ast_node_kind::identifier) fatal_(-1,"Expected frame id.");
		auto id = ceps::ast::name(ceps::ast::as_id_ref(args[0]));
		auto it_frame_gen = frame_generators().find(id);
		if (it_frame_gen == frame_generators().end()) fatal_(-1,id+" is not a raw frame id.");
		size_t ds;
		char* msg_block = (char*) it_frame_gen->second->gen_msg(this,ds,{});
		bool r = ds >= args.size()-1;
		if(r)for(int i = 1; i < (int)args.size();++i){
			if (args[i]->kind() != ceps::ast::Ast_node_kind::int_literal) continue;
			unsigned char u = (unsigned char) ceps::ast::value(ceps::ast::as_int_ref(args[i]));
			if (u != (unsigned char)msg_block[i-1]){r = false;break;}
		}
		delete[] msg_block;
		return new ceps::ast::Int( (r?1:0) ,ceps::ast::all_zero_unit(),nullptr,nullptr,nullptr);
	} else if (id == "assert" || id == "ASSERT") {
		if (args.size() == 0 || args[0]->kind() != ceps::ast::Ast_node_kind::int_literal) fatal_(-1,"assert:first argument must be an integer.");
		if (ceps::ast::value(ceps::ast::as_int_ref(args[0])) == 0){
         std::string msg;
         if (args.size() > 1 && args[1]->kind() == ceps::ast::Ast_node_kind::string_literal) msg = ceps::ast::value(ceps::ast::as_string_ref(args[1]));
         if (msg.length() == 0) fatal_(-1,"Assert failed.");
         else fatal_(-1,"Assert failed:'"+msg+"'");
		}
		return new ceps::ast::Int( 1 ,ceps::ast::all_zero_unit(),nullptr,nullptr,nullptr);
	}

	auto it = name_to_smcore_plugin_fn.find(id);
	if (it != name_to_smcore_plugin_fn.end())
		return it->second(params);

	{

		ceps::ast::Nodebase_ptr body = nullptr;
		if(active_smp){
		 //auto it = active_smp->actions().find(State_machine::Transition::Action(id));
		 //if (it != active_smp->actions().end() && it->body_ != nullptr) body = it->body_;
		 auto it = active_smp->find_action(id);
		 if (it != nullptr && it->body_ != nullptr) body = it->body_;
		}

		if (body == nullptr){
			auto it = global_funcs().find(id);
			if (it != global_funcs().end()) body = it->second;
		}

		if (body)
		{
			ceps::ast::Nodeset ns( ceps::ast::as_struct_ptr(body)->children());
			auto parameters = ns["params"];
			bool pop_scope = false;
			if (parameters.size())
			{

				ceps_env_current().get_global_symboltable().push_scope();pop_scope = true;
				size_t i=0;
				for(auto p : parameters.nodes())
				{
				  if (p->kind() != ceps::ast::Ast_node_kind::identifier && p->kind() != ceps::ast::Ast_node_kind::symbol) continue;
				  std::string param_id;
				  if (p->kind() == ceps::ast::Ast_node_kind::identifier) param_id = ceps::ast::name(ceps::ast::as_id_ref(p));
				  else param_id = ceps::ast::name(ceps::ast::as_symbol_ref(p));
				  auto v = ceps_env_current().get_global_symboltable().lookup(param_id,true,true,false);
				  v->category = ceps::parser_env::Symbol::VAR;
				  v->name = id;
				  if (i < args.size()) v->payload = args[i]; else fatal_(-1,"Function '"+id+"': unbound argument : '"+param_id+"'");
				  ++i;
				}
			}

			auto result = execute_action_seq(active_smp,body);
			if (pop_scope) ceps_env_current().get_global_symboltable().pop_scope();
			return result;
		}
	}

	if (args.size() == 0)
	{ 
		{
			auto it = regfntbl_i_.find(id);
			if (it != regfntbl_i_.end()) return new  ceps::ast::Int(it->second(), ceps::ast::all_zero_unit(), nullptr, nullptr, nullptr);
		}
		{
			auto it = regfntbl_d_.find(id);
			if (it != regfntbl_d_.end()) return new  ceps::ast::Double(it->second(), ceps::ast::all_zero_unit(), nullptr, nullptr, nullptr);
		}
	}

	if (args.size() == 1)
	{
		bool arg1isint = args[0]->kind() == ceps::ast::Ast_node_kind::int_literal;
		bool arg1isdouble = args[0]->kind() == ceps::ast::Ast_node_kind::float_literal;
		bool arg1isstring = args[0]->kind() == ceps::ast::Ast_node_kind::string_literal;

		if (arg1isint) {
			{
				auto it = regfntbl_ii_.find(id);
				if (it != regfntbl_ii_.end()) 
					return new  ceps::ast::Int(it->second(ceps::ast::value(ceps::ast::as_int_ref(args[0]))), ceps::ast::all_zero_unit(), nullptr, nullptr, nullptr);
			}
			{
				auto it = regfntbl_di_.find(id);
				if (it != regfntbl_di_.end()) return new  ceps::ast::Double(it->second(ceps::ast::value(ceps::ast::as_int_ref(args[0]))), ceps::ast::all_zero_unit(), nullptr, nullptr, nullptr);
			}
		}
		else if (arg1isdouble) {
			{
				auto it = regfntbl_id_.find(id);
				if (it != regfntbl_id_.end())
					return new  ceps::ast::Int(it->second(ceps::ast::value(ceps::ast::as_double_ref(args[0]))), ceps::ast::all_zero_unit(), nullptr, nullptr, nullptr);
			}
			{
				auto it = regfntbl_dd_.find(id);
				if (it != regfntbl_dd_.end()) 
					return new  ceps::ast::Double(it->second(ceps::ast::value(ceps::ast::as_double_ref(args[0]))), ceps::ast::all_zero_unit(), nullptr, nullptr, nullptr);
			}
		}else if (arg1isstring) {
			{
				auto it = regfntbl_is_.find(id);
				if (it != regfntbl_is_.end())
					return new  ceps::ast::Int(it->second(ceps::ast::value(ceps::ast::as_string_ref(args[0]))), ceps::ast::all_zero_unit(), nullptr, nullptr, nullptr);
			}
			{
				auto it = regfntbl_ss_.find(id);
				if (it != regfntbl_ss_.end())
					return new  ceps::ast::String(it->second(ceps::ast::value(ceps::ast::as_string_ref(args[0]))));
			}
		}
	}

	if (args.size() == 2)
	{
		bool arg1isint = args[0]->kind() == ceps::ast::Ast_node_kind::int_literal;
		bool arg1isdouble = args[0]->kind() == ceps::ast::Ast_node_kind::float_literal;
		bool arg2isint = args[1]->kind() == ceps::ast::Ast_node_kind::int_literal;
		bool arg2isdouble = args[1]->kind() == ceps::ast::Ast_node_kind::float_literal;

		if (arg1isint) {
			if (arg2isint) {
				{
					auto it = regfntbl_iii_.find(id);
					if (it != regfntbl_iii_.end())
						return new  ceps::ast::Int(it->second(ceps::ast::value(ceps::ast::as_int_ref(args[0])), ceps::ast::value(ceps::ast::as_int_ref(args[1]))), ceps::ast::all_zero_unit(), nullptr, nullptr, nullptr);
				}
				{
					auto it = regfntbl_dii_.find(id);
					if (it != regfntbl_dii_.end())
						return new  ceps::ast::Double(it->second(ceps::ast::value(ceps::ast::as_int_ref(args[0])), ceps::ast::value(ceps::ast::as_int_ref(args[1]))), ceps::ast::all_zero_unit(), nullptr, nullptr, nullptr);
				}
			}
			if (arg2isdouble) {
				{
					auto it = regfntbl_iid_.find(id);
					if (it != regfntbl_iid_.end())
						return new  ceps::ast::Int(it->second(ceps::ast::value(ceps::ast::as_int_ref(args[0])), ceps::ast::value(ceps::ast::as_double_ref(args[1]))), ceps::ast::all_zero_unit(), nullptr, nullptr, nullptr);
				}
				{
					auto it = regfntbl_did_.find(id);
					if (it != regfntbl_did_.end())
						return new  ceps::ast::Double(it->second(ceps::ast::value(ceps::ast::as_int_ref(args[0])), ceps::ast::value(ceps::ast::as_double_ref(args[1]))), ceps::ast::all_zero_unit(), nullptr, nullptr, nullptr);
				}
			}
		}
		else if (arg1isdouble) {
			if (arg2isint) {
				{
					auto it = regfntbl_idi_.find(id);
					if (it != regfntbl_idi_.end())
						return new  ceps::ast::Int(it->second(ceps::ast::value(ceps::ast::as_double_ref(args[0])), ceps::ast::value(ceps::ast::as_int_ref(args[1]))), ceps::ast::all_zero_unit(), nullptr, nullptr, nullptr);
				}
				{
					auto it = regfntbl_ddi_.find(id);
					if (it != regfntbl_ddi_.end())
						return new  ceps::ast::Double(it->second(ceps::ast::value(ceps::ast::as_double_ref(args[0])), ceps::ast::value(ceps::ast::as_int_ref(args[1]))), ceps::ast::all_zero_unit(), nullptr, nullptr, nullptr);
				}
			}
			if (arg2isdouble) {
				{
					auto it = regfntbl_idd_.find(id);
					if (it != regfntbl_idd_.end())
						return new  ceps::ast::Int(it->second(ceps::ast::value(ceps::ast::as_double_ref(args[0])), ceps::ast::value(ceps::ast::as_double_ref(args[1]))), ceps::ast::all_zero_unit(), nullptr, nullptr, nullptr);
				}
				{
					auto it = regfntbl_ddd_.find(id);
					if (it != regfntbl_ddd_.end())
						return new  ceps::ast::Double(it->second(ceps::ast::value(ceps::ast::as_double_ref(args[0])), ceps::ast::value(ceps::ast::as_double_ref(args[1]))), ceps::ast::all_zero_unit(), nullptr, nullptr, nullptr);
				}
			}
		}
	}


	if (args.size() == 6)
	{
		bool allint = args[0]->kind() == ceps::ast::Ast_node_kind::int_literal && args[1]->kind() == ceps::ast::Ast_node_kind::int_literal && args[2]->kind() == ceps::ast::Ast_node_kind::int_literal
				                         && args[3]->kind() == ceps::ast::Ast_node_kind::int_literal && args[4]->kind() == ceps::ast::Ast_node_kind::int_literal && args[5]->kind() == ceps::ast::Ast_node_kind::int_literal;

		if (allint) {
			{
				auto it = regfntbl_iiiiiii_.find(id);
				if (it != regfntbl_iiiiiii_.end())
					return new  ceps::ast::Int(	it->second(ceps::ast::value(ceps::ast::as_int_ref(args[0])),ceps::ast::value(ceps::ast::as_int_ref(args[1])),ceps::ast::value(ceps::ast::as_int_ref(args[2])),
							ceps::ast::value(ceps::ast::as_int_ref(args[3])),ceps::ast::value(ceps::ast::as_int_ref(args[4])),ceps::ast::value(ceps::ast::as_int_ref(args[5])) ) ,
							                    ceps::ast::all_zero_unit(), nullptr, nullptr, nullptr);
			}
		}
	}

	//fatal_(-1,"Undefined function '"+id+"' called.");
	return nullptr;
}


static std::map<std::string, std::pair<int,bool> > type_descr_to_bitwidth_and_signedness = {
		{"bit", {1,false}},
		{"bool", {1,false}},
		{"boolean", {1,false}},
		{"int2", {2,true}},
		{"uint2", {2,false}},
		{"int3", {3,true}},
		{"uint3", {3,false}},
		{"int4", {4,true}},
		{"uint4", {4,false}},
		{"int5", {5,true}},
		{"uint5", {5,false}},
		{"int6", {6,true}},
		{"uint6", {6,false}},
		{"int7", {7,true}},
		{"uint7", {7,false}},
		{"byte", {8,true}},
		{"ubyte", {8,false}},
		{"sbyte", {8,true}},
		{"char", {8,true}},
		{"schar", {8,true}},
		{"int8", {8,true}},
		{"uint8", {8,false}},
		{"int16", {16,true}},
		{"uint16", {16,false}},
		{"int24", {24,true}},
		{"uint24", {24,false}},
		{"int32", {32,true}},
		{"uint32", {32,false}},
		{"int40", {40,true}},
		{"uint40", {40,false}},
		{"int48", {48,true}},
		{"uint48", {48,false}},
		{"int56", {56,true}},
		{"uint56", {56,false}},
		{"int64", {64,true}},
		{"uint64", {64,false}}
};

static std::size_t make_byte_sequence_helper(
		State_machine_simulation_core *smc,
		State_machine* active_smp,
		std::vector<ceps::ast::Nodebase_ptr>,
		char * chunk = nullptr,
		bool do_write=false,
		int bitwidth = 8,
		bool is_signed = false,
		bool is_real = false,
		int endianess = 0,
		std::size_t pos = 0);
static std::size_t make_byte_sequence_helper(
		State_machine_simulation_core *smc,
		State_machine* active_smp,
		ceps::ast::Nodebase_ptr,
		char * chunk = nullptr,
		bool do_write=false,
		int bitwidth = 8,
		bool is_signed = false,
		bool is_real = false,
		int endianess = 0,
		std::size_t pos = 0);

static char* make_byte_sequence(State_machine_simulation_core *smc,State_machine* active_smp, std::vector<ceps::ast::Nodebase_ptr> args,std::size_t& size){
 size = make_byte_sequence_helper(smc,active_smp,args);
// std:: cout << "size= "<<size << std::endl;
 if (size == 0) return nullptr;
 char* chunk = new char[size];
 make_byte_sequence_helper(smc,active_smp,args,chunk,true);
 //for(int i = 0;i!=size;++i){std::cout << (int)*((unsigned char*)chunk+i) << " "; } std::cout << std::endl;
 return chunk;
}

static std::size_t make_byte_sequence_helper(
		State_machine_simulation_core *smc,
		State_machine* active_smp,
		std::vector<ceps::ast::Nodebase_ptr> v,
		char * chunk,
		bool do_write,
		int bw,
		bool iss,
		bool is_real,
		int en,
		std::size_t pos){
 std::size_t s = 0;
 for(auto e: v){
  s += make_byte_sequence_helper(smc,active_smp,e,chunk,do_write,bw,iss,is_real,en,pos+s);
 }
 return s;
}

static std::size_t make_byte_sequence_helper(
		State_machine_simulation_core *smc,
		State_machine* active_smp,
		ceps::ast::Nodebase_ptr node,
		char * chunk,
		bool do_write,
		int bw,
		bool iss,
		bool is_real,
		int en,
		std::size_t pos){
 std::size_t s = 0;
 if (node->kind() == ceps::ast::Ast_node_kind::func_call){
  ceps::ast::Func_call& func_call = *dynamic_cast<ceps::ast::Func_call*>(node);
  ceps::ast::Identifier& func_id = *dynamic_cast<ceps::ast::Identifier*>(func_call.children()[0]);
  auto it = type_descr_to_bitwidth_and_signedness.find(ceps::ast::name(func_id));
  if (it != type_descr_to_bitwidth_and_signedness.end()){
	  ceps::ast::Nodebase_ptr params_ = func_call.children()[1];
	  ceps::ast::Call_parameters& params = *dynamic_cast<ceps::ast::Call_parameters*>(params_);
	  std::vector<ceps::ast::Nodebase_ptr> args;
	  flatten_args(smc,params.children()[0], args);
	  return make_byte_sequence_helper(smc,active_smp,args,chunk,do_write,it->second.first,it->second.second,is_real,en,pos);
  } else if (ceps::ast::name(func_id) == "real32" || ceps::ast::name(func_id) == "float" ) {
	  ceps::ast::Nodebase_ptr params_ = func_call.children()[1];
	  ceps::ast::Call_parameters& params = *dynamic_cast<ceps::ast::Call_parameters*>(params_);
	  std::vector<ceps::ast::Nodebase_ptr> args;
	  flatten_args(smc,params.children()[0], args);
	  return make_byte_sequence_helper(smc,active_smp,args,chunk,do_write,32,true,true,en,pos);
  } else if (ceps::ast::name(func_id) == "make_byte_sequence") {
	  ceps::ast::Nodebase_ptr params_ = func_call.children()[1];
	  ceps::ast::Call_parameters& params = *dynamic_cast<ceps::ast::Call_parameters*>(params_);
	  std::vector<ceps::ast::Nodebase_ptr> args;
	  flatten_args(smc,params.children()[0], args);
	  return make_byte_sequence_helper(smc,active_smp,args,chunk,do_write,bw,iss,is_real,en,pos);
  } else {
	std::stringstream ss;  ss << *node;
	smc->fatal_(-1,"make_byte_sequence(): unsupported argument:"+ss.str());
  }
 }

 if (!do_write) return bw / 8;

 if (node->kind() == ceps::ast::Ast_node_kind::int_literal) {
  std::size_t bytes_to_write = bw / 8;
  if (!is_real){
   if (!iss){
    std::uint64_t j = ceps::ast::value(ceps::ast::as_int_ref(node));
    std::memcpy(chunk+pos,(char*)&j,bytes_to_write);
   } else {
	std::int64_t j = ceps::ast::value(ceps::ast::as_int_ref(node));
	std::memcpy(chunk+pos,(char*)&j,bytes_to_write);
   }
  }else /*real*/{
    if (bw == 32){
     float j = ceps::ast::value(ceps::ast::as_int_ref(node));
     std::memcpy(chunk+pos,(char*)&j,bytes_to_write);
    } else {
     double j = ceps::ast::value(ceps::ast::as_int_ref(node));
     std::memcpy(chunk+pos,(char*)&j,bytes_to_write);
    }
  }
  return bytes_to_write;
 } else if (node->kind() == ceps::ast::Ast_node_kind::float_literal) {
  std::size_t bytes_to_write = bw / 8;
  if (!is_real){
    if(!iss){
	 std::uint64_t j = ceps::ast::value(ceps::ast::as_double_ref(node));
     std::memcpy(chunk+pos,(char*)&j,bytes_to_write);
    } else {
 	 std::int64_t j = ceps::ast::value(ceps::ast::as_double_ref(node));
	 std::memcpy(chunk+pos,(char*)&j,bytes_to_write);
	}
  }else /*real*/{
   if (bw == 32){
	float j = ceps::ast::value(ceps::ast::as_double_ref(node));
	std::memcpy(chunk+pos,(char*)&j,bytes_to_write);
	} else {
	  double j = ceps::ast::value(ceps::ast::as_double_ref(node));
	  std::memcpy(chunk+pos,(char*)&j,bytes_to_write);
	}
   }
   return bytes_to_write;
 } else {
  std::stringstream ss;  ss << *node;
  smc->fatal_(-1,"make_byte_sequence(): unsupported argument:"+ss.str());
 }
}

static ceps::ast::Nodebase_ptr handle_make_byte_sequence(State_machine_simulation_core *smc,
		                                                 std::vector<ceps::ast::Nodebase_ptr> args,
														 State_machine* active_smp){
 std::vector<unsigned char> byte_sequence;
 std::size_t size;
 auto seq = make_byte_sequence(smc,active_smp, args,size);
 if (seq){
  std::copy(seq,seq+size,back_inserter(byte_sequence));
 }
 return new ceps::ast::Byte_array(byte_sequence);
}

static ceps::ast::Nodebase_ptr handle_send_cmd(State_machine_simulation_core *smc, std::vector<ceps::ast::Nodebase_ptr> args,State_machine* active_smp){
 if(args.size() == 2
	&& args[0]->kind() == ceps::ast::Ast_node_kind::identifier
	&& args[1]->kind() == ceps::ast::Ast_node_kind::identifier) {

  auto id = ceps::ast::name(ceps::ast::as_id_ref(args[0]));
  auto ch_id = ceps::ast::name(ceps::ast::as_id_ref(args[1]));
  auto it_frame_gen = smc->frame_generators().find(id);
  if (it_frame_gen == smc->frame_generators().end()) smc->fatal_(-1,id+" is not a frame id.");
  auto channel = smc->get_out_channel(ch_id);
  if (channel == nullptr) smc->fatal_(-1,ch_id+" is not an output channel.");
  size_t ds;
  char* msg_block = (char*) it_frame_gen->second->gen_msg(smc,ds,smc->out_encodings[ch_id]);
  if (ds > 0 && msg_block != nullptr){
   int frame_id = 0;
   if (it_frame_gen->second->header_length() == 0){
	auto it = smc->channel_frame_name_to_id[ch_id].find(id);
	if (it == smc->channel_frame_name_to_id[ch_id].end())
	  smc->fatal_(-1,"send: No header/frame id mapping found for the channel/frame combination '"+ch_id+"/"+id+"'");
	  frame_id = it->second;
	}
	channel->push(std::make_tuple(msg_block,ds,it_frame_gen->second->header_length(),frame_id));
	return new ceps::ast::Int(1,ceps::ast::all_zero_unit(),nullptr,nullptr,nullptr);
   }
   else smc->fatal_(-1, "send() : failed to insert message into queue.");
 } else if (args.size() == 3 && args[0]->kind() == ceps::ast::Ast_node_kind::identifier && args[1]->kind() == ceps::ast::Ast_node_kind::identifier
		    && args[2]->kind() == ceps::ast::Ast_node_kind::byte_array){
   auto ch_id = ceps::ast::name(ceps::ast::as_id_ref(args[0]));
   auto channel = smc->get_out_channel(ch_id);
   if (channel == nullptr) smc->fatal_(-1,ch_id+" is not an output channel.");
   auto id = ceps::ast::name(ceps::ast::as_id_ref(args[1]));
   auto &seq = ceps::ast::bytes(ceps::ast::as_byte_array_ref(args[2]));
   if (seq.size() != 0){
   		auto it = smc->channel_frame_name_to_id[ch_id].find(id);
   		if (it == smc->channel_frame_name_to_id[ch_id].end())
   		  smc->fatal_(-1,"send: No header/frame id mapping found for the channel/frame combination '"+ch_id+"/"+id+"'");
 		int frame_id = it->second;
   		channel->push(std::make_tuple((char*)seq.data(),seq.size(),0,frame_id));
   	 }
 } else {
	 std::stringstream ss;
	 for(auto e : args) ss <<"   "<< *e << "\n";
	 smc->fatal_(-1, " send() : wrong number/types of arguments.\n   Current arguments are:\n"+ss.str());
 }
 return new ceps::ast::Int(0,ceps::ast::all_zero_unit(),nullptr,nullptr,nullptr);
}

static std::size_t breakup_byte_sequence_helper(
		State_machine_simulation_core *smc,
		State_machine* active_smp,
		ceps::parser_env::Symboltable & sym_table,
        ceps::ast::Nodebase_ptr arg,
        unsigned char * chunk=0,
		std::size_t size=0,
		bool do_read=false,
		int bw=8,
		bool iss=false,
		bool is_real=false,
		int en=0,
		std::size_t pos=0,
		std::size_t bit_pos=0);
template <typename I> static std::size_t breakup_byte_sequence_helper(
		State_machine_simulation_core *smc,
		State_machine* active_smp,
		ceps::parser_env::Symboltable & sym_table,
		I b,
		I e,
		unsigned char * chunk=0,
		std::size_t size=0,
		bool do_read=false,
		int bw=8,
		bool iss=false,
		bool is_real=false,
		int en=0,
		std::size_t pos=0,
		std::size_t bit_pos=0){
 std::size_t s = 0;
 for(;b!=e;++b){
  s += breakup_byte_sequence_helper(smc,active_smp,sym_table,*b,chunk,size,do_read,bw,iss,is_real,en,(s+bit_pos) / 8 + pos,(s+bit_pos) % 8);
 }
 return s;
}

static std::size_t breakup_byte_sequence_helper(
		State_machine_simulation_core *smc,
		State_machine* active_smp,
		ceps::parser_env::Symboltable & sym_table,
        ceps::ast::Nodebase_ptr node,
        unsigned char * chunk,
		std::size_t size,
		bool do_read,
		int bw,
		bool iss,
		bool is_real,
		int en,
		std::size_t pos,
		std::size_t bit_pos)
{
 std::size_t s = 0;
 if (node->kind() == ceps::ast::Ast_node_kind::func_call){
	  ceps::ast::Func_call& func_call = *dynamic_cast<ceps::ast::Func_call*>(node);
	  ceps::ast::Identifier& func_id = *dynamic_cast<ceps::ast::Identifier*>(func_call.children()[0]);
	  auto it = type_descr_to_bitwidth_and_signedness.find(ceps::ast::name(func_id));
	  if (it != type_descr_to_bitwidth_and_signedness.end()){
		  ceps::ast::Nodebase_ptr params_ = func_call.children()[1];
		  ceps::ast::Call_parameters& params = *dynamic_cast<ceps::ast::Call_parameters*>(params_);
		  std::vector<ceps::ast::Nodebase_ptr> args;
		  flatten_args(smc,params.children()[0], args);
		  return breakup_byte_sequence_helper(smc,
				                              active_smp,
											  sym_table,
											  args.begin(),
											  args.end(),
											  chunk,size,do_read,it->second.first,it->second.second,is_real,en,pos,bit_pos);
	  } else {
		std::stringstream ss;  ss << *node;
		smc->fatal_(-1,"breakup_byte_sequence(): unsupported type:"+ss.str());
	  }
	 }

 if (!do_read) return bw;
 if (bw == 0) return 0;
 unsigned short bytes_spawned = bw / 8;
 constexpr auto buffer_size = 10;

 unsigned char buffer[buffer_size]={0};

 if (bit_pos == 0 && bw % 8 == 0){
	std::memcpy(buffer,chunk+pos,bw / 8);
	if (iss && (*(buffer + bytes_spawned - 1) & 0x80) ){
     for(auto j = bytes_spawned; j!= buffer_size; ++j) *(buffer+j) = 0xFF;
	}
 } else if (bit_pos == 0){
	std::memcpy(buffer,chunk+pos,bytes_spawned = (bw+7) / 8  );
	unsigned char b = 1;
	for(short j = 1; (bw % 8) - j  ;++j) b = (b << 1) | 1;
	*(buffer+bytes_spawned - 1) &= b;
	if (iss){
	 b = 1 << ((bw % 8)-1);
	 if (b & *(buffer+bytes_spawned - 1)){
	  if (bw%8 == 1) *(buffer+bytes_spawned - 1) |=0xFE;
	  else if (bw%8 == 2) *(buffer+bytes_spawned - 1) |=0xFC;
	  else if (bw%8 == 3) *(buffer+bytes_spawned - 1) |=0xF8;
	  else if (bw%8 == 4) *(buffer+bytes_spawned - 1) |=0xF0;
	  else if (bw%8 == 5) *(buffer+bytes_spawned - 1) |=0xE0;
	  else if (bw%8 == 6) *(buffer+bytes_spawned - 1) |=0xC0;
	  else if (bw%8 == 7) *(buffer+bytes_spawned - 1) |=0x80;
	  for(auto j = bytes_spawned; j!= buffer_size; ++j) *(buffer+j) = 0xFF;
	 }
	}
 } else {
	 unsigned short tail = 0;
	 if (bit_pos + bw > 8) tail = (bit_pos + bw)  %  8;
	 unsigned short bytes_in_chunk_spawned = (bw + 7) / 8 + ( tail != 0 ? 1 : 0) ;
	 bytes_spawned = (bw + 7) / 8;

	 //std::cout <<"iss= "<< iss << " bit_pos=" << bit_pos << " tail= " << tail << " bytes_spawned= "<< bytes_spawned << " bytes_in_chunk_spawned="<< bytes_in_chunk_spawned<< "\n";
	 std::memcpy(buffer,chunk+pos,bytes_in_chunk_spawned);
	 if (tail) {
		 if (tail == 1) *(buffer+bytes_in_chunk_spawned-1) &= 1;
		 else if (tail == 2) *(buffer+bytes_in_chunk_spawned-1) &= 3;
		 else if (tail == 3) *(buffer+bytes_in_chunk_spawned-1) &= 7;
		 else if (tail == 4) *(buffer+bytes_in_chunk_spawned-1) &= 15;
		 else if (tail == 5) *(buffer+bytes_in_chunk_spawned-1) &= 31;
		 else if (tail == 6) *(buffer+bytes_in_chunk_spawned-1) &= 63;
		 else if (tail == 7) *(buffer+bytes_in_chunk_spawned-1) &= 127;
	 }
	 //std::cout << (int)buffer[0] << " " << (int)buffer[1] << std::endl;
	 for(short j = 0; j != bytes_in_chunk_spawned; ++j) {
		 *(buffer+j) >>= bit_pos ;
		 if (j+1 == bytes_in_chunk_spawned) break;
		 unsigned char upper_part = *(buffer + j + 1) << (8-bit_pos);
		 *(buffer + j) |= upper_part;
	 }
	 //std::cout << (int)buffer[0] << " " << (int)buffer[1] << std::endl;
	 if (bytes_spawned == 1) {
		 if (bw == 1) *buffer &= 1;
		 else if (bw == 2) *buffer &= 3;
		 else if (bw == 3) *buffer &= 7;
		 else if (bw == 4) *buffer &= 15;
		 else if (bw == 5) *buffer &= 31;
		 else if (bw == 6) *buffer &= 63;
		 else if (bw == 7) *buffer &= 127;
	 }
	 if (bytes_in_chunk_spawned != bytes_spawned){
      if (tail <= bit_pos) *(buffer+bytes_in_chunk_spawned-1) = 0;
      else *(buffer+bytes_in_chunk_spawned-1) >>= bit_pos;
	 }
	 if (iss){
		 unsigned char b = 0x80;
		 if (bw % 8 != 0) {b = 1; b << ( (bw % 8) -1 );}
		 if (b & *(buffer + bytes_spawned - 1)){
		  if (bw%8 == 1) *(buffer+bytes_spawned - 1) |=0xFE;
		  else if (bw%8 == 2) *(buffer+bytes_spawned - 1) |=0xFC;
		  else if (bw%8 == 3) *(buffer+bytes_spawned - 1) |=0xF8;
		  else if (bw%8 == 4) *(buffer+bytes_spawned - 1) |=0xF0;
		  else if (bw%8 == 5) *(buffer+bytes_spawned - 1) |=0xE0;
		  else if (bw%8 == 6) *(buffer+bytes_spawned - 1) |=0xC0;
		  else if (bw%8 == 7) *(buffer+bytes_spawned - 1) |=0x80;
		  for(auto j = bytes_spawned; j!= buffer_size; ++j) *(buffer+j) = 0xFF;
		 }
	 }
 }


 ceps::ast::Nodebase_ptr dest=nullptr;
 std::string name, kind;
 if (!is_id_or_symbol(node, name, kind)){
	 std::stringstream ss; ss << *node;
 	 smc->fatal_(-1,"breakup_byte_sequence(). Expected a variable, got "+ss.str());
 }

 if (name == "any") return bw;

 ceps::parser_env::Symbol *sym = nullptr;

 if (kind == "") sym = sym_table.lookup(name);

 if(sym != nullptr){
  if (sym->category != ceps::parser_env::Symbol::VAR ||  sym->payload == nullptr)
 	   smc->fatal_(-1,"breakup_byte_sequence(). '"+name+"' not a variable.[1]");
  dest = (ceps::ast::Nodebase_ptr)sym->payload;
 } else {
   std::lock_guard<std::recursive_mutex>g(smc->states_mutex());
   auto it_cur = smc->get_global_states().find(name);
   if(it_cur == smc->get_global_states().end())
 	  smc->fatal_(-1,"breakup_byte_sequence(). '"+name+"' not a variable.[2]");
   dest = it_cur->second;
 }

 if (dest->kind() == ceps::ast::Ast_node_kind::byte_array){
	 auto & v = ceps::ast::as_byte_array_ref(dest);
	 std::vector<unsigned char> & seq = ceps::ast::bytes(v);
     seq.clear();
     std::copy(buffer,buffer+bytes_spawned,std::back_inserter(seq));
 } else  if (dest->kind() == ceps::ast::Ast_node_kind::int_literal){
     auto & v = ceps::ast::as_int_ref(dest);
	 ceps::ast::value(v) = *((int*)buffer) ;
 } else if (dest->kind() == ceps::ast::Ast_node_kind::float_literal){
	 auto & v = ceps::ast::as_double_ref(dest);
	 ceps::ast::value(v) = *((double*)buffer);
 } else if (dest->kind() == ceps::ast::Ast_node_kind::string_literal){
	 auto & v = ceps::ast::as_string_ref(dest);
	 ceps::ast::value(v) = std::string{(char*)buffer};
 } else smc->fatal_(-1,"breakup_byte_sequence(). Type of '"+name+"' not supported.[1]");
 return bw;
}

static ceps::ast::Nodebase_ptr handle_breakup_byte_sequence(
		State_machine_simulation_core *smc,
		std::vector<ceps::ast::Nodebase_ptr> args,
		State_machine* active_smp,
		ceps::parser_env::Symboltable & sym_table)
{
 if (args.size() < 2)
	 smc->fatal_(-1,"breakup_byte_sequence(). Illformed argument(s).");
 std::string name,kind;
 if (!is_id_or_symbol(args[0], name, kind))
	 smc->fatal_(-1,"breakup_byte_sequence(). Expect a variable as first argument.");
 auto sym = sym_table.lookup(name);
 std::vector<unsigned char>* seq = nullptr;
 if(sym != nullptr && kind.length()==0){
  if (sym->category != ceps::parser_env::Symbol::VAR ||
	  sym->payload == nullptr ||
	  ((ceps::ast::Nodebase_ptr)sym->payload)->kind()!= ceps::ast::Ast_node_kind::byte_array)
	   smc->fatal_(-1,"breakup_byte_sequence(). '"+name+"' not a variable.[1]");
  auto & byte_array = ceps::ast::as_byte_array_ref((ceps::ast::Nodebase_ptr)sym->payload);
  seq = & ceps::ast::bytes(byte_array);
 } else {
  std::lock_guard<std::recursive_mutex>g(smc->states_mutex());
  auto it_cur = smc->get_global_states().find(name);
  if(it_cur == smc->get_global_states().end() || it_cur->second->kind() != ceps::ast::Ast_node_kind::byte_array)
	  smc->fatal_(-1,"breakup_byte_sequence(). '"+name+"' not a byte sequence.[2]");
  auto & byte_array = ceps::ast::as_byte_array_ref(it_cur->second);
  seq = & ceps::ast::bytes(byte_array);
 }
 auto bits_read = breakup_byte_sequence_helper(
 		smc,
 		active_smp,
		sym_table,
 		args.begin()+1,
 		args.end(),
 		seq->data(),
 		seq->size(),
 		true);
}




