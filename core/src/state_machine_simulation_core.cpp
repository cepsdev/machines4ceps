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


#include <algorithm>

#define _CRT_SECURE_NO_WARNINGS

#include "core/include/state_machine_simulation_core.hpp"
#include "core/include/sm_comm_naive_msg_prot.hpp"
#include "core/include/sm_ev_comm_layer.hpp"
#include "core/include/serialization.hpp"
#include "core/include/sm_raw_frame.hpp"
#include "core/include/sm_xml_frame.hpp"
#include "core/include/modelling/partitions.hpp"
#include "core/include/modelling/cover_path.hpp"
#include "core/include/api/websocket/ws_api.hpp"
#include "utils/concept_dependency_graph.hpp"
#include "utils/stddoc.hpp"
#include "utils/fibex_import.hpp"
#include "core/include/docgen/docgenerator.hpp"

#include "pugixml.hpp"
#ifdef __gnu_linux__

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <dlfcn.h>
#include <dirent.h>

#else

#endif
#include <sys/types.h>
#include <limits>
#include <cstring>

#include "core/include/base_defs.hpp"
#include <cassert>



typedef void (*init_plugin_t)(IUserdefined_function_registry*);



void register_plugin(State_machine_simulation_core* smc,std::string const& id,ceps::ast::Nodebase_ptr (*fn) (ceps::ast::Call_parameters* ))
{
    smc->register_plugin_fn(id,fn);
}


int State_machine_simulation_core::SM_COUNTER = 0;

bool is_unique_event(State_machine_simulation_core::event_t const & ev){return ev.unique_;}
std::string const & id(State_machine_simulation_core::event_t const & ev){return ev.id_;}

extern bool node_isrw_state(ceps::ast::Nodebase_ptr node) {
	using namespace ceps::ast;
	if (node == nullptr) return false;
	return node->kind() == ceps::ast::Ast_node_kind::symbol
		&& (kind(as_symbol_ref(node)) == "Systemstate" || kind(as_symbol_ref(node)) == "Systemparameter");
}

std::recursive_mutex Debuglogger::mtx_;
bool PRINT_DEBUG_INFO = false;

std::vector<std::thread*> comm_threads;


bool read_func_call_values(State_machine_simulation_core* smc,	ceps::ast::Nodebase_ptr root_node,
							std::string & func_name,
							std::vector<ceps::ast::Nodebase_ptr>& args);

extern void define_a_struct(State_machine_simulation_core* smc,ceps::ast::Struct_ptr sp, std::map<std::string, ceps::ast::Nodebase_ptr> & vars,std::string prefix);

void comm_sender_generic_tcp_out_thread(State_machine_simulation_core::frame_queue_t* frames,
			     State_machine_simulation_core* smc,
			     std::string ip,
			     std::string port,
			     std::string som,
			     std::string eof,
			     std::string sock_name,
			     bool reuse_socket, bool reg_socket);

void comm_generic_tcp_in_dispatcher_thread(int id,
				 Rawframe_generator* gen,
				 std::string ev_id,
				 std::vector<std::string> params,
			     State_machine_simulation_core* smc,
			     std::string ip,
			     std::string port,std::string som,std::string eof,std::string sock_name,bool reg_sock,bool reuse_sock,
			     void (*handler_fn) (int,Rawframe_generator*,std::string,std::vector<std::string> ,State_machine_simulation_core* , sockaddr_storage,int,std::string,std::string));

void comm_generic_tcp_in_thread_fn(int id,
		 Rawframe_generator* gen,
		 std::string ev_id,
		 std::vector<std::string> params,
		 State_machine_simulation_core* smc,
		 sockaddr_storage claddr,int sck,std::string som,std::string eof);

static std::stringstream read(Statefulscanner<Memory<char>,char>& scanner){
    std::stringstream r;
    Statefulscanner<Memory<char>,char>::Token t;
    for(;scanner.gettoken(t);)
    {
     if (t.kind() == Statefulscanner<Memory<char>,char>::Token::TOK_STR)
              r << "\"" <<  t.sval_ << "\"";
     else if (t.kind() == Statefulscanner<Memory<char>,char>::Token::TOK_ENDL) r << "\n";
     else if (t.kind() == Statefulscanner<Memory<char>,char>::Token::TOK_BLANK) r << " ";
     else if (t.kind() == Statefulscanner<Memory<char>,char>::Token::TOK_DOUBLE_QUOTE) r << "\"";
     else r << t.sval_;
    }
    return r;
}

std::vector<ceps::ast::Nodebase_ptr> State_machine_simulation_core::process_files(
        std::vector<std::string> const & file_names,
        std::string& last_file_processed,
		Result_process_cmd_line result_cmd_line)
{
	ceps::docgen::context docgen_context; // Information which needs to be preserved between docgen calls

	std::vector<ceps::ast::Nodebase_ptr> generated_nodes;
    bool next_is_expression = false;
    for(auto const & file_name : file_names)
	{
        if (next_is_expression){
            next_is_expression = false;
            std::stringstream def_file{ last_file_processed = file_name};
            if (!def_file) fatal_(ERR_FILE_OPEN_FAILED,file_name);

            Ceps_parser_driver driver{ceps_env_current().get_global_symboltable(),def_file};
            ceps::Cepsparser parser{driver};
            if (parser.parse() != 0 || driver.errors_occured())
                fatal_(ERR_CEPS_PARSER, file_name);
            ceps::interpreter::evaluate(current_universe(),
                                        driver.parsetree().get_root(),
                                        ceps_env_current().get_global_symboltable(),
                                        ceps_env_current().interpreter_env(),
                                        &generated_nodes
                                        );
            continue;

        }
        else if (file_name == "-e"){
          next_is_expression = true;
          continue;
        }
        else if (file_name.length() > 3 && file_name.substr(file_name.length()-4,4) == ".xml"){

			auto root_node = ceps::ast::read_xml_file(file_name);
			if (root_node == nullptr) fatal_(ERR_FILE_OPEN_FAILED, file_name);
			auto v = ceps::ast::nlf_ptr(root_node);
			for(auto p:v->children()){
				if (p->kind() != ceps::ast::Ast_node_kind::structdef) continue;
				if (ceps::ast::name(as_struct_ref(p)) == "fx:FIBEX" ){
					auto rr = sm4ceps::utils::import_fibex(this,as_struct_ptr(p),ceps_env_current().get_global_symboltable(),ceps_env_current().interpreter_env());
					if (rr.size())
						current_universe().nodes().insert(current_universe().nodes().end(),rr.nodes().begin(), rr.nodes().end());
				} else {
				 current_universe().nodes().push_back(p);
				}
			}
			continue;
        } else if (file_name.length() > 8 && file_name.substr(file_name.length()-9,9) == ".ceps.lex"){
            std::shared_ptr<lexer> lex{new lexer};
            Memory<char> * rules_data = new Memory<char>{};
            if(!readfile_to_memory(*rules_data , file_name.c_str() )) {delete rules_data;fatal_(ERR_FILE_OPEN_FAILED,file_name);}
            lex->data_chunks.push_back(rules_data = rules_data);
            size_t ttt = 0;
            for(;default_ops[ttt++];);
            lex->scanner.tokentable().clear_and_read_table(default_ops,--ttt);
            lex->scanner.activate_toplevel_onexit_trigger(false);
            lex->scanner.set_input(*rules_data);
            Statefulscanner<Memory<char>,char>::Token t;
            try{
             for(;lex->scanner.gettoken(t););
            }
            catch(Statefulscanner<Memory<char>,char>::match_ex & e) {fatal_(ERR_CEPS_PARSER, file_name);}
            catch(Statefulscanner<Memory<char>,char>::eval_ex & e) {fatal_(ERR_CEPS_PARSER, file_name);}
            for(auto p : lex->scanner.pragmas){
                if (p.length() > 4 && p.substr(0,4) =="ext_"){
                    lex->file_exts.push_back(p.substr(4));
                }
            }
            this->lexers.push_back(lex);
            continue;
        } else if (file_name.length() > 3 && file_name.substr(file_name.length()-4,4) != ".ceps"){
            auto ext = file_name.substr(file_name.length()-3,3);
            auto l = find_lexer_by_file_ext(ext);
            if(l){
                Memory<char> * data = new Memory<char>{};
                if(!readfile_to_memory(*data , file_name.c_str() )) {delete data; fatal_(ERR_FILE_OPEN_FAILED,file_name);}
                l->data_chunks.push_back(data);
                l->scanner.set_input(*data);

                try{
                 auto fs = read(l->scanner);
                 Ceps_parser_driver driver{ceps_env_current().get_global_symboltable(),fs};
                 ceps::Cepsparser parser{driver};

                 if (parser.parse() != 0 || driver.errors_occured())
                     fatal_(ERR_CEPS_PARSER, file_name);

                 ceps::interpreter::evaluate(current_universe(),
                                             driver.parsetree().get_root(),
                                             ceps_env_current().get_global_symboltable(),
                                             ceps_env_current().interpreter_env(),
                                             &generated_nodes
                                             );
                }
                catch(Statefulscanner<Memory<char>,char>::match_ex & e) {fatal_(ERR_CEPS_PARSER, file_name);}
                catch(Statefulscanner<Memory<char>,char>::eval_ex & e) {fatal_(ERR_CEPS_PARSER, file_name);}
                continue;
            }
        }

		//We are assuming here the file is in ceps format

		std::fstream def_file{ last_file_processed = file_name};
		if (!def_file) fatal_(ERR_FILE_OPEN_FAILED,file_name);

		Ceps_parser_driver driver{ceps_env_current().get_global_symboltable(),def_file};
		ceps::Cepsparser parser{driver};
		if (parser.parse() != 0 || driver.errors_occured())
			fatal_(ERR_CEPS_PARSER, file_name);

		if (result_cmd_line.print_raw_input_tree)
		 ceps::docgen::fmt_out(std::cout, 
		         ceps::ast::nlf_ptr(driver.parsetree().get_root())->children(),
				 docgen_context,
				 result_cmd_line.output_format_flags,
				 false,
				 &ceps_env_current().get_global_symboltable());

		ceps::interpreter::evaluate(current_universe(),
									driver.parsetree().get_root(),
									ceps_env_current().get_global_symboltable(),
									ceps_env_current().interpreter_env(),
									&generated_nodes
									);
	}//for
	return generated_nodes;
}//process_files


std::string gen_guard_id(std::string qual_sim_id, int ctr)
{
	std::string r = qual_sim_id;
	return r + ".guard_"+std::to_string(ctr);
}

bool read_qualified_id(ceps::ast::Nodebase_ptr p, std::vector<std::string> & q_id)
{
	if (p->kind() == ceps::ast::Ast_node_kind::identifier)
	{
		q_id.push_back(name(as_id_ref(p)));return true;
	} else if (p->kind() == ceps::ast::Ast_node_kind::binary_operator && ceps::ast::op(ceps::ast::as_binop_ref(p)) == '.')
	{
		auto & root = ceps::ast::as_binop_ref(p);
		if(!read_qualified_id(root.children()[0],  q_id)) return false;
		p = root.children()[1];
		if (p->kind() != ceps::ast::Ast_node_kind::identifier) return false;
		q_id.push_back(name(as_id_ref(p)));return true;
	}
	return false;
}

void debug_print_qualified_id(std::vector<std::string> const & q_id)
{
	for(size_t i = 0; i < q_id.size();++i)
	{
		std::cerr << q_id[i];
		if (i + 1 < q_id.size()) std::cerr << ".";
	}
}


void print_qualified_id(std::ostream& os,std::vector<std::string> const & q_id)
{
	for(size_t i = 0; i < q_id.size();++i)
	{
		os<< q_id[i];
		if (i + 1 < q_id.size()) os << ".";
	}
}

