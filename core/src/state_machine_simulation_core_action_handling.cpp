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


#define _CRT_SECURE_NO_WARNINGS

#include "core/include/state_machine_simulation_core.hpp"
#include "core/include/base_defs.hpp"
#include "pugixml.hpp"
#include <time.h>
#include "core/include/base_defs.hpp"

extern std::string default_text_representation(ceps::ast::Nodebase_ptr root_node);

static bool read_func_call_values(State_machine_simulation_core* smc,	ceps::ast::Nodebase_ptr root_node,
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
                                else {smc->fatal_(-1," Access to argv: Out of bounds.");}

			}
		}
	}
	v.push_back(r);
}

extern std::string to_string(std::vector<ceps::ast::Nodebase_ptr>const& v)
{
	if (v.size() == 0) return "[]";
	std::stringstream ss;
	ss << "[";
	for(size_t i = 0; i < v.size()-1;ss << * v[i] << " ",++i);
	ss << *v[v.size()-1];
	ss << "]";
	return ss.str();
}

extern std::string to_string(State_machine_simulation_core* smc,ceps::ast::Nodebase_ptr p)
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
		std::lock_guard<std::recursive_mutex>g(smc->states_mutex());
		auto v = smc->get_global_states()[ceps::ast::name(ceps::ast::as_symbol_ref(p))];
		return to_string(smc,v);
	}
	if (p->kind() == ceps::ast::Ast_node_kind::symbol && ceps::ast::kind(ceps::ast::as_symbol_ref(p)) == "Guard")
	{
		std::lock_guard<std::recursive_mutex>g(smc->states_mutex());
		auto v = smc->guards()[ceps::ast::name(ceps::ast::as_symbol_ref(p))];
		return to_string(smc,v);
	}
	if (p->kind() == ceps::ast::Ast_node_kind::identifier)
	{
		std::lock_guard<std::recursive_mutex>g(smc->states_mutex());
			auto id = ceps::ast::name(ceps::ast::as_id_ref(p));
			auto it_frame_gen = smc->frame_generators().find(id);
			if (it_frame_gen != smc->frame_generators().end())
			{
				size_t size;
				unsigned char* msg_block= (unsigned char*) std::get<1>(it_frame_gen->second->gen_msg(smc,size,{}));
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

static bool read_func_call_values(State_machine_simulation_core* smc,	ceps::ast::Nodebase_ptr root_node,
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







#if 0
	struct timer_table_entry_t{
		bool in_use = false;
		bool kill = false;
		bool fresh = false;
		std::string name;
		int id;
		long period_in_ms = 0;
		long time_remaining_in_ms = 0;
		bool periodic = false;
		int fd = -1;
		event_t event;
	};

	size_t timer_table_size = 128;

	mutable std::mutex timer_table_mtx;
	std::vector<timer_table_entry_t> timer_table;
#endif


#ifndef __gnu_linux__

bool State_machine_simulation_core::exec_action_timer(double t,
		                                              sm4ceps_plugin_int::ev ev_,
													  sm4ceps_plugin_int::id id_,
													  bool periodic_timer,
													  sm4ceps_plugin_int::Variant (*fp)()){

	if (t < 0) return true;
	std::string ev_id = ev_.name_;
	if (fp != nullptr){
			ev_id = "@@queued_action";
	}
	std::string timer_id;

	if (id_.name_.length())
	{
		timer_id = id_.name_;
		kill_named_timer(timer_id);
	}

	double delta = t;
	ceps::ast::Unit_rep u;

	//log() << "[QUEUEING TIMED EVENT][Delta = "<< std::setprecision(8)<< delta << " sec.][Event = " << ev_id <<"]" << "\n";

	event_t ev_to_send(ev_id);
	ev_to_send.unique_ = this->unique_events().find(ev_id) != this->unique_events().end();
	ev_to_send.already_sent_to_out_queues_ = false;
	ev_to_send.glob_func_ = fp;
	if (ev_.args_.size())
		ev_to_send.payload_native_ = ev_.args_;

	int timer_thread_id = -1;
	{
	 std::lock_guard<std::mutex> lk(timer_threads_m);
	 if (timer_threads.size() == 0){
	  timer_threads.resize(1024);
	  for(auto& tinf : timer_threads){
		std::get<TIMER_THREAD_FN_CTRL_THREADOBJ>(tinf) = nullptr;
		std::get<TIMER_THREAD_FN_CTRL_TERMINATION_REQUESTED>(tinf) = false;
		std::get<TIMER_THREAD_FN_CTRL_TERMINATED>(tinf)= true;
	  }
	  timed_events_active_ = 0;
	 }
	 assert(timer_threads.size()>0);

 	 for(size_t i = 0; i < timer_threads.size(); ++i)
	  if (std::get<TIMER_THREAD_FN_CTRL_TERMINATED>(timer_threads[i])){
	   std::get<TIMER_THREAD_FN_CTRL_TERMINATED>(timer_threads[i]) = false;
	   std::get<TIMER_THREAD_FN_CTRL_TERMINATION_REQUESTED>(timer_threads[i]) = false;
	   std::get<TIMER_THREAD_FN_CTRL_ID>(timer_threads[i]) = timer_id;
	   if (std::get<TIMER_THREAD_FN_CTRL_THREADOBJ>(timer_threads[i])){ std::get<TIMER_THREAD_FN_CTRL_THREADOBJ>(timer_threads[i])->join();
	   delete std::get<TIMER_THREAD_FN_CTRL_THREADOBJ>(timer_threads[i]);}
	   timer_thread_id = i; break;
	  }

	  if (timer_thread_id < 0) fatal_(-1,"Out of resources: timer.");
	  inc_timed_events();
	  std::get<TIMER_THREAD_FN_CTRL_THREADOBJ>(timer_threads[timer_thread_id]) = new std::thread{timer_thread_fn,this,timer_thread_id,periodic_timer,delta,ev_to_send};
	}
}
#else



#include <poll.h>
#include <unistd.h>


ceps::ast::Nodebase_ptr  State_machine_simulation_core::ceps_fn_start_signal_gen(std::string const & id ,
				                                                const std::vector<ceps::ast::Nodebase_ptr> & args,
																State_machine* ){
 sm4ceps::datasources::Signalgenerator* siggen = nullptr;
 auto h = find_sig_gen(ceps::ast::name(ceps::ast::as_id_ref(args[0])));
 if (h < 0) return nullptr;
 siggen = sig_gen(h);
 siggen->target_state() = ceps::ast::name(ceps::ast::as_symbol_ref(args[1]));
 exec_action_timer(siggen->delta(),sm4ceps_plugin_int::ev{},sm4ceps_plugin_int::id{},true,nullptr,siggen);
 return nullptr;
}



#endif




void State_machine_simulation_core::x_path(sm4ceps_plugin_int::xml_node_set& xs,std::string path){
 pugi::xml_document* xml_doc = (pugi::xml_document*)xs.xml_doc;
 if (xml_doc == nullptr) fatal_(-1,"State_machine_simulation_core::x_path:: xml_doc is null.");
 std::string xpath_expr = path;
 pugi::xpath_node_set r;
 try{
  r = xml_doc->select_nodes(xpath_expr.c_str());
 } catch(pugi::xpath_exception const & e){
  fatal_(-1,e.what());
 }
 pugi::xpath_node_set* ns = new pugi::xpath_node_set(r);
 if (xs.xpath_node_set != nullptr){ delete (pugi::xpath_node_set*)xs.xpath_node_set;xs.xpath_node_set=nullptr;}
 xs.xpath_node_set = ns;
}



int State_machine_simulation_core::as_int(sm4ceps_plugin_int::xml_node_set& xs){
	pugi::xpath_node_set* ns = (pugi::xpath_node_set*)xs.xpath_node_set;
	if (ns == nullptr) fatal_(-1,"as_int: node set is null.\n");
	if (ns->size() == 0)
				fatal_(-1,"as_int: node set is empty.\n");
	return ns->first().node().text().as_int();
}
double State_machine_simulation_core::as_double(sm4ceps_plugin_int::xml_node_set& xs){
	pugi::xpath_node_set* ns = (pugi::xpath_node_set*)xs.xpath_node_set;
	if (ns == nullptr) fatal_(-1,"as_double: node set is null.\n");
	if (ns->size() == 0)
					fatal_(-1,"as_double: node set is empty.\n");
	return ns->first().node().text().as_double();
}
std::string State_machine_simulation_core::as_string(sm4ceps_plugin_int::xml_node_set& xs){
	pugi::xpath_node_set* ns = (pugi::xpath_node_set*)xs.xpath_node_set;
	if (ns == nullptr) fatal_(-1,"as_string: node set is null.\n");
	if (ns->size() == 0)
					fatal_(-1,"as_string: node set is empty.\n");
	return ns->first().node().text().as_string();
}
bool State_machine_simulation_core::empty(sm4ceps_plugin_int::xml_node_set& xs){
	pugi::xpath_node_set* ns = (pugi::xpath_node_set*)xs.xpath_node_set;
	if (ns == nullptr) fatal_(-1,"empty: node set is null.\n");
	return ns->size() == 0;
}

void State_machine_simulation_core::start_timer(double t,sm4ceps_plugin_int::ev ev_){
 exec_action_timer(t,ev_,sm4ceps_plugin_int::id{},false);
}
void State_machine_simulation_core::start_timer(double t,sm4ceps_plugin_int::ev ev_,sm4ceps_plugin_int::id id_){
 exec_action_timer(t,ev_,id_,false);
}
void State_machine_simulation_core::start_periodic_timer(double t,sm4ceps_plugin_int::ev ev_){
	exec_action_timer(t,ev_,sm4ceps_plugin_int::id{},true);
}
void State_machine_simulation_core::start_periodic_timer(double t,sm4ceps_plugin_int::ev ev_,sm4ceps_plugin_int::id id_){
	exec_action_timer(t,ev_,id_,true);
}
void State_machine_simulation_core::stop_timer(sm4ceps_plugin_int::id id_){
        kill_named_timer_main_timer_table(id_.name_);
}
void State_machine_simulation_core::start_periodic_timer(double t,sm4ceps_plugin_int::Variant (*fp)()){
	exec_action_timer(t,sm4ceps_plugin_int::ev{},sm4ceps_plugin_int::id{},true,fp);
}
void State_machine_simulation_core::start_periodic_timer(double t,sm4ceps_plugin_int::Variant (*fp)(),sm4ceps_plugin_int::id id_){
	exec_action_timer(t,sm4ceps_plugin_int::ev{},id_,true,fp);
}
bool State_machine_simulation_core::in_state(std::initializer_list<sm4ceps_plugin_int::id> state_ids){
	bool found = false;
	for(auto p : state_ids){
		auto state = resolve_state_qualified_id(p.name_,current_smp());
		if(!state.valid())
		{
		 std::stringstream ss;
		 ss << p.name_;
		 fatal_(-1,"in_state : illformed argument, unknown state: "+ss.str());
		}
		if (enforce_native()){
			auto& ctxt = executionloop_context();
			found = ctxt.current_states[state.id_];
		} else
		for(auto s:current_states())
		{
			if (s == state) {found = true; break;}
		}
		if(found) break;
	}
	return found;
}

void State_machine_simulation_core::register_global_function(std::string name,sm4ceps_plugin_int::Variant (*fp)()){
	glob_funcs()[name] = fp;
}


void State_machine_simulation_core::send_raw_frame(void* chunk,size_t len,size_t header_len,std::string const & channel_id){
 if(len == 0 || chunk == nullptr) return;
 auto channel_info = get_out_channel(channel_id);
 auto channel = std::get<0>(channel_info);
 if (channel == nullptr) fatal_(-1,channel_id+" is not an output channel.");
 char* msg_block = new char[len];
 memcpy(msg_block,chunk,len);
 channel->push(std::make_tuple(Rawframe_generator::gen_msg_return_t{Rawframe_generator::IS_BINARY,msg_block},len,header_len,0));
}

static ceps::ast::Nodebase_ptr ceps_interface_eval_func_callback(std::string const & id, ceps::ast::Call_parameters* params, void* context,ceps::parser_env::Symboltable & sym_table)
{
	if (context == nullptr) return nullptr;
	ceps_interface_eval_func_callback_ctxt_t* ctxt = (ceps_interface_eval_func_callback_ctxt_t*) context;

	return ctxt->smc->ceps_interface_eval_func(ctxt->active_smp,id,params,sym_table);
}

ceps::ast::Nodebase_ptr ceps_interface_binop_resolver( ceps::ast::Binary_operator_ptr binop,
	 	 	 	  	  	  	  	  	  	  	  	  	  	  	  ceps::ast::Nodebase_ptr lhs ,
	 	 	 	  	  	  	  	  	  	  	  	  	  	  	  ceps::ast::Nodebase_ptr rhs,
	 	 	 	  	  	  	  	  	  	  	  	  	  	  	  void* cxt,ceps::ast::Nodebase_ptr parent_node)
{
	if (cxt == nullptr) return nullptr;
	auto smc = (State_machine_simulation_core*)cxt;
	if (ceps::ast::op(*binop) == '.' && node_isrw_state(lhs))
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
	std::shared_ptr<ceps::parser_env::Scope> sms_global_scope = nullptr;
	State_machine * root_sms = containing_smp;
	if (containing_smp){
		for(;root_sms->parent();root_sms = root_sms->parent());
	    if (root_sms->global_scope){
	    	sms_global_scope = root_sms->global_scope;
	    }
	}

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

    if (sms_global_scope) smc->ceps_env_current().get_global_symboltable().scopes.push_back(sms_global_scope);
	auto r = ceps::interpreter::evaluate_generic(node,
			smc->ceps_env_current().get_global_symboltable(),
			smc->ceps_env_current().interpreter_env(),root_node,nullptr,nullptr	);
	if (sms_global_scope) smc->ceps_env_current().get_global_symboltable().scopes.pop_back();


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
					  if (sym->payload) {get_global_states()[state_id] = (ceps::ast::Nodebase_ptr)sym->payload;}
					} else {
					 auto it = type_definitions().find(id);
					 if (it == type_definitions().end())
							fatal_(-1,id+" is not a type.\n");
					 define_a_struct(this,ceps::ast::as_struct_ptr(it->second),get_global_states(),name(as_symbol_ref(node.left())) );
					}
				} else 	{ 
					ceps::ast::Nodebase_ptr old_value {nullptr};
					{ 
						std::lock_guard<std::recursive_mutex> g{states_mutex()};
						old_value = get_global_states()[state_id];
						get_global_states()[state_id] = rhs->clone();
					}					  
					if (old_value) delete old_value;
					//delete rhs;
				}
			}
			else {
				std::stringstream ss;ss << *n;
			 	fatal_(-1,"Unsupported assignment:"+ss.str()+"\n");
			}
		} else if (n->kind() == ceps::ast::Ast_node_kind::identifier) {
			if (containing_smp != nullptr)
			{
				auto it = containing_smp->find_action(ceps::ast::name(ceps::ast::as_id_ref(n)));
				if (it != /*containing_smp->actions().end()*/nullptr && it->body() != nullptr){
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
			using namespace ceps::ast;

			auto ifelse = as_ifelse_ptr(n);
			Nodebase_ptr cond = eval_locked_ceps_expr(this,containing_smp,ifelse->children()[0],n);
			bool take_left_branch{true};
			bool erroneous_cond{false};

			if (is<Ast_node_kind::nodeset>(cond)){
				auto& set_of_nodes{as_ast_nodeset_ref(cond)};
				if (!set_of_nodes.children().size()) take_left_branch = false;
				else{
					auto p = set_of_nodes.children()[0];
					if (is<Ast_node_kind::int_literal>(p)) take_left_branch = value(as_int_ref(p)) != 0;
					else if (is<Ast_node_kind::float_literal>(p)) take_left_branch = value(as_double_ref(p)) != 0;
					else if (is<Ast_node_kind::string_literal>(p)) take_left_branch = value(as_string_ref(p)) != "1";
					else erroneous_cond = true;
				}
			} else if (is<Ast_node_kind::int_literal>(cond) )
				take_left_branch = value(as_int_ref(cond)) != 0;
			else if (is<Ast_node_kind::float_literal>(cond) ) 
			    take_left_branch = value(as_double_ref(cond)) != 0;
			else  erroneous_cond = true;
			

			if (erroneous_cond){
				std::stringstream ss; ss << *cond;
				fatal_(-1,"Expression in conditional illformed: >>"+ ss.str()+"<<.");
			}
			
			Nodebase_ptr branch_to_take = nullptr;

			if (take_left_branch && ifelse->children().size() > 1) branch_to_take = ifelse->children()[1];
			else if (!take_left_branch && ifelse->children().size() > 2) branch_to_take = ifelse->children()[2];
			if (branch_to_take == nullptr) continue;
			Nodebase_ptr result_of_branch = nullptr;
			if (branch_to_take->kind() != Ast_node_kind::structdef && branch_to_take->kind() != Ast_node_kind::scope)
			{
				Scope scope(branch_to_take);scope.owns_children() = false;
				result_of_branch=execute_action_seq(containing_smp,&scope);
				scope.children().clear();
			} else { result_of_branch=execute_action_seq(containing_smp,branch_to_take);}
			if (result_of_branch != nullptr) return result_of_branch;
		} else if (n->kind() == ceps::ast::Ast_node_kind::symbol && ceps::ast::kind(ceps::ast::as_symbol_ref(n)) == "Event")
		{
			//log() << "[QUEUEING EVENT][" << ceps::ast::name(ceps::ast::as_symbol_ref(n)) <<"]" << "\n";
			event_t ev(ceps::ast::name(ceps::ast::as_symbol_ref(n)));
			ev.unique_ = this->unique_events().find(ev.id_) != this->unique_events().end();
			ev.already_sent_to_out_queues_ = false;
			enqueue_event(ev,true);
		} else if (n->kind() == ceps::ast::Ast_node_kind::func_call)
		{
			std::vector<ceps::ast::Nodebase_ptr> args;
			std::string  func_name;
			if (!read_func_call_values(this,n, func_name,args)){
				std::stringstream ss;
				ss << *n << "\n";
				fatal_(-1,"Internal Error:State_machine_simulation_core::execute_action_seq:"+ss.str());
			}

			if (is_global_event(func_name))
			{
				//log() << "[QUEUEING EVENT WITH PAYLOAD][" << func_name <<"]" << "\n";
				{
					for(size_t i = 0; i != args.size(); ++i)
					{
						args[i] = eval_locked_ceps_expr(this,containing_smp,args[i],n);

						//args[i]  = ceps::interpreter::evaluate(args[i],ceps_env_current().get_global_symboltable(),ceps_env_current().interpreter_env(),n	);
					}
				}
				event_t ev(func_name,args);
				ev.already_sent_to_out_queues_ = false;
				ev.unique_ = this->unique_events().find(ev.id_) != this->unique_events().end();
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
				for(auto& n : args) {
					n = eval_locked_ceps_expr(this,containing_smp,n,nullptr);
				}
                std::stringstream ss;
                bool do_flush = false;
				for(auto& n : args)
				{
                    if (n->kind() == ceps::ast::Ast_node_kind::byte_array){
					 auto& seq = ceps::ast::bytes(ceps::ast::as_byte_array_ref(n));
                     for(auto c: seq){
                    	 ss << (int)c << " ";
                     }
                    } else if (n->kind() == ceps::ast::Ast_node_kind::symbol &&
                               ceps::ast::kind(ceps::ast::as_symbol_ref(n)) == "IOManip" &&
                               ceps::ast::name(ceps::ast::as_symbol_ref(n)) == "endl" ) do_flush = true;
                    else ss << default_text_representation(n);
                }//for
                if(live_logger()){
                   this->live_logger_out()->log_console(ss.str());
                } else {std::cout << ss.str(); if (do_flush) std::cout << std::endl; }
				//for(auto n : args) delete(n);

			}
			else if (func_name == "kill_timer" || func_name == "stop_timer")
			{

				if (args.size() == 0){
					DEBUG << "[KILLING ALL TIMERS]\n";

					this->kill_named_timer(std::string{});
				}else{
					std::string timer_id;
					if (args[0]->kind() != ceps::ast::Ast_node_kind::identifier)
						fatal_(-1,"stop_timer: first argument (the timer id) has to be an unbound identifier.\n");
					timer_id = ceps::ast::name(ceps::ast::as_id_ref(args[0]));
					DEBUG << "[KILLING NAMED TIMERS][TIMER_ID="<< timer_id <<"]\n";
					this->kill_named_timer(timer_id);
				}
			} else{
                auto r = eval_locked_ceps_expr(this,containing_smp,n,nullptr);
				//if (r) std::cerr <<"===> "<< *r << std::endl;
                //if(r) delete r;
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

void State_machine_simulation_core::regfn(std::string name, int(*fn) (std::string)) {
	regfntbl_is_[name] = fn;
}

void State_machine_simulation_core::regfn(std::string name, std::string(*fn) (std::string)) {
	regfntbl_ss_[name] = fn;
}


void State_machine_simulation_core::regfn(std::string name, int(*fn) (int,int,int,int,int,int)) {
	regfntbl_iiiiiii_[name] = fn;
}












