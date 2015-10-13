#include "core/include/state_machine_simulation_core.hpp"
#include "core/include/base_defs.hpp"
#include "pugixml.hpp"
#include <time.h>
//#include "core/include/state_machine.hpp"

#ifdef __GNUC__
#define __funcname__ __PRETTY_FUNCTION__
#else
#define __funcname__ __FUNCSIG__
#endif
#define DEBUG_FUNC_PROLOGUE 	Debuglogger debuglog(__funcname__,this,this->print_debug_info_);
#define DEBUG (debuglog << "[DEBUG]", debuglog)

bool read_func_call_values(State_machine_simulation_core* smc,	ceps::ast::Nodebase_ptr root_node,
							std::string & func_name,
							std::vector<ceps::ast::Nodebase_ptr>& args);

extern void flatten_args(State_machine_simulation_core* smc,ceps::ast::Nodebase_ptr r, std::vector<ceps::ast::Nodebase_ptr>& v, char op_val = ',')
{
	if (r == nullptr) return;
	if (r->kind() == ceps::ast::Ast_node_kind::binary_operator && op(as_binop_ref(r)) ==  op_val)
	{
		auto& t = as_binop_ref(r);
		flatten_args(smc,t.left(),v,op_val);
		flatten_args(smc,t.right(),v,op_val);
		return;
	} else if (r->kind() == ceps::ast::Ast_node_kind::func_call)
	{
		using namespace ceps::ast;
		std::string func_name;
		std::vector<ceps::ast::Nodebase_ptr> args;
		read_func_call_values(smc,r,func_name,args);

		if (func_name == "argv")
		{
			if (args.size() > 0 && args[0]->kind() == ceps::ast::Ast_node_kind::int_literal){
				auto idx = value(as_int_ref(args[0]));
				if (idx == 0){ v.push_back(new ceps::ast::String(smc->current_event().id_,nullptr,nullptr,nullptr)); return;}
				else if (idx > 0 && idx-1 < smc->current_event().payload_.size() ) { v.push_back(smc->current_event().payload_[idx-1]);return;}
				else {v.push_back(new ceps::ast::Int(0,ceps::ast::all_zero_unit(), nullptr, nullptr, nullptr)); return;}

			}
		}
	}
	v.push_back(r);
}

std::string to_string(std::vector<ceps::ast::Nodebase_ptr>const& v)
{
	if (v.size() == 0) return "[]";
	std::stringstream ss;
	ss << "[";
	for(size_t i = 0; i < v.size()-1;ss << * v[i] << " ",++i);
	ss << *v[v.size()-1];
	ss << "]";
	return ss.str();
}

std::string to_string(State_machine_simulation_core* smc,ceps::ast::Nodebase_ptr p)
{
	if(p == nullptr) return "(null)";
	if(p->kind() == ceps::ast::Ast_node_kind::string_literal)
	{
		return (ceps::ast::value(ceps::ast::as_string_ref(p)));
	}
	if(p->kind() == ceps::ast::Ast_node_kind::int_literal)
	{
		ceps::ast::Int v = *dynamic_cast<ceps::ast::Int*>(p);
		auto vv = ceps::ast::value(v);
		return std::to_string(vv);
	}
	if(p->kind() == ceps::ast::Ast_node_kind::float_literal)
	{
			ceps::ast::Double v = *dynamic_cast<ceps::ast::Double*>(p);
			auto vv = ceps::ast::value(v);
			return std::to_string(vv);
	}
	if (p->kind() == ceps::ast::Ast_node_kind::symbol && ceps::ast::kind(ceps::ast::as_symbol_ref(p)) == "Systemstate")
	{
		auto v = smc->get_global_states()[ceps::ast::name(ceps::ast::as_symbol_ref(p))];
		return to_string(smc,v);
	}
	if (p->kind() == ceps::ast::Ast_node_kind::symbol && ceps::ast::kind(ceps::ast::as_symbol_ref(p)) == "Guard")
	{
			auto v = smc->guards()[ceps::ast::name(ceps::ast::as_symbol_ref(p))];
			return to_string(smc,v);
	}
	else{
		std::stringstream ss;
		ss << "(CEPS-EXPRESSION: " << *p << ")";
		return ss.str();
	}
	return "";
}

