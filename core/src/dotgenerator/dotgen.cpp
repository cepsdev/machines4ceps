#define _CRT_SECURE_NO_WARNINGS

#include "core/include/state_machine_simulation_core.hpp"
#include "core/include/base_defs.hpp"
#include <map>
#include <iostream>
#include <set>
#include <unordered_set>
#include <queue>
#include "core/include/dotgenerator/dotgen.hpp"
 
using namespace ceps::ast;
static void flatten_args2(ceps::ast::Nodebase_ptr r, std::vector<ceps::ast::Nodebase_ptr>& v, char op_val = ',')
{
	using namespace ceps::ast;
	if (r == nullptr) return;
	if (r->kind() == ceps::ast::Ast_node_kind::binary_operator && op(as_binop_ref(r)) ==  op_val)
	{
		auto& t = as_binop_ref(r);
		flatten_args2(t.left(),v,op_val);
		flatten_args2(t.right(),v,op_val);
		return;
	}
	v.push_back(r);
}


static bool is_func_call(ceps::ast::Nodebase_ptr p,std::string& fid,std::vector<ceps::ast::Nodebase_ptr>& args){
 if (p->kind() != ceps::ast::Ast_node_kind::func_call) return false;
 ceps::ast::Func_call& func_call = *dynamic_cast<ceps::ast::Func_call*>(p);
 ceps::ast::Identifier& id = *dynamic_cast<ceps::ast::Identifier*>(func_call.children()[0]);
 ceps::ast::Call_parameters& params = *dynamic_cast<ceps::ast::Call_parameters*>(func_call.children()[1]);
 if (params.children().size()) flatten_args2(params.children()[0], args);
 fid = ceps::ast::name(id);
 return true;
}

static bool is_func_call(ceps::ast::Nodebase_ptr p,std::string& fid,std::vector<ceps::ast::Nodebase_ptr>& args, std::size_t number_of_arguments){
	auto r = is_func_call(p,fid,args);
	if (!r) return r;
	return number_of_arguments == args.size();
}

static bool is_id_or_symbol(ceps::ast::Nodebase_ptr p, std::string& n, std::string& k){
	using namespace ceps::ast;
	if (p->kind() == Ast_node_kind::identifier) {n = name(as_id_ref(p));k = ""; return true;}
	if (p->kind() == Ast_node_kind::symbol) {n = name(as_symbol_ref(p));k = kind(as_symbol_ref(p)); return true;}
	return false;
}

static bool is_id(ceps::ast::Nodebase_ptr p, std::string & result, std::string& base_kind){
	using namespace ceps::ast;
	std::string k,l;
	if (p->kind() == Ast_node_kind::binary_operator && op(as_binop_ref(p)) == '.'){
	 if (!is_id_or_symbol(as_binop_ref(p).right(),k,l)) return false;

	 if (!is_id(as_binop_ref(p).left(),result,base_kind)) return false;
	 result = result + "." + k;
	 return true;
	} else if (is_id_or_symbol(p,k,l)){ base_kind = l; result = k; return true; }
	return false;
}

static std::string replace_all(std::string str, const std::string& from, const std::string& to) {
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
    }
    return str;
}

static bool is_dot_op(ceps::ast::Nodebase_ptr p){
	if (p->kind() != ceps::ast::Ast_node_kind::binary_operator) return false;
	if (ceps::ast::op(ceps::ast::as_binop_ref(p)) == '.') return true;
	return false;
}

static ceps::ast::Struct_ptr as_struct(ceps::ast::Nodebase_ptr p){
	if (p->kind() == ceps::ast::Ast_node_kind::structdef) return ceps::ast::as_struct_ptr(p);
	return nullptr;
}

static void dump_style_vec(std::ostream& os, std::vector<std::string> v, std::string delim = ","){
	if (v.size() == 0) return;
	for(auto i = 0 ;i + 1 != v.size(); ++i){
		os << v[i] << delim;
	}
	os << v.back();
}

State_machine* get_sm(std::pair<std::string,State_machine*> const & elem){
	return elem.second;
}

bool is_toplevel_sm(std::pair<std::string,State_machine*> const & elem){
	return elem.first.find('.') == std::string::npos;
}

std::string full_name_of_sm(std::pair<std::string,State_machine*> const & elem){
	return elem.first;
}
template<typename F, typename T,typename S> void traverse_sm(std::unordered_set<State_machine*>& m,State_machine* sm, T const & sms,std::string name,bool traverse_sub_sms, F f,S& s){
    if(!f(sm,name)) return;
	if (!traverse_sub_sms) return;
	for(auto state: sm->states()){
		if (!state->is_sm() || state->smp() == nullptr) continue;
		if (m.find(state->smp()) != m.end()) continue;
		m.insert(state->smp());
        traverse_sm(m,state->smp(),sms,name+"."+state->id_,traverse_sub_sms,f,s);
	}

	for(auto subsm: sm->children()){
		//assert(m.find(subsm) != m.end());
		if (m.find(subsm) != m.end()) continue;
		m.insert(subsm);
        traverse_sm(m,subsm,sms,name+"."+subsm->id_,traverse_sub_sms,f,s);
	}
}

template<typename F, typename T, typename S> void traverse_sms(T const & sms, bool traverse_sub_sms, F f,S& s){
	std::unordered_set<State_machine*> m;
	for(auto sm_: sms){
	 if(!is_toplevel_sm(sm_)) continue;
	 auto sm = get_sm(sm_);
	 if (m.find(sm) != m.end()) continue;
	 traverse_sm(m,sm,sms,full_name_of_sm(sm_),traverse_sub_sms,f,s);
	 m.insert(sm);
	}
}

static void write_copyright_and_timestamp(std::ostream& out, std::string title,bool b,Result_process_cmd_line const& result_cmd_line){
	if(!b) return;
	time_t timer;time(&timer);tm * timeinfo;timeinfo = localtime(&timer);
	out
		<< "/* "<< title <<" " << std::endl
		<< "   CREATED " << asctime(timeinfo) << std::endl
		<< "   GENERATED BY THE sm4ceps C++ GENERATOR VERSION 0.81 (c) 2016,2017 Tomas Prerovsky <tomas.prerovsky@gmail.com>, ALL RIGHTS RESERVED. \n"
		<< "   BASED ON cepS "<< ceps::get_version_info() << "\n\n"

		<< "   Input files:\n";
	for( auto const & f: result_cmd_line.definition_file_rel_paths) {
	out << "      "<< f << "\n";
	}

	out << "\n";
	out	<< "   THIS IS A GENERATED FILE.\n\n"
		<< "   *** DO NOT MODIFY. ***\n*/\n"
		<< std::endl << std::endl;
}


void Dotgenerator::dump_sm(std::ostream& o,std::string name,State_machine* sm,std::set<State_machine*>* expand,std::set<int>& highlighted_states){
     if (expand != nullptr && expand->find(sm) == expand->end()){
       bool highlight = (highlighted_states.size()==0)?false:(highlighted_states.find(sm->idx_)!=highlighted_states.end());
       o << " "<<sm2dotname[sm]<<"[ label=\""<< sm->id() << "\\n...\" ," <<state_style(sm->id(),highlight)<< ",fontsize=14];\n";
       return;
     }
	 o << " subgraph "<<n2dotname[name]<<"{\n";
     o << "  label=\"" << sm->id() << "\";\nfontname=\"Arial\";\nfontsize=14;\n";

     {
      auto it = userdefined_style_infos.find(sm->idx_);
      if (it != userdefined_style_infos.end()){dump_style_vec(o,it->second,";\n");}
     }

     if (highlighted_states.size() && highlighted_states.find(sm->idx_)!=highlighted_states.end()){

     }

	 for(auto const& s:sm->states()) if (!s->is_sm_){
		 std::string t = n2dotname[name+"."+s->id()];
         bool highlight = (highlighted_states.size()==0)?false:(highlighted_states.find(s->idx_)!=highlighted_states.end());
         o << " "<<t<<"["<<label(s) << "," <<state_style(s->id(),highlight)<< ",fontsize=14]";
         auto it = userdefined_style_infos.find(s->idx_);
         if (it != userdefined_style_infos.end())
         	 {o << "["; dump_style_vec(o,it->second); o << "]";}
         o << ";\n";
	 }

	 for(auto subsm:sm->children()){
         dump_sm(o,name+"."+subsm->id(),subsm,expand,highlighted_states);
	 }

	 o << " }\n";
}

void State_machine_simulation_core::do_generate_dot_code(std::map<std::string,State_machine*> const & sms,
                                                         std::set<State_machine*>* expand,
                                                         std::set<int>& highlighted_states,
														 Dotgenerator dotgen,
                                                         std::ostream& o){
    //expand = nullptr;
    int cluster_counter = 0;
    int pure_state_counter = 0;

    if (highlighted_states.size()) {
        o << "// Highlighted States: ";
        for(auto e : highlighted_states) o << e << " ";
        o << "\n";
    }
    o << "digraph Root {\ncompound=true;fillcolor=cornsilk;style=\"rounded,filled\";/*\nnodesep=1.1;*/\nnode [shape=box, fontname=\"Arial\"];\n";

    auto map_names = [&](State_machine* sm,std::string name){
        if (expand != nullptr && expand->find(sm) == expand->end()){
            dotgen.n2dotname[name] = dotgen.sm2dotname[sm] = dotgen.sm2initial[sm] = "cluster_proxy"+std::to_string(cluster_counter++);
            for(auto const& s:sm->states()) if (!s->is_sm_){
                dotgen.n2dotname[name+"."+s->id()] = dotgen.sm2initial[sm];
            }
            for(auto s:sm->children())
                dotgen.n2dotname[name+"."+s->id()] = dotgen.n2dotname[name+"."+s->id()] = dotgen.sm2initial[s] = dotgen.sm2initial[sm];

            return false;
        }
        dotgen.n2dotname[name] = "cluster"+std::to_string(cluster_counter);
        dotgen.sm2dotname[sm] = "cluster"+std::to_string(cluster_counter++);
        for(auto const& s:sm->states()) if (!s->is_sm_){
            dotgen.n2dotname[name+"."+s->id()] = "node"+std::to_string(pure_state_counter++)+"_is_"+s->id();
            if (s->id() == "Initial" || s->id() == "initial") dotgen.sm2initial[sm] = dotgen.n2dotname[name+"."+s->id()];
        }
        return true;
    };

    auto dump_toplevel_sm = [&](State_machine* sm,std::string name)->bool{
        dotgen.dump_sm(o,name,sm,expand,highlighted_states);return true;
    };

    auto dump_transitions =[&](State_machine* sm, std::string name)->bool{
         bool out_only = false;
         if (expand != nullptr  && expand->find(sm) == expand->end() ) out_only = true;
         for(auto& t : sm->transitions()){
             if (out_only && (!t.to_.is_sm_ || t.to_.smp()->parent() == sm) ) continue;
             bool edge_dumped = false;
             if ((t.from_.parent_ == sm || t.from_.parent_ == nullptr)&& (t.to_.parent_ == sm || t.to_.parent_ == nullptr) && !(t.from_.is_sm_ || t.to_.is_sm_) ){
                 if (dotgen.n2dotname.find(name+"."+t.from_.id()) != dotgen.n2dotname.end() && dotgen.n2dotname.find(name+"."+t.to_.id()) != dotgen.n2dotname.end() ){
                     o << dotgen.n2dotname[name+"."+t.from_.id()] << "->" << dotgen.n2dotname[name+"."+t.to_.id()] << "[penwidth=1"<<  dotgen.edge_label(t,sm)<<"]";
                     edge_dumped = true;
                 }
             }
             else if (!t.from_.is_sm_ && t.to_.is_sm_ && dotgen.n2dotname.find(name+"."+t.from_.id()) != dotgen.n2dotname.end() && dotgen.n2dotname.find(name+"."+t.to_.id()) != dotgen.n2dotname.end() ){
                 o << dotgen.n2dotname[name+"."+t.from_.id()] << "->" <<dotgen.sm2initial[t.to_.smp_];
                 o << "[";
                 if (expand == nullptr  || expand->find(t.to_.smp_) != expand->end())
                     o << "lhead=\""<< dotgen.sm2dotname[t.to_.smp_] << "\",";
                 o <<"penwidth=1"<< dotgen.edge_label(t,sm) << "]";
                 edge_dumped = true;
             }
             else if (t.from_.is_sm_ && !t.to_.is_sm_ && dotgen.sm2initial.find(t.from_.smp_) != dotgen.sm2initial.end()){
                 o << dotgen.sm2initial[t.from_.smp_] << "->" << dotgen.n2dotname[name+"."+t.to_.id()];
                 o << "[";
                 if (expand == nullptr  || expand->find(t.from_.smp_) != expand->end())
                     o << "ltail=\""<< dotgen.sm2dotname[t.from_.smp_] << "\",";
                 o << "penwidth=1"<<  dotgen.edge_label(t,sm)<<"]";
                 edge_dumped = true;
             }
             else if (t.from_.is_sm_ && t.to_.is_sm_
                     && dotgen.sm2initial.find(t.from_.smp_) != dotgen.sm2initial.end()
                     && dotgen.sm2initial.find(t.to_.smp_) != dotgen.sm2initial.end() ){
                 o << dotgen.sm2initial[t.from_.smp_] << "->" << dotgen.sm2initial[t.to_.smp_];
                 o << "[";
                 if (expand == nullptr  || expand->find(t.from_.smp_) != expand->end())
                 {
                     o << "ltail=\""<< dotgen.sm2dotname[t.from_.smp_] << "\", ";
                 }
                 if (expand == nullptr  || expand->find(t.to_.smp_) != expand->end())
                 {
                     o << "lhead=\""<< dotgen.sm2dotname[t.to_.smp_] << "\", ";
                 }
                 o << "penwidth=1"<<  dotgen.edge_label(t,sm)<<"]";
                 edge_dumped = true;
             }
             if (edge_dumped){
            	 auto it = dotgen.userdefined_edge_style_infos.find(t.id_);
            	 if (it != dotgen.userdefined_edge_style_infos.end()){
            		 o << "["; dump_style_vec(o,it->second); o <<"]";
            	 }
            	 o << ";\n";
             }
         }
         return true;
    };
    traverse_sms(sms,true,map_names,dotgen);
    traverse_sms(sms,false,dump_toplevel_sm,dotgen);
    traverse_sms(sms,true,dump_transitions,dotgen);

    o << "}\n";
}