std::string qualified_id_to_str(std::vector<std::string> const & q_id)
{
	std::string r;
	for(size_t i = 0; i < q_id.size();++i)
	{
		r.append( q_id[i] );
		if (i + 1 < q_id.size()) r.append( "." );
	}
	return r;
}

bool resolve_imports(State_machine & sm, void* context)
{
	auto THIS = static_cast<State_machine_simulation_core*>(context);
#ifdef PRINT_DEBUG
	DEBUG_FUNC_PROLOGUE2
#endif
	for (auto & sub_machine : sm.children_)
	{
        #ifdef PRINT_DEBUG
		DEBUG << "[PROCESS_SUB_MACHINE_START][(REL)ID = "<<  sub_machine->id_ << "]\n";
        #endif

		if (sub_machine->definition_complete()) continue;
        #ifdef PRINT_DEBUG
		DEBUG << "[SUBMACHINE_NOT_COMPLETE][(REL)ID = "<<  sub_machine->id_ << "]\n";
        #endif

		State_machine::unresolve_import_t import_clause;
		bool bfound = false;
		for(auto & e : sm.unresolved_imports())
		{
			if (std::get<2>(e) == sub_machine){import_clause=e;bfound = true;break;}
		}
		if(!bfound) {
            #ifdef PRINT_DEBUG
			 DEBUG << "[PROCESS_SUB_MACHINE_END_IMPORT_CLAUSE_NOT_MATCHED][(REL)ID = "<<  sub_machine->id_ << "]\n";
            #endif
			return false;
		}

		auto template_sm = State_machine::statemachines[std::get<1>(import_clause)];
		if (template_sm == nullptr) {
			THIS->fatal_(-1,"State machine '"+sm.id()+"': import: state machine '"+std::get<1>(import_clause)+"' not found.");
			return false;
		}
		if(template_sm->definition_complete())
		{

		} else resolve_imports(*template_sm,context);
        #ifdef PRINT_DEBUG
		DEBUG << "[CLONE_FROM(1)]\n";
        #endif
		auto rr = THIS->get_qualified_id(sub_machine);
		assert(rr.first);
		sub_machine->clone_from(template_sm,State_machine_simulation_core::SM_COUNTER,rr.second,resolve_imports,context);
		sub_machine->definition_complete() = true;
        #ifdef PRINT_DEBUG
		DEBUG << "[CLONE_FROM(2)]\n";
		DEBUG << "[PROCESS_SUB_MACHINE_END][(REL)ID = "<<  sub_machine->id_ << "]\n";
        #endif
	}
	sm.definition_complete() = true;

	return true;
}

bool State_machine_simulation_core::resolve_imports(State_machine & sm)
{
#ifdef PRINT_DEBUG
	DEBUG_FUNC_PROLOGUE
#endif
	return ::resolve_imports(sm,this);
}

bool State_machine_simulation_core::resolve_q_id(State_machine* smp,
		std::vector<std::string> const & q_id, State_machine::State & s)
{
	assert(smp != nullptr);
	State_machine* current_parent = smp;
	int start_pos = 0;
	if (q_id.size() == 0)
		fatal_(-1,"resolve_q_id(): invalid arguments.");

	if (q_id.size() == 1)
	{
		State_machine::State temp_s(q_id[0]);
		if (smp->lookup(temp_s))
		{
			s = temp_s;
			s.parent_ = current_parent;
			s.unresolved_ = false;
			return true;
		}
		auto temp_sm = State_machine::statemachines[q_id[0]];
		if (temp_sm == nullptr)
		{
			std::stringstream ss; print_qualified_id(ss,q_id);
			fatal_(-1,ss.str()+" is not a state.");
		}
		s.parent_ = temp_sm->parent();
		s.is_sm_ = true; s.id_ = temp_sm->id();s.smp_ = temp_sm;s.q_id_.clear();
		return true;
	}


	if(current_parent == nullptr )
	{
		current_parent = State_machine::statemachines[q_id[0]];
		start_pos = 1;
	}

	if (q_id.size()-start_pos > 1 && current_parent)
	{
	  if (start_pos == 0 && current_parent->get_sub_machine_by_name(q_id[0]) == nullptr)
	  {
	 	 current_parent = State_machine::statemachines[q_id[0]];
	 	 start_pos = 1;
	  }

	  for(size_t i = start_pos; (i < q_id.size() - 1) && current_parent; ++i)
	  {
	    //std::cerr <<"*** parent=" << current_parent->id_ << " #children= "<<current_parent->children().size() <<" Seeking: " << q_id[i] << "\n";
		current_parent = current_parent->get_sub_machine_by_name(q_id[i]);
	  }//for
	}

	if(current_parent == nullptr)
	{
	 std::stringstream ss; print_qualified_id(ss,q_id);
	 fatal_(-1,ss.str()+" is not a state.");
	}



	std::string const & last_step =  q_id[q_id.size()-1];

	{
		bool found = false;
		for(auto p: current_parent->states())
		{
			if(p->id() != last_step) continue;
			found = true;
			s = *p;
			s.parent_ = current_parent;
			s.unresolved_ = false;
			break;
		}
		if(found) return true;
	}

	assert(current_parent != nullptr);
	auto pp = current_parent->get_sub_machine_by_name(last_step);
	if (pp == nullptr){
		std::stringstream ss; print_qualified_id(ss,q_id);
		fatal_(-1,"'"+ss.str()+"'is not a state/submachine");
	}

	if(current_parent == nullptr)
	{
	 std::stringstream ss; print_qualified_id(ss,q_id);
	 fatal_(-1,ss.str()+" is not a state.");
	}

	s.is_sm_ = true; s.id_ = pp->id_; s.smp_ = pp; s.parent_ = current_parent;
	s.unresolved_ = false;
	return true;
}



std::string State_machine_simulation_core::get_full_qualified_id(State_machine::State const & s)
{
	state_rep_t sr;
	sr.is_sm_ = s.is_sm_;
	sr.sid_ = s.id_;
	sr.smp_ = s.smp_;
	sr.valid_ = true;
	return get_fullqualified_id(sr);
}


State_machine::State* State_machine_simulation_core::find_state(std::string compound_id, State_machine* parent){
    using namespace ceps::ast;
    if (compound_id.length() == 0) return nullptr;
    if (compound_id.find_first_of('.') == std::string::npos){
     if(parent == nullptr) return nullptr;
     else
      for(auto p: parent->states()){
           if (p->id() == compound_id) return p;
      }
    } else {
     auto pos = compound_id.find_first_of('.');
     auto base_id = compound_id.substr(0,pos);
     auto rest = compound_id.substr(pos+1);
     if (parent == nullptr){
      auto it = State_machine::statemachines.find(base_id);
      if (it == State_machine::statemachines.end()) return nullptr;
      return find_state(rest, it->second);
     }
     for(auto const & c : parent->children())
      if (c->id_ == base_id) return find_state(rest, c);
     for(auto p: parent->states()){
      if (!p->is_sm_) continue;
      if (p->id_ == base_id) return find_state(rest, p->smp_);
     }
    }
    return nullptr;
}

state_rep_t State_machine_simulation_core::resolve_state_qualified_id(std::string compound_id, State_machine* parent)
{
 using namespace ceps::ast;
 if (compound_id.length() == 0) return state_rep_t{};
 if (compound_id.find_first_of('.') == std::string::npos){
  std::string id = compound_id;
  if (parent == nullptr) {
   auto it = State_machine::statemachines.find(id);
   if (it == State_machine::statemachines.end()) return state_rep_t{};
   return state_rep_t(true,true,it->second,it->second->id_,it->second->idx_);
  } else {
   for(auto const & c : parent->children()){
	if (c->id_ == id) return state_rep_t(true,true,c,c->id_,c->idx_);
   }
   State_machine::State s(id);
   State_machine::State* it = nullptr;//parent->states.find(s);
   for(auto p: parent->states()){
	if (p->id() == id) {it = p;break;}
   }
   if (it == /*parent->states.end()*/nullptr) return state_rep_t{};
   return state_rep_t(true,false,parent,it->id_,it->idx_);
  }
 } else {
  auto pos = compound_id.find_first_of('.');
  auto base_id = compound_id.substr(0,pos);
  auto rest = compound_id.substr(pos+1);
  if (parent == nullptr){
   auto it = State_machine::statemachines.find(base_id);
   if (it == State_machine::statemachines.end()) return state_rep_t{};
   return resolve_state_qualified_id(rest, it->second);
  }
  for(auto const & c : parent->children())
   if (c->id_ == base_id) return resolve_state_qualified_id(rest, c);
  for(auto p: parent->states()){
   if (!p->is_sm_) continue;
   if (p->id_ == base_id) return resolve_state_qualified_id(rest, p->smp_);
  }
 }
 return state_rep_t{};
}

state_rep_t State_machine_simulation_core::resolve_state_or_transition_given_a_qualified_id(ceps::ast::Nodebase_ptr p, State_machine* parent, int* transition_number)
{
 using namespace ceps::ast;
 if (p == nullptr) return state_rep_t{};
 if (p->kind() == ceps::ast::Ast_node_kind::identifier || p->kind() == ceps::ast::Ast_node_kind::symbol){
  std::string id;
  if (p->kind() == ceps::ast::Ast_node_kind::identifier)
	  id = name(as_id_ref(p));
  else id = ceps::ast::name(as_symbol_ref(p));

  if (parent == nullptr) {
	auto it = State_machine::statemachines.find(id);
	if (it == State_machine::statemachines.end()) return state_rep_t{};
	return state_rep_t(true,true,it->second,it->second->id_,it->second->idx_);
  } else {
	for(auto const & c : parent->children())
	{
	 if (c->id_ == id) return state_rep_t(true,true,c,c->id_,c->idx_);
	}
	State_machine::State s(id);

	State_machine::State* it = nullptr;//parent->states.find(s);
	for(auto p: parent->states()){
	 if (p->id() == id) {it = p;break;}
	}
	if (it == nullptr) return state_rep_t{};
	return state_rep_t(true,false,parent,it->id_,it->idx_);
  }
 } else if (p->kind() == ceps::ast::Ast_node_kind::binary_operator && op(ceps::ast::as_binop_ref(p)) == '.') {
  auto root = ceps::ast::as_binop_ptr(p);
  auto l_ = root->left();
  auto l = resolve_state_or_transition_given_a_qualified_id(l_,nullptr);
  if (!l.valid()){
   return l;
  }
  if(!l.is_sm_) return state_rep_t{};
  if (transition_number != nullptr && root->right()->kind() == Ast_node_kind::int_literal)
  {
	  *transition_number = value(as_int_ref(root->right()));
	  return l;
  }
  return resolve_state_or_transition_given_a_qualified_id(root->right(),l.smp_);
 }
 return state_rep_t{};
}