bool read_func_call_values(State_machine_simulation_core* smc,	ceps::ast::Nodebase_ptr root_node,
							std::string & func_name,
							std::vector<ceps::ast::Nodebase_ptr>& args)
{
	try
	{
		using namespace ceps::ast;
		ceps::ast::Func_call& func_call = *dynamic_cast<ceps::ast::Func_call*>(root_node);
		ceps::ast::Identifier& id = *dynamic_cast<ceps::ast::Identifier*>(func_call.children()[0]);
		func_name = name(id);
		args.clear();
		if (nlf_ptr(func_call.children()[1])->children().size()) flatten_args(smc,nlf_ptr(func_call.children()[1])->children()[0],args);
	} catch (...)
	{
		return false;
	}
	return true;
}

bool is_second(ceps::ast::Unit_rep unit)
{
	return unit.ampere == 0 && unit.candela == 0 && unit.kelvin == 0 && unit.kg == 0 && unit.m == 0 && unit.mol == 0 && unit.s == 1;
}


void State_machine_simulation_core::exec_action_timer(std::vector<ceps::ast::Nodebase_ptr>& args,bool periodic_timer)
{

	if (args.size() >= 2 &&
		( args[1]->kind() == ceps::ast::Ast_node_kind::symbol &&
		kind(ceps::ast::as_symbol_ref(args[1])) == "Event"
		|| args[1]->kind() == ceps::ast::Ast_node_kind::func_call
	    || args[1]->kind() == ceps::ast::Ast_node_kind::identifier
		)
		)
	{
		bool public_event = true;
		std::string ev_id;
		std::vector<ceps::ast::Nodebase_ptr> fargs;

		if (args[1]->kind() == ceps::ast::Ast_node_kind::symbol) ev_id = name(ceps::ast::as_symbol_ref(args[1]));
		else if (args[1]->kind() == ceps::ast::Ast_node_kind::identifier)
		{
			ev_id = "@@queued_action";
			fargs.push_back(args[1]);
			public_event = false;
		}
		else {
			std::string  func_name;
			if (!read_func_call_values(this,args[1], func_name,fargs))
				fatal_(-1,"Internal Error.");

			if (is_global_event(func_name))
			{
				for(size_t i = 0; i != args.size(); ++i)
												fargs[i]  = args[i];
				ev_id = func_name;

			} else fatal_(-1,"start_timer/start_periodic_timer: second argument has to be an event.");
		}
		std::string timer_id;
		if (args.size() > 2)
		{
			if (args[2]->kind() != ceps::ast::Ast_node_kind::identifier)
			fatal_(-1,"start_timer: third argument (the timer id) has to be an unbound identifier.\n");
			timer_id = ceps::ast::name(ceps::ast::as_id_ref(args[2]));
		}
		double delta;
		ceps::ast::Unit_rep u;
		if (args[0]->kind() == ceps::ast::Ast_node_kind::int_literal)
		{
			auto t = ceps::ast::as_int_ref(args[0]);
			delta = value(t);
			if(delta < 0) return;
			u = ceps::ast::unit(t);
		}
		else if (args[0]->kind() == ceps::ast::Ast_node_kind::float_literal)
		{
			auto t = ceps::ast::as_double_ref(args[0]);
			delta = value(t);
			if(delta < 0) return;
			u = ceps::ast::unit(t);
		}else fatal_(-1,"Timer expectes first argument to be a numerical value.");

		if ( ! is_second(u) )
			fatal_(-1,"Timer function expects first argument to be a duration (SI unit seconds).");
			log() << "[QUEUEING TIMED EVENT][Delta = "<< std::setprecision(8)<< delta << " sec.][Event = " << ev_id <<"]" << "\n";

			event_t ev_to_send(ev_id,
						clock_type::duration((long int) ((clock_type::duration::period::den*delta)/clock_type::duration::period::num)),
						timer_id,
						periodic_timer);
			ev_to_send.already_sent_to_out_queues_ = !public_event;
			if (fargs.size())
				ev_to_send.payload_ = fargs;

			enqueue_event(ev_to_send);
		}

}


