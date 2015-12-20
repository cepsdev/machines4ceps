#define _CRT_SECURE_NO_WARNINGS

#include "core/include/state_machine_simulation_core.hpp"
#include "core/include/base_defs.hpp"
#include "pugixml.hpp"
#include <time.h>
#include "core/include/base_defs.hpp"

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
				else if (idx > 0 && idx-1 < (int)smc->current_event().payload_.size() ) { v.push_back(smc->current_event().payload_[idx-1]);return;}
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
	if (node_isrw_state(p))
	{
		auto v = smc->get_global_states()[ceps::ast::name(ceps::ast::as_symbol_ref(p))];
		return to_string(smc,v);
	}
	if (p->kind() == ceps::ast::Ast_node_kind::symbol && ceps::ast::kind(ceps::ast::as_symbol_ref(p)) == "Guard")
	{
			auto v = smc->guards()[ceps::ast::name(ceps::ast::as_symbol_ref(p))];
			return to_string(smc,v);
	}
	if (p->kind() == ceps::ast::Ast_node_kind::identifier)
	{
			auto id = ceps::ast::name(ceps::ast::as_id_ref(p));
			auto it_frame_gen = smc->frame_generators().find(id);
			if (it_frame_gen != smc->frame_generators().end())
			{
				size_t size;
				unsigned char* msg_block = (unsigned char*) it_frame_gen->second->gen_msg(smc,size);
				if (msg_block == nullptr) return "(undef)";
				else{
					std::string t;
					for(int i = 0; i < (int)size;++i)
					{
						t+="byte #"+std::to_string(i)+" =\t"+std::to_string(msg_block[i])+"\n";
					}
					return t;
				}
			}
			return "(ID "+ceps::ast::name(ceps::ast::as_id_ref(p))+")";
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


bool State_machine_simulation_core::kill_named_timer(std::string const & timer_id){
	DEBUG_FUNC_PROLOGUE
	DEBUG << "[KILLING NAMED TIMERS][TIMER_ID="<< timer_id <<"]\n";
	bool r = false;
	std::priority_queue<event_t> event_queue_temp;
	std::lock_guard<std::mutex> lk(main_event_queue().data_mutex());
	for(;main_event_queue().data().size();)
	{
		auto top = main_event_queue().data().top();
		if (top.timer_id_ == timer_id)
			{ main_event_queue().data().pop();r=true;}
		else {
			event_queue_temp.push(top);
			main_event_queue().data().pop();
		}
	}
	main_event_queue().data() = event_queue_temp;
	return r;
}

void State_machine_simulation_core::exec_action_timer(std::vector<ceps::ast::Nodebase_ptr>& args,bool periodic_timer)
{

	if (args.size() >= 2 && (( args[1]->kind() == ceps::ast::Ast_node_kind::symbol && kind(ceps::ast::as_symbol_ref(args[1])) == "Event")
		                               || args[1]->kind() == ceps::ast::Ast_node_kind::func_call  || args[1]->kind() == ceps::ast::Ast_node_kind::identifier)
		)
	{
		std::string ev_id;
		std::vector<ceps::ast::Nodebase_ptr> fargs;

		if (args[1]->kind() == ceps::ast::Ast_node_kind::symbol) ev_id = name(ceps::ast::as_symbol_ref(args[1]));
		else if (args[1]->kind() == ceps::ast::Ast_node_kind::identifier)
		{
			ev_id = "@@queued_action";
			fargs.push_back(args[1]);
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
			kill_named_timer(timer_id);
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
		ev_to_send.already_sent_to_out_queues_ = false;
			if (fargs.size())
				ev_to_send.payload_ = fargs;

			enqueue_event(ev_to_send,/*public_event*/false);
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
			else if (idx > 0 && idx-1 < (int)current_event().payload_.size() )
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
			for(auto s:current_states())
			{
			 if (s == state) {found = true; break;}
			}
			if(found) break;
		} return new ceps::ast::Int( found ? 1 : 0, ceps::ast::all_zero_unit(), nullptr, nullptr, nullptr);
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

	} else if (id == "send") {
		if(args.size() == 2
				&& args[0]->kind() == ceps::ast::Ast_node_kind::identifier
				&& args[1]->kind() == ceps::ast::Ast_node_kind::identifier) {
			auto id = ceps::ast::name(ceps::ast::as_id_ref(args[0]));
			auto ch_id = ceps::ast::name(ceps::ast::as_id_ref(args[1]));
			auto it_frame_gen = frame_generators().find(id);
			//std::cout << "************************* \n";
			//std::cout << id << std::endl;
			//std::cout << "************************* \n";
			if (it_frame_gen == frame_generators().end()) fatal_(-1,id+" is not a frame id.");
			auto channel = get_out_channel(ch_id);
			if (channel == nullptr) fatal_(-1,ch_id+" is not an output channel.");
			size_t ds;
			char* msg_block = (char*) it_frame_gen->second->gen_msg(this,ds);
			if (ds > 0 && msg_block != nullptr){
				channel->push(std::make_pair(msg_block,ds));
				return new ceps::ast::Int(1,ceps::ast::all_zero_unit(),nullptr,nullptr,nullptr);
			}
			else fatal_(-1, "send: failed to insert message into queue.");
		}
		return new ceps::ast::Int(0,ceps::ast::all_zero_unit(),nullptr,nullptr,nullptr);
	} else if (id == "write_read_frm"){
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
			char* msg_block = (char*) it_out_frame_gen->second->gen_msg(this,ds);
			if (ds == 0 || msg_block == nullptr) fatal_(-1,"Frame-Pattern'"+out_frame_id+"' couldn't create a valid frame.");
			std::vector<std::string> v1;
			std::vector<ceps::ast::Nodebase_ptr> v2;
			bool r = it_in_frame_gen->second->read_msg(msg_block,ds,this,v1,v2);
			delete[] msg_block;
			if (r) return new ceps::ast::Int(1,ceps::ast::all_zero_unit(),nullptr,nullptr,nullptr);
			return new ceps::ast::Int(0,ceps::ast::all_zero_unit(),nullptr,nullptr,nullptr);
		}
	} else if (id == "changed"){
	 if(args.size() == 1 && args[0]->kind() == ceps::ast::Ast_node_kind::string_literal){
       auto state_id = ceps::ast::value(ceps::ast::as_string_ref(args[0]));
	   {
		   std::lock_guard<std::recursive_mutex>g(states_mutex());
		   auto it_cur = global_systemstates().find(state_id);
		   //std::cout << "FOUND?" << (it_cur != global_systemstates().end()) << std::endl;
           if (it_cur == global_systemstates().end()) {
        	   warn_(-1, "changed(): Uninitialized Systemstate/Systemparameter '" + state_id + "' will be set to 0.");
        	   global_systemstates()[state_id] = new ceps::ast::Int(0, ceps::ast::all_zero_unit(), nullptr, nullptr, nullptr);
        	   it_cur = global_systemstates().find(state_id);
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
			    )

				std::cout << to_string(this,n);

		}

	} else if (id == "size") {
		if (args.size() == 0) return  new ceps::ast::Int(0,ceps::ast::all_zero_unit(),nullptr,nullptr,nullptr);
		else if (args[0]->kind() == ceps::ast::Ast_node_kind::identifier)
		{
			auto id = ceps::ast::name(ceps::ast::as_id_ref(args[0]));
			auto it_frame_gen = frame_generators().find(id);
			if (it_frame_gen != frame_generators().end())
			{
				size_t ds;
				char* msg_block = (char*) it_frame_gen->second->gen_msg(this,ds);
				delete[] msg_block;
				return new ceps::ast::Int(ds,ceps::ast::all_zero_unit(),nullptr,nullptr,nullptr);
			}
		}
	} else if (id == "raw_frame_starts_with") {
		if (args[0]->kind() != ceps::ast::Ast_node_kind::identifier) fatal_(-1,"Expected frame id.");
		auto id = ceps::ast::name(ceps::ast::as_id_ref(args[0]));
		auto it_frame_gen = frame_generators().find(id);
		if (it_frame_gen == frame_generators().end()) fatal_(-1,id+" is not a raw frame id.");
		size_t ds;
		char* msg_block = (char*) it_frame_gen->second->gen_msg(this,ds);
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
	if (ceps::ast::op(*binop) == '.' && node_isrw_state(lhs)/*
	    lhs->kind() == ceps::ast::Ast_node_kind::symbol &&
	    "Systemstate" == ceps::ast::kind(ceps::ast::as_symbol_ref(lhs))*/)
	{

		std::string id_rhs;
		if (rhs->kind() == ceps::ast::Ast_node_kind::identifier) id_rhs = ceps::ast::name(ceps::ast::as_id_ref(rhs));
		if (rhs->kind() == ceps::ast::Ast_node_kind::symbol) id_rhs = ceps::ast::name(ceps::ast::as_symbol_ref(rhs));
		if (parent_node != nullptr && parent_node->kind() == ceps::ast::Ast_node_kind::binary_operator && ceps::ast::op(ceps::ast::as_binop_ref(parent_node)) =='.' )
				return new ceps::ast::Symbol(ceps::ast::name(ceps::ast::as_symbol_ref(lhs))+"."+id_rhs,"Systemstate",nullptr,nullptr,nullptr);
		else{
			auto it = smc->get_global_states().find(ceps::ast::name(ceps::ast::as_symbol_ref(lhs))+"."+id_rhs);
			if (it == smc->get_global_states().end())
						smc->fatal_(-1, "Systemstate/Systemparameter '"+ceps::ast::name(ceps::ast::as_symbol_ref(lhs))+"."+id_rhs+"' not defined.");
			return it->second;
		}
		/*} else {
			auto r =
			new ceps::ast::Symbol(ceps::ast::name(ceps::ast::as_symbol_ref(lhs))+"."+id_rhs,"Systemstate",nullptr,nullptr,nullptr);

			return r;
		}*/
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
	smc->ceps_env_current().interpreter_env().symbol_mapping()["Systemparameter"] = &smc->get_global_states();

	ceps::interpreter::Environment::func_callback_t old_callback;
	void * old_func_callback_context_data;
	ceps::interpreter::Environment::func_binop_resolver_t old_binop_res;
	void * old_cxt;

	smc->ceps_env_current().interpreter_env().get_func_callback(old_callback,old_func_callback_context_data);
	smc->ceps_env_current().interpreter_env().get_binop_resolver(old_binop_res,old_cxt);

	smc->ceps_env_current().interpreter_env().set_func_callback(ceps_interface_eval_func_callback,&ctxt);
	smc->ceps_env_current().interpreter_env().set_binop_resolver(ceps_interface_binop_resolver,smc);
	auto r = ceps::interpreter::evaluate(node,
			smc->ceps_env_current().get_global_symboltable(),
			smc->ceps_env_current().interpreter_env(),root_node	);
	smc->ceps_env_current().interpreter_env().set_func_callback(old_callback,old_func_callback_context_data);
	smc->ceps_env_current().interpreter_env().set_binop_resolver(old_binop_res,old_cxt);
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
	for(auto & n : actions->children())
	{
		if (verbose_log)log() << "[EXECUTE STATEMENT]" << *n << "\n";

		if (n->kind() == ceps::ast::Ast_node_kind::ret)
		{
			auto & node = as_return_ref(n);
			return eval_locked_ceps_expr(this,containing_smp,node.children()[0],n);

		} else	if ( is_assignment_op(n) )
		{
			auto & node = as_binop_ref(n);
			std::string state_id;


			if (is_assignment_to_guard(node))
			{
				eval_guard_assign(node);
			} else if (is_assignment_to_state(node,state_id))
			{


				auto rhs = eval_locked_ceps_expr(this,containing_smp,node.right(),n);

				if (rhs == nullptr) continue;

				if (node.right()->kind() == ceps::ast::Ast_node_kind::identifier)
				{
					std::lock_guard<std::recursive_mutex>g(states_mutex());
					std::string id = ceps::ast::name(ceps::ast::as_id_ref(node.right()));
					auto sym = this->ceps_env_current().get_global_symboltable().lookup(id);
					if (sym != nullptr) {
					  //std::cout <<  *(ceps::ast::Nodebase_ptr)sym->payload << std::endl;
					  if (sym->payload) {global_states[state_id] = (ceps::ast::Nodebase_ptr)sym->payload;}
					} else {
					 auto it = type_definitions().find(id);
					 if (it == type_definitions().end())
							fatal_(-1,id+" is not a type.\n");
					 define_a_struct(this,ceps::ast::as_struct_ptr(it->second),global_states,name(as_symbol_ref(node.left())) );
					}
				} else 	{ std::lock_guard<std::recursive_mutex>g(states_mutex());global_states[state_id] = rhs;}
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
				fatal_(-1,"Expression in conditional should evaluate to integral type (int or double). Read:"+ ss.str());
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
				ceps::ast::Scope scope(branch_to_take);scope.owns_children() = false;
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
					for(size_t i = 0; i != args.size(); ++i)
					{
						args[i] = eval_locked_ceps_expr(this,containing_smp,args[i],n);

						//args[i]  = ceps::interpreter::evaluate(args[i],ceps_env_current().get_global_symboltable(),ceps_env_current().interpreter_env(),n	);
					}
				}
				event_t ev(func_name,args);
				ev.already_sent_to_out_queues_ = false;
				enqueue_event(ev,true);
			}

			else if (func_name == "timer" || func_name == "start_timer" || func_name == "start_periodic_timer")
			{
				for(size_t i = 0; i != args.size(); ++i)
				 {
					args[i] = eval_locked_ceps_expr(this,containing_smp,args[i],n);
				 }
				exec_action_timer(args,func_name == "start_periodic_timer");
			}
			else if (func_name == "print")
			{

				for(auto& n : args) n = eval_locked_ceps_expr(this,containing_smp,n,nullptr);

				for(auto& n : args)
				{
					if (n->kind() == ceps::ast::Ast_node_kind::int_literal ||
					    n->kind() == ceps::ast::Ast_node_kind::float_literal ||
					    n->kind() == ceps::ast::Ast_node_kind::string_literal ||
					    n->kind() == ceps::ast::Ast_node_kind::identifier
					    )
					{
						std::cout << to_string(this,n);
						continue;
					}
				}

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
				eval_locked_ceps_expr(this,containing_smp,n,nullptr);
			}
		}
	}
	if (verbose_log) log() << "[EXECUTE STATEMENTS][END]\n";
	return nullptr;
}

void State_machine_simulation_core::regfn(std::string name, int(*fn) ()) {
	regfntbl_i_[name] = fn;
}
void State_machine_simulation_core::regfn(std::string name, double(*fn) ()) {
	regfntbl_d_[name] = fn;
}
void State_machine_simulation_core::regfn(std::string name, int(*fn) (int)) {
	regfntbl_ii_[name] = fn;
}
void State_machine_simulation_core::regfn(std::string name, double(*fn) (int)) {
	regfntbl_di_[name] = fn;
}
void State_machine_simulation_core::regfn(std::string name, int(*fn) (double)) {
	regfntbl_id_[name] = fn;
}
void State_machine_simulation_core::regfn(std::string name, double(*fn) (double)) {
	regfntbl_dd_[name] = fn;
}
void State_machine_simulation_core::regfn(std::string name, int(*fn) (int, int)) {
	regfntbl_iii_[name] = fn;
}
void State_machine_simulation_core::regfn(std::string name, double(*fn) (int, int)) {
	regfntbl_dii_[name] = fn;
}
void State_machine_simulation_core::regfn(std::string name, int(*fn) (double, int)) {
	regfntbl_idi_[name] = fn;
}
void State_machine_simulation_core::regfn(std::string name, double(*fn) (double, int)) {
	regfntbl_ddi_[name] = fn;
}
void State_machine_simulation_core::regfn(std::string name, int(*fn) (int, double)) {
	regfntbl_iid_[name] = fn;
}
void State_machine_simulation_core::regfn(std::string name, double(*fn) (int, double)) {
	regfntbl_did_[name] = fn;
}
void State_machine_simulation_core::regfn(std::string name, int(*fn) (double, double)) {
	regfntbl_idd_[name] = fn;
}
void State_machine_simulation_core::regfn(std::string name, double(*fn) (double, double)) {
	regfntbl_ddd_[name] = fn;
}
