#include <algorithm>

const auto SM4CEPS_VER_MAJ = 0;
const auto SM4CEPS_VER_MINOR = .600;

int get_sm4ceps_ver_maj() { return SM4CEPS_VER_MAJ; }
double get_sm4ceps_ver_minor() { return SM4CEPS_VER_MINOR; }
int odd(int i) { return i % 2; }
int even(int i) { return !odd(i); }
double truncate(double i) { return (int)i; }
double mymin(double a, double b) { return std::min(a,b); }
double mymin(double a, int b) { return std::min(a, (double)b); }
double mymin(int a, double b) { return std::min((double)a, b); }
int mymin(int a, int b) { return std::min(a, b); }

#define _CRT_SECURE_NO_WARNINGS

#include "core/include/state_machine_simulation_core.hpp"
#include "core/include/sm_comm_naive_msg_prot.hpp"
#include "core/include/sm_ev_comm_layer.hpp"
#include "core/include/serialization.hpp"
#include "core/include/sm_raw_frame.hpp"
#include "core/include/sm_xml_frame.hpp"
#include "pugixml.hpp"
#ifdef __gnu_linux__

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <dlfcn.h>

#else

#endif
#include <sys/types.h>
#include <limits>
#include <cstring>

#include "core/include/base_defs.hpp"
int State_machine_simulation_core::SM_COUNTER = 0;

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