void State_machine_simulation_core::do_generate_dot_code(ceps::Ceps_Environment& ceps_env,
													  ceps::ast::Nodeset& universe,
													  std::map<std::string, ceps::ast::Nodebase_ptr> const & all_guards,
													  Result_process_cmd_line const& result_cmd_line){


	std::set<int> highlight_states;
	Dotgenerator dotgen;
	auto props = universe["dot_gen_properties"];
	auto hi_states = universe["dot_gen_properties"]["highlight"];
	auto ctr = 0;
	for(auto e : hi_states.nodes()){
		++ctr;
		std::string n,k;
		state_rep_t s;
		if (e->kind() == ceps::ast::Ast_node_kind::string_literal){
          s = resolve_state_qualified_id(ceps::ast::value(ceps::ast::as_string_ref(e)),nullptr);
		} else if (is_id_or_symbol(e,n,k) || is_dot_op(e) ){
          s = resolve_state_or_transition_given_a_qualified_id(e,nullptr);
		}
		if (s.id_ < 0 || !s.valid()) {
			std::stringstream ss; ss << *e;
			warn_(-1, "Element #"+std::to_string(ctr)+" of root.dot_gen_properties.highlight: '"+ss.str()+"' not a valid fully qualified state identifier");
		} else highlight_states.insert(s.id_);
	}
	for(auto e : props.nodes()){


	 {
	  std::string n,k;
	  if (is_id(e,n,k)){
		 if (n == "hide_edge_labels")
			 dotgen.global_prop_show_edges() = false;
		 continue;
	  }
	 }
	 auto s = as_struct(e);
     if (!s) continue;

     if (ceps::ast::name(*s) == "node_display_properties"){
      state_rep_t st;
      auto ctr = 0;
      std::vector<std::string> u;
      for (auto e: s->children()){
       if (ctr==0) if (e->kind() == ceps::ast::Ast_node_kind::string_literal) st = resolve_state_qualified_id(ceps::ast::value(ceps::ast::as_string_ref(e)),nullptr);
       else st = resolve_state_or_transition_given_a_qualified_id(e,nullptr);
       if(!st.valid() || st.id_ < 0){
  		std::stringstream ss; ss << *e;
  		warn_(-1, "Element #"+std::to_string(ctr)+" of root.dot_gen_properties.node_display_properties: '"+ss.str()+"' not a valid fully qualified state identifier");
  		break;
  	   }
       ++ctr;
       if (e->kind() == ceps::ast::Ast_node_kind::string_literal)
        u.push_back(ceps::ast::value(ceps::ast::as_string_ref(e)));
      }//for
      if (u.size()){
    	  auto & v = dotgen.userdefined_style_infos[st.id_];
    	  std::copy(u.begin(),u.end(),std::back_inserter(v));
      }
     } else if (ceps::ast::name(*s) == "edge_display_properties") {
      state_rep_t st;
      int edge_no = 0;
      int transition_id = -1;
      auto ctr = 0;
      std::vector<std::string> u;
      ceps::ast::Nodebase_ptr edge_id = nullptr;
      for (auto e: s->children()){
       if (ctr==0) {
    	   if (e->kind() == ceps::ast::Ast_node_kind::int_literal){
    		transition_id = ceps::ast::value(ceps::ast::as_int_ref(e));
    	   } else
    	    st = resolve_state_or_transition_given_a_qualified_id(edge_id=e,nullptr,&edge_no);
       }
       if( (!st.valid() || st.id_ < 0) && transition_id < 0){
     	std::stringstream ss; ss << *e;
     	warn_(-1, "Element #"+std::to_string(ctr)+" of root.dot_gen_properties.node_display_properties: '"+ss.str()+"' not a valid fully qualified transition identifier");
     	break;
       }
       ++ctr;
       if (e->kind() == ceps::ast::Ast_node_kind::string_literal)
        u.push_back(ceps::ast::value(ceps::ast::as_string_ref(e)));
      }//for
      if (u.size() && (edge_id || transition_id) ) {
       constexpr auto invalid_transition = -1;
       int matched_transition = invalid_transition;
       if (transition_id < 0) {
    	   if (st.is_sm_ && st.smp_->transitions().size() > edge_no) matched_transition = st.smp_->transitions()[edge_no].id_;
       } else {
    	   if (transition_id < this->executionloop_context().transitions.size() ) matched_transition = transition_id;
       }
       if(matched_transition == -1){
     	std::stringstream ss; ss << *edge_id;
     	warn_(-1, "Element #"+std::to_string(ctr)+" of root.dot_gen_properties.node_display_properties: '"+ss.str()+"' not a valid fully qualified transition identifier");
       } else {
    	   auto& v = dotgen.userdefined_edge_style_infos[matched_transition];
    	   std::copy(u.begin(),u.end(),std::back_inserter(v));
       }
      }
      }//edge_display_properties
	}//for

	if (!result_cmd_line.dot_gen_one_file_per_top_level_statemachine){
	 std::ofstream o{"out.dot"};
     write_copyright_and_timestamp(o, "out.dot",true,result_cmd_line);
     do_generate_dot_code(State_machine::statemachines,nullptr,highlight_states,dotgen,o);
	} else {
		for(auto s : statemachines() ){
		 std::map<std::string,State_machine*> m = { {s.first, s.second} };
		 std::ofstream o{ replace_all(s.first,".","__")+".dot"};
	     write_copyright_and_timestamp(o, "out.dot",true,result_cmd_line);
	     do_generate_dot_code(m,nullptr,highlight_states,dotgen,o);
		}
	}
}