event_rep_t State_machine_simulation_core::resolve_event_qualified_id(ceps::ast::Nodebase_ptr p, State_machine* parent)
	{
		using namespace ceps::ast;
		if (p == nullptr) return event_rep_t{};
		if (p->kind() == ceps::ast::Ast_node_kind::symbol)
		{
			std::string kind = ceps::ast::kind(as_symbol_ref(p));
			std::string name = ceps::ast::name(as_symbol_ref(p));
			if (kind != "Event") return event_rep_t{};
			return event_rep_t(true,nullptr,name);
		}

		if (p->kind() == ceps::ast::Ast_node_kind::identifier){
			std::string id = name(as_id_ref(p));
			if (parent == nullptr) {
				auto it = State_machine::statemachines.find(id);
				if (it == State_machine::statemachines.end()) return event_rep_t{};
				return event_rep_t(true,it->second,"");
			} else
			{
				for(auto const & c : parent->children())
				{
					if (c->id_ == id) return event_rep_t(true,c,"");
				}
				State_machine::Transition::Event ev(id);

				auto it = parent->events().find(ev);
				if (it == parent->events().end()) return event_rep_t{};
				return event_rep_t(true,parent,it->id_);
			}

		} else if (p->kind() == ceps::ast::Ast_node_kind::binary_operator && op(ceps::ast::as_binop_ref(p)) == '.') {
			auto root = ceps::ast::as_binop_ptr(p);
			auto l_ = root->left();
			auto l = resolve_event_qualified_id(l_,nullptr);
			if (!l.valid()) return l;
			if(l.smp_ == nullptr) return event_rep_t{};
			return resolve_event_qualified_id(root->right(),l.smp_);
		}
		return event_rep_t{};
	}


void State_machine_simulation_core::process_simulation(ceps::ast::Nodeset& sim,ceps::Ceps_Environment& ceps_env,ceps::ast::Nodeset& universe)
{
	auto start_states_ns = sim["Start"];
	states_t states;
	/*for(auto const& n: start_states_ns.nodes())
	{
		if (n->kind() != ceps::ast::Ast_node_kind::identifier && n->kind() != ceps::ast::Ast_node_kind::binary_operator) continue;
		if (n->kind() == ceps::ast::Ast_node_kind::binary_operator && op(ceps::ast::as_binop_ref(n)) != '.') continue;
		auto state = resolve_state_qualified_id(n,nullptr);
		std::stringstream ss; ss << ceps::ast::Nodeset(n);
		if (!state.valid()) fatal_(-1,"Expression doesn't evaluate to an existing state: "+ss.str());
		states.push_back(state);
	}//for*/
	//if(enforce_native()) run_simulation(sim,states,ceps_env,universe);
	//else simulate(sim,states,ceps_env,universe);
	run_simulation(sim,states,ceps_env,universe);
}

bool State_machine_simulation_core::is_assignment_op(ceps::ast::Nodebase_ptr n)
{
	return n->kind() == ceps::ast::Ast_node_kind::binary_operator && op(as_binop_ref(n)) == '=';
}

bool State_machine_simulation_core::is_assignment_to_guard(ceps::ast::Binary_operator & node)
{
	return node.left()->kind() == ceps::ast::Ast_node_kind::symbol && kind(as_symbol_ref(node.left())) == "Guard" ;
}
bool State_machine_simulation_core::is_assignment_to_state(ceps::ast::Binary_operator & node, std::string& lhs_id)
{

	using namespace ceps::ast;
	if (node_isrw_state(node.left()))
	 {
		lhs_id = name(as_symbol_ref(node.left())); 
		return true ;
	 }

	if ( node.left()->kind() != ceps::ast::Ast_node_kind::binary_operator || '.' != ceps::ast::op(ceps::ast::as_binop_ref(node.left())))
		return false;
	std::vector<std::string> ids;
	auto p = node.left();
	for(;p->kind() == Ast_node_kind::binary_operator && '.' == op(as_binop_ref(p));)
	{
		auto pp = as_binop_ptr(p);

		if (pp->right()->kind() == Ast_node_kind::identifier)
			ids.push_back(name(as_id_ref(pp->right())));
		else if (pp->right()->kind() == Ast_node_kind::symbol)
			ids.push_back(name(as_symbol_ref(pp->right())));
		else fatal_(-1,"Illformed qualified id (ERRSSSHNMNTSTT1055).\n");
		p = pp->left();
	}

	if (p->kind()!=ceps::ast::Ast_node_kind::symbol) return false;
	lhs_id = name(as_symbol_ref(p));
	if (ids.size())for(int j = ids.size()-1;j >=0;--j) lhs_id.append(".").append(ids[j]);
	return true;
}

void State_machine_simulation_core::eval_guard_assign(ceps::ast::Binary_operator & root)
{
	//std::cerr << "State_machine_simulation_core::eval_guard_assign: " << name(as_symbol_ref(root.left())) << "\n";
	global_guards[name(as_symbol_ref(root.left()))] = root.right();
}

void define_a_struct(State_machine_simulation_core* smc,
		ceps::ast::Struct_ptr sp, std::map<std::string, ceps::ast::Nodebase_ptr> & vars,std::string prefix)
{
#ifdef PRINT_DEBUG
	DEBUG_FUNC_PROLOGUE2
#endif

	if (sp->children().size() == 0){
		auto it = vars.find(prefix);
		if (it == vars.end())
			vars[prefix] = new ceps::ast::Int(0,ceps::ast::all_zero_unit(),nullptr,nullptr,nullptr);
		return;
	}
	bool contains_structs_or_ids = false;
	for(auto p : sp->children()){
		if (p->kind() == ceps::ast::Ast_node_kind::structdef || p->kind() == ceps::ast::Ast_node_kind::identifier)
		{
			contains_structs_or_ids = true;break;
		}
	}
	for(auto p : sp->children())
	{
		if (p->kind() == ceps::ast::Ast_node_kind::structdef)
			define_a_struct(smc,ceps::ast::as_struct_ptr(p), vars, prefix+"."+ceps::ast::name(ceps::ast::as_struct_ref(p)));
		else if (p->kind() == ceps::ast::Ast_node_kind::identifier)		{
			std::string id = prefix + "." + ceps::ast::name(ceps::ast::as_id_ref(p));
			auto it = vars.find(id);
			if (it == vars.end())
				vars[id] = new ceps::ast::Int(0,ceps::ast::all_zero_unit(),nullptr,nullptr,nullptr);
		} else {
			if (contains_structs_or_ids){
				smc->fatal_(-1,"Structuredefinition  illformed (mixes scalars with substructs): "+prefix);
				continue;
			}
			std::string id = prefix;
			auto it = vars.find(id);
			if (it == vars.end())
				vars[id] = p;
		}
	}
}
extern ceps::ast::Nodebase_ptr eval_locked_ceps_expr(State_machine_simulation_core* smc,
		 State_machine* containing_smp,
		 ceps::ast::Nodebase_ptr node,
		 ceps::ast::Nodebase_ptr root_node);

void State_machine_simulation_core::eval_state_assign(ceps::ast::Binary_operator & root,std::string const & lhs_id)
{
#ifdef PRINT_DEBUG
	DEBUG_FUNC_PROLOGUE
	DEBUG << "[STATE_ASSIGNMENT][LHS=" << lhs_id << "]\n";
#endif
	std::lock_guard<std::recursive_mutex>g(states_mutex());

	if (root.right()->kind() == ceps::ast::Ast_node_kind::identifier)
	{
		std::string id = ceps::ast::name(ceps::ast::as_id_ref(root.right()));
		auto it = type_definitions().find(id);
		if (it == type_definitions().end())
			fatal_(-1,id+" is not a type.\n");

		define_a_struct(this,ceps::ast::as_struct_ptr(it->second),get_global_states(),lhs_id );
		return;
	}
	auto rhs = eval_locked_ceps_expr(this,nullptr,root.right(),&root);

	auto pp = get_global_states()[lhs_id];
	if (pp) global_states_prev_[lhs_id] = pp;
	get_global_states()[lhs_id] = rhs;
	if(!pp) global_states_prev_[lhs_id] = rhs;
#ifdef PRINT_DEBUG
	DEBUG << "[STATE_ASSIGNMENT_DONE][LHS=" << lhs_id << "]\n";
#endif
}

void State_machine_simulation_core::add(states_t& states, state_rep_t s)
{
	for(size_t i = 0; i < states.size();++i)
		if (states[i].sid_ == s.sid_ && states[i].smp_ == s.smp_) return;
	states.push_back(s);
}


std::string State_machine_simulation_core::get_fullqualified_id(state_rep_t const & s,std::string delim)
{
	if (!s.valid()) {return "";fatal_(-1,"Invalid Identifier.");}
	State_machine* root = s.smp_;
	if(root != nullptr && root->parent() == nullptr && s.is_sm_) return s.sid_;
	if(root != nullptr && root->parent() == nullptr &&   !s.is_sm_) return root->id_ +delim+s.sid_ ;

	std::vector<State_machine*> v;
	for(;root && root->parent_;root = root->parent_) v.push_back(root->parent_);
	std::string r;
	for(int i = v.size()-1;i >= 0;--i)
	{
		r.append(v[i]->id());
		if (i > 0) r.append(delim);
	}

	if (!s.is_sm_){
		assert(s.smp_ != nullptr);
		return r.append(delim).append(s.smp_->id_).append(delim).append(s.sid_) ;
	}

	return r.append(delim).append(s.sid_);
}

void State_machine_simulation_core::print_info(states_t& states_from, states_t& states_to,std::set<state_rep_t> const& new_states_triggered_set,
		std::set<state_rep_t> const& states_with_no_transition)
{
#ifdef PRINT_DEBUG
	DEBUG_FUNC_PROLOGUE
#endif
	log() << "[";
	auto n1 = states_from.size();

	decltype(n1) i = 0;

	for(const auto & s: states_from)
	{
		if (s.is_sm_) continue;
		log() << get_fullqualified_id(s);
		if (states_with_no_transition.find(s) != states_with_no_transition.end() ) log() << "-";
		log()<< " " /*(i+1 < n1 ? ",":"")*/;++i;
	}

	log() << "] ==> ";
	log() << "[";
	i = 0;
	for(const auto & s: states_to)
	{
		if (s.is_sm_) continue;
		log() << get_fullqualified_id(s) ;
		if (new_states_triggered_set.find(s) != new_states_triggered_set.end() ) log() << "*";
		log() << " " /*(i+1 < n2 ? ",":"")*/;++i;
	}
	log() << "]\n";
}

void State_machine_simulation_core::print_info(states_t const & states)
{


	log() << "[";
	auto n1 = states.size();

	decltype(n1) i = 0;

	for(const auto & s: states)
	{
		if (s.is_sm_) continue;
		log() << get_fullqualified_id(s) << " " /*(i+1 < n1 ? ",":"")*/;++i;
	}

	log() << "]\n";
}

void State_machine_simulation_core::trans_hull_of_containment_rel(states_t& states_in,states_t& states_out)
{
	std::set<state_rep_t> s(states_in.begin(),states_in.end());
	std::set<State_machine*> sms_which_contain_at_least_one_state_in_s;
	for(auto const & state : s) sms_which_contain_at_least_one_state_in_s.insert(state.containing_sm());
	size_t elem_last_count = 0;
	for(;elem_last_count < s.size();)
	{
		elem_last_count = s.size();
		for(auto const & state : s)
		{
			if(!state.valid()) continue;
			if (state.smp_ && state.smp_->parent_) {s.insert(
														state_rep_t(true,
																	true,
																	state.smp_->parent_,
																	state.smp_->parent_->id_,state.smp_->parent_->idx_));
									}//insert possible parent
			if (state.smp_ && state.is_sm_ && state.smp_->has_initial_state()) {
				if (sms_which_contain_at_least_one_state_in_s.find(state.smp_) == sms_which_contain_at_least_one_state_in_s.end() ){
					int idx = -1;
					for (auto s : state.smp_->states()) if (s->id_ == "Initial"){idx = s->idx_;break;}
					s.insert(state_rep_t(true,false,state.smp_,"Initial",idx));
				}
			}
			if(state.is_sm_ && state.smp_->contains_threads())
				for(auto p : state.smp_->children()) if (p->is_thread()) {s.insert(state_rep_t(true,true,p,p->id(),p->idx_));}
		}
	}
	states_out = states_t(s.begin(),s.end());
}


bool State_machine_simulation_core::compute_successor_states_kernel_under_event(event_rep_t ev,
												 states_t& states,
												 std::map<state_rep_t,state_rep_t>& pred,
												 states_t& states_without_transition,
												 ceps::Ceps_Environment& ceps_env,
                                                 ceps::ast::Nodeset ,
												 std::map<state_rep_t,
												 std::vector<State_machine::Transition::Action> >& associated_actions,
												 std::set<state_rep_t> & remove_states,
												 std::set<state_rep_t> & removed_states)
{
	using namespace ceps::ast;
#ifdef PRINT_DEBUG
	DEBUG_FUNC_PROLOGUE
#endif
	associated_actions.clear();states_without_transition.clear();pred.clear();
	std::set<state_rep_t> states_from(states.begin(),states.end());
	std::set<state_rep_t> states_to;
	std::set<State_machine*> threaded_sm_with_all_threads_in_final;
	std::set<State_machine*> active_sms;

	//Assumption: with any state s in states the sm containing s is also in states
	for(auto & s : states)
	{
		if (s.containing_sm()) active_sms.insert(s.containing_sm());
		if (!s.is_sm_) continue;
		if (!s.smp_->join()) continue;
		bool all_threads_in_final = true;
		for(auto th: s.smp_->children())
		{
			if (!th->is_thread()) continue;
			auto it = states_from.find(state_rep_t(true,false,th,"Final",-1));
			if (it == states_from.end()){all_threads_in_final=false;break;}
		}
		if (all_threads_in_final) {
            #ifdef PRINT_DEBUG
			 DEBUG << "[JOIN ACTIVE]["<< s.smp_->id() <<"]" << "\n";
            #endif
			threaded_sm_with_all_threads_in_final.insert(s.smp_);
		}
	}

	bool transition_taken = false;

	for(auto const & s_from : states_from)
	{
		if (remove_states.find(s_from) != remove_states.end()) { removed_states.insert(s_from); continue;}
		if (!s_from.is_sm_) {
		 state_rep_t srep(true,true,s_from.containing_sm(),s_from.smp_->id_ ,s_from.id_);
		 bool ff = remove_states.find(srep) != remove_states.end();

		 if (ff) {removed_states.insert(s_from);continue;}
		}
		bool trans_found = false;
		State_machine* containing_smp = s_from.smp_;
		//assert(s_from.smp_ != nullptr);
		//assert(containing_smp != nullptr);

		//bool ff = false;
		//for(auto m : State_machine::statemachines) if (m.second == containing_smp) {ff = true;break;}
		//assert(ff);
		if (s_from.is_sm_ && s_from.smp_->parent_ ) containing_smp = s_from.smp_->parent_;
        #ifdef PRINT_DEBUG
		 DEBUG << "[CHECKING][STATE="<< s_from.sid_ <<"]" << "\n";
        #endif
		if(s_from.sid_ == "Final" && containing_smp->is_thread() &&
		   threaded_sm_with_all_threads_in_final.find(containing_smp->parent_) != threaded_sm_with_all_threads_in_final.end())
		{
			auto to_state = containing_smp->parent_->join_state();
			states_to.insert(state_rep_t(true,to_state.is_sm_,to_state.smp_,to_state.id_,to_state.idx_));
			transition_taken=trans_found=true;
			pred[state_rep_t(true,to_state.is_sm_,to_state.smp_,to_state.id_,to_state.idx_)] = s_from;
            #ifdef PRINT_DEBUG
			 DEBUG << "[JOIN TAKEN][JOIN_STATE='"<<to_state.id_ <<"']" << "\n";
            #endif
		}

		for(auto  & t : containing_smp->transitions())
		{
			//assert(!t.from().unresolved());
			//assert(!t.to().unresolved());
			//assert(t.from().id_.size() > 0);
			//assert(t.to().id_.size() > 0);
            #ifdef PRINT_DEBUG
			 DEBUG << "[CHECKING TRANSITION]["<<t.from().id_ << "=>"<< t.to().id_ <<"]" << "\n";
            #endif

			bool valid = false;
			if(t.from().is_initial() && s_from.initial()) valid = true;
			if(!valid && t.from().is_final() && s_from.final()) valid = true;
			if(!valid && t.from().id_ == s_from.sid_) valid = true;
			if(!valid) continue;

			bool triggered = (t.events().size()==0);

			if(!triggered)for (auto const & e : t.events())
			{
				if (e.id() == ev.sid_){triggered = true; break;}
			}

			if(!triggered) continue;

			// evaluate guard
			if (t.guard_native_) triggered = (*t.guard_native_)();
			else if (t.has_guard())
			{
				auto it =  get_user_supplied_guards().find(t.guard());
				 if (it !=  get_user_supplied_guards().end()){
					 t.guard_native_ = it->second;
					 triggered = (*t.guard_native_)();
				 } else
				  triggered = eval_guard(ceps_env,t.guard(),states);
			}

			if (!triggered) continue;

			#ifdef PRINT_DEBUG
			 DEBUG << "[TRIGGERED STATE]" << "\n";
            #endif

			if(t.to_.is_sm_ && t.to_.smp_ && active_sms.find(t.to_.smp_) ==  active_sms.end() )
			{
				auto sm  = t.to_.smp_;
				active_sms.insert(sm);
				auto it = sm->find_action("on_enter");
				if (enforce_native() && nullptr != it && nullptr == it->native_func() )
				 fatal_(-1,"Expecting native implementation (--enforce_native):"+it->id());
				if (nullptr != it && nullptr != it->native_func()){
				   current_smp() = it->associated_sm_;
				   it->native_func()();
				} else{
				 if (it != nullptr && it->body_ != nullptr ){
					execute_action_seq(sm,it->body());
				 }
				}
			}

			auto const & to_state = t.to_;
			states_to.insert(state_rep_t(true,to_state.is_sm_,to_state.smp_,to_state.id_,to_state.idx_));
			trans_found=true;transition_taken=true;
			pred[state_rep_t(true,to_state.is_sm_,to_state.smp_,to_state.id_,to_state.idx_)] = s_from;

			for(auto & temp: t.actions()) temp.associated_sm_ = containing_smp;
			//if (t.actions().size()) associated_actions[state_rep_t(true,to_state.is_sm_,to_state.smp_,to_state.id_)] = t.actions();
			if (t.actions().size()){
				//std::cout << "t.actions().size()" << t.actions().size() << std::endl;
				//std::cout << containing_smp->id_ << ":" << t.from_.id_ << "->" << t.to_.id_ << std::endl;
				auto & v = associated_actions[state_rep_t(true,to_state.is_sm_,to_state.smp_,to_state.id_,to_state.idx_)];
				v.insert(v.end(),t.actions().begin(), t.actions().end());

			}
		}
		if(!trans_found) {states_without_transition.push_back(s_from);}
	}
	states = states_t(states_to.begin(),states_to.end());
	return transition_taken;

}

void State_machine_simulation_core::leave_sm(State_machine* smp,states_t & states,std::set<State_machine*>& sms_exited,std::vector<State_machine*>& on_exit_seq)
{
	if(smp == nullptr) return;
	sms_exited.insert(smp);
	for(auto const & sub_sm: smp->children())
	{
		for(auto const & s:states)
		{
			if (s.smp_ == sub_sm && sms_exited.find(sub_sm) == sms_exited.end()){
				leave_sm(sub_sm,states, sms_exited,on_exit_seq);
			}
		}
	}
		// call on_exit here
	on_exit_seq.push_back(smp);
}

void State_machine_simulation_core::enter_sm(bool triggered_by_immediate_child_state, State_machine* smp,
											 std::set<State_machine*>& sms_entered,
											 std::vector<State_machine*>& on_enter_seq,
											 std::set<state_rep_t>& new_states_set,
											 std::vector<State_machine::Transition::Action>& on_enter_sm_derived_action_list,
											 states_t const& current_states)
{
#ifdef PRINT_DEBUG
	DEBUG_FUNC_PROLOGUE
#endif
	if(smp == nullptr) return;
	if(sms_entered.find(smp) != sms_entered.end()) return;// state machine already visited
	sms_entered.insert(smp);
	on_enter_seq.push_back(smp);
	if(triggered_by_immediate_child_state) return;

	if(smp->contains_threads())
	{
		for(auto child : smp->children())
		{
			if(!child->is_thread()) continue;
			new_states_set.insert(state_rep_t(true,true,child,child->id(),child->idx_));
			enter_sm(false,child,sms_entered,on_enter_seq,new_states_set,on_enter_sm_derived_action_list,current_states);
		}
	}

	if(!smp->has_initial_state()) return;

	bool in_initial_state = true;
	for(auto  & t: smp->transitions())
	{
		if (!t.from_.is_initial()) continue;
		if (t.events().size()) continue;
		if (t.has_guard())
		{
			if (!eval_guard(this->ceps_env_current(),t.guard(),current_states)) continue;
		}


		//assert(t.to_.is_sm_ || t.to_.smp_);
		new_states_set.insert(state_rep_t(true,t.to_.is_sm_,t.to_.smp_,t.to_.id(),t.to_.idx_));
		in_initial_state = false;
		for(auto& dd :t.action_ ) dd.associated_sm_ = smp;
		if (t.action_.size()) on_enter_sm_derived_action_list.insert( on_enter_sm_derived_action_list.end(),t.action_.begin(),t.action_.end());
		if (!t.to_.is_sm_) continue;
		enter_sm(false,t.to_.smp_,sms_entered,on_enter_seq,new_states_set,on_enter_sm_derived_action_list,current_states);
	}
	if(in_initial_state) { assert(smp != nullptr); new_states_set.insert(state_rep_t(true,false,smp,"Initial",-1));}
}


void State_machine_simulation_core::start_processing_init_script(ceps::ast::Nodeset& sim,std::size_t& pos,states_t states)
{
#ifdef PRINT_DEBUG
	DEBUG_FUNC_PROLOGUE
#endif
	using namespace ceps::ast;
	event_rep_t ev;
	bool dummy;
	std::vector<State_machine*> on_enter_seq;
	bool r = fetch_event(ev,sim,pos,states,dummy,on_enter_seq,true,true,true);
	if(r)--pos;
}

bool state_machine_sim_core_default_stepping()
{
	std::cout << "\nS>>";
	char buffer[4096] = {0};
	if(!std::cin.getline(buffer,4095)) return true;
	std::string cmd(buffer);
	if (cmd == "quit") return true;
	return false;
}


static ceps::ast::Nodebase_ptr sym_undefined_clbk(ceps::ast::Nodebase_ptr n,ceps::ast::Nodebase_ptr pred, void* ctxt){

	if (ctxt == nullptr) return n;
	return ((State_machine_simulation_core*)ctxt)->eval_found_sym_undefined(n,pred);
}

ceps::ast::Nodebase_ptr State_machine_simulation_core::eval_found_sym_undefined(ceps::ast::Nodebase_ptr n,ceps::ast::Nodebase_ptr pred){
	if (n == nullptr) return nullptr;
	if (pred != nullptr && pred->kind() == ceps::ast::Ast_node_kind::binary_operator && ceps::ast::op(ceps::ast::as_binop_ref(pred)) == '.' ) return n;
	fatal_(-1,"Object '"+ceps::ast::name(ceps::ast::as_symbol_ref(n))+"' of kind '"+ceps::ast::kind(ceps::ast::as_symbol_ref(n))+"' is not initialized.");
	return nullptr;
}