ceps::ast::Nodebase_ptr State_machine_simulation_core::ceps_interface_eval_func(State_machine* active_smp,
																				std::string const & id,
																				ceps::ast::Call_parameters* params)
{
	std::vector<ceps::ast::Nodebase_ptr> args;
	if (params != nullptr && params->children().size()) flatten_args(this,params->children()[0], args, ',');

	if (id == "argv")
	{
		auto const & args =  params->children();
		if (args.size() > 0 && args[0]->kind() == ceps::ast::Ast_node_kind::int_literal){
			auto idx = value(as_int_ref(args[0]));
			if (idx == 0)
				return new ceps::ast::String(current_event().id_,nullptr,nullptr,nullptr);
			else if (idx > 0 && idx-1 < current_event().payload_.size() )
				return current_event().payload_[idx-1];
		}
	} else if (id == "argc")
	{
		return new ceps::ast::Int( current_event().payload_.size(), ceps::ast::all_zero_unit(), nullptr, nullptr, nullptr);
	}
	else if (id == "expect")
	{
		if (!(args.size() >= 1 && args[0]->kind() == ceps::ast::Ast_node_kind::int_literal && 0!=ceps::ast::value(ceps::ast::as_int_ref(args[0])) ))
		{
			std::cerr << "***EXPECTATION NOT SATISFIED";
			if (args.size() > 1) std::cerr << ":\n\t";
			for(int i = 1; i < args.size() ;++i){
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
	  //auto const & args =  params->children();

	  /*log() << "[ACTIVE_STATES]";
	  for(auto s:current_states())
	  {
		  log() << get_fullqualified_id(s) <<";";
	  }
	  log() << "\n\n";*/
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
			for(auto s:current_states())
			{
			 if (s == state) {found = true; break;}
			}
			if(found) break;
		} return new ceps::ast::Int( found ? 1 : 0, ceps::ast::all_zero_unit(), nullptr, nullptr, nullptr);
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
	 } else if (id == "as_double"){
		if (args.size() == 0) return new ceps::ast::Double(0,ceps::ast::all_zero_unit(),nullptr,nullptr,nullptr);
		if (args[0]->kind() == ceps::ast::Ast_node_kind::user_defined &&
				CEPS_REP_PUGI_XML_NODE_SET == ceps::ast::id(ceps::ast::as_user_defined_ref(args[0]))){
			auto& udef = ceps::ast::as_user_defined_ref(args[0]);
			pugi::xpath_node_set* ns = (pugi::xpath_node_set*)ceps::ast::get<1>(udef);
			if (ns->size() == 0)
				fatal_(-1,"as_double: node set is empty.\n");
			return new ceps::ast::Double(ns->first().node().text().as_double(),ceps::ast::all_zero_unit(),nullptr,nullptr,nullptr);
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
		if(args.size() == 0) sleep(1);
	}
	auto it = name_to_smcore_plugin_fn.find(id);
	if (it != name_to_smcore_plugin_fn.end())
		return it->second(params);

	if(active_smp){

		ceps::ast::Nodebase_ptr body = nullptr;
		{
		 auto it = active_smp->actions().find(State_machine::Transition::Action(id));
		 if (it != active_smp->actions().end() && it->body_ != nullptr) body = it->body_;
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

	//fatal_(-1,"Undefined function '"+id+"' called.");
	return nullptr;
}



ceps::ast::Nodebase_ptr ceps_interface_eval_func_callback(std::string const & id, ceps::ast::Call_parameters* params, void* context)
{
	if (context == nullptr) return nullptr;
	ceps_interface_eval_func_callback_ctxt_t* ctxt = (ceps_interface_eval_func_callback_ctxt_t*) context;

	return ctxt->smc->ceps_interface_eval_func(ctxt->active_smp,id,params);
}

ceps::ast::Nodebase_ptr ceps_interface_binop_resolver( ceps::ast::Binary_operator_ptr binop,
	 	 	 	  	  	  	  	  	  	  	  	  	  	  	  ceps::ast::Nodebase_ptr lhs ,
	 	 	 	  	  	  	  	  	  	  	  	  	  	  	  ceps::ast::Nodebase_ptr rhs,
	 	 	 	  	  	  	  	  	  	  	  	  	  	  	  void* cxt,ceps::ast::Nodebase_ptr parent_node)
{
	if (cxt == nullptr) return nullptr;

	auto smc = (State_machine_simulation_core*)cxt;
	if (ceps::ast::op(*binop) == '.' &&
	    lhs->kind() == ceps::ast::Ast_node_kind::symbol &&
	    "Systemstate" == ceps::ast::kind(ceps::ast::as_symbol_ref(lhs)))
	{
		std::string id_rhs;
		if (rhs->kind() == ceps::ast::Ast_node_kind::identifier) id_rhs = ceps::ast::name(ceps::ast::as_id_ref(rhs));
		if (rhs->kind() == ceps::ast::Ast_node_kind::symbol) id_rhs = ceps::ast::name(ceps::ast::as_symbol_ref(rhs));
		if (parent_node != nullptr){
			if (parent_node->kind() == ceps::ast::Ast_node_kind::binary_operator && ceps::ast::op(ceps::ast::as_binop_ref(parent_node)) =='.' )
				return new ceps::ast::Symbol(ceps::ast::name(ceps::ast::as_symbol_ref(lhs))+"."+id_rhs,"Systemstate",nullptr,nullptr,nullptr);
			else{
				if (parent_node->kind() == ceps::ast::Ast_node_kind::binary_operator && ceps::ast::op(ceps::ast::as_binop_ref(parent_node)) =='=')
				{

				} else {
					auto it = smc->get_global_states().find(ceps::ast::name(ceps::ast::as_symbol_ref(lhs))+"."+id_rhs);
					if (it == smc->get_global_states().end())
						smc->fatal_(-1, "Systemstate '"+ceps::ast::name(ceps::ast::as_symbol_ref(lhs))+"."+id_rhs+"' not defined.");
					return it->second;
				}
			}
		} else return new ceps::ast::Symbol(ceps::ast::name(ceps::ast::as_symbol_ref(lhs))+"."+id_rhs,"Systemstate",nullptr,nullptr,nullptr);
	}
	return nullptr;
}

ceps::ast::Nodebase_ptr eval_locked_ceps_expr(State_machine_simulation_core* smc,
										 State_machine* containing_smp,
										 ceps::ast::Nodebase_ptr node,
										 ceps::ast::Nodebase_ptr root_node)
{
	ceps_interface_eval_func_callback_ctxt_t ctxt;
	ctxt.active_smp = containing_smp;
	ctxt.smc  = smc;
	std::lock_guard<std::recursive_mutex>g(smc->states_mutex());

	smc->ceps_env_current().interpreter_env().symbol_mapping()["Systemstate"] = &smc->get_global_states();
	smc->ceps_env_current().interpreter_env().set_func_callback(ceps_interface_eval_func_callback,&ctxt);
	smc->ceps_env_current().interpreter_env().set_binop_resolver(ceps_interface_binop_resolver,smc);
	auto r = ceps::interpreter::evaluate(node,
			smc->ceps_env_current().get_global_symboltable(),
			smc->ceps_env_current().interpreter_env(),root_node	);
	smc->ceps_env_current().interpreter_env().set_func_callback(nullptr,nullptr);
	smc->ceps_env_current().interpreter_env().set_binop_resolver(nullptr,nullptr);
	smc->ceps_env_current().interpreter_env().symbol_mapping().clear();

	return r;
}

extern void define_a_struct(State_machine_simulation_core*,ceps::ast::Struct_ptr sp, std::map<std::string, ceps::ast::Nodebase_ptr> & vars,std::string prefix);

ceps::ast::Nodebase_ptr State_machine_simulation_core::execute_action_seq(
		State_machine* containing_smp,
		ceps::ast::Nodebase_ptr ac_seq)
{
	using namespace std;
	using namespace std::chrono;
	DEBUG_FUNC_PROLOGUE
	const auto verbose_log = false;

	if (ac_seq == nullptr) return nullptr;
	if (ac_seq->kind() != ceps::ast::Ast_node_kind::structdef && ac_seq->kind() != ceps::ast::Ast_node_kind::scope) return nullptr;
	auto actions = ceps::ast::nlf_ptr(ac_seq);
	if (verbose_log) log() << "[EXECUTE STATEMENTS][START]\n";
	ceps_interface_eval_func_callback_ctxt_t ctxt;
	ctxt.active_smp = containing_smp;
	ctxt.smc  = this;
	for(auto & n : actions->children())
	{
		if (verbose_log)log() << "[EXECUTE STATEMENT]" << *n << "\n";

		if (n->kind() == ceps::ast::Ast_node_kind::ret)
		{
			auto & node = as_return_ref(n);
			std::lock_guard<std::recursive_mutex>g(states_mutex_);
			ceps_env_current().interpreter_env().symbol_mapping()["Systemstate"] = &global_states;
			ceps_env_current().interpreter_env().set_func_callback(ceps_interface_eval_func_callback,&ctxt);
			auto result = ceps::interpreter::evaluate(node.children()[0],ceps_env_current().get_global_symboltable(),ceps_env_current().interpreter_env(),n	);
			ceps_env_current().interpreter_env().set_func_callback(nullptr,nullptr);
			return result;
		} else	if ( is_assignment_op(n) )
		{
			auto & node = as_binop_ref(n);
			std::string state_id;


			if (is_assignment_to_guard(node))
			{
				eval_guard_assign(node);
			} else if (is_assignment_to_state(node,state_id))
			{
				ceps::ast::Nodebase_ptr rhs = nullptr;
				{
				 std::lock_guard<std::recursive_mutex>g(states_mutex_);
				 ceps_env_current().interpreter_env().symbol_mapping()["Systemstate"] = &global_states;
				 ceps_env_current().interpreter_env().set_func_callback(ceps_interface_eval_func_callback,&ctxt);
				 rhs = ceps::interpreter::evaluate(node.right(),ceps_env_current().get_global_symboltable(),ceps_env_current().interpreter_env(),n	);
				 ceps_env_current().interpreter_env().set_func_callback(nullptr,nullptr);

				}
				if (rhs == nullptr) continue;

				if (node.right()->kind() == ceps::ast::Ast_node_kind::identifier)
				{
					std::string id = ceps::ast::name(ceps::ast::as_id_ref(node.right()));
					auto sym = this->ceps_env_current().get_global_symboltable().lookup(id);
					if (sym != nullptr) {
					  //std::cout <<  *(ceps::ast::Nodebase_ptr)sym->payload << std::endl;
					  if (sym->payload) { std::lock_guard<std::recursive_mutex>g(states_mutex_);global_states[state_id] = (ceps::ast::Nodebase_ptr)sym->payload;}
					} else {
					 auto it = type_definitions().find(id);
					 if (it == type_definitions().end())
							fatal_(-1,id+" is not a type.\n");
					 define_a_struct(this,ceps::ast::as_struct_ptr(it->second),global_states,name(as_symbol_ref(node.left())) );
					}
				} else 	{ std::lock_guard<std::recursive_mutex>g(states_mutex_);global_states[state_id] = rhs;}
			}
			else {
			 std::stringstream ss;ss << *n;
			 fatal_(-1,"Unsupported assignment:"+ss.str()+"\n");
			}

		} else if (n->kind() == ceps::ast::Ast_node_kind::identifier) {
			if (containing_smp != nullptr)
			{
				auto it = containing_smp->actions().find(State_machine::Transition::Action(ceps::ast::name(ceps::ast::as_id_ref(n))));
				if (it != containing_smp->actions().end() && it->body() != nullptr){
					execute_action_seq(containing_smp,it->body());
					continue;
				}
			}

			auto it = global_funcs().find(name(ceps::ast::as_id_ref(n)));
			if (it != global_funcs().end()){
				auto body = it->second;
				execute_action_seq(nullptr,body);continue;
			}
			std::stringstream ss;ss << *n;
			fatal_(-1,"Invalid statement:"+ss.str());
		} else if (n->kind() == ceps::ast::Ast_node_kind::ifelse) {
			auto ifelse = ceps::ast::as_ifelse_ptr(n);
			ceps::ast::Nodebase_ptr cond = eval_locked_ceps_expr(this,containing_smp,ifelse->children()[0],n);

			if (cond->kind() != ceps::ast::Ast_node_kind::int_literal &&  cond->kind() != ceps::ast::Ast_node_kind::float_literal){
				std::stringstream ss; ss << *cond;
				fatal_(-1,"Expression in conditional should evaluate to int or double. Read:"+ ss.str());
			}
			bool take_left_branch = true;

			if (cond->kind() == ceps::ast::Ast_node_kind::int_literal) take_left_branch = ceps::ast::value(ceps::ast::as_int_ref(cond)) != 0;
			else if (cond->kind() == ceps::ast::Ast_node_kind::float_literal) take_left_branch = ceps::ast::value(ceps::ast::as_double_ref(cond)) != 0;

			ceps::ast::Nodebase_ptr branch_to_take = nullptr;

			if (take_left_branch && ifelse->children().size() > 1) branch_to_take = ifelse->children()[1];
			else if (!take_left_branch && ifelse->children().size() > 2) branch_to_take = ifelse->children()[2];
			if (branch_to_take == nullptr) continue;

			if (branch_to_take->kind() != ceps::ast::Ast_node_kind::structdef && branch_to_take->kind() != ceps::ast::Ast_node_kind::scope)
			{
				ceps::ast::Scope scope(branch_to_take);
				execute_action_seq(containing_smp,&scope);
				scope.children().clear();
			} else { execute_action_seq(containing_smp,branch_to_take);}
		} else if (n->kind() == ceps::ast::Ast_node_kind::symbol && ceps::ast::kind(ceps::ast::as_symbol_ref(n)) == "Event")
		{
			log() << "[QUEUEING EVENT][" << ceps::ast::name(ceps::ast::as_symbol_ref(n)) <<"]" << "\n";
			event_t ev(ceps::ast::name(ceps::ast::as_symbol_ref(n)));
			ev.already_sent_to_out_queues_ = false;
			enqueue_event(ev,true);
		} else if (n->kind() == ceps::ast::Ast_node_kind::func_call)
		{
			std::vector<ceps::ast::Nodebase_ptr> args;
			std::string  func_name;
			if (!read_func_call_values(this,n, func_name,args))
				fatal_(-1,"Internal Error.");

			if (is_global_event(func_name))
			{
				log() << "[QUEUEING EVENT WITH PAYLOAD][" << func_name <<"]" << "\n";
				{
					std::lock_guard<std::recursive_mutex>g(states_mutex_);
					ceps_env_current().interpreter_env().symbol_mapping()["Systemstate"] = &global_states;
					for(size_t i = 0; i != args.size(); ++i)
					{

						args[i]  = ceps::interpreter::evaluate(args[i],ceps_env_current().get_global_symboltable(),ceps_env_current().interpreter_env(),n	);
					}
				}
				event_t ev(func_name,args);
				ev.already_sent_to_out_queues_ = false;
				enqueue_event(ev,true);
			}

			else if (func_name == "timer" || func_name == "start_timer" || func_name == "start_periodic_timer")
			{


				{
					std::lock_guard<std::recursive_mutex>g(states_mutex_);
					ceps_env_current().interpreter_env().symbol_mapping()["Systemstate"] = &global_states;
					ceps_env_current().interpreter_env().set_func_callback(ceps_interface_eval_func_callback,&ctxt);
					ceps_env_current().interpreter_env().set_binop_resolver(ceps_interface_binop_resolver,this);
					for(size_t i = 0; i != args.size(); ++i)
					 {args[i] = ceps::interpreter::evaluate(args[i],
																		ceps_env_current().get_global_symboltable(),
																		ceps_env_current().interpreter_env(),n	);

					 }
					ceps_env_current().interpreter_env().set_func_callback(nullptr,nullptr);
					ceps_env_current().interpreter_env().set_binop_resolver(nullptr,nullptr);
				}


				exec_action_timer(args,func_name == "start_periodic_timer");
			}
			else if (func_name == "inc")
			{
				std::lock_guard<std::recursive_mutex>g(states_mutex_);
				for(auto& n : args)
				{
					if (n->kind() != ceps::ast::Ast_node_kind::symbol) continue;
					if (ceps::ast::kind(ceps::ast::as_symbol_ref(n)) != "Systemstate") continue;
					auto v = get_global_states()[ceps::ast::name(ceps::ast::as_symbol_ref(n))];
					if(v == nullptr) continue;
					if (v->kind() == ceps::ast::Ast_node_kind::int_literal){
						ceps::ast::Int* val = dynamic_cast<ceps::ast::Int*>(v);
						auto& vv = ceps::ast::value(*val);++vv;
					}
				}
			}
			else if (func_name == "print")
			{


				for(auto& n : args)
				{
					if (n->kind() == ceps::ast::Ast_node_kind::int_literal ||
					    n->kind() == ceps::ast::Ast_node_kind::float_literal ||
					    n->kind() == ceps::ast::Ast_node_kind::string_literal
					    )
					{
						std::cout << to_string(this,n);
						continue;
					}
					ceps::ast::Nodebase_ptr r;
					{
						std::lock_guard<std::recursive_mutex>g(states_mutex_);
						ceps_env_current().interpreter_env().symbol_mapping()["Systemstate"] = &global_states;
						ceps_env_current().interpreter_env().set_func_callback(ceps_interface_eval_func_callback,&ctxt);
						ceps_env_current().interpreter_env().set_binop_resolver(ceps_interface_binop_resolver,this);
					     r = ceps::interpreter::evaluate(n,
														   ceps_env_current().get_global_symboltable(),
														   ceps_env_current().interpreter_env(),n	);

					     //std::cout << *r << std::endl;

					    ceps_env_current().interpreter_env().set_func_callback(nullptr,nullptr);
						ceps_env_current().interpreter_env().set_binop_resolver(nullptr,nullptr);
					}
					std::cout << to_string(this,r);
				}

			}
			else if (func_name == "flush")
			{
				std::cout.flush();
			}
			else if (func_name == "kill_timer" || func_name == "stop_timer")
			{
				if (args.size() == 0){
					DEBUG << "[KILLING ALL TIMERS]\n";

					std::priority_queue<event_t> event_queue_temp;
					std::lock_guard<std::mutex> lk(main_event_queue().data_mutex());
					for(;main_event_queue().data().size();)
					{
						auto top = main_event_queue().data().top();
						if (top.delta_time_ != top.delta_time_.zero() || top.timer_id_.size() > 0)
							main_event_queue().data().pop();
						else {
							event_queue_temp.push(top);
							main_event_queue().data().pop();
						}
					}
					main_event_queue().data()  = event_queue_temp;
				}else{
					std::string timer_id;
					if (args[0]->kind() != ceps::ast::Ast_node_kind::identifier)
						fatal_(-1,"stop_timer: first argument (the timer id) has to be an unbound identifier.\n");
					timer_id = ceps::ast::name(ceps::ast::as_id_ref(args[0]));
					DEBUG << "[KILLING NAMED TIMERS][TIMER_ID="<< timer_id <<"]\n";
					std::priority_queue<event_t> event_queue_temp;
					std::lock_guard<std::mutex> lk(main_event_queue().data_mutex());
					for(;main_event_queue().data().size();)
					{
						auto top = main_event_queue().data().top();
						if (top.timer_id_ == timer_id)
							main_event_queue().data().pop();
						else {
							event_queue_temp.push(top);
							main_event_queue().data().pop();
						}
					}
					main_event_queue().data() = event_queue_temp;
				}
			} else{
				auto r = eval_locked_ceps_expr(this,containing_smp,n,nullptr);
			}
		}
	}
	if (verbose_log) log() << "[EXECUTE STATEMENTS][END]\n";
	return nullptr;
}