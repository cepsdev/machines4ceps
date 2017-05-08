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

static void make_section_title(State_machine* sm,State_machine_simulation_core* smc,std::string title,std::vector<ceps::ast::Nodebase_ptr>& content){
 using namespace sm4ceps::modelling::gensm;
 using namespace ceps::ast;
 using namespace std;
 content.push_back( (new strct{"h3",title})->p_strct );
}

static void make_subsection_title(State_machine* sm,State_machine_simulation_core* smc,std::string title,std::vector<ceps::ast::Nodebase_ptr>& content){
 using namespace sm4ceps::modelling::gensm;
 using namespace ceps::ast;
 using namespace std;
 content.push_back( (new strct{"h4",title})->p_strct );
}

static void make_states_list(State_machine* sm, std::vector<ceps::ast::Nodebase_ptr>& content,State_machine_simulation_core* smc,ceps::interpreter::Environment& env){
 using namespace sm4ceps::modelling::gensm;using namespace ceps::ast;using namespace std;
 vector<Nodebase_ptr> states;
 for_all_states(sm,[&](State_machine::State& st){
  states.push_back(( new strct{"li",st.id()} ) ->p_strct );
 });
 content.push_back((new strct{"ul",states})->p_strct);
}

static void make_compound_states_list(State_machine* sm,std::string prefix, std::vector<ceps::ast::Nodebase_ptr>& content,State_machine_simulation_core* smc,ceps::interpreter::Environment& env){
 using namespace sm4ceps::modelling::gensm;using namespace ceps::ast;using namespace std;
 vector<Nodebase_ptr> compound_states;
 for_all_children(sm,[&](State_machine& child){
	 compound_states.push_back(( new strct{"li",prefix+child.id()} ) ->p_strct );
 });
 content.push_back((new strct{"ul",compound_states})->p_strct);
}

static void make_event(State_machine* sm, State_machine::Transition::Event & ev,std::vector<ceps::ast::Nodebase_ptr>& content,State_machine_simulation_core* smc,ceps::interpreter::Environment& env){
 using namespace sm4ceps::modelling::gensm;using namespace ceps::ast;using namespace std;
 content.push_back((new strct{"span",ev.id()})->p_strct);
}

static void make_action(State_machine* sm, State_machine::Transition::Action & action,std::vector<ceps::ast::Nodebase_ptr>& content,State_machine_simulation_core* smc,ceps::interpreter::Environment& env){
 using namespace sm4ceps::modelling::gensm;using namespace ceps::ast;using namespace std;
 content.push_back((new strct{"span",action.id()})->p_strct);
}

static ceps::ast::Nodebase_ptr make_guard(State_machine* sm, State_machine::Transition & t,State_machine_simulation_core* smc,ceps::interpreter::Environment& env){
 using namespace sm4ceps::modelling::gensm;using namespace ceps::ast;using namespace std;
 return (new strct{"span",t.guard()})->p_strct;
}

static void make_sm_dot_graph_link_fully_collapsed(State_machine* sm,State_machine_simulation_core* smc,std::string name,std::vector<ceps::ast::Nodebase_ptr>& content){
 using namespace sm4ceps::modelling::gensm;
 using namespace ceps::ast;
 using namespace std;
 content.push_back( (new strct{"div",strct{"attr",strct{"id",name+"_fully_collapsed"}},strct{"attr",strct{"class","sm_graph"}}, strct{"img",strct{"attr",strct{"src",name+".svg"}} }})->p_strct );
}

static std::string replace_all(std::string str, const std::string& from, const std::string& to) {
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
    }
    return str;
}

static void make_content_sm(State_machine* sm,
		                    std::string name,
							std::vector<ceps::ast::Nodebase_ptr>& content,
							State_machine_simulation_core* smc,
							ceps::interpreter::Environment& env){
 using namespace sm4ceps::modelling::gensm;
 using namespace ceps::ast;
 using namespace std;
 vector<Nodebase_ptr> rows;
 if (!sm->parent()) make_section_title(sm,smc,name,content);
 else make_subsection_title(sm,smc,name,content);
 make_states_list(sm,content,smc,env);
 make_compound_states_list(sm,"",content,smc,env);

 for_all_transitions(sm,[&](State_machine::Transition& t){
  vector<Nodebase_ptr> cols;
  cols.push_back((new strct{"td",t.from().id()})->p_strct);
  cols.push_back((new strct{"td",ident{"&rarr;"}})->p_strct);
  cols.push_back((new strct{"td",t.to().id()})->p_strct);
  vector<Nodebase_ptr> evs;
  for(auto ev : t.events()){
	  make_event(sm,ev,evs,smc,env);
  }
  cols.push_back((new strct{"td",evs})->p_strct);
  vector<Nodebase_ptr> guards = {make_guard(sm,t,smc,env)};
  cols.push_back((new strct{"td",guards})->p_strct);

  vector<Nodebase_ptr> actions;
  for(auto a : t.actions()){
	  make_action(sm,a,actions,smc,env);
  }
  cols.push_back((new strct{"td",actions})->p_strct);

  auto row = new strct{"tr",cols};
  rows.push_back(row->p_strct);
 });

 auto table = new
  strct{
   "table",
   rows
  };
 auto div = new strct{"div",strct{"attr",strct{"class","sm_details"}},table};
 content.push_back(div->p_strct);
 make_sm_dot_graph_link_fully_collapsed(sm,smc,replace_all(name,".","__"),content);
 for_all_children(sm,[&](State_machine& s){make_content_sm(&s,name+"."+s.id(),content,smc,env);});
}