void init_state_machine_simulation(	int argc,
									char ** argv,
									State_machine_simulation_core* smc,
									Result_process_cmd_line& result_cmd_line)
{
	using namespace std;
	if (smc == nullptr) return;
	result_cmd_line = process_cmd_line(argc,argv,result_cmd_line);
	smc->print_debug_info(result_cmd_line.debug_mode);
	smc->quiet_mode() = result_cmd_line.quiet;
	smc->conf_ignore_unresolved_state_id_in_directives() = result_cmd_line.ignore_unresolved_state_id_in_directives;
	smc->logtrace() = result_cmd_line.logtrace;
	smc->generate_cpp_code() = result_cmd_line.cppgen;
	string last_file_processed;

	std::vector<std::string> v = result_cmd_line.definition_file_rel_paths;
	v.insert(v.end(),result_cmd_line.post_processing_rel_paths.begin(),result_cmd_line.post_processing_rel_paths.end() );
    bool skip_next = false;
    for(std::string const & f : v){
      if (skip_next){skip_next=false;continue;}
      if (f =="-e"){skip_next=true;continue;}
      if (!std::ifstream{f})
      {
         last_file_processed = f;
         std::stringstream ss;
         ss << "\n***Error: Couldn't open file '" << f << "' " << std::endl << std::endl;
         throw std::runtime_error(ss.str()) ;
      }
    }


#ifdef __gnu_linux__
    for(auto const & plugin_lib : result_cmd_line.plugins){
        void* handle = dlopen( plugin_lib.c_str(), RTLD_LAZY);
        if (handle == nullptr)
            smc->fatal_(-1,dlerror());

        dlerror();
        void* init_fn_ = dlsym(handle,"init_plugin");
        if (init_fn_ == nullptr)
            smc->fatal_(-1,dlerror());
        auto init_fn = (init_plugin_t)init_fn_;
        init_fn(smc);
    }
#endif
#ifdef _WIN32
/*	for (auto const & plugin_lib : result_cmd_line.plugins) {
        auto handle = dlopen(plugin_lib.c_str(), RTLD_NOW);
        if (handle == nullptr)
            smc->fatal_(-1, dlerror());

        auto init_fn_ = dlsym(handle, "init_plugin");
        if (init_fn_ == nullptr)
            smc->fatal_(-1, dlerror());
        auto init_fn = (init_plugin_t)init_fn_;
        init_fn(smc, register_plugin);
    }*/
#endif


    ceps::interpreter::register_struct_rewrite_rule(
            smc->ceps_env_current().get_global_symboltable(),"partition", sm4ceps::modelling::standard_value_partition_sm, smc);
    ceps::interpreter::register_struct_rewrite_rule(
            smc->ceps_env_current().get_global_symboltable(),"cover_path", sm4ceps::modelling::cover_path, smc);

    ceps::interpreter::register_struct_rewrite_rule(
    		smc->ceps_env_current().get_global_symboltable(),"build_concept_dependency_graph", sm4ceps::utils::build_concept_dependency_graph, smc);
    ceps::interpreter::register_struct_rewrite_rule(
        		smc->ceps_env_current().get_global_symboltable(),"make_stddoc", sm4ceps::utils::make_stddoc, smc);

    smc->ceps_env_current().interpreter_env().reg_sym_undefined_clbk(sym_undefined_clbk,smc);



    if (result_cmd_line.push_dir.length()){
        auto r = opendir(result_cmd_line.push_dir.c_str());
        if (r == NULL){
           auto rr = mkdir(result_cmd_line.push_dir.c_str(),S_ISUID | S_ISGID| S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH );
           if (rr != 0)
               smc->fatal_(-1,"mkdir failed (--push_dir)");
           r = opendir(result_cmd_line.push_dir.c_str());
           if (r == NULL) smc->fatal_(-1,"opendir failed (--push_dir)");
        }
        closedir(r);
        {
            std::string push_package_file;
            std::string push_dir = push_package_file=result_cmd_line.push_dir+(result_cmd_line.push_dir[result_cmd_line.push_dir.length()-1]!='/'?"/":"");
            smc->push_dir = push_dir;
            std::ifstream f{push_dir+"package.ceps"};
            if (f){
                Ceps_parser_driver driver{smc->ceps_env_current().get_global_symboltable(),f};
                ceps::Cepsparser parser{driver};
                if (parser.parse() != 0 || driver.errors_occured())
                    smc->fatal_(State_machine_simulation_core::ERR_CEPS_PARSER, push_package_file);
                std::vector<ceps::ast::Nodebase_ptr> generated_nodes;
                ceps::interpreter::evaluate_without_modifying_universe(smc->current_universe(),
                                            driver.parsetree().get_root(),
                                            smc->ceps_env_current().get_global_symboltable(),
                                            smc->ceps_env_current().interpreter_env(),
                                            &generated_nodes
                                            );
                ceps::ast::Nodeset ns(generated_nodes);
                auto modules = ns["package"]["modules"];
                for(auto e : modules.nodes()){
                    if (e->kind() != ceps::ast::Ast_node_kind::string_literal) continue;
                    result_cmd_line.definition_file_rel_paths.push_back(push_dir+ceps::ast::value(ceps::ast::as_string_ref(e)));
                    smc->push_modules.push_back(ceps::ast::value(ceps::ast::as_string_ref(e)));
                }
            }
        }
    }

    smc->process_files(result_cmd_line.definition_file_rel_paths,last_file_processed,result_cmd_line);
}





void run_state_machine_simulation(State_machine_simulation_core* smc,Result_process_cmd_line const& result_cmd_line)
{
	using namespace std;
	DEBUG_FUNC_PROLOGUE2
	if(result_cmd_line.timeout.length()) smc->timeout_ = 5000;
	if (result_cmd_line.start_in_server_mode)
	{
		DEBUG << "[SERVER_MODE_RECOGNIZED][PORT=" << result_cmd_line.server_port << "]\n";
		register_dispatcher_handler(NMP_SIMPLE_EVENT, &State_machine_simulation_core::process_event_from_remote);
		register_dispatcher_handler(NMP_EVENT_FLAT_PAYLOAD, &State_machine_simulation_core::process_event_from_remote);
		comm_threads.push_back(new std::thread{comm_dispatcher_thread,0,smc,"",result_cmd_line.server_port,nmp_consumer_thread_fn});
		smc->running_as_node() = true;
	}

	if(result_cmd_line.monitor.length())
	{
		DEBUG << "[REQUEST_FOR_MONITORING_SERVICE_DETECTED][PORT=" << result_cmd_line.monitor << "]\n";
		comm_threads.push_back(new std::thread{comm_dispatcher_thread,0,smc,"",result_cmd_line.monitor,nmp_monitoring_thread_fn});
	}

	if (result_cmd_line.remote_nodes.size())
	{
		for(size_t i = 0;i < result_cmd_line.remote_nodes.size();++i )
		{
			DEBUG << "[REMOTE_NODE][IP="<< result_cmd_line.remote_nodes[i].first <<"][PORT=" << result_cmd_line.remote_nodes[i].second << "]\n";
			comm_threads.push_back(new std::thread{ comm_sender_thread,
													smc->allocate_out_event_queue(),
													smc,
													result_cmd_line.remote_nodes[i].first,
													result_cmd_line.remote_nodes[i].second,false});
			smc->running_as_node() = true;
		}
	}

	smc->processs_content(result_cmd_line);
	if (!result_cmd_line.interactive_mode) return;
	string last_file_processed;
	for(;std::cin;)
	{
		try {
		std::cout << ">>";
		char buffer[4096] = {0};
		if(!std::cin.getline(buffer,4095)) break;
		std::string cmd(buffer);


		if (cmd == "quit") break;
		if (cmd.substr(0,5) == "load ")
		{
			std::string file = cmd.substr(5);

			std::vector<std::string> v;
			v.push_back(file);
			smc->reset_universe();
			smc->process_files(v,last_file_processed);
		}
		else if (cmd.substr(0,3) == "run")
		{
			smc->processs_content(result_cmd_line);
		}
		else if (cmd.substr(0,4) == "step")
		{
				smc->set_step_handler(state_machine_sim_core_default_stepping);
				smc->processs_content(result_cmd_line);
		}
		else if (cmd.substr(0,8) == "machines")
		{
			for(auto  m : State_machine::statemachines )
			{
				std::cout << m.first;
				if (m.second->contains_threads()) std::cout << " (multithreaded)";
				if (m.second->join()) std::cout << " (join state="<< m.second->join_state().id() << ")";
				if (m.second->is_thread()) std::cout << " (thread)";


				std::cout << "\n";
			}

		}
		else if (cmd.substr(0,6) == "guards")
		{
			if (smc->guards().size() == 0) std::cout << "***No guards defined.\n";
			for(auto g : smc->guards())
			{
				auto guard_expr = g.second;
				if (guard_expr == nullptr) continue;
				//smc->ceps_env_current().interpreter_env().symbol_mapping()["Systemstate"] = &smc->states();
				//auto  result = ceps::interpreter::evaluate(guard_expr,smc->ceps_env_current().get_global_symboltable(),smc->ceps_env_current().interpreter_env()	);

				std::cout << g.first << ": ";
				std::cout << *g.second << "\n";
				//std::cout << "  Value = " << *result << std::endl;
			}
		}
		else if (cmd.substr(0,6) == "states")
		{
			if (smc->get_global_states().size() == 0) std::cout << "***No states defined.\n";
			for(auto s : smc->get_global_states())
			{
				auto state_expr = s.second;
				if (state_expr == nullptr) continue;
				std::cout << s.first << ": ";
				std::cout << *s.second << "\n";
			}
		}

		else std::cerr << "***Unknown command:" << cmd << std::endl;

		} catch (ceps::interpreter::semantic_exception & se)
			{
				std::cerr << "***Fatal error: "<< se.what() << std::endl;
			}
			catch (std::runtime_error & re)
			{
				std::cerr << "***Fatal error: " << re.what() << std::endl;
			}
	}//for

}

void state_machine_simulation_fatal(int code, std::string const & msg )
{
	using namespace std;
	stringstream ss;
	if (State_machine_simulation_core::ERR_FILE_OPEN_FAILED == code)
		ss << "Couldn't open file '" << msg <<"'.";
	else if (State_machine_simulation_core::ERR_CEPS_PARSER == code)
		ss << "A parser exception occured in file '" << msg << "'.";
	else ss << msg;
	throw runtime_error{ss.str()};
}

void state_machine_simulation_warn(int code, std::string const & msg)
{
	using namespace std;
	if (State_machine_simulation_core::WARN_XML_PROPERTIES_MISSING_PREFIX_DEFINITION == code)
		cerr << "\n***WARNING. No xml file prefix defined for '"
		<< msg
		<< "', will use default (empty string).\nIf you want different behaviour please add the following to the global namespace:\n\txml{gen_file_prefix{"
		<< msg
		<< "{\"MY_XML_FILENAME_PREFIX_HERE\";};};.\n";
	else if (State_machine_simulation_core::WARN_CANNOT_FIND_TEMPLATE == code)
		cerr << "\n***WARNING. No template found which matches the path "
		<< msg
		<< "." << endl;
	else if (State_machine_simulation_core::WARN_NO_INVOCATION_MAPPING_AND_NO_TABLE_DEF_FOUND == code)
		cerr << "\n***WARNING. There exists neither a xml invocation mapping nor a db2 table definition for  "
		<< msg
		<< ". No file for this particular object will be generated." << endl;
	else if (State_machine_simulation_core::WARN_TESTCASE_ID_ALREADY_DEFINED == code)
		cerr << "\n***WARNING. Id already defined in another testcase precondition: "
		<< msg
		<< "." << endl;
	else if (State_machine_simulation_core::WARN_TESTCASE_EMPTY == code)
		cerr << "\n***WARNING. Testcase precondition is empty: "
		<< msg
		<< "." << endl;
	else if (State_machine_simulation_core::WARN_NO_STATEMACHINES == code)
		cerr << "\n***WARNING. No statemachines found"
		<< msg
		<< "." << endl;
	else
		cerr << "\n***WARNING. " << msg <<"."<< endl;

}