void comm_sender_generic_tcp_out_thread(threadsafe_queue< std::pair<char*,size_t>, std::queue<std::pair<char*,size_t> >>* frames,
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

void State_machine_simulation_core::process_files(	std::vector<std::string> const & file_names,
						std::string& last_file_processed)
{
	for(auto const & file_name : file_names)
	{

		std::fstream def_file{ last_file_processed = file_name};
		if (!def_file) fatal_(ERR_FILE_OPEN_FAILED,file_name);

		Ceps_parser_driver driver{ceps_env_current().get_global_symboltable(),def_file};
		ceps::Cepsparser parser{driver};

		if (parser.parse() != 0 || driver.errors_occured())
			fatal_(ERR_CEPS_PARSER, file_name);

		std::vector<ceps::ast::Nodebase_ptr> generated_nodes;
		ceps::interpreter::evaluate(current_universe(),
									driver.parsetree().get_root(),
									ceps_env_current().get_global_symboltable(),
									ceps_env_current().interpreter_env(),
									&generated_nodes
									);
	}//for
}//process_def_files


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

void State_machine_simulation_core::process_statemachine_helper_handle_transitions(
					State_machine* current_statemachine,
					std::vector<State_machine::Transition>& trans,
					std::string id,
					ceps::ast::Nodeset& transitions,
					int& guard_ctr)
{
 using namespace std;
 DEBUG_FUNC_PROLOGUE

 int t_ctr=0;

 for (auto transition_ : transitions )
  {
	++t_ctr;
    auto transition = transition_["Transition"];
    if (transition.nodes().size() < 2)
     fatal_(-1,"Statemachine '"+id+"' not enough parameters: Transitions contain at least two elements (source- and target-nodes).");


    auto from_state_orig = transition.nodes()[0];
    auto to_state_orig = transition.nodes()[1];
    State_machine::State from,to;

    if (from_state_orig->kind() == ceps::ast::Ast_node_kind::identifier && to_state_orig->kind() == ceps::ast::Ast_node_kind::identifier){

        string s1 = name(as_id_ref(transition.nodes()[0]));
        string s2 = name(as_id_ref(transition.nodes()[1]));
        DEBUG << "[INSERT LOCAL TRANSITION]" <<  s1 <<"==>"<<s2 << "\n";
        from.id_ = s1; to.id_=s2;
        if (!current_statemachine->lookup(from))
        	fatal_(-1,"Statemachine '"+id+"': "+ s1+" is not a state (or statemachine).");
        if (!current_statemachine->lookup(to))
        	fatal_(-1,"Statemachine '"+id+"': "+ s2+" is not a state (or statemachine).");
    } else
    {
       	std::vector<std::string> q_from_id;std::vector<std::string> q_to_id;
    	bool r = read_qualified_id(from_state_orig,q_from_id );
    	if (!r) fatal_(-1,"State machine '"+ id +"', transition #" + std::to_string(t_ctr) + ": illformed origin state");
    	r = read_qualified_id(to_state_orig,q_to_id );
    	if (!r) fatal_(-1,"State machine '"+ id +"', transition #" + std::to_string(t_ctr) + ": illformed destination state");
    	//std::cerr << "*******\n";debug_print_qualified_id(q_from_id); std::cerr << "\n"; debug_print_qualified_id(q_to_id);std::cerr << "\n";

    	DEBUG << "[INSERT TRANSITION WITH AT LEAST ONE FULLY QUALIFID STATE]" << qualified_id_to_str(q_from_id) <<"==>"<< qualified_id_to_str(q_to_id) << "\n";
    	from = State_machine::State(q_from_id);
    	to = State_machine::State(q_to_id);
    }
    trans.push_back(State_machine::Transition(from,to));

    for(size_t h = 2; h < transition.nodes().size();++h)
    {
      auto node = transition.nodes()[h];
      if (node->kind() == ceps::ast::Ast_node_kind::symbol)
      {
    	  auto  & symbol = ceps::ast::as_symbol_ref(node);
    	  if (kind(symbol) != "Guard" && kind(symbol) != "Event")
    		  fatal_(-1,"State machine '"+ id +"', transition #" + std::to_string(t_ctr) + ": Guards/Events must be of kind 'Guard'/'Event' not '"+kind(symbol)+"'.");
    	  if (kind(symbol) == "Guard") trans[trans.size()-1].guard() = name(symbol);
    	  else trans[trans.size()-1].events().insert(name(symbol));
    	  continue;
      }

      if (node->kind() == ceps::ast::Ast_node_kind::binary_operator || node->kind() == ceps::ast::Ast_node_kind::unary_operator ||
    		  node->kind() == ceps::ast::Ast_node_kind::int_literal || node->kind() == ceps::ast::Ast_node_kind::string_literal ||
    		  node->kind() == ceps::ast::Ast_node_kind::float_literal || node->kind() == ceps::ast::Ast_node_kind::func_call)
      {

    	  std::string g = gen_guard_id(id,guard_ctr++);
    	  trans[trans.size()-1].guard() = g;
    	  global_guards[g] = node;
      }

      if (node->kind() != ceps::ast::Ast_node_kind::identifier)
    	  continue;


      string cur_id = name(as_id_ref(transition.nodes()[h]));


      State_machine::Transition::Event ev(cur_id);
      State_machine::Transition::Action ac(cur_id);
      bool whether_action_nor_event = true;
      if (current_statemachine->lookup_info(ev,current_statemachine->events()))
      {
       trans[trans.size()-1].events().insert(ev);whether_action_nor_event = false;
      }
      if (current_statemachine->lookup_info(ac,current_statemachine->actions()))
      {
    	trans[trans.size()-1].actions().push_back(ac);whether_action_nor_event = false;
      }
      if(whether_action_nor_event) fatal_(-1,"Transition definition in state machine:'"+id+"': '"+cur_id+"' is neither an event nor an action");
    }
  }
}



void State_machine_simulation_core::process_statemachine(	ceps::ast::Nodeset& sm_definition,
							std::string prefix,
							State_machine* parent,
							int depth,
							int thread_ctr,
							bool is_thread)
{
  using namespace std;
  using namespace ceps::ast;

  DEBUG_FUNC_PROLOGUE

  auto states = sm_definition["States"];
  auto transitions = sm_definition[all{"Transition"}];
  auto events = sm_definition[all{"Events"}];
  auto actions = sm_definition[all{"Actions"}];
  auto threads = sm_definition[all{"thread"}];
  auto imports = sm_definition[all{"import"}];
  auto join = sm_definition[all{"join"}];
  auto on_enter = sm_definition[all{"on_enter"}];
  auto on_exit = sm_definition[all{"on_exit"}];

  int anonymous_guard_ctr = 1;


  std::string id;
  std::string sm_name;
  auto id_ = sm_definition["id"];
  if (id_.size() == 0)
  {
	  if (!is_thread)
	  {
		  log() << sm_definition << "\n";
		  fatal_(-1,"Statemachine definition: no id found.");
	  } else {
		  sm_name = "thread_"+std::to_string(thread_ctr);
		  id = prefix + sm_name;
	  }
  } else {
	  if (id_.nodes()[0]->kind() != ceps::ast::Ast_node_kind::identifier)
		  fatal_(-1,"Statemachine definition: illformed id, expect an IDENTIFIER.");
	  sm_name = name(as_id_ref(id_.nodes()[0])); id = prefix+name(as_id_ref(id_.nodes()[0]));
  }

  DEBUG << "[PROCESSING CONTENT OF SM="<< id << "]\n";


  auto& current_statemachine = State_machine::statemachines[id];
  if (current_statemachine == nullptr)
  {
	  State_machine::statemachines[id] = current_statemachine = new State_machine(SM_COUNTER++,sm_name,parent,depth);
	  set_qualified_id(current_statemachine,id);
  }

  current_statemachine->is_thread() = is_thread;
  if (is_thread && parent) parent->contains_threads() = true;
  if (parent != nullptr) parent->add_child(current_statemachine);

  //INVARIANT: current_statemachine non null points to correct machine

  DEBUG <<"[PROCESS SM("<< id <<")]"<< "[HANDLING_IMPORTS]\n";

  std::set<std::string> unresolved_imports_set;
  for(auto it =  current_statemachine->unresolved_imports().begin(); it != current_statemachine->unresolved_imports().end(); ++it)
	  unresolved_imports_set.insert(get<1>(*it));

  current_statemachine->definition_complete() = imports.size() == 0;

  for(auto imp_ : imports)
  {
	  auto imp = imp_["import"];
	  for(auto child: imp.nodes())
	  {
		  if (child->kind() != ceps::ast::Ast_node_kind::identifier)
		   { warn_(-1,"State Machine '"+id+"': Import sections should contain identifiers only. Read something other, will be ignored"); continue;}

		  State_machine* ptr = nullptr;
		  std::string alias;
		  std::string sm_name;

		  sm_name = ceps::ast::name(ceps::ast::as_id_ref(child));
		  alias = sm_name;
		  if(unresolved_imports_set.find(alias) != unresolved_imports_set.end())
		  {
			  warn_(-1,"State Machine '"+id+"' Duplicate import: '"+alias+"' already included");
			  continue;
		  }
		  unresolved_imports_set.insert(alias);
		  current_statemachine->unresolved_imports().push_back(
				  State_machine::unresolve_import_t( sm_name,alias,ptr = new State_machine(SM_COUNTER++,alias,current_statemachine,depth) ) );
		  ptr->definition_complete() = false;
		  current_statemachine->add_child(ptr);

		  State_machine::statemachines[id+"."+alias] = ptr;
		  set_qualified_id(ptr,id+"."+alias);
	  }
  }

  auto statemachines = sm_definition[all{"Statemachine"}];

  for (auto sm_ : statemachines)
  {
	  auto sm = sm_["Statemachine"];
	  process_statemachine(sm,id+".",current_statemachine,depth+1,0);
  }//for
  {
	  int thread_ctr_local = 1;
	  for (auto sm_ : threads)
	  {
		  auto sm = sm_["thread"];
		  process_statemachine(sm,id+".",current_statemachine,depth+1,thread_ctr_local,true);++thread_ctr_local;
	  }//for
  }


  for (auto event_ : events )
  {
    auto event = event_["Events"];
    for(auto & v : event.nodes()) current_statemachine->events().insert(State_machine::Transition::Event(name(as_id_ref(v))));

  }
  for (auto action_ : actions )
  {
    auto action = action_["Actions"];
    for(auto & v : action.nodes())
    	{

    	   if (v->kind() == ceps::ast::Ast_node_kind::identifier)
    		   current_statemachine->actions().insert(State_machine::Transition::Action(name(as_id_ref(v))));
    	   else if (v->kind() == ceps::ast::Ast_node_kind::structdef)
    		   current_statemachine->actions().insert(State_machine::Transition::Action(name(as_struct_ref(v)),v));
    	   else {
    		   std::stringstream ss;
    		   ss << *v;
    		   fatal_(-1,"State machine '"+ id +"': Illformed Action declaration. Should be of the form IDENT; or IDENT{...};. Found:"+ss.str());
    	   }
    	}
  }

  if (on_enter.size() > 1) fatal_(-1,"Statemachine '"+id+"': not more than one 'on_enter' allowed.");
  if (on_exit.size() > 1) fatal_(-1,"Statemachine '"+id+"': not more than one 'on_exit' allowed.");

  if (on_enter.size())
	  current_statemachine->actions().insert(State_machine::Transition::Action(current_statemachine,name(as_struct_ref(on_enter.nodes()[0])),on_enter.nodes()[0]));
  if (on_exit.size())
 	  current_statemachine->actions().insert(State_machine::Transition::Action(current_statemachine,name(as_struct_ref(on_exit.nodes()[0])),on_exit.nodes()[0]));



  for (auto state : states)
  {
	 if (state.nodes().size() != 1 || state.nodes()[0]->kind() != ceps::ast::Ast_node_kind::identifier)
		 fatal_(-1,"State machine '"+id+"': Definition of states: Illformed expression: expected an unbound identifier, got: "
				 + ceps::ast::ast_node_kind_to_text[(int)state.nodes()[0]->kind()] );
     current_statemachine->insert_state( State_machine::State( name(as_id_ref(state.nodes()[0])) ) );
  }
  process_statemachine_helper_handle_transitions(current_statemachine,current_statemachine->transitions(),id,transitions,anonymous_guard_ctr);

  if (join.size())
  {
	  if(join.size() != 1)
		  fatal_ (-1,"State machine '"+id+"': Only one join allowed.\n");
	  auto join_state_ = join["join"];
	  if(join_state_.size() != 1)
	  		  fatal_ (-1,"State machine '"+id+"': illformed join statement: expected a single (qualified) state.\n");
	  auto join_state = join_state_.nodes()[0];
	  if (join_state->kind() == ceps::ast::Ast_node_kind::identifier ){
        string s1 = name(as_id_ref(join_state));
        DEBUG << "[INSERT LOCAL JOIN]" <<  s1 << "\n";
        current_statemachine->join() = true;
        current_statemachine->join_state().id_ = s1;
        if (!current_statemachine->lookup(current_statemachine->join_state()))
       	 fatal_(-1,"Statemachine '"+id+"': '"+ s1+"' is not a state/state machine.");
	  } else {
        std::vector<std::string> q_join_state_id;

	    bool r = read_qualified_id(join_state,q_join_state_id );
	    if (!r) fatal_(-1,"State machine '"+ id +"', join: illformed state id.\n");
	   	DEBUG << "[INSERT GLOBAL JOIN]" << qualified_id_to_str(q_join_state_id) << "\n";
	    current_statemachine->join() = true;
	    current_statemachine->join_state() = State_machine::State(q_join_state_id);
	  }

  }
}

bool resolve_imports(State_machine & sm, void* context)
{
	auto THIS = static_cast<State_machine_simulation_core*>(context);
	DEBUG_FUNC_PROLOGUE2
	for (auto & sub_machine : sm.children_)
	{

		DEBUG << "[PROCESS_SUB_MACHINE_START][(REL)ID = "<<  sub_machine->id_ << "]\n";
		if (sub_machine->definition_complete()) continue;
		DEBUG << "[SUBMACHINE_NOT_COMPLETE][(REL)ID = "<<  sub_machine->id_ << "]\n";

		State_machine::unresolve_import_t import_clause;
		bool bfound = false;
		for(auto & e : sm.unresolved_imports())
		{
			if (std::get<2>(e) == sub_machine){import_clause=e;bfound = true;break;}
		}
		if(!bfound) {DEBUG << "[PROCESS_SUB_MACHINE_END_IMPORT_CLAUSE_NOT_MATCHED][(REL)ID = "<<  sub_machine->id_ << "]\n";return false;}

		auto template_sm = State_machine::statemachines[std::get<1>(import_clause)];
		if (template_sm == nullptr) {
			THIS->fatal_(-1,"State machine '"+sm.id()+"': import: state machine '"+std::get<1>(import_clause)+"' not found.");
			return false;
		}
		if(template_sm->definition_complete())
		{

		} else resolve_imports(*template_sm,context);
		DEBUG << "[CLONE_FROM(1)]\n";
		auto rr = THIS->get_qualified_id(sub_machine);
		assert(rr.first);
		sub_machine->clone_from(template_sm,State_machine_simulation_core::SM_COUNTER,rr.second,resolve_imports,context);
		sub_machine->definition_complete() = true;
		DEBUG << "[CLONE_FROM(2)]\n";
		DEBUG << "[PROCESS_SUB_MACHINE_END][(REL)ID = "<<  sub_machine->id_ << "]\n";
	}
	sm.definition_complete() = true;

	return true;
}

bool State_machine_simulation_core::resolve_imports(State_machine & sm)
{
	DEBUG_FUNC_PROLOGUE
	return ::resolve_imports(sm,this);
}

bool State_machine_simulation_core::resolve_q_id(State_machine* smp,
		std::vector<std::string> const & q_id, State_machine::State & s)
{

	DEBUG_FUNC_PROLOGUE
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
	auto it = current_parent->states.find(State_machine::State(last_step));
	if (it != current_parent->states.end())
	{
		s = *it;
		s.parent_ = current_parent;
		s.unresolved_ = false;
		return true;
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
	return get_fullqualified_id(sr);
}


void State_machine_simulation_core::processs_content(State_machine **entry_machine)
{
	using namespace ceps::ast;
	using namespace std;
	DEBUG_FUNC_PROLOGUE;

	regfn("sm4ceps_version_major",get_sm4ceps_ver_maj);
	regfn("sm4ceps_version_minor", get_sm4ceps_ver_minor);
	regfn("odd", odd);
	regfn("even", even);
	regfn("truncate",truncate);
	regfn("min", static_cast<double(*)(double,double)> (mymin));
	regfn("min", static_cast<double(*)(int, double)> (mymin));
	regfn("min", static_cast<double(*)(double, int)> (mymin));
	regfn("min", static_cast<int(*)(int, int)> (mymin));

	ceps::ast::Nodeset ns = current_universe();

	auto statemachines = ns[all{"Statemachine"}];
	auto globalfunctions = ns["global_functions"];
	auto frames = ns[all{"raw_frame"}];
	auto xmlframes = ns[all{"xml_frame"}];
	auto all_sender = ns[all{"sender"}];
	auto all_receiver = ns[all{"receiver"}];

	post_event_processing() = ns["post_event_processing"];

	auto can_frames = ns[all{"frame"}];
	frames.nodes().insert(frames.nodes().end(),can_frames.nodes().begin(),can_frames.nodes().end());


	for(auto p:globalfunctions.nodes())
	{
		if(p->kind() != ceps::ast::Ast_node_kind::structdef) continue;
		std::string id = ceps::ast::name(ceps::ast::as_struct_ref(p));
		global_funcs()[id] = p;
	}

	if (statemachines.empty())
	 if(warn_) warn_(WARN_NO_STATEMACHINES,"");

	for (auto statemachine_ : statemachines)
	{
		auto statemachine = statemachine_["Statemachine"];
		process_statemachine(statemachine,"",nullptr,1,0);
	}//for
	auto entry_ = ns["main"];

	if (!entry_.empty() && entry_machine != nullptr) {
		string entry_name = name(as_id_ref(entry_.nodes()[0]));
		*entry_machine = State_machine::statemachines[entry_name ];
		if (entry_machine == nullptr)
		  fatal_(-1,"No statemachine with id "+entry_name+" found: No main statemachine defined.");
	}


	//BACK PATCH IMPORTS

	for(;;)
	{

		bool imports_resolved = false;
		auto s_t = State_machine::statemachines;
		for(auto & sim_ : s_t)
		{
			DEBUG << "[RESOLVE_IMPORTS][ID =  " << sim_.second->id_ << "]\n";
			auto & sim = sim_.second;
			if (sim->definition_complete() || sim->unresolved_imports().size() == 0) continue;

			if (!resolve_imports(*sim))
				{
				 fatal_(-1,sim->id()+": Error while resolving imports");
				}
			imports_resolved = true;
		}
		if (!imports_resolved) break;
	}

	DEBUG << "[BACK PATCH JOINS]\n";

	for(auto & sim_ : State_machine::statemachines)
	{
		if (!sim_.second->join()) continue;
		if (!sim_.second->join_state().unresolved()) continue;

		DEBUG << "[BACK PATCH JOIN STATE][SM=" << sim_.first <<"]\n";
		State_machine::State s;
		resolve_q_id(sim_.second, sim_.second->join_state().q_id(), s);
		sim_.second->join_state() = s;

	}


	DEBUG << "[BACK PATCH TRANSITIONS]\n";


	//STEP#1 resolve transitions
	DEBUG << "[BACK PATCH TRANSITIONS][STEP(1):RESOLVE_TRANSITIONS]\n";
	std::set<State_machine*> machines_with_transitions_which_may_have_foreign_source;
	for(auto & sim_ : State_machine::statemachines)
	{
		for(size_t i = 0; i < sim_.second->transitions().size(); ++i)
		{
			auto & t = sim_.second->transitions()[i];
			if (t.from_.unresolved()) {
				DEBUG << "[RESOLVE FROM]\n";
				State_machine::State s;
				resolve_q_id(sim_.second, t.from_.q_id(), s);
				auto orig_q_id = t.from_.q_id();
				t.from_ = s;
				assert(t.from_.is_sm_ || t.from_.smp_ != nullptr);
				assert(t.from_.id_.size());
				assert(!t.from_.unresolved());

				if (print_debug_info_){
					state_rep_t sr,sr2;
					sr.valid_ = sr2.valid_ = true;
					sr.is_sm_ = s.is_sm_;
					sr.sid_ = s.id_;
					sr.smp_ = s.smp_;
					sr2.is_sm_ = true;
					sr2.sid_ = sim_.second->id_;
					sr2.smp_ = sim_.second;
					DEBUG << "[RESOLVE Q-ID STATE][SM=" << get_fullqualified_id(sr2) << "]"
						  <<"[" << qualified_id_to_str(orig_q_id)<< " ==> " << get_fullqualified_id(sr) << "]\n";
				}

				if (t.from_.parent() == sim_.second) t.from_.parent_ = nullptr;
				machines_with_transitions_which_may_have_foreign_source.insert(sim_.second);

			}
			if (t.to_.unresolved()){
				DEBUG << "[RESOLVE TO]\n";
				State_machine::State s;
				resolve_q_id(sim_.second, t.to_.q_id(), s);
				auto orig_q_id = t.to_.q_id();
				t.to_ = s;
				assert(t.to_.is_sm_ || t.to_.smp_ != nullptr);
				assert(t.to_.id_.size());
				assert(!t.to_.unresolved());
				if (print_debug_info_){
					state_rep_t sr,sr2;
					sr.valid_ = sr2.valid_ = true;
					sr.is_sm_ = s.is_sm_;
					sr.sid_ = s.id_;
					sr.smp_ = s.smp_;
					sr2.is_sm_ = true;
					sr2.sid_ = sim_.second->id_;
					sr2.smp_ = sim_.second;
					DEBUG << "[RESOLVE Q-ID STATE][SM=" << get_fullqualified_id(sr2) << "][" << qualified_id_to_str(orig_q_id)<< " ==> " << get_fullqualified_id(sr) << "]\n";
				}
			}
		}//for
	}//for


	DEBUG << "[BACK PATCH TRANSITIONS][STEP(1):MOVE_FOREIGN_TRANSITIONS]\n";
	for(auto sm :  machines_with_transitions_which_may_have_foreign_source)
	{
		auto it = std::stable_partition(sm->transitions().begin(),sm->transitions().end(),
				[&] (State_machine::Transition const & t) {
					return !t.from_.is_foreign(sm);
					}
				);
		if (it == sm->transitions().end()) continue;

		std::for_each(it,sm->transitions().end(),
				[&](State_machine::Transition & t) {
					DEBUG << "[TRANSITION_MOVE(PRE)][FROM="<< get_full_qualified_id(t.from_) << "][TO=" << get_full_qualified_id(t.to_) << "]\n";
					auto parent = t.from_.parent();t.from_.parent_=nullptr;t.from_.q_id_.clear();
					if(!t.from_.is_sm_)t.from_.smp_ = parent;
					assert(sm != parent);
					parent->transitions().push_back(t);
					DEBUG << "[TRANSITION_MOVE(POST)][FROM="<< get_full_qualified_id(t.from_) << "][TO=" << get_full_qualified_id(t.to_) << "]\n";

				}
		);
		sm->transitions().erase(it,sm->transitions().end());
	}

	for (auto m: State_machine::statemachines)
	{
		for(auto& t: m.second->transitions())
		{
			assert(!t.from().unresolved());
			assert(!t.to().unresolved());
			assert(t.from().id_.size() > 0);
			assert(t.to().id_.size() > 0);
		}
	}

	DEBUG << "[PROCESSING TYPE DEFINITIONS][START]\n";

	auto typedefs = ns["typedef"];
	if (typedefs.size())
		for(auto node: typedefs.nodes())
		{
			if(node->kind() != ceps::ast::Ast_node_kind::structdef) continue;
			auto td = ceps::ast::as_struct_ptr(node);
			type_definitions()[ceps::ast::name(*td)] = td;
			DEBUG << "[PROCESSING TYPE DEFINITIONS][TYPEDEF] "<< ceps::ast::name(*td) <<"\n";
		}
	DEBUG << "[PROCESSING TYPE DEFINITIONS][END]\n";

	//Handle CALL frames

	for(auto rawframe_ : frames)
	{
		auto rawframe = rawframe_["raw_frame"];
		if(!rawframe.size()) rawframe = rawframe_["frame"];

		auto gen = new Podframe_generator;
		if (rawframe["id"].size() != 1)
			fatal_(-1,"raw_frame definition: missing id.");
		if (rawframe["id"].nodes()[0]->kind() != ceps::ast::Ast_node_kind::identifier)
			fatal_(-1,"raw_frame definition: id must be an unbound identifier.");

		std::string id = ceps::ast::name(ceps::ast::as_id_ref(rawframe["id"].nodes()[0]));
		DEBUG << "[PROCESSING RAW_FRAME_SPEC("<< id <<")][START]\n";
		gen->readfrom_spec(rawframe);
		frame_generators()[id] = gen;
		DEBUG << "[PROCESSING RAW_FRAME_SPEC][FINISHED]\n";
	}

	for(auto xmlframe_ : xmlframes)
	{
		auto xmlframe = xmlframe_["xml_frame"];
		auto gen = new Xmlframe_generator;
		if (xmlframe["id"].size() != 1)
			fatal_(-1,"xml_frame definition: missing id.");
		if (xmlframe["id"].nodes()[0]->kind() != ceps::ast::Ast_node_kind::identifier)
			fatal_(-1,"xml_frame definition: id must be an unbound identifier.");

		std::string id = ceps::ast::name(ceps::ast::as_id_ref(xmlframe["id"].nodes()[0]));
		DEBUG << "[PROCESSING XML_FRAME_SPEC("<< id <<")][START]\n";
		gen->readfrom_spec(xmlframe);
		frame_generators()[id] = gen;
		DEBUG << "[PROCESSING XML_FRAME_SPEC][FINISHED]\n";
	}

	//Handle CALL sender
	for(auto sender_ : all_sender)
	{
		DEBUG << "[PROCESSING SENDER]\n";
		auto sender = sender_["sender"];
		auto when = sender["when"];
		auto emit = sender["emit"];
		auto transport  = sender["transport"];

		if (transport["generic_tcp_out"].empty()) {
			//Handle user defined transport layers
			std::string call_name="(NULL)";
			if (transport.nodes().size() && transport.nodes()[0]->kind() == ceps::ast::Ast_node_kind::structdef)
				call_name = ceps::ast::name(ceps::ast::as_struct_ref(transport.nodes()[0]));
			auto r = handle_userdefined_sender_definition(call_name,sender);
			
			if (!r) 
				fatal_(-1, "Sender definition: '"+call_name+"' unknown CAL-identifier (Communication Abstraction Layer).\n");
			continue;
		}

		std::string sock_name;
		std::string port;
		std::string ip;
		std::string channel_id;
		bool reuse_sock = false;
		bool reg_socket = false;

		//std::cout << when << std::endl;
		bool condition_defined = true;
		bool emit_defined = true;

		if (when.size() != 1 || when.nodes()[0]->kind() != ceps::ast::Ast_node_kind::symbol || "Event" != ceps::ast::kind(ceps::ast::as_symbol_ref( when.nodes()[0])) )
			condition_defined=false;
		if ( emit.size() != 1 || emit.nodes()[0]->kind() != ceps::ast::Ast_node_kind::identifier)
			emit_defined=false;

		if (transport["use"].size() )
		{
			if (transport["use"].nodes()[0]->kind() == ceps::ast::Ast_node_kind::identifier){
				reuse_sock = true;
				sock_name = ceps::ast::name(ceps::ast::as_id_ref(transport["use"].nodes()[0]));
			}
		} else {
			if (!transport["generic_tcp_out"]["port"].empty()) port = transport["generic_tcp_out"]["port"].as_str();
			if (!transport["generic_tcp_out"]["ip"].empty()) ip = transport["generic_tcp_out"]["ip"].as_str();
		}


        if (sender["id"].size()) {
        	auto p = sender["id"].nodes()[0];
        	if (p->kind() != ceps::ast::Ast_node_kind::identifier) fatal_(-1,"Id field in sender definition should contain a single id");
        	channel_id = sock_name = ceps::ast::name(ceps::ast::as_id_ref(p));
        	reg_socket = true;
        }


		std::string ev_id;
		if (condition_defined) ev_id = ceps::ast::name(ceps::ast::as_symbol_ref( when.nodes()[0]));
		std::string frame_id;
		if (emit_defined) frame_id = ceps::ast::name(ceps::ast::as_id_ref( emit.nodes()[0]));
		std::string eof="";
		std::string som="";

		if (transport["eom"].size()){
			auto  v = transport["eom"].nodes();
			//DOESN'T WORK!!!!!!: GCC BUG ??????: auto&  v = transport["generic_tcp_out"]["eof"].nodes();
			for(auto  e : v)
			{
				if (e->kind() == ceps::ast::Ast_node_kind::string_literal)
					eof.append(ceps::ast::value(ceps::ast::as_string_ref(e)));
				else if (e->kind() == ceps::ast::Ast_node_kind::int_literal && ceps::ast::value(ceps::ast::as_int_ref(e)) != 0)
					{eof.append(" "); eof[eof.length()-1] =  ceps::ast::value(ceps::ast::as_int_ref(e));}
			}
		}
		if (transport["som"].size()){
			auto  v = transport["som"].nodes();
			//DOESN'T WORK!!!!!!: GCC BUG ??????: auto&  v = transport["generic_tcp_out"]["eof"].nodes();
			for(auto  e : v)
			{
				if (e->kind() == ceps::ast::Ast_node_kind::string_literal)
					som.append(ceps::ast::value(ceps::ast::as_string_ref(e)));
				else if (e->kind() == ceps::ast::Ast_node_kind::int_literal && ceps::ast::value(ceps::ast::as_int_ref(e)) != 0)
					{som.append(" "); som[som.length()-1] =  ceps::ast::value(ceps::ast::as_int_ref(e));}
			}
		}



		if (condition_defined && emit_defined){
		 DEBUG << "[PROCESSING_EVENT_TRIGGERED_SENDER][ev_id="
			   << ev_id << "]"
			   <<"[frame_id="
			   << frame_id
			   <<"]"
			   <<"[ip="
			   << ip
			   << "]"
			   <<  "[port="
			   << port
			   <<"][eom='"<<eof<<"']"
			   <<"][som='"<<som<<"']"
			   <<"\n";

		 auto it = frame_generators().find(frame_id);
		 if (it == frame_generators().end()) fatal_(-1,"sender declaration: Unknown frame with id '"+frame_id+"'");
		 auto gen = it->second;
		 event_triggered_sender_t descr;
		 descr.event_id_ = ev_id;
		 descr.frame_gen_ = gen;
		 descr.frame_id_ = frame_id;
		 descr.frame_queue_ = new threadsafe_queue< std::pair<char*,size_t>, std::queue<std::pair<char*,size_t> >>;

		 comm_threads.push_back(new std::thread{comm_sender_generic_tcp_out_thread,descr.frame_queue_,this,ip,port,som,eof,sock_name,reuse_sock,reg_socket });
		 running_as_node() = true;

		 event_triggered_sender().push_back(descr);
		} else if (!condition_defined && !emit_defined && channel_id.length()){
			 DEBUG << "[PROCESSING_UNCONDITIONED_SENDER]"
				   <<"\n";
			 auto channel = new threadsafe_queue< std::pair<char*,size_t>, std::queue<std::pair<char*,size_t> >>;
			 this->set_out_channel(channel_id,channel);
			 running_as_node() = true;
			 comm_threads.push_back(
					 new std::thread{comm_sender_generic_tcp_out_thread,
				                     channel,
				                     this,
				                     ip,
				                     port,
				                     som,
				                     eof,
				                     sock_name,
				                     reuse_sock,
				                     reg_socket });
		} else {
            warn_(-1,"Sender definition incomplete, will be ignored.");
		}

	}
	//Handle CALL receiver
	for(auto receiver_ : all_receiver)
	{
		DEBUG << "[PROCESSING RECEIVER][START]\n";
		auto receiver = receiver_["receiver"];
		auto when = receiver["when"];
		auto emit = receiver["emit"];
		auto transport  = receiver["transport"];
		std::string port;
		std::string ip;
		bool reuse_sock=false;
		bool reg_sock = false;

		std::string sock_name;
		bool no_when_emit = false;

		if (transport["generic_tcp_in"].empty()) {
			//Handle user defined transport layers
			std::string call_name = "(NULL)";
			if (transport.nodes().size() && transport.nodes()[0]->kind() == ceps::ast::Ast_node_kind::structdef)
				call_name = ceps::ast::name(ceps::ast::as_struct_ref(transport.nodes()[0]));
			auto r = handle_userdefined_receiver_definition(call_name, receiver);

			if (!r)
				fatal_(-1, "Receiver definition: '" + call_name + "' unknown CAL-identifier (Communication Abstraction Layer).\n");
			continue;
		}


		if (when.empty() && emit.empty()) no_when_emit = true;
		else if (when.size() != 1 || when.nodes()[0]->kind() != ceps::ast::Ast_node_kind::identifier)
            {warn_(-1,"Illformed receiver definition."); continue;}

		if (transport["generic_tcp_in"].empty() && transport["use"].empty())
            {warn_(-1,"Illformed receiver definition."); continue;}

		if (!transport["use"].empty()){
			if (transport["use"].nodes()[0]->kind() == ceps::ast::Ast_node_kind::identifier){
					reuse_sock = true;
					sock_name = ceps::ast::name(ceps::ast::as_id_ref(transport["use"].nodes()[0]));
			} else fatal_(-1,"Receiver definition illformed");
		} else
		{
			if (!transport["generic_tcp_in"]["port"].empty()) port = transport["generic_tcp_in"]["port"].as_str();
			if (!transport["generic_tcp_in"]["ip"].empty()) ip = transport["generic_tcp_in"]["ip"].as_str();
		}

		std::string eof;
		std::string som;


		if (receiver["id"].size()) {
			auto p = receiver["id"].nodes()[0];
			if (p->kind() != ceps::ast::Ast_node_kind::identifier) fatal_(-1,"Id field in receiver definition should contain a single id");
			if (!reuse_sock){ sock_name = ceps::ast::name(ceps::ast::as_id_ref(p)); reg_sock = true; }
		}


		if (transport["eom"].size()) {
			//eof = transport["generic_tcp_in"]["eof"].as_str();
			auto  v = transport["eom"].nodes();
			//DOESN'T WORK!!!!!!: GCC BUG ??????: auto&  v = transport["generic_tcp_out"]["eof"].nodes();
			for(auto  e : v)
			{
				if (e->kind() == ceps::ast::Ast_node_kind::string_literal)
					eof.append(ceps::ast::value(ceps::ast::as_string_ref(e)));
				else if (e->kind() == ceps::ast::Ast_node_kind::int_literal && ceps::ast::value(ceps::ast::as_int_ref(e)) != 0)
					{eof.append(" "); eof[eof.length()-1] =  ceps::ast::value(ceps::ast::as_int_ref(e));}
			}
		}
		if (transport["som"].size()){
			auto  v = transport["som"].nodes();
			//DOESN'T WORK!!!!!!: GCC BUG ??????: auto&  v = transport["generic_tcp_out"]["eof"].nodes();
			for(auto  e : v)
			{
				if (e->kind() == ceps::ast::Ast_node_kind::string_literal)
					som.append(ceps::ast::value(ceps::ast::as_string_ref(e)));
				else if (e->kind() == ceps::ast::Ast_node_kind::int_literal && ceps::ast::value(ceps::ast::as_int_ref(e)) != 0)
					{som.append(" "); som[som.length()-1] =  ceps::ast::value(ceps::ast::as_int_ref(e));}
			}
		}

		std::string ev_id;
		std::vector<std::string> ev_params;

		if (emit.size() == 1 && emit.nodes()[0]->kind() == ceps::ast::Ast_node_kind::symbol && "Event" == ceps::ast::kind(ceps::ast::as_symbol_ref( emit.nodes()[0])) )
			ev_id = ceps::ast::name(ceps::ast::as_symbol_ref( emit.nodes()[0]));
		else if (emit.size() == 1 && emit.nodes()[0]->kind() == ceps::ast::Ast_node_kind::func_call)
		{
			std::vector<ceps::ast::Nodebase_ptr> args;
			read_func_call_values(this,	emit.nodes()[0],ev_id,args);
			for(auto p:args){
				if (p->kind() == ceps::ast::Ast_node_kind::identifier)
				{
					ev_params.push_back(ceps::ast::name(ceps::ast::as_id_ref(p)));
				}
			}
		}

		std::string frame_id;
		if (!when.nodes().empty()) frame_id = ceps::ast::name(ceps::ast::as_id_ref( when.nodes()[0]));
		if (!no_when_emit){
			DEBUG << "[PROCESSING EVENT_TRIGGERING_RECEIVER][ev_id="<< ev_id << "]"<<"[frame_id="<< frame_id <<"]" <<"[ip="<< ip << "]" <<  "[port=" << port <<"]" <<"\n";

			auto it = frame_generators().find(frame_id);

			if (it == frame_generators().end()) fatal_(-1,"receiver declaration: Unknown frame with id '"+frame_id+"'");

			auto gen = it->second;

			comm_threads.push_back(new std::thread{comm_generic_tcp_in_dispatcher_thread,
												   -1,
												   gen,
												   ev_id,
												   ev_params,
												   this,
												   ip,
												   port,
												   som,
												   eof,
												   sock_name,reg_sock,reuse_sock,
												   comm_generic_tcp_in_thread_fn });
			running_as_node() = true;
		} else {
			DEBUG << "[PROCESSING_UNCONDITIONED_RECEIVER]" <<"[ip="<< ip << "]" <<  "[port=" << port <<"]" <<"\n";
			auto handlers = receiver[all{"on_msg"}];
			if (handlers.empty()) fatal_(-1,"on_msg required in receiver definition.");

			int dispatcher_id=-1;
			auto& ctxt = allocate_dispatcher_thread_ctxt(dispatcher_id);
			DEBUG << "[PROCESSING_UNCONDITIONED_RECEIVER]" <<"[dispatcher_id="<< dispatcher_id << "]\n";

			for(auto const & handler_ : handlers){
				auto const & handler = handler_["on_msg"];
				auto frame_id_ = handler["frame_id"];
				auto handler_func_ = handler["handler"];

				if (frame_id_.size() != 1 || frame_id_.nodes()[0]->kind() != ceps::ast::Ast_node_kind::identifier)
					fatal_(-1,"Receiver definition illformed: frame_id not an identifier / wrong number of arguments.");
				if (handler_func_.size() != 1 || handler_func_.nodes()[0]->kind() != ceps::ast::Ast_node_kind::identifier)
					fatal_(-1,"Receiver definition illformed: handler not an identifier / wrong number of arguments.");

				auto frame_id = ceps::ast::name(ceps::ast::as_id_ref(frame_id_.nodes()[0]));
				auto handler_id = ceps::ast::name(ceps::ast::as_id_ref(handler_func_.nodes()[0]));
				auto it_frame = frame_generators().find(frame_id);
				if (it_frame == frame_generators().end()) fatal_(-1,"Receiver definition: on_msg : frame_id unknown.");
				auto it_func = global_funcs().find(handler_id);
				if (it_func == global_funcs().end()) fatal_(-1,"Receiver definition: on_msg : function unknown.");
				ctxt.handler.push_back(std::make_pair(it_frame->second,it_func->second));
			}

			comm_threads.push_back(new std::thread{comm_generic_tcp_in_dispatcher_thread,
												   dispatcher_id,
												   nullptr,
												   "",
												   ev_params,
												   this,
												   ip,
												   port,
												   som,
												   eof,
												   sock_name,reg_sock,reuse_sock,
												   comm_generic_tcp_in_thread_fn });
			running_as_node() = true;

		}
	}

	auto simulations = ns[all{"Simulation"}];
	if (simulations.size())
	{
		for (auto simulation_ : simulations)
		{
			auto simulation = simulation_["Simulation"];
			process_simulation(simulation,ceps_env_current(),current_universe());
		}//for
	}
}


state_rep_t State_machine_simulation_core::resolve_state_qualified_id(ceps::ast::Nodebase_ptr p, State_machine* parent)
{
	DEBUG_FUNC_PROLOGUE
	using namespace ceps::ast;
	if (p == nullptr) return state_rep_t{};
	if (p->kind() == ceps::ast::Ast_node_kind::identifier){
		std::string id = name(as_id_ref(p));
		if (parent == nullptr) {
			auto it = State_machine::statemachines.find(id);
			if (it == State_machine::statemachines.end()) return state_rep_t{};
			return state_rep_t(true,true,it->second,it->second->id_);
		} else
		{
			for(auto const & c : parent->children())
			{
				if (c->id_ == id) return state_rep_t(true,true,c,c->id_);
			}
			State_machine::State s(id);
			auto it = parent->states.find(s);
			if (it == parent->states.end()) return state_rep_t{};
			return state_rep_t(true,false,parent,it->id_);
		}

	} else if (p->kind() == ceps::ast::Ast_node_kind::binary_operator && op(ceps::ast::as_binop_ref(p)) == '.') {
		auto root = ceps::ast::as_binop_ptr(p);
		auto l_ = root->left();
		auto l = resolve_state_qualified_id(l_,nullptr);
		if (!l.valid()){DEBUG<<"[INVALID_STATE_REP]\n"; return l;}
		if(!l.is_sm_) return state_rep_t{};
		return resolve_state_qualified_id(root->right(),l.smp_);
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
	simulate(sim,states,ceps_env,universe);
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
	if (node_isrw_state(node.left()) 
		//node.left()->kind() == ceps::ast::Ast_node_kind::symbol && kind(as_symbol_ref(node.left())) == "Systemstate"
		)
	 {
		lhs_id = name(as_symbol_ref(node.left())); return true ;
	 }

	if ( node.left()->kind() != ceps::ast::Ast_node_kind::binary_operator && '.' == ceps::ast::op(ceps::ast::as_binop_ref(node.left())))
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
	DEBUG_FUNC_PROLOGUE2

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
	DEBUG_FUNC_PROLOGUE
	DEBUG << "[STATE_ASSIGNMENT][LHS=" << lhs_id << "]\n";
	if (root.right()->kind() == ceps::ast::Ast_node_kind::identifier)
	{
		std::string id = ceps::ast::name(ceps::ast::as_id_ref(root.right()));
		auto it = type_definitions().find(id);
		if (it == type_definitions().end())
			fatal_(-1,id+" is not a type.\n");

		define_a_struct(this,ceps::ast::as_struct_ptr(it->second),global_states,lhs_id );
		return;
	}
	auto rhs = eval_locked_ceps_expr(this,nullptr,root.right(),&root);


	auto pp = global_states[lhs_id];
	if (pp) global_states_prev_[lhs_id] = pp;
	global_states[lhs_id] = rhs;
	if(!pp) global_states_prev_[lhs_id] = rhs;

	DEBUG << "[STATE_ASSIGNMENT_DONE][LHS=" << lhs_id << "]\n";
}

void State_machine_simulation_core::add(states_t& states, state_rep_t s)
{
	for(size_t i = 0; i < states.size();++i)
		if (states[i].sid_ == s.sid_ && states[i].smp_ == s.smp_) return;
	states.push_back(s);
}


std::string State_machine_simulation_core::get_fullqualified_id(state_rep_t const & s)
{
	if (!s.valid()) {return "";fatal_(-1,"Invalid Identifier.");}
	State_machine* root = s.smp_;
	if(root != nullptr && root->parent() == nullptr && s.is_sm_) return s.sid_;
	if(root != nullptr && root->parent() == nullptr &&   !s.is_sm_) return root->id_ +"."+s.sid_ ;

	std::vector<State_machine*> v;
	for(;root && root->parent_;root = root->parent_) v.push_back(root->parent_);
	std::string r;
	for(int i = v.size()-1;i >= 0;--i)
	{
		r.append(v[i]->id());
		if (i > 0) r.append(".");
	}

	if (!s.is_sm_){
		assert(s.smp_ != nullptr);
		return r.append(".").append(s.smp_->id_).append(".").append(s.sid_) ;
	}

	return r.append(".").append(s.sid_);
}

void State_machine_simulation_core::print_info(states_t& states_from, states_t& states_to,std::set<state_rep_t> const& new_states_triggered_set,
		std::set<state_rep_t> const& states_with_no_transition)
{
	DEBUG_FUNC_PROLOGUE
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
																	state.smp_->parent_->id_));
									}//insert possible parent
			if (state.smp_ && state.is_sm_ && state.smp_->has_initial_state()) {
				if (sms_which_contain_at_least_one_state_in_s.find(state.smp_) == sms_which_contain_at_least_one_state_in_s.end() )
					s.insert(state_rep_t(true,false,state.smp_,"Initial"));
			}
			if(state.is_sm_ && state.smp_->contains_threads())
				for(auto p : state.smp_->children()) if (p->is_thread()) {s.insert(state_rep_t(true,true,p,p->id()));}
		}
	}
	states_out = states_t(s.begin(),s.end());
}