static void make_content( std::vector<ceps::ast::Nodebase_ptr>& content,State_machine_simulation_core* smc,ceps::interpreter::Environment& env)
{
 using namespace sm4ceps::modelling::gensm;
 using namespace ceps::ast;
 using namespace std;
 for (auto sm_ : smc->statemachines()){
	 if (!is_toplevel_sm(sm_.second)) continue;
	 make_content_sm(sm_.second,sm_.first,content,smc,env);
 }
 for (auto element : env.associated_universe()->nodes()){
 }
}

static ceps::ast::Struct_ptr as_struct(ceps::ast::Nodebase_ptr p){
	if (p->kind() == ceps::ast::Ast_node_kind::structdef) return ceps::ast::as_struct_ptr(p);
	return nullptr;
}


static void default_text_representation_impl(std::stringstream& ss,ceps::ast::Nodebase_ptr root_node, bool enable_check_for_html = false){
	if (root_node->kind() == ceps::ast::Ast_node_kind::identifier) {
		ss << name(as_id_ref(root_node));
	} else if (root_node->kind() == ceps::ast::Ast_node_kind::string_literal) {
		ss << value(as_string_ref(root_node));
	} else if (root_node->kind() == ceps::ast::Ast_node_kind::int_literal) {
		ss << value(as_int_ref(root_node));
	} else if (root_node->kind() == ceps::ast::Ast_node_kind::float_literal) {
		ss << value(as_double_ref(root_node));
	}
}

static std::string default_text_representation(ceps::ast::Nodebase_ptr root_node){
	std::stringstream ss;
	default_text_representation_impl(ss,root_node);
	return ss.str();
}

static void dump_html_impl(std::ostream& fout,ceps::ast::Nodebase_ptr elem){
 if (elem->kind() == ceps::ast::Ast_node_kind::structdef){

  auto& cont = ceps::ast::as_struct_ref(elem);
  if (cont.children().size() == 0){ fout << "<" << ceps::ast::name(cont) << "/>";}

  else{

	  fout << "<" << ceps::ast::name(cont);
	  for(auto e: cont.children()){
		  if(e->kind() != ceps::ast::Ast_node_kind::structdef) continue;
		  if (ceps::ast::name(ceps::ast::as_struct_ref(e)) != "attr") continue;
		  fout << " " << ceps::ast::name( ceps::ast::as_struct_ref(ceps::ast::as_struct_ref(e).children()[0])) << "=\"";
		  auto& attr_cont = ceps::ast::as_struct_ref(ceps::ast::as_struct_ref(e).children()[0]);

		  for(auto ee: attr_cont.children())	fout << default_text_representation(ee);
		  fout << "\"";
	  }
	  fout << ">";
	  for(auto e: cont.children()){
		  if (e->kind() == ceps::ast::Ast_node_kind::structdef && ceps::ast::name(ceps::ast::as_struct_ref(e)) == "attr") continue;
		  if (e->kind() == ceps::ast::Ast_node_kind::structdef){
			  dump_html_impl(fout,e);
		  } else fout << default_text_representation(e);
	  }
	  fout << "</" << ceps::ast::name(cont)<< ">";
  }
 } else fout << default_text_representation(elem);

}

static void dump_html(std::ostream& fout, ceps::ast::Struct& html){
 fout << "<!DOCTYPE html><html>\n";
 fout << R"(
<meta charset="UTF-8">
<!--[if IE]><meta http-equiv="X-UA-Compatible" content="IE=edge"><![endif]-->
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<meta name="generator" content="ceps">
<link rel="stylesheet" type="text/css" href="ceps_stddoc_style.css">

 )";
 for (auto e: html.children()) dump_html_impl(fout,e);
 fout << "</html>\n";
}

static void write_html5doc(std::ostream& os, ceps::ast::Nodeset& ns){
 using namespace sm4ceps::modelling::gensm;
 using namespace ceps::ast;
 using namespace std;
 dump_html(os,as_struct_ref(ns.nodes()[0]));
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
  write_html5doc(os,r);
 }
 return r;
}
