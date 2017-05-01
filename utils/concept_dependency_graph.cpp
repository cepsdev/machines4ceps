#include "concept_dependency_graph.hpp"
#include "core/include/modelling/gensm.hpp"
#include "core/include/state_machine_simulation_core.hpp"

#define INVARIANT(x)


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
static inline State_machine* get_toplevel(State_machine* sm){
	for(;sm->parent();sm = sm->parent());return sm;
}


ceps::ast::Nodeset sm4ceps::utils::build_concept_dependency_graph(
		ceps::ast::Struct_ptr what,
        ceps::ast::Nodebase_ptr root,
        ceps::parser_env::Symbol* sym,
		ceps::parser_env::Symboltable & symtab,
		ceps::interpreter::Environment& env,
        ceps::ast::Nodebase_ptr ,ceps::ast::Nodebase_ptr){
 using namespace sm4ceps::modelling::gensm;
 using namespace ceps::ast;
 ceps::ast::Nodeset r;

 State_machine_simulation_core* smc = (State_machine_simulation_core*)sym->data;


 auto gen_sm = sm("concept_dependency_graph_shadowing");
 gen_sm.header = new ceps::ast::strct{"blabla",0};
 gen_sm.add_state("Initial");
 for (auto toplevel_sm : smc->statemachines()) if (toplevel_sm.second->parent()) continue; else gen_sm.add_state(toplevel_sm.first);
 for (auto toplevel_sm : smc->statemachines()) if (toplevel_sm.second->parent()) continue; else {
	 std::set<std::pair<State_machine*,State_machine*>> m;
	 walk_sm(toplevel_sm.second,[&](State_machine* sm){
		 auto the_toplevel_sm = toplevel_sm.second;
		 auto the_toplevel_sm_name = toplevel_sm.first;
		 for_all_states(sm,[&](State_machine::State& s){
			 if (s.shadow.valid()){
				 if (m.find(std::make_pair(get_toplevel(s.shadow.smp_),the_toplevel_sm)) != m.end()) return;
				 m.insert(std::make_pair(get_toplevel(s.shadow.smp_),the_toplevel_sm));
				 gen_sm.add_transition(smc->get_qualified_id(get_toplevel(s.shadow.smp_)).second,the_toplevel_sm_name,nullptr);
			 }
		 });
	 });
 }

 r.nodes().push_back(gen_sm.ns().nodes()[0]);

 return r;
}