void State_machine_simulation_core::process_event_from_remote(nmp_header h,char* data)
{
	if (h.id == NMP_EVENT_FLAT_PAYLOAD)
	{
		int ev_name_len = ntohl(*((int*)data));
		char buffer[1024] = {0};
		strncpy(buffer,data+sizeof(int),std::min(1023,(int)ev_name_len));

		size_t offs = ev_name_len+sizeof(int);
		event_t ev(buffer);
		for(;offs < h.len;)
		{
			int nmp_payload_id = ntohl(*(int*)(data+offs));
			if (nmp_payload_id == NMP_PAYLOAD_INT)	{
				offs+=sizeof(int);
				int v;
				ceps::ast::Unit_rep::sc_t m,kg,s,ampere,kelvin,mol,candela;
				auto r = ceps::deserialize_value(v, data+offs, h.len-offs);offs+=r;
				v = ntohl(v);
				r = ceps::deserialize_value(m, data+offs, h.len-offs);m = (ceps::ast::Unit_rep::sc_t)ntohl(m);offs+=r;
				r = ceps::deserialize_value(kg, data+offs, h.len-offs);kg = (ceps::ast::Unit_rep::sc_t)ntohl(kg);offs+=r;
				r = ceps::deserialize_value(s, data+offs, h.len-offs);s = (ceps::ast::Unit_rep::sc_t)ntohl(s);offs+=r;
				r = ceps::deserialize_value(ampere, data+offs, h.len-offs);ampere = (ceps::ast::Unit_rep::sc_t)ntohl(ampere);offs+=r;
				r = ceps::deserialize_value(kelvin, data+offs, h.len-offs);kelvin = (ceps::ast::Unit_rep::sc_t)ntohl(kelvin);offs+=r;
				r = ceps::deserialize_value(mol, data+offs, h.len-offs);mol = (ceps::ast::Unit_rep::sc_t)ntohl(mol);offs+=r;
				r = ceps::deserialize_value(candela, data+offs, h.len-offs);candela = (ceps::ast::Unit_rep::sc_t)ntohl(candela);offs+=r;
				if (this->enforce_native()) ev.payload_native_.push_back(sm4ceps_plugin_int::Variant{v});
				else ev.payload_.push_back(new ceps::ast::Int( v, ceps::ast::Unit_rep(m,kg,s,ampere,kelvin,mol,candela), nullptr, nullptr, nullptr));
			} else if (nmp_payload_id == NMP_PAYLOAD_DOUBLE)	{
				offs+=sizeof(int);
				double v;
				ceps::ast::Unit_rep::sc_t m,kg,s,ampere,kelvin,mol,candela;
				auto r = ceps::deserialize_value(v, data+offs, h.len-offs);offs+=r;
				r = ceps::deserialize_value(m, data+offs, h.len-offs);m = (ceps::ast::Unit_rep::sc_t)ntohl(m);offs+=r;
				r = ceps::deserialize_value(kg, data+offs, h.len-offs);kg = (ceps::ast::Unit_rep::sc_t)ntohl(kg);offs+=r;
				r = ceps::deserialize_value(s, data+offs, h.len-offs);s = (ceps::ast::Unit_rep::sc_t)ntohl(s);offs+=r;
				r = ceps::deserialize_value(ampere, data+offs, h.len-offs);ampere = (ceps::ast::Unit_rep::sc_t)ntohl(ampere);offs+=r;
				r = ceps::deserialize_value(kelvin, data+offs, h.len-offs);kelvin = (ceps::ast::Unit_rep::sc_t)ntohl(kelvin);offs+=r;
				r = ceps::deserialize_value(mol, data+offs, h.len-offs);mol = (ceps::ast::Unit_rep::sc_t)ntohl(mol);offs+=r;
				r = ceps::deserialize_value(candela, data+offs, h.len-offs);candela = (ceps::ast::Unit_rep::sc_t)ntohl(candela);offs+=r;
				if (this->enforce_native()) ev.payload_native_.push_back(sm4ceps_plugin_int::Variant{v});
				else ev.payload_.push_back(new ceps::ast::Double( v, ceps::ast::Unit_rep(m,kg,s,ampere,kelvin,mol,candela), nullptr, nullptr, nullptr));
			} else if (nmp_payload_id == NMP_PAYLOAD_STRING)	{
				offs+=sizeof(int);
				std::string v;
				auto r = ceps::deserialize_value(v, data+offs, h.len-offs);offs+=r;
				if (this->enforce_native()) ev.payload_native_.push_back(sm4ceps_plugin_int::Variant{v});
				else ev.payload_.push_back(new ceps::ast::String( v, nullptr, nullptr, nullptr));
			} else {
				return;
			}
		}
		ev.already_sent_to_out_queues_=true;
		main_event_queue().push(ev);
	} else {
	 char buffer[1024] = {0};
	 strncpy(buffer,data,std::min(1023,(int)h.len));
	 event_t ev(buffer);
	 ev.already_sent_to_out_queues_=true;
	 ev.unique_ = this->unique_events().find(ev.id_) != this->unique_events().end();
	 main_event_queue().push(ev);
	}
}


void State_machine_simulation_core::register_plugin_fn(std::string const & id,smcore_plugin_fn_t fn){
	name_to_smcore_plugin_fn[id] = fn;
}

void State_machine_simulation_core::drop_all_sms(){
	State_machine::statemachines.clear();
}

void* State_machine_simulation_core::create_sm(std::string name, std::string full_name,int depth, int order){
	auto sm = new State_machine(order,name,nullptr,depth);
	State_machine::statemachines[full_name] = sm;
	set_qualified_id(sm,full_name);
	return sm;
}

bool State_machine_simulation_core::sm_set_parent(void * sm, void * parent){
	if (sm == nullptr) return false;
	((State_machine*)sm)->parent_ = (State_machine*)parent;
	return true;
}

bool State_machine_simulation_core::sm_add_child(void* sm, void * child){
	if (sm == nullptr || child == nullptr) return false;
	((State_machine*)sm)->add_child((State_machine*)child);
	return true;
}
bool State_machine_simulation_core::sm_set_misc_attributes(void* sm_, bool is_thread, bool contains_threads, bool complete, bool join, bool idx){
	if (sm_ == nullptr) return false;
	auto sm = (State_machine*)sm_;
	sm->is_thread_ = is_thread;
	sm->contains_threads_ = contains_threads;
	sm->complete_ = complete;
	sm->join_ = join;
	sm->idx_ = idx;
	return true;
}

void State_machine_simulation_core::sm_add_state(void* sm_, std::string id, bool is_sm, void* smp, void* parent,bool unresolved,bool idx ){
	if (sm_ == nullptr) return;
	auto sm = (State_machine*)sm_;
	State_machine::State* s = new State_machine::State(id);
	s->id_ = id; s->is_sm_ = is_sm; s->smp_ = (State_machine*)smp; s->parent_ = (State_machine*)parent;s->unresolved_ = unresolved;s->idx_ = idx;
	sm->states_.insert(s);
}

void State_machine_simulation_core::sm_add_transition(void* sm_,int slot, std::string guard, void * orig_parent){
	if (sm_ == nullptr) return;
	auto sm = (State_machine*)sm_;

	State_machine::Transition * tp = nullptr;
	if (slot < 0) {sm->transitions().push_back(State_machine::Transition());tp = &sm->transitions()[sm->transitions().size()-1];}
	else {sm->threads()[slot].push_back(State_machine::Transition());tp = &sm->threads()[slot][sm->threads()[slot].size()-1];}

	tp->guard_ = guard;
	tp->orig_parent_ = (State_machine*)orig_parent;
}
void State_machine_simulation_core::sm_transition_set_from(void* sm_,int slot, std::string id , bool is_sm ,
		                                                   void * smp, void * parent, bool unresolved, int idx){
	if (sm_ == nullptr) return;
	auto sm = (State_machine*)sm_;

	State_machine::Transition * tp = nullptr;
	if (slot < 0) tp = &sm->transitions()[sm->transitions().size()-1];
	else tp = &sm->threads()[slot][sm->threads()[slot].size()-1];

	tp->from_.id_ = id;
	tp->from_.idx_ = idx;
	tp->from_.is_sm_ = is_sm;
	tp->from_.parent_ = (State_machine*) parent;
	tp->from_.smp_ = (State_machine*) smp;
	tp->from_.unresolved_ = unresolved;
}

void State_machine_simulation_core::sm_transition_set_to(void* sm_,int slot, std::string id , bool is_sm ,
		                                                   void * smp, void * parent, bool unresolved, int idx){
	if (sm_ == nullptr) return;
	auto sm = (State_machine*)sm_;

	State_machine::Transition * tp = nullptr;
	if (slot < 0) tp = &sm->transitions()[sm->transitions().size()-1];
	else tp = &sm->threads()[slot][sm->threads()[slot].size()-1];

	tp->to_.id_ = id;
	tp->to_.idx_ = idx;
	tp->to_.is_sm_ = is_sm;
	tp->to_.parent_ = (State_machine*) parent;
	tp->to_.smp_ = (State_machine*) smp;
	tp->to_.unresolved_ = unresolved;
}
void State_machine_simulation_core::sm_transition_add_ev(void* sm_,int slot,std::string id, int idx){
	if (sm_ == nullptr) return;
	auto sm = (State_machine*)sm_;

	State_machine::Transition * tp = nullptr;
	if (slot < 0) tp = &sm->transitions()[sm->transitions().size()-1];
	else tp = &sm->threads()[slot][sm->threads()[slot].size()-1];

	State_machine::Transition::Event ev;
	ev.id_ = id;
	ev.evid_ = idx;
	tp->events_.insert(ev);
}
void State_machine_simulation_core::sm_transition_add_action(void* sm_,int slot,std::string id, void* assoc_sm){
	if (sm_ == nullptr) return;
	auto sm = (State_machine*)sm_;

	State_machine::Transition * tp = nullptr;
	if (slot < 0) tp = &sm->transitions()[sm->transitions().size()-1];
	else tp = &sm->threads()[slot][sm->threads()[slot].size()-1];

	State_machine::Transition::Action ac;
	ac.associated_sm_ = (State_machine*)assoc_sm;
	ac.id_ = id;
	tp->actions().push_back(ac);
}

void State_machine_simulation_core::sm_transition_add_action(void* sm_,std::string id, void* assoc_sm){
	if (sm_ == nullptr) return;
	auto sm = (State_machine*)sm_;

	State_machine::Transition::Action ac;
	ac.associated_sm_ = (State_machine*)assoc_sm;
	ac.id_ = id;
	sm->actions().push_back(ac);
}

void State_machine_simulation_core::sm_set_join_state(void* sm_, std::string id, bool is_sm, void* smp, void* parent,bool unresolved,bool idx ){
	if (sm_ == nullptr) return;
	auto sm = (State_machine*)sm_;

    sm->join_state_.id_ = id;
    sm->join_state_.is_sm_ = is_sm;
    sm->join_state_.smp_ = (State_machine*)smp;
    sm->join_state_.parent_ = (State_machine*)parent;
    sm->join_state_.unresolved_ = unresolved;
    sm->join_state_.idx_ =idx;
}

void State_machine_simulation_core::sm_add_ref_to_sm_at_least_one_transition_was_moved_to(void* sm_, void* sm_to){
	if (sm_ == nullptr) return;
	auto sm = (State_machine*)sm_;

	sm->smps_containing_moved_transitions_.push_back((State_machine*) sm_to);
}

