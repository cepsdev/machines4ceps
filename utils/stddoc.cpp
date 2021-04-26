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
 bool is_concept = get_toplevel(sm)->is_concept();

 if (!is_concept)content.push_back( (new strct{"h3",strct{"attr",strct{"class","sm_section"}},title})->p_strct );
 else content.push_back( (new strct{"h3",
	                                strct{"attr",strct{"class","sm_section"}},
							        title,
							        strct{"span",strct{"attr",strct{"class","concept_tag_section_title"}},"Concept" } })->p_strct );

}

static void make_subsection_title(State_machine* sm,State_machine_simulation_core* smc,std::string title,std::vector<ceps::ast::Nodebase_ptr>& content){
 using namespace sm4ceps::modelling::gensm;
 using namespace ceps::ast;
 using namespace std;
 bool is_concept = get_toplevel(sm)->is_concept();

 if (!is_concept)content.push_back( (new strct{"h4",strct{"attr",strct{"class","sm_subsection"}},title})->p_strct );
 else content.push_back( (new strct{"h4",
                                    strct{"attr",strct{"class","sm_section"}},
	                                title,
	                                strct{"span",strct{"attr",strct{"class","concept_tag_sub_section_title"}},"Concept" } })->p_strct );
}

static std::vector<ceps::ast::Nodebase_ptr> make_states_list(State_machine* sm, State_machine_simulation_core* smc,ceps::interpreter::Environment& env){
 using namespace sm4ceps::modelling::gensm;using namespace ceps::ast;using namespace std;
 vector<Nodebase_ptr> states;
 for_all_states(sm,[&](State_machine::State& st){
  if(st.is_initial() || st.is_final() || sm->join_state().id() == st.id()) return;
  states.push_back(( new strct{"span", strct{"attr",strct{"class","sm_state"}} , st.id()} ) ->p_strct );
 });
 for_all_children(sm,[&](State_machine& s){
  states.push_back(( new strct{"span", strct{"attr",strct{"class","sm_composite_state"}} , s.id()} ) ->p_strct );
 });
 return states;
}

/*static void make_compound_states_list(State_machine* sm,std::string prefix, std::vector<ceps::ast::Nodebase_ptr>& content,State_machine_simulation_core* smc,ceps::interpreter::Environment& env){
 using namespace sm4ceps::modelling::gensm;using namespace ceps::ast;using namespace std;
 vector<Nodebase_ptr> compound_states;
 for_all_children(sm,[&](State_machine& child){
	 compound_states.push_back(( new strct{"li",prefix+child.id()} ) ->p_strct );
 });
 content.push_back((new strct{"ul",compound_states})->p_strct);
}*/

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

