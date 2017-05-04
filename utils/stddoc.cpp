#include "stddoc.hpp"
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

static bool is_toplevel_sm(State_machine* sm){ return sm->parent() == nullptr;}

static std::vector<std::string> stddoc_sm_table_header = {"From","To","Event","Guard","Actions"};
static std::vector<std::string> stddoc_concept_sm_table_header = {"From","To","Event","Guard","Actions"};

static void make_content( std::vector<ceps::ast::Nodebase_ptr>& content,State_machine_simulation_core* smc,ceps::interpreter::Environment& env)
{
 using namespace sm4ceps::modelling::gensm;
 using namespace ceps::ast;
 using namespace std;
 for (auto sm_ : smc->statemachines()){
	 if (!is_toplevel_sm(sm_.second)) continue;
	 auto  sm = sm_.second;
	 auto header = new strct{"header",stddoc_sm_table_header};
	 auto format = new strct{"format"};
	 vector<Nodebase_ptr> rows;
	 for_all_transitions(sm,[&](State_machine::Transition& t){
	  vector<Nodebase_ptr> cols;
	  auto col = new strct{"col",t.from().id()};cols.push_back(col->p_strct);
	  col = new strct{"col",t.to().id()};cols.push_back(col->p_strct);
	  col = new strct{"col",""};cols.push_back(col->p_strct);
	  col = new strct{"col",""};cols.push_back(col->p_strct);
	  col = new strct{"col",""};cols.push_back(col->p_strct);
	  auto row = new strct{"row",cols};
      rows.push_back(row->p_strct);
	 });
	 auto data = new strct{"data",rows};
	 auto table = new
	  strct{
	   "table",
	   header,
	   format,
	   data
	  };
	 content.push_back(table->p_strct);
 }
 for (auto element : env.associated_universe()->nodes()){
 }
}

static ceps::ast::Struct_ptr as_struct(ceps::ast::Nodebase_ptr p){
	if (p->kind() == ceps::ast::Ast_node_kind::structdef) return ceps::ast::as_struct_ptr(p);
	return nullptr;
}

static void write_adoc(std::ostream& os, ceps::ast::Nodeset& ns){
 using namespace sm4ceps::modelling::gensm;
 using namespace ceps::ast;
 using namespace std;
 for(auto n : as_struct_ref(ns.nodes()[0]).children() ){
  Struct_ptr s = as_struct(n);if(s == nullptr) continue;
  if (name(*s) == "table"){
   Nodeset table = Nodeset{s->children()};
   os << "|===\n";
   for(auto h : table["header"]) os << "|"<<h.as_str()<< " ";
   os << "\n";
   for(auto r : table["data"][all{"row"}]) {
    auto row = r["row"];
	for (auto c : row[all{"col"}]){
	 auto col = c["col"];
	 os << "|" << col.as_str() << " ";
	}
	os << "\n";
   }
   os << "|===\n";
  }
 }
}

ceps::ast::Nodeset sm4ceps::utils::make_stddoc(
        ceps::ast::Struct_ptr what,
        ceps::ast::Nodebase_ptr root,
        ceps::parser_env::Symbol* sym,
        ceps::parser_env::Symboltable & symtab,
        ceps::interpreter::Environment& env,
        ceps::ast::Nodebase_ptr ,ceps::ast::Nodebase_ptr)
{
 using namespace sm4ceps::modelling::gensm;
 using namespace ceps::ast;
 using namespace std;
 Nodeset r;
 vector<ceps::ast::Nodebase_ptr> content;
 State_machine_simulation_core* smc = (State_machine_simulation_core*)sym->data;
 make_content(content,smc,env);
 auto stddoc = new
 strct{
  "stddoc",
  content
 };
 r.nodes().push_back(stddoc->get_root());
 auto out_file = Nodeset(what->children())["out"];
 if (out_file.size() == 1){
  string fname  = out_file.as_str();
  std::ofstream os{fname};
  write_adoc(os,r);
 }
 return r;
}