void State_machine_simulation_core::reg_ceps_plugin(std::string name, smcore_plugin_fn_t fn){
    register_plugin_fn(name,fn);
}

void* State_machine_simulation_core::get_sm(std::string name){
 return State_machine::statemachines[name];
}

// Reporting
static ceps::ast::Binary_operator* mkop(std::string o,ceps::ast::Nodebase_ptr l, ceps::ast::Nodebase_ptr r){
	return new ceps::ast::Binary_operator(o[0],(o.size() > 1 ? o : ""),l,r,nullptr);
}

static ceps::ast::Nodebase_ptr compute_dot_expr_from_sm_state_given_as_string(std::string sm_state){
	size_t beg = 0;
	auto dot_pos = sm_state.find_first_of(".");
    if (dot_pos == std::string::npos) return new ceps::ast::Identifier(sm_state.substr(beg));
	auto dot_pos2 = sm_state.substr(dot_pos+1).find_first_of(".");
    if (dot_pos2 == std::string::npos)
    	return mkop(".",new ceps::ast::Identifier(sm_state.substr(beg,dot_pos)),new ceps::ast::Identifier(sm_state.substr(dot_pos+1)));
    dot_pos2 += dot_pos+1;
    ceps::ast::Nodebase_ptr left_side =
    		mkop(".",
    			  new ceps::ast::Identifier(sm_state.substr(beg,dot_pos)),
				  new ceps::ast::Identifier(sm_state.substr(dot_pos+1,dot_pos2-dot_pos-1)));
    beg = dot_pos2+1;
	for(;beg < sm_state.length();){
		auto dot_pos = sm_state.substr(beg).find_first_of(".");
		if (dot_pos != std::string::npos){
			dot_pos += beg;
			left_side = mkop(".",left_side,new ceps::ast::Identifier(sm_state.substr(beg,dot_pos-beg)));
            beg = dot_pos + 1;
		} else
         return mkop(".",left_side,new ceps::ast::Identifier(sm_state.substr(beg)));
	}
	return nullptr;
}

static std::vector<ceps::ast::Nodebase_ptr> mk_sm_state_exprs(std::vector<std::string> const & v){
	std::vector<ceps::ast::Nodebase_ptr> r;
	for(auto const & e : v){
		r.push_back(compute_dot_expr_from_sm_state_given_as_string(e));
	}
	return r;
}

static ceps::ast::Nodebase_ptr compute_ceps_expr_from_sm_state_transition(const executionloop_context_t & ctx,int t){
	ceps::ast::Nodebase_ptr r = nullptr;

	auto from_sm = compute_dot_expr_from_sm_state_given_as_string(ctx.idx_to_state_id.find(ctx.transitions[t].from)->second);
    int start = t;
	for(;start > 0;--start) if (ctx.transitions[start].smp != ctx.transitions[start-1].smp) break;
    int idx = 0;
    for(int i = start; i != t;++i) idx += ctx.transitions[t].from == ctx.transitions[i].from;
    r = mkop("-",from_sm,new ceps::ast::Int(idx,ceps::ast::all_zero_unit(), nullptr, nullptr, nullptr));
	return r;
}

static std::vector<ceps::ast::Nodebase_ptr>  mk_sm_state_transition_exprs(State_machine_simulation_core* smc, std::vector<int> v){
	std::vector<ceps::ast::Nodebase_ptr> r;
	for(auto const & e : v){
		auto expr = compute_ceps_expr_from_sm_state_transition(smc->executionloop_context(),e);
		if(expr) r.push_back(expr);
	}
	return r;
}

static void print_report_coverage(std::ostream& os, ceps::ast::Nodeset coverage, std::string indent,Result_process_cmd_line const& result_cmd_line){
    if (coverage["state_coverage"].empty()) return;

    double v1 = coverage["state_coverage"]["ratio"].as_double();
    auto state_coverage_valid = coverage["state_coverage"]["valid"].as_int();
    double v2 = coverage["transition_coverage"]["ratio"].as_double();
    auto transition_coverage_valid = coverage["transition_coverage"]["valid"].as_int();

    if (!state_coverage_valid || !transition_coverage_valid) return;
	os << indent << "State Coverage: ";
	os << v1 << " ( "<< coverage["state_coverage"]["percentage"].as_double() << "% )"<< "\n";
	os << indent << "Transition Coverage: ";
	os << v2 << " ( "<< coverage["transition_coverage"]["percentage"].as_double() << "% )"<< "\n";
}

static void print_report_categories(std::ostream& os, ceps::ast::Nodeset categories, std::string indent,Result_process_cmd_line const& result_cmd_line){
    if (categories.empty()) return;
    os << indent << "Categories in total:\n";
    for(auto p : categories.nodes())
        os << indent << " "<< ceps::ast::name(ceps::ast::as_struct_ref(p)) << "\n";
    os << indent << "Categories active:\n";
    for(auto p : categories.nodes())
        os << indent << " "<< ceps::ast::name(ceps::ast::as_struct_ref(p))<<": "
           << ceps::ast::value(ceps::ast::as_int_ref(ceps::ast::as_struct_ref(p).children()[0])) << "\n";

}

static void print_report(std::ostream& os, ceps::ast::Nodeset report,Result_process_cmd_line const& result_cmd_line ){
	if (result_cmd_line.report_format_sexpression) {
		os << ceps::ast::Nodebase::pretty_print << report;
		return;
	}
	auto summary = report["summary"];
	if (result_cmd_line.debug_mode)os << "States count: " << report["summary"]["general"]["states_total"].as_int() << "\n";
    print_report_coverage(os, summary["coverage"], "",result_cmd_line);
    if (result_cmd_line.report_includes_cat) print_report_categories(os, summary["categories"], "",result_cmd_line);
}

static inline State_machine* get_toplevel(State_machine* sm){
 for(;sm->parent();sm = sm->parent());return sm;
}


#include <unordered_set>
template<typename F, typename T> void traverse_sm(std::unordered_set<State_machine*>& m,State_machine* sm, T const & sms, F f){
    f(sm);

    for(auto state: sm->states()){
        if (!state->is_sm() || state->smp() == nullptr) continue;
        if (m.find(state->smp()) != m.end()) continue;
        m.insert(state->smp());
        traverse_sm(m,state->smp(),sms,f);
    }

    for(auto subsm: sm->children()){
        //assert(m.find(subsm) != m.end());
        if (m.find(subsm) != m.end()) continue;

        m.insert(subsm);
        traverse_sm(m,subsm,sms,f);
    }
}

template<typename F, typename T> void traverse_sms(T const & sms, F f){
    std::unordered_set<State_machine*> m;
    for(auto sm: sms){
     if (m.find(sm.second) != m.end()) continue;
     traverse_sm(m,sm.second,sms,f);

     m.insert(sm.second);
    }
}

// "2018-07-07 00:00:00.000"
template <typename T> static std::string gen_timestamp(std::chrono::time_point<T> tmp, bool with_msec = true){
    using namespace std;
    auto time_now = chrono::high_resolution_clock::to_time_t(tmp);
    auto ms_mod = std::chrono::duration_cast<std::chrono::milliseconds>(tmp - std::chrono::time_point<T>{}).count() % 1000;
    auto t = std::localtime(&time_now);
    std::stringstream s;
    s << t->tm_year + 1900;
    s << "-"; if (t->tm_mon+1 < 10) s << "0";s << t->tm_mon+1;
    s << "-"; if (t->tm_mday < 10) s << "0";s << t->tm_mday;
    s << " ";
    if (t->tm_hour < 10)s << "0"; s << t->tm_hour <<":";
    if (t->tm_min < 10) s << "0"; s << t->tm_min <<":";
    if (t->tm_sec < 10) s << "0"; s << t->tm_sec;
    if(!with_msec) return s.str();
    s << ".";
    if (ms_mod < 100) s << "0";
    if (ms_mod < 10) s << "0";
    s << ms_mod;
    return s.str();
}

template <typename T> static std::string gen_gmt_timestamp(std::chrono::time_point<T> tmp, bool with_msec = true){
    using namespace std;
    auto time_now = chrono::high_resolution_clock::to_time_t(tmp);
    auto ms_mod = std::chrono::duration_cast<std::chrono::milliseconds>(tmp - std::chrono::time_point<T>{}).count() % 1000;
    auto t = std::gmtime(&time_now);
    std::stringstream s;
    s << t->tm_year + 1900;
    s << "-"; if (t->tm_mon+1 < 10) s << "0";s << t->tm_mon+1;
    s << "-"; if (t->tm_mday < 10) s << "0";s << t->tm_mday;
    s << " ";
    if (t->tm_hour < 10)s << "0"; s << t->tm_hour <<":";
    if (t->tm_min < 10) s << "0"; s << t->tm_min <<":";
    if (t->tm_sec < 10) s << "0"; s << t->tm_sec;
    if(!with_msec) return s.str();
    s << ".";
    if (ms_mod < 100) s << "0";
    if (ms_mod < 10) s << "0";
    s << ms_mod;
    return s.str();
}

template <typename T> static int get_ms_mod_1000(std::chrono::time_point<T> tmp){
    using namespace std;
    return std::chrono::duration_cast<std::chrono::milliseconds>(tmp - std::chrono::time_point<T>{}).count() % 1000;
}
template <typename T> static int get_secs(std::chrono::time_point<T> tmp){
    using namespace std;
    return chrono::high_resolution_clock::to_time_t(tmp);
}

