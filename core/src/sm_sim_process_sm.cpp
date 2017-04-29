#define _CRT_SECURE_NO_WARNINGS

#include "core/include/state_machine_simulation_core.hpp"

#include "core/include/base_defs.hpp"
#include <cassert>
#include <algorithm>

std::string gen_guard_id(std::string qual_sim_id, int ctr);
bool read_qualified_id(ceps::ast::Nodebase_ptr p, std::vector<std::string> & q_id);

static bool is_id_or_symbol(ceps::ast::Nodebase_ptr p, std::string& n, std::string& k){
	using namespace ceps::ast;
	if (p->kind() == Ast_node_kind::identifier) {n = name(as_id_ref(p));k = ""; return true;}
	if (p->kind() == Ast_node_kind::symbol) {n = name(as_symbol_ref(p));k = kind(as_symbol_ref(p)); return true;}
	return false;
}

static ceps::ast::Struct_ptr as_struct(ceps::ast::Nodebase_ptr p){
	if (p->kind() == ceps::ast::Ast_node_kind::structdef) return ceps::ast::as_struct_ptr(p);
	return nullptr;
}




void State_machine_simulation_core::process_statemachine_helper_handle_transitions(
					State_machine* current_statemachine,
					std::vector<State_machine::Transition>& trans,
					std::string id,
					ceps::ast::Nodeset& sm_definition,
					int& guard_ctr, bool is_abstract)
{
 using namespace std;

 int t_ctr=0;

 for (auto p : sm_definition.nodes() )
  {
   auto transition_ptr = as_struct(p);
   if (transition_ptr == nullptr) continue;
   if (ceps::ast::name(*transition_ptr) != "Transition" && ceps::ast::name(*transition_ptr) != "transition" && ceps::ast::name(*transition_ptr) != "t") continue;
   ceps::ast::Nodeset transition(transition_ptr->children());

	bool abstract_trans = is_abstract;
	++t_ctr;
    if (transition.nodes().size() < 2)
     fatal_(-1,"Statemachine '"+id+"' not enough parameters: Transitions contain at least two elements (source- and target-nodes).");

    auto from_state_orig = transition.nodes()[0];
    auto to_state_orig = transition.nodes()[1];
    State_machine::State from,to;

    if (from_state_orig->kind() == ceps::ast::Ast_node_kind::identifier && to_state_orig->kind() == ceps::ast::Ast_node_kind::identifier){
        string s1 = name(as_id_ref(transition.nodes()[0]));
        string s2 = name(as_id_ref(transition.nodes()[1]));
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
    	from = State_machine::State(q_from_id);
    	to = State_machine::State(q_to_id);
    }
    trans.push_back(State_machine::Transition(from,to));
    if (!from.is_initial() && !to.is_final()) trans[trans.size()-1].abstract = abstract_trans;

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

static void extract_sm_sections(ceps::ast::Nodeset& sm_definition,ceps::ast::Nodeset& states, ceps::ast::Nodeset& events, ceps::ast::Nodeset& actions, ceps::ast::Nodeset& threads,
		                        ceps::ast::Nodeset& imports, ceps::ast::Nodeset& join, ceps::ast::Nodeset& on_enter, ceps::ast::Nodeset& on_exit,
								ceps::ast::Nodeset& cover_method, ceps::ast::Nodeset& implements, ceps::ast::Nodeset& where, ceps::ast::Nodeset& extends){
 using namespace ceps::ast;
 states = sm_definition["States"];
 auto states2 = sm_definition["states"];
 std::copy(states2.nodes().begin(), states2.nodes().end(),std::back_inserter(states.nodes()));

 events = sm_definition[all{"Events"}];
 auto events2 = sm_definition["events"];
 std::copy(events2.nodes().begin(), events2.nodes().end(),std::back_inserter(events.nodes()));

 actions = sm_definition[all{"Actions"}];
 auto actions2 = sm_definition["actions"];
 std::copy(actions2.nodes().begin(), actions2.nodes().end(),std::back_inserter(actions.nodes()));

 threads = sm_definition[all{"thread"}];
 imports = sm_definition[all{"import"}];
 join = sm_definition[all{"join"}];
 on_enter = sm_definition[all{"on_enter"}];
 on_exit = sm_definition[all{"on_exit"}];
 cover_method = sm_definition["cover"];
 implements = sm_definition["implements"];
 where = sm_definition["where"];
 extends = sm_definition["extends"];
}

static void extract_sm_modifiers(ceps::ast::Nodeset& sm_definition,bool& is_abstract, bool& is_concept){
 std::string n; std::string k;
 for(auto p:sm_definition.nodes()){
  if (!is_id_or_symbol(p,n,k)) continue;
  if (n == "abstract") is_abstract = true;
  if (n == "concept") {is_concept=is_abstract = true;}
 }
}

static void extract_sm_name_and_id(ceps::ast::Nodeset& sm_definition,std::string& id, std::string& sm_name, bool is_thread,std::string prefix,int thread_ctr,State_machine_simulation_core* smp){
  auto id_ = sm_definition["id"];
  if (id_.size() == 0)
  {
	  if (!is_thread)
	  {
		  std::string n, k;
		  if (sm_definition.nodes().size() == 0 || !is_id_or_symbol(sm_definition.nodes()[0],n,k)){
		   smp->log() << sm_definition << "\n";
		   smp->fatal_(-1,"Statemachine definition: no id found.");
		  } else {
			sm_name = n; id = prefix+n;
		  }
	  } else {
		  sm_name = "thread_"+std::to_string(thread_ctr);
		  id = prefix + sm_name;
	  }
  } else {
	  if (id_.nodes()[0]->kind() != ceps::ast::Ast_node_kind::identifier)
		  smp->fatal_(-1,"Statemachine definition: illformed id, expect an IDENTIFIER.");
	  sm_name = name(as_id_ref(id_.nodes()[0])); id = prefix+name(as_id_ref(id_.nodes()[0]));
  }
}

template<typename F> void walk_sm(State_machine* sm,  F f){
	f(sm);

	for(auto state: sm->states()){
		if (!state->is_sm() || state->smp() == nullptr) continue;
		walk_sm(state->smp(),f);
	}

	for(auto subsm: sm->children()){
		walk_sm(subsm,f);
	}
}

static void replace_state(State_machine* toplevel_sm,State_machine::State state_to_replace,State_machine::State* state){
	walk_sm(toplevel_sm,[&](State_machine* s){
	 for(auto & t : s->transitions()){
		if (t.from_.id_ == state_to_replace.id_ && t.from_.smp_ == toplevel_sm) {t.from_ = *state;}
		if (t.to_.id_ == state_to_replace.id_ && t.to_.smp_ == toplevel_sm) {t.to_ = *state;}
	 }
	});
}

template <typename F> void for_all_transitions(State_machine* sm,F f){
   for(auto & t : sm->transitions())
	   f(t);
}

template <typename F> void for_all_states(State_machine* sm,F f){
   for(auto & t : sm->states())
	   f(*t);
}

template <typename F> void for_all_children(State_machine* sm,F f){
   for(auto & t : sm->children())
	   f(*t);
}

static void set_owner(State_machine* old_owner,State_machine* new_owner ){
    walk_sm(old_owner,[&](State_machine* sm){
    for_all_children(sm,[&](State_machine& s){
     if(s.parent_ == old_owner) s.parent_ = new_owner;
    });
    for_all_states(sm,[&](State_machine::State& s){
      if(s.smp_ == old_owner) s.smp_ = new_owner;
    });
    for_all_transitions(sm,[&](State_machine::Transition& t){
      if(t.from_.smp_ == old_owner) t.from_.smp_ = new_owner;
      if(t.to_.smp_ == old_owner) t.to_.smp_ = new_owner;
     });
    });
}

static void remove_all_shadowing(State_machine* toplevel_sm){
 walk_sm(toplevel_sm,[&](State_machine* sm){
  sm->shadowing_me().clear();
  for_all_states(sm,[&](State_machine::State& s){
   s.shadow.valid_ = false;
  });
  for_all_transitions(sm,[&](State_machine::Transition& t){
   t.from_.shadow.valid_ = false;
   t.to_.shadow.valid_ = false;
  });
 });
}

static inline State_machine* get_toplevel(State_machine* sm){
	for(;sm->parent();sm = sm->parent());return sm;
}

static std::set<State_machine*> compute_all_shadowed_sms(State_machine* toplevel_sm){
 std::set<State_machine*> r;
 walk_sm(toplevel_sm,[&](State_machine* sm){
  for_all_states(sm,[&](State_machine::State& s){
	  if (s.shadow.valid()) r.insert(s.shadow.smp_);
  });
 });
 return r;
}

template<typename F> void walk_two_structurally_identical_sms(State_machine* sm,State_machine* sm_clone,  F f){
	f(sm,sm_clone);
	for(std::size_t j = 0;j != sm->children().size() && j != sm_clone->children().size();++j){
		walk_two_structurally_identical_sms(sm->children()[j],sm_clone->children()[j],f);
	}
}

static std::unordered_set<State_machine*> compute_all_shadowing_sms(State_machine* sm){
	return {};
}

State_machine* State_machine_simulation_core::merge_state_machines(std::vector<State_machine*> sms,
		                                                           bool delete_purely_abstract_transitions,
																   bool turn_abstract_transitions_to_normal,
																   int order,
																   std::string id,
																   State_machine* parent,
																   int depth )
{
 auto handle_shadowing = [&](State_machine* clone,State_machine* orig){
  if (orig->is_concept()){
   auto shadowing_sms = compute_all_shadowing_sms(orig);
   if (shadowing_sms.empty()){
    walk_two_structurally_identical_sms(orig,clone,[&](State_machine* sm_a,State_machine* sm_b){
     for_all_states(sm_a,[&](State_machine::State& s){
      if(s.is_initial()) return;
      if(s.is_final()) return;
	  for(auto s2 : sm_b->states())
	   if(s2->id_ == s.id_){
        s2->shadow = state_rep_t(true,false,sm_a,s.id_,-1);
		break;
	   }
      });
    });
   } else {
   }
  }
 };

 if (sms.size() == 0) return new State_machine(SM_COUNTER++,id,parent,depth);
 std::vector<State_machine*> clones;
 for(auto sm : sms ){
  auto temp = new State_machine(SM_COUNTER++,sm->id(),parent,depth);
  temp->clone_from(sm,SM_COUNTER,sm->id(),nullptr,nullptr);
  remove_all_shadowing(temp);
  clones.push_back(temp);
 }
 auto main_sm = clones.front();
 main_sm->id() = id;
 handle_shadowing(main_sm,sms[0]);
 for(std::size_t j = 1; j != clones.size(); ++j){
  State_machine::State* new_init;
  auto cur_sm = clones[j];
  main_sm->states().insert(new_init = new State_machine::State("Initial_"+cur_sm->id()));
  new_init->dont_cover() = true;
  new_init->smp_ = main_sm;
  replace_state(cur_sm,State_machine::State("Initial"),new_init);
  set_owner(cur_sm,main_sm);
  handle_shadowing(cur_sm,sms[j]);
  main_sm->merge(*cur_sm);
  State_machine::Transition init_to_new_init;
  init_to_new_init.from_ = main_sm->get_initial_state();
  init_to_new_init.to_ = *new_init;
  main_sm->transitions().push_back(init_to_new_init);
 }
 if (delete_purely_abstract_transitions)
  walk_sm(main_sm,[&](State_machine* sm){
   std::vector<State_machine::Transition> v;
    for_all_transitions(sm,[&](State_machine::Transition& t){
     if (t.abstract && t.guard().length() == 0 && t.events().empty() && t.actions().empty()) return;
     if (turn_abstract_transitions_to_normal) t.abstract = false;
     v.push_back(t);
    });
   sm->transitions() = v;
  });
 return main_sm;
}

void State_machine_simulation_core::process_statemachine(	ceps::ast::Nodeset& sm_definition,
							std::string prefix,
							State_machine* parent,
							int depth,
							int thread_ctr,
							bool is_thread,
							bool is_abstract)
{
  using namespace std;
  using namespace ceps::ast;
  bool is_concept = false;

  ceps::ast::Nodeset states,events,actions,threads,imports,join,on_enter,on_exit,cover_method,implements,where,extends;
  extract_sm_sections(sm_definition,states,events,actions,threads,imports,join,on_enter,on_exit,cover_method,implements,where,extends);
  extract_sm_modifiers(sm_definition,is_abstract,is_concept);
  int anonymous_guard_ctr = 1;
  std::string id;std::string sm_name;
  extract_sm_name_and_id(sm_definition,id,sm_name,is_thread,prefix,thread_ctr,this);

  State_machine* current_statemachine = nullptr;
  {
	  auto it = State_machine::statemachines.find(id);
	  if (it != State_machine::statemachines.end()) current_statemachine = it->second;
  }
  if (current_statemachine == nullptr)
  {
	std::vector<State_machine*> implemented_machines;
	std::vector<State_machine*> extended_machines;

	if (implements.size()){
	  for(auto p: implements.nodes()){
	   auto s = resolve_state_or_transition_given_a_qualified_id(p,nullptr, nullptr);
	   if (!s.valid()) fatal_(-1,"Invalid id.");
	   if (!s.is_sm_) fatal_(-1,"has to be state machine.");
	   implemented_machines.push_back(s.smp_);
	  }
    }

	if (extends.size()){
	  for(auto p: extends.nodes()){
	   auto s = resolve_state_or_transition_given_a_qualified_id(p,nullptr, nullptr);
	   if (!s.valid()) fatal_(-1,"Invalid id.");
	   if (!s.is_sm_) fatal_(-1,"has to be state machine.");
	   extended_machines.push_back(s.smp_);
	  }
    }

	if (implemented_machines.size()){
	  current_statemachine = merge_state_machines(implemented_machines,true,true,SM_COUNTER++,sm_name,parent,depth);
	} else if (extends.size() == 0) current_statemachine = new State_machine(SM_COUNTER++,sm_name,parent,depth);

	if (extended_machines.size() && current_statemachine == nullptr){
	 current_statemachine = merge_state_machines(extended_machines,false,false,SM_COUNTER++,sm_name,parent,depth);
	} else if (extended_machines.size()){
     std::vector<State_machine *>v;v.push_back(current_statemachine);
     std::copy(extended_machines.begin(),extended_machines.end(),std::back_inserter(v));
     current_statemachine = merge_state_machines(v,false,false,SM_COUNTER++,sm_name,parent,depth);
	}

	State_machine::statemachines[id] = current_statemachine;
	set_qualified_id(current_statemachine,id);
  }

  current_statemachine->cover() = !cover_method.empty() || is_concept;
  current_statemachine->is_concept() = is_concept;
  current_statemachine->is_thread() = is_thread;
  if (is_thread && parent) parent->contains_threads() = true;
  if (parent != nullptr) parent->add_child(current_statemachine);

  //INVARIANT: current_statemachine now points to correct machine

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
	  process_statemachine(sm,id+".",current_statemachine,depth+1,0,false,is_abstract);
  }//for
  {
	  int thread_ctr_local = 1;
	  for (auto sm_ : threads)
	  {
		  auto sm = sm_["thread"];
		  process_statemachine(sm,id+".",current_statemachine,depth+1,thread_ctr_local,true,is_abstract);++thread_ctr_local;
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
    		   current_statemachine->insert_action(State_machine::Transition::Action(name(as_id_ref(v))));
    	   else if (v->kind() == ceps::ast::Ast_node_kind::structdef)
    		   current_statemachine->insert_action(State_machine::Transition::Action(name(as_struct_ref(v)),v));
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
	  current_statemachine->insert_action(State_machine::Transition::Action(current_statemachine,name(as_struct_ref(on_enter.nodes()[0])),on_enter.nodes()[0]));
  if (on_exit.size())
 	  current_statemachine->insert_action(State_machine::Transition::Action(current_statemachine,name(as_struct_ref(on_exit.nodes()[0])),on_exit.nodes()[0]));



  for (auto state : states)
  {
	 if (state.nodes().size() != 1 || state.nodes()[0]->kind() != ceps::ast::Ast_node_kind::identifier)
		 fatal_(-1,"State machine '"+id+"': Definition of states: Illformed expression: expected an unbound identifier, got: "
				 + ceps::ast::ast_node_kind_to_text[(int)state.nodes()[0]->kind()] );
     current_statemachine->insert_state( State_machine::State( name(as_id_ref(state.nodes()[0])) ) );
  }
  process_statemachine_helper_handle_transitions(current_statemachine,current_statemachine->transitions(),id,sm_definition,anonymous_guard_ctr,is_abstract);

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

        current_statemachine->join() = true;
        current_statemachine->join_state().id_ = s1;
        if (!current_statemachine->lookup(current_statemachine->join_state()))
       	 fatal_(-1,"Statemachine '"+id+"': '"+ s1+"' is not a state/state machine.");
	  } else {
        std::vector<std::string> q_join_state_id;

	    bool r = read_qualified_id(join_state,q_join_state_id );
	    if (!r) fatal_(-1,"State machine '"+ id +"', join: illformed state id.\n");

	    current_statemachine->join() = true;
	    current_statemachine->join_state() = State_machine::State(q_join_state_id);
	  }

  }
}