static ceps::ast::Nodebase_ptr make_sm_dot_graph_link(State_machine* sm,State_machine_simulation_core* smc,std::string name){
 using namespace sm4ceps::modelling::gensm;
 using namespace ceps::ast;
 using namespace std;
 return (new strct{"div",strct{"attr",strct{"id",name}},strct{"attr",strct{"class","sm_graph"}}, strct{"img",strct{"attr",strct{"src",name+".svg"}} }})->p_strct ;
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
                            ceps::interpreter::Environment& env,
                            std::string img_path_prefix){
 using namespace sm4ceps::modelling::gensm;
 using namespace ceps::ast;
 using namespace std;
 vector<Nodebase_ptr> rows;
 if (!sm->parent()) make_section_title(sm,smc,name,content);
 else make_subsection_title(sm,smc,name,content);

 auto states_list = make_states_list(sm,smc,env);

 for_all_transitions(sm,[&](State_machine::Transition& t){
  vector<Nodebase_ptr> cols;
  cols.push_back((new strct{"td",t.from().id()})->p_strct);
  cols.push_back((new strct{"td",ident{"<span class=\"glyphicon glyphicon-arrow-right\"></span> "}})->p_strct);
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

 strct* table = nullptr;

 if (sm->transitions().size()) table = new
  strct{
   "table",
   strct{"attr",strct{"border","1"}},
   new strct{"tr",new strct{"th",new strct{"attr",new strct{"colspan",3} },"Transition"},new strct{"th","Event"},new strct{"th","Guard"},new strct{"th","Action"}   },
   rows
  };

 /*Nodebase_ptr sm_details_div = nullptr;

 if (table) sm_details_div = (new strct{"div",strct{"attr",strct{"class","panel panel-default"}},table})->p_strct;
 else sm_details_div = (new strct{"div",strct{"attr",strct{"class","sm_details"}}})->p_strct;
 */

 auto dot_div = make_sm_dot_graph_link(sm,smc,img_path_prefix+replace_all(name,".","__"));
 content.push_back(
   ( new strct{"table",strct{"attr",strct{"class","sm_details_and_sm_graph"}},
    /* strct{"tr",strct{"td",strct{"attr",strct{"colspan",2}},strct{"attr",strct{"class","sm_states_overview"}},states_list }},
     strct{"tr",strct{"td",std::vector<Nodebase_ptr>{sm_details_div} }   },*/
     strct{"tr",strct{"td", std::vector<Nodebase_ptr>{dot_div} }} } )->p_strct );

 for_all_children(sm,[&](State_machine& s){make_content_sm(&s,name+"."+s.id(),content,smc,env,img_path_prefix);});
}


static ceps::ast::Struct_ptr as_struct(ceps::ast::Nodebase_ptr p){
	if (p->kind() == ceps::ast::Ast_node_kind::structdef) return ceps::ast::as_struct_ptr(p);
	return nullptr;
}

static void do_indent(std::ostream & os, std::size_t j){
	for (std::size_t h = 0; h != j; ++h ) os << " ";
}

static bool is_atomic(ceps::ast::Nodebase_ptr p){
	return p->kind() == ceps::ast::Ast_node_kind::int_literal || p->kind() == ceps::ast::Ast_node_kind::float_literal || p->kind() == ceps::ast::Ast_node_kind::string_literal;
}

static std::pair<std::string,ceps::ast::Nodebase_ptr> node_with_single_leaf_element(ceps::ast::Nodebase_ptr root){
 auto s = as_struct(root);
 if (s == nullptr) return std::make_pair(std::string{},nullptr);
 if (s->children().size() != 1) return std::make_pair(ceps::ast::name(*s),nullptr);
 auto candidate = s->children()[0];
 if (!is_atomic(candidate)) return std::make_pair(ceps::ast::name(*s),nullptr);
 return std::make_pair(ceps::ast::name(*s),s->children()[0]);
}

static void make_json(ceps::ast::Nodeset ns,std::stringstream& ss, std::size_t indent = 0){


 ss << "{\n";++indent;
 for(std::size_t i = 0; i != ns.nodes().size();++i){
  auto single_element_assignment = node_with_single_leaf_element(ns.nodes()[i]);
  if (single_element_assignment.second){
	  do_indent(ss,indent);ss << "\""<< single_element_assignment.first << "\" : ";
	  auto p = single_element_assignment.second;
	  if(p->kind() == ceps::ast::Ast_node_kind::int_literal)
		  ss << ceps::ast::value(ceps::ast::as_int_ref(p));
	  else if(p->kind() == ceps::ast::Ast_node_kind::float_literal)
		  ss << ceps::ast::value(ceps::ast::as_double_ref(p));
  } else if (ns.nodes()[i]->kind() == ceps::ast::Ast_node_kind::structdef){
	  do_indent(ss,indent);ss << "\""<< ceps::ast::name(ceps::ast::as_struct_ref(ns.nodes()[i])) << "\" : ";
	  make_json(ceps::ast::Nodeset(ceps::ast::as_struct_ref(ns.nodes()[i]).children()),ss,indent);
  } else continue;
  if (i + 1 != ns.nodes().size()) ss << ",\n"; else ss << "\n";
 }//for
 --indent;do_indent(ss,indent);ss<<"}";
}

static std::string javascript_overview_barchart(std::string canvas_id, std::string title, std::string data){
	return std::string{R"(

	<script>
	(function(ctx,title_text,e){
	 var lbls = [], dt = [];
	 var background_colors = [];
	 var border_colors = [];
	 var yellow1 = 'rgba(255, 206, 86, 0.6)';
	 var yellow2 = 'rgba(255, 206, 86, 1)';
	 var red1  = 'rgba(255, 99, 132, 0.6)';
	 var red2  = 'rgba(255, 99, 132, 1)';
	 var green1 = 'rgba(58, 153, 68, 0.6)';
	 var green2 = 'rgba(58, 153, 68, 0.6)';
	 for(x in e ){
	  lbls.push(x);
	  dt.push(e[x]*100.0);
	  if (e[x] <= 0.33) {background_colors.push(red1);border_colors.push(red2);}
	  else if (e[x] <= 0.66) {background_colors.push(yellow1);border_colors.push(yellow2);}
	  else {background_colors.push(green1);border_colors.push(green2);}
	 }
	 var the_chart = new Chart(ctx, {
	    type: 'bar',
	    data: {
	        labels: lbls,
	        datasets: [{
	            label: null,
	            data: dt,
	            backgroundColor: background_colors,
	            borderColor: border_colors,
	            borderWidth: 1
	        }]
	    },
	    options: {
	        title: {
	            display: true,
	            text: title_text
	        },
	        legend: {
	            display: false
	        },
	        scales: {
	            yAxes: [{
	                ticks: {
                        max : 100,
                        min : 0,
	                    beginAtZero:true,
	                    callback: function(value, index, values) {
	                        return value+"%";
	                    }
	                }
	            }]
	        }
	    }
	});

	}))"}+
	"(document.getElementById(\""+canvas_id+"\"),\""+title+"\",ceps_coverage_data"+data+");</script>";
}

static void make_content( std::vector<ceps::ast::Nodebase_ptr>& content,State_machine_simulation_core* smc,ceps::interpreter::Environment& env, std::string img_path_prefix)
{
 using namespace sm4ceps::modelling::gensm;
 using namespace ceps::ast;
 using namespace std;
 std::stringstream ss;
 /*make_json(smc->current_universe()["summary"],ss);
 content.push_back((new strct{"@pass_through","<script src=\"js/Chart.js\"></script><script>\nvar ceps_coverage_data="+ss.str()+";\n</script>" })->p_strct);

 content.push_back((new strct{"@pass_through","<div id=\"overview_state_coverage\"><canvas id=\"canvas_overview_state_coverage\"></canvas></div>" })->p_strct);
 content.push_back((new strct{"@pass_through",
	                          javascript_overview_barchart("canvas_overview_state_coverage",
	                        		                       "State Coverage Chart",
														   "['coverage']['state_coverage']['toplevel_state_machines']") })->p_strct);
 content.push_back((new strct{"@pass_through","<div id=\"overview_transition_coverage\"><canvas id=\"canvas_overview_transition_coverage\"></canvas></div>" })->p_strct);
 content.push_back((new strct{"@pass_through",
	                          javascript_overview_barchart("canvas_overview_transition_coverage",
	                        		                       "Transition Coverage Chart",
                                                           "['coverage']['transition_coverage']['toplevel_state_machines']") })->p_strct);*/

 for (auto sm_ : smc->statemachines()){
	 if (!is_toplevel_sm(sm_.second)) continue;
     make_content_sm(sm_.second,sm_.first,content,smc,env,img_path_prefix);
 }
 /*for (auto element : env.associated_universe()->nodes()){
 }*/
}



static void default_text_representation_impl(std::stringstream& ss,ceps::ast::Nodebase_ptr root_node, bool enable_check_for_html = false){
	std::cerr <<"()())()())()(" <<std::endl;
	if (root_node->kind() == ceps::ast::Ast_node_kind::identifier) {
		ss << name(as_id_ref(root_node));
	} else if (root_node->kind() == ceps::ast::Ast_node_kind::string_literal) {
		ss << value(as_string_ref(root_node));
	} else if (root_node->kind() == ceps::ast::Ast_node_kind::int_literal) {
		ss << value(as_int_ref(root_node));
	} else if (root_node->kind() == ceps::ast::Ast_node_kind::float_literal) {
	 	ss << value(as_double_ref(root_node));
	} else if (root_node->kind() == ceps::ast::Ast_node_kind::long_literal) {
		ss << value(as_int64_ref(root_node));
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

	  if ("@pass_through" != ceps::ast::name(cont)){ fout << "<" << ceps::ast::name(cont);
	  for(auto e: cont.children()){
		  if(e->kind() != ceps::ast::Ast_node_kind::structdef) continue;
		  if (ceps::ast::name(ceps::ast::as_struct_ref(e)) != "attr") continue;
		  fout << " " << ceps::ast::name( ceps::ast::as_struct_ref(ceps::ast::as_struct_ref(e).children()[0])) << "=\"";
		  auto& attr_cont = ceps::ast::as_struct_ref(ceps::ast::as_struct_ref(e).children()[0]);

		  for(auto ee: attr_cont.children())	fout << default_text_representation(ee);
		  fout << "\"";
	  }
	  fout << ">";}
	  for(auto e: cont.children()){
		  if (e->kind() == ceps::ast::Ast_node_kind::structdef && ceps::ast::name(ceps::ast::as_struct_ref(e)) == "attr") continue;
		  if (e->kind() == ceps::ast::Ast_node_kind::structdef){
			  dump_html_impl(fout,e);
		  } else fout << default_text_representation(e);
	  }
	  if ("@pass_through" != ceps::ast::name(cont)) {fout << "</" << ceps::ast::name(cont)<< ">";}
  }
 } else fout << default_text_representation(elem);

}

static void dump_html(std::ostream& fout, ceps::ast::Struct& html){

 for (auto e: html.children()) dump_html_impl(fout,e);

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

 std::string img_path_prefix = "";
 auto img_path_prefix_ = Nodeset(what->children())["img_path_prefix"];
 if (img_path_prefix_.size() == 1){
  img_path_prefix  = img_path_prefix_.as_str();
 }

 make_content(content,smc,env,img_path_prefix);
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