ceps::ast::Nodeset State_machine_simulation_core::make_report(){
	using namespace ceps::ast;

    if (!stateindex2categories_valid){
        //Compute categorizations
        auto f = [this](State_machine* cur_sm){
            for(auto it = cur_sm->states().begin(); it != cur_sm->states().end(); ++it) {
             auto state = *it;
             std::vector<std::string> v;
             for(auto e: state->categories()) {v.push_back(e);statistics_category[e]=0;}
             stateindex2categories[state->idx_] = v;
            }
         };
        traverse_sms(State_machine::statemachines,f);
        stateindex2categories_valid = true;
    } else if (statistics_category.size()){
        for(auto & e:statistics_category) e.second = 0;
    }
    bool do_cat = statistics_category.size();
	ceps::ast::Nodeset result;

	double state_coverage = 0.0;
	double transition_coverage = 0.0;

	int number_of_states_to_cover = 0;
	int number_of_transitions_to_cover = 0;
	int number_of_states_covered = 0;
	int number_of_transitions_covered = 0;
	std::vector<std::string> state_coverage_state_list;
	std::vector<std::string> states_hit_list;
	std::vector<int> states_hit_counter;

	std::vector<std::string> state_coverage_missing_states_list;
	std::vector<ceps::ast::Nodebase_ptr> state_coverage_states_list_ceps_expr;
	std::vector<ceps::ast::Nodebase_ptr> state_coverage_missing_states_list_ceps_expr;

	std::vector<int> transition_coverage_list;
	std::vector<int> transition_coverage_missing_list;
	std::vector<ceps::ast::Nodebase_ptr> transition_coverage_list_ceps_expr;
	std::vector<ceps::ast::Nodebase_ptr> transition_coverage_missing_list_ceps_expr;

	std::vector<ceps::ast::Nodebase_ptr> toplevel_state_machines_state_coverage_stats;
    std::vector<ceps::ast::Nodebase_ptr> enter_times;
    std::vector<ceps::ast::Nodebase_ptr> exit_times;
	std::map<State_machine*,int> sm_states_covered;
	std::map<State_machine*,int> sm_states_not_covered;

	std::vector<ceps::ast::Nodebase_ptr> toplevel_state_machines_transition_coverage_stats;
	std::map<State_machine*,int> sm_transitions_covered;
	std::map<State_machine*,int> sm_transitions_not_covered;


    auto const& ctx=executionloop_context();
    bool state_coverage_defined = false;
    bool transition_coverage_defined = false;
    number_of_states_to_cover = 0;

    if (do_cat){
        auto const& v = executionloop_context().current_states;
        for(std::size_t i = 0; i != v.size(); ++i) {
            if (!v[i]) continue;
            auto const& vv = stateindex2categories[i];
            for(auto const & s : vv)  ++statistics_category[s];
        }
    }

	//Distill information relating to state coverage
	if ( (state_coverage_defined = ctx.start_of_covering_states_valid()) ){
		//number_of_states_to_cover = ctx.coverage_state_table.size();
		for(std::size_t i = 0;i != ctx.coverage_state_table.size();++i){
		 if (ctx.get_inf(i+ctx.start_of_covering_states,executionloop_context_t::INIT) ||
             ctx.get_inf(i+ctx.start_of_covering_states,executionloop_context_t::FINAL) ||
             ctx.get_inf(i+ctx.start_of_covering_states,executionloop_context_t::DONT_COVER)
			 /*|| ctx.get_inf(i+ctx.start_of_covering_states,executionloop_context_t::SM)*/ ) continue;
		 number_of_states_covered += ctx.coverage_state_table[i] != 0;
		 ++number_of_states_to_cover;
		 if (ctx.coverage_state_table[i]){
			    auto sid_of_state = ctx.idx_to_state_id.find(i+ctx.start_of_covering_states)->second;
			    states_hit_list.push_back(sid_of_state);
				states_hit_counter.push_back(ctx.coverage_state_table[i]);
				state_coverage_state_list.push_back(sid_of_state);
				assert(ctx.assoc_sm[i+ctx.start_of_covering_states] != nullptr);
				++sm_states_covered[get_toplevel(ctx.assoc_sm[i+ctx.start_of_covering_states])];
		 } else {
				state_coverage_missing_states_list.push_back(ctx.idx_to_state_id.find(i+ctx.start_of_covering_states)->second);
		        assert(ctx.assoc_sm[i+ctx.start_of_covering_states] != nullptr);
		        ++sm_states_not_covered[get_toplevel(ctx.assoc_sm[i+ctx.start_of_covering_states])];
		}
	   }
	}

	for (auto sm : statemachines()){
		if (sm.second->parent() != nullptr) continue;
        if (0 == sm_states_covered[sm.second] + sm_states_not_covered[sm.second]) continue;
		double ratio = (double)sm_states_covered[sm.second] / (double)(sm_states_covered[sm.second] + sm_states_not_covered[sm.second]);
		toplevel_state_machines_state_coverage_stats.push_back(
		 (new strct{sm.first,ratio })->p_strct
		);
	}

	state_coverage_states_list_ceps_expr = mk_sm_state_exprs(state_coverage_state_list);
	auto states_hit_list_v =  mk_sm_state_exprs(states_hit_list);
	state_coverage_missing_states_list_ceps_expr = mk_sm_state_exprs(state_coverage_missing_states_list);


	//Distill information relating to transition coverage (aka edge- or "branch"-coverage)
	if( (transition_coverage_defined = ctx.start_of_covering_transitions_valid()) ){
	   for(std::size_t i = 0;i != ctx.coverage_transitions_table.size();++i){
		   if (ctx.transitions[ctx.start_of_covering_transitions + i].start()) continue;
		   auto from_state = ctx.transitions[ctx.start_of_covering_transitions + i].from;
		   auto to_state = ctx.transitions[ctx.start_of_covering_transitions + i].to;

           if (
               ctx.get_inf(from_state,executionloop_context_t::DONT_COVER) ||
               ctx.get_inf(from_state,executionloop_context_t::INIT) ||
		   	   ctx.get_inf(from_state,executionloop_context_t::FINAL) ||
		   	   ctx.get_inf(from_state,executionloop_context_t::SM) ) continue;

           if (
               ctx.get_inf(to_state,executionloop_context_t::DONT_COVER) ||
               ctx.get_inf(to_state,executionloop_context_t::INIT) ||
		   	   ctx.get_inf(to_state,executionloop_context_t::FINAL) ||
		   	   ctx.get_inf(to_state,executionloop_context_t::SM) ) continue;
           if (to_state == from_state && ctx.get_inf(ctx.get_parent(from_state),executionloop_context_t::DONT_COVER_LOOPS))
               continue;
           //std::cerr << from_state << "=>" << to_state << std::endl;
		   auto sms = ctx.get_assoc_sm(ctx.transitions[ctx.start_of_covering_transitions + i].root_sms);
		   assert(sms != nullptr);
		   number_of_transitions_covered += ctx.coverage_transitions_table[i] != 0;
		   if (ctx.coverage_transitions_table[i]) {
			   transition_coverage_list.push_back(ctx.start_of_covering_transitions + i);
			   ++sm_transitions_covered[sms];
		   }
		   else {
			   transition_coverage_missing_list.push_back(ctx.start_of_covering_transitions + i);
               ++sm_transitions_not_covered[sms];
		   }
		   ++number_of_transitions_to_cover;
	   }
	}

	for (auto sm : statemachines()){
		if (sm.second->parent() != nullptr) continue;
        if (0 == sm_transitions_covered[sm.second] + sm_transitions_not_covered[sm.second]) continue;
		double ratio = (double)sm_transitions_covered[sm.second] / (double)(sm_transitions_covered[sm.second] + sm_transitions_not_covered[sm.second]);

		toplevel_state_machines_transition_coverage_stats.push_back(
		 (new strct{sm.first,ratio })->p_strct
		);
	}

	transition_coverage_list_ceps_expr = mk_sm_state_transition_exprs(this,transition_coverage_list);
	transition_coverage_missing_list_ceps_expr= mk_sm_state_transition_exprs(this,transition_coverage_missing_list);

	state_coverage = (double)number_of_states_covered / (double)number_of_states_to_cover;
    transition_coverage = (double) number_of_transitions_covered / (double) number_of_transitions_to_cover;

    //enter- / exit- times

    for(std::size_t i = 0;i != executionloop_context().inf_vec.size();++i){
        auto log_time = [&](executionloop_context_t::exit_times_t const & m,std::vector<ceps::ast::Nodebase_ptr> & d){
            auto it = m.find(i);
            if (it != m.end()){
                ceps::ast::Struct* t = new ceps::ast::Struct{"timestamp_str"
                };
                t->children().push_back(
                new ceps::ast::Struct{"localtime_with_ms_accuracy",
                  new ceps::ast::String{gen_timestamp(it->second)}
                });
                t->children().push_back(

                new ceps::ast::Struct{"gmtime_with_ms_accuracy",
                  new ceps::ast::String{gen_gmt_timestamp(it->second)}
                });
                t->children().push_back(

                new ceps::ast::Struct{"localtime",
                  new ceps::ast::String{gen_timestamp(it->second,false)}
                });

                t->children().push_back(

                new ceps::ast::Struct{"gmtime",
                  new ceps::ast::String{gen_gmt_timestamp(it->second,false)}
                });

                d.push_back(
                   new ceps::ast::Struct{"entry",
                                         new ceps::ast::Struct{"id_int",
                                           new ceps::ast::Int{(int)i,ceps::ast::all_zero_unit()}
                                         },
                                         t,
                                         new ceps::ast::Struct{"timestamp",
                                           new ceps::ast::Struct{"utc_secs_since_epoch",
                                             new ceps::ast::Int{get_secs(it->second),ceps::ast::all_zero_unit()}
                                           },
                                           new ceps::ast::Struct{"ms_mod_1000",
                                             new ceps::ast::Int{get_ms_mod_1000(it->second),ceps::ast::all_zero_unit()}
                                           }
                                         }
                            }
                );
            }
        };
        if (ctx.get_inf(i,executionloop_context_t::LOG_ENTER_TIME)) {
            log_time(ctx.enter_times,enter_times);
        }
        if (ctx.get_inf(i,executionloop_context_t::LOG_EXIT_TIME)) {
            log_time(ctx.exit_times,exit_times);
        }
    }

    auto summary =
     new strct{ "summary",
	 	  *ceps::ast::mk_symbol("coverage_summary","@@coverage_summary"),
    	  strct{"general",
    	   strct{"states_total",ctx.number_of_states},
		   strct{"total_of_states_to_cover",number_of_states_to_cover},
		   strct{"total_of_states_covered",number_of_states_covered}
          },
          strct{"log",
             strct{"enter_times",enter_times},
             strct{"exit_times",exit_times},
          },
		  strct{"coverage",
		   strct{"state_coverage",
		    strct{"valid",state_coverage_defined},
		    strct{"ratio",state_coverage},
            strct{"percentage",state_coverage*100.0},
		    strct{"covered_states",state_coverage_states_list_ceps_expr},
		    strct{"covered_states_visit_count",states_hit_counter},
		    strct{"not_covered_states",state_coverage_missing_states_list_ceps_expr},
            strct{"toplevel_state_machines",toplevel_state_machines_state_coverage_stats}
		   },
		   strct{"transition_coverage",
		    strct{"valid",transition_coverage_defined},
		    strct{"ratio",transition_coverage},
            strct{"percentage",transition_coverage*100.0},
		    strct{"covered_transitions",transition_coverage_list_ceps_expr},
            strct{"not_covered_transitions",transition_coverage_missing_list_ceps_expr},
			strct{"covered_transitions_by_id",transition_coverage_list},
            strct{"not_covered_transitions_by_id",transition_coverage_missing_list},
            strct{"toplevel_state_machines",toplevel_state_machines_transition_coverage_stats}
		 }
	    }
	};
	result.nodes().push_back(summary->get_root());
    if (do_cat){
        auto cat_stat = new ceps::ast::Struct{"categories"};
        for(auto e : statistics_category){
            auto elem = new ceps::ast::Struct{e.first, new ceps::ast::Int{e.second,ceps::ast::all_zero_unit()}};
            cat_stat->children().push_back(elem);
        }
        //std::cout << *cat_stat << std::endl;
        summary->get_root()->children().push_back(cat_stat);
    }
	return result;
}

void State_machine_simulation_core::print_report(Result_process_cmd_line const& result_cmd_line,
						ceps::Ceps_Environment& ceps_env,
						ceps::ast::Nodeset& universe){
 ceps::ast::Nodeset report = make_report();
 current_universe().nodes().insert(current_universe().nodes().end(), report.nodes().begin(),report.nodes().end());
 std::ostream& os = std::cout;
 ::print_report(os,report,result_cmd_line);
}


ceps::ast::Nodebase_ptr State_machine_simulation_core::lookup(lookup_table_t& t,ceps::ast::Nodebase_ptr p){
 if (p == nullptr) return nullptr;
 if (p->kind() == ceps::ast::Ast_node_kind::identifier){
	 // id -> x
	 std::string const & s = ceps::ast::name(ceps::ast::as_id_ref(p));
	 for(auto e : t){
       if (e.first->kind() != ceps::ast::Ast_node_kind::identifier) fatal_(-1,"Illformed lookup table.");
       if (ceps::ast::name(ceps::ast::as_id_ref(e.first)) == s) return e.second;
	 }
	 return nullptr;
 }
 else if (p->kind() == ceps::ast::Ast_node_kind::string_literal){
	 // string -> x
 }
 else if (p->kind() == ceps::ast::Ast_node_kind::int_literal){
	 // id -> x
 }
 fatal_(-1,"Unsupported type of lookup table.");
 return nullptr;
}






