bool State_machine_simulation_core::compute_successor_states_kernel_under_event(event_rep_t ev,
												 states_t& states,
												 std::map<state_rep_t,state_rep_t>& pred,
												 states_t& states_without_transition,
												 ceps::Ceps_Environment& ceps_env,
												 ceps::ast::Nodeset universe,
												 std::map<state_rep_t,
												 std::vector<State_machine::Transition::Action> >& associated_actions,
												 std::set<state_rep_t> & remove_states,
												 std::set<state_rep_t> & removed_states)
{
	using namespace ceps::ast;
	DEBUG_FUNC_PROLOGUE

	associated_actions.clear();states_without_transition.clear();pred.clear();
	std::set<state_rep_t> states_from(states.begin(),states.end());
	std::set<state_rep_t> states_to;
	std::set<State_machine*> threaded_sm_with_all_threads_in_final;
	//Assumption: with any state s in states the sm containing s is also in states
	for(auto & s : states)
	{
		if (!s.is_sm_) continue;
		if (!s.smp_->join()) continue;
		bool all_threads_in_final = true;
		for(auto th: s.smp_->children())
		{
			if (!th->is_thread()) continue;
			auto it = states_from.find(state_rep_t(true,false,th,"Final"));
			if (it == states_from.end()){all_threads_in_final=false;break;}
		}
		if (all_threads_in_final) {
			DEBUG << "[JOIN ACTIVE]["<< s.smp_->id() <<"]" << "\n";
			threaded_sm_with_all_threads_in_final.insert(s.smp_);
		}
	}

	bool transition_taken = false;

	/*std::cout << "*******************\n";

	if (!remove_states.empty()){
		for(auto & st : remove_states) {

			std::cout << st.is_sm_ << "/" << st.sid_ << "/" << (void*) st.smp_ << std::endl;
		}
	}
	std::cout << "*******************\n";*/

	for(auto const & s_from : states_from)
	{
		//std::cout << remove_states.size() << std::endl;
		//std::cout <<"s_from: " << s_from.is_sm_ << "/" << s_from.sid_ << "/" << (void*) s_from.smp_ << std::endl;
		//std::cout << "Found: " << (remove_states.find(s_from) != remove_states.end()) << std::endl;
		if (remove_states.find(s_from) != remove_states.end()) { removed_states.insert(s_from); continue;}
		if (!s_from.is_sm_) {
		 state_rep_t srep(true,true,s_from.containing_sm(),s_from.smp_->id_ );
		 bool ff = remove_states.find(srep) != remove_states.end();
		 //std::cout << "Found: " << ff << std::endl;

		 if (ff) {removed_states.insert(s_from);continue;}
		}
		bool trans_found = false;
		State_machine* containing_smp = s_from.smp_;
		assert(s_from.smp_ != nullptr);
		assert(containing_smp != nullptr);

		bool ff = false;
		for(auto m : State_machine::statemachines) if (m.second == containing_smp) {ff = true;break;}
		assert(ff);
		if (s_from.is_sm_ && s_from.smp_->parent_ ) containing_smp = s_from.smp_->parent_;
		DEBUG << "[CHECKING][STATE="<< s_from.sid_ <<"]" << "\n";
		if(s_from.sid_ == "Final" && containing_smp->is_thread() &&
		   threaded_sm_with_all_threads_in_final.find(containing_smp->parent_) != threaded_sm_with_all_threads_in_final.end())
		{
			auto to_state = containing_smp->parent_->join_state();
			states_to.insert(state_rep_t(true,to_state.is_sm_,to_state.smp_,to_state.id_));
			transition_taken=trans_found=true;
			pred[state_rep_t(true,to_state.is_sm_,to_state.smp_,to_state.id_)] = s_from;
			DEBUG << "[JOIN TAKEN][JOIN_STATE='"<<to_state.id_ <<"']" << "\n";
		}

		for(auto  & t : containing_smp->transitions())
		{
			assert(!t.from().unresolved());
			assert(!t.to().unresolved());
			assert(t.from().id_.size() > 0);
			assert(t.to().id_.size() > 0);
			DEBUG << "[CHECKING TRANSITION]["<<t.from().id_ << "=>"<< t.to().id_ <<"]" << "\n";
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
			if (t.has_guard())
			{
				triggered = eval_guard(ceps_env,t.guard(),states);
			}

			if (!triggered) continue;
			DEBUG << "[TRIGGERED STATE]" << "\n";
			auto const & to_state = t.to_;
			states_to.insert(state_rep_t(true,to_state.is_sm_,to_state.smp_,to_state.id_));
			trans_found=true;transition_taken=true;
			pred[state_rep_t(true,to_state.is_sm_,to_state.smp_,to_state.id_)] = s_from;

			for(auto & temp: t.actions()) temp.associated_sm_ = containing_smp;
			//if (t.actions().size()) associated_actions[state_rep_t(true,to_state.is_sm_,to_state.smp_,to_state.id_)] = t.actions();
			if (t.actions().size()){
				auto & v = associated_actions[state_rep_t(true,to_state.is_sm_,to_state.smp_,to_state.id_)];
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
	DEBUG_FUNC_PROLOGUE
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
			new_states_set.insert(state_rep_t(true,true,child,child->id()));
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


		assert(t.to_.is_sm_ || t.to_.smp_);
		new_states_set.insert(state_rep_t(true,t.to_.is_sm_,t.to_.smp_,t.to_.id()));
		in_initial_state = false;
		for(auto& dd :t.action_ ) dd.associated_sm_ = smp;
		if (t.action_.size()) on_enter_sm_derived_action_list.insert( on_enter_sm_derived_action_list.end(),t.action_.begin(),t.action_.end());
		if (!t.to_.is_sm_) continue;
		enter_sm(false,t.to_.smp_,sms_entered,on_enter_seq,new_states_set,on_enter_sm_derived_action_list,current_states);
	}
	if(in_initial_state) { assert(smp != nullptr); new_states_set.insert(state_rep_t(true,false,smp,"Initial"));}
}


void State_machine_simulation_core::start_processing_init_script(ceps::ast::Nodeset& sim,int& pos,states_t states)
{
	DEBUG_FUNC_PROLOGUE
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


typedef void (*init_plugin_t)(State_machine_simulation_core* ,
	                     void (*)(State_machine_simulation_core*,
	                    		  std::string const&,ceps::ast::Nodebase_ptr (ceps::ast::Call_parameters* )));


void register_plugin(State_machine_simulation_core* smc,std::string const& id,ceps::ast::Nodebase_ptr (*fn) (ceps::ast::Call_parameters* ))
{
	smc->register_plugin_fn(id,fn);
}

void init_state_machine_simulation(	int argc,
									char ** argv,
									State_machine_simulation_core* smc,
									Result_process_cmd_line& result_cmd_line)
{
	using namespace std;
	if (smc == nullptr) return;
	result_cmd_line = process_cmd_line(argc,argv);
	smc->print_debug_info(result_cmd_line.debug_mode);
	smc->quiet_mode() = result_cmd_line.quiet;
	smc->conf_ignore_unresolved_state_id_in_directives() = result_cmd_line.ignore_unresolved_state_id_in_directives;
	string last_file_processed;

	for(std::string const & f : result_cmd_line.definition_file_rel_paths)
		 if (!std::ifstream{f})
		 {
			 last_file_processed = f;
			 std::stringstream ss;
			 ss << "\n***Error: Couldn't open file '" << f << "' " << std::endl << std::endl;
			 throw std::runtime_error(ss.str()) ;
		 }

#ifdef __gnu_linux__
	for(auto const & plugin_lib : result_cmd_line.plugins){
		auto handle = dlopen( plugin_lib.c_str(), RTLD_NOW);
		if (handle == nullptr)
			smc->fatal_(-1,dlerror());

	    auto init_fn_ = dlsym(handle,"init_plugin");
	    if (init_fn_ == nullptr)
	    	smc->fatal_(-1,dlerror());
	    auto init_fn = (init_plugin_t)init_fn_;
	    init_fn(smc,register_plugin);
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

	smc->process_files(result_cmd_line.definition_file_rel_paths,last_file_processed);
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

	smc->processs_content();
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
			smc->processs_content();
		}
		else if (cmd.substr(0,4) == "step")
		{
				smc->set_step_handler(state_machine_sim_core_default_stepping);
				smc->processs_content();
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
			if (smc->states().size() == 0) std::cout << "***No states defined.\n";
			for(auto s : smc->states())
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
	DEBUG_FUNC_PROLOGUE
	if (h.id == NMP_EVENT_FLAT_PAYLOAD)
	{
		DEBUG << "[State_machine_simulation_core::process_event_from_remote][NMP_EVENT_FLAT_PAYLOAD][SIZE="<< h.len << "]\n";
		int ev_name_len = ntohl(*((int*)data));
		char buffer[1024] = {0};
		strncpy(buffer,data+sizeof(int),std::min(1023,(int)ev_name_len));
		DEBUG << "[State_machine_simulation_core::process_event_from_remote][EV_NAME_LEN="<< ev_name_len  <<"]\n" ;
		DEBUG << "[State_machine_simulation_core::process_event_from_remote][EV_NAME='"<< buffer  <<"']\n" ;

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
				v = ntohl(v);DEBUG << "[State_machine_simulation_core::process_event_from_remote][READ_INT]["<< v <<"]\n" ;
				//std::cout << v << std::endl;
				r = ceps::deserialize_value(m, data+offs, h.len-offs);m = (ceps::ast::Unit_rep::sc_t)ntohl(m);offs+=r;
				r = ceps::deserialize_value(kg, data+offs, h.len-offs);kg = (ceps::ast::Unit_rep::sc_t)ntohl(kg);offs+=r;
				r = ceps::deserialize_value(s, data+offs, h.len-offs);s = (ceps::ast::Unit_rep::sc_t)ntohl(s);offs+=r;
				r = ceps::deserialize_value(ampere, data+offs, h.len-offs);ampere = (ceps::ast::Unit_rep::sc_t)ntohl(ampere);offs+=r;
				r = ceps::deserialize_value(kelvin, data+offs, h.len-offs);kelvin = (ceps::ast::Unit_rep::sc_t)ntohl(kelvin);offs+=r;
				r = ceps::deserialize_value(mol, data+offs, h.len-offs);mol = (ceps::ast::Unit_rep::sc_t)ntohl(mol);offs+=r;
				r = ceps::deserialize_value(candela, data+offs, h.len-offs);candela = (ceps::ast::Unit_rep::sc_t)ntohl(candela);offs+=r;
				ev.payload_.push_back(new ceps::ast::Int( v, ceps::ast::Unit_rep(m,kg,s,ampere,kelvin,mol,candela), nullptr, nullptr, nullptr));
			} else if (nmp_payload_id == NMP_PAYLOAD_DOUBLE)	{
				offs+=sizeof(int);
				double v;
				ceps::ast::Unit_rep::sc_t m,kg,s,ampere,kelvin,mol,candela;
				auto r = ceps::deserialize_value(v, data+offs, h.len-offs);offs+=r;
				DEBUG << "[State_machine_simulation_core::process_event_from_remote][READ_DOUBLE]["<< v <<"]\n" ;
				r = ceps::deserialize_value(m, data+offs, h.len-offs);m = (ceps::ast::Unit_rep::sc_t)ntohl(m);offs+=r;
				r = ceps::deserialize_value(kg, data+offs, h.len-offs);kg = (ceps::ast::Unit_rep::sc_t)ntohl(kg);offs+=r;
				r = ceps::deserialize_value(s, data+offs, h.len-offs);s = (ceps::ast::Unit_rep::sc_t)ntohl(s);offs+=r;
				r = ceps::deserialize_value(ampere, data+offs, h.len-offs);ampere = (ceps::ast::Unit_rep::sc_t)ntohl(ampere);offs+=r;
				r = ceps::deserialize_value(kelvin, data+offs, h.len-offs);kelvin = (ceps::ast::Unit_rep::sc_t)ntohl(kelvin);offs+=r;
				r = ceps::deserialize_value(mol, data+offs, h.len-offs);mol = (ceps::ast::Unit_rep::sc_t)ntohl(mol);offs+=r;
				r = ceps::deserialize_value(candela, data+offs, h.len-offs);candela = (ceps::ast::Unit_rep::sc_t)ntohl(candela);offs+=r;
				ev.payload_.push_back(new ceps::ast::Double( v, ceps::ast::Unit_rep(m,kg,s,ampere,kelvin,mol,candela), nullptr, nullptr, nullptr));
			} else if (nmp_payload_id == NMP_PAYLOAD_STRING)	{
				offs+=sizeof(int);
				std::string v;
				auto r = ceps::deserialize_value(v, data+offs, h.len-offs);offs+=r;
				DEBUG << "[State_machine_simulation_core::process_event_from_remote][READ_STR]["<< v <<"]\n" ;
				ev.payload_.push_back(new ceps::ast::String( v, nullptr, nullptr, nullptr));
			} else {
				DEBUG << "[State_machine_simulation_core::process_event_from_remote][UNKNOWN_SERIALIZATION_TAG][IGNORE_PACKET]["<< nmp_payload_id <<"]\n";
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
	 main_event_queue().push(ev);
	}
	//std::cerr << "State_machine_simulation_core::process_event_from_remote\n";
}


void State_machine_simulation_core::register_plugin_fn(std::string const & id,smcore_plugin_fn_t fn){
	name_to_smcore_plugin_fn[id] = fn;
}




