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

#include "core/include/docgen/docgenerator.hpp"
#include <memory>
using namespace ceps::ast;
using ceps::docgen::fmt_out_ctx;

void ceps::docgen::Statemachine::print_left_margin(std::ostream& os, fmt_out_ctx& ctx){	
	os << std::setw(coverage_statistics.hits_col_width) << coverage_statistics.hits << " ";
	int bar_width = std::round(8 *  ((double)coverage_statistics.hits / (double)coverage_statistics.max_hits));
	os << "\033[1m";
	for(int i = 0; i < bar_width; ++i) os << "░";
	for(int i = 0; i < ctx.indent - coverage_statistics.hits_col_width - bar_width -1;++i) os << " ";
}

extern std::string default_text_representation(ceps::ast::Nodebase_ptr root_node);

void ceps::docgen::Statemachine::print_transitions_tabularized(std::ostream& os, Doc_writer* doc_writer){
	//Compute format
	bool event_column 	{};
	bool guard_column 	{};
	bool action_column 	{};
	bool visited_column {};

	for (auto t: transitions){
		action_column = action_column || t.actions.size() > 0;
		guard_column = guard_column || t.guards.size() > 0;
		event_column = event_column || t.ev != nullptr;
		if (action_column && guard_column && event_column) break;
	}

	auto write_from = [](ceps::docgen::Statemachine::transition& t, std::ostream& os, Doc_writer* doc_writer){doc_writer->out(os,default_text_representation(t.from));};
	
	auto write_to = [](ceps::docgen::Statemachine::transition& t, std::ostream& os, Doc_writer* doc_writer){doc_writer->out(os,default_text_representation(t.to));};
	
	auto write_events = [](ceps::docgen::Statemachine::transition& t, std::ostream& os, Doc_writer* doc_writer){
		if (t.ev) fmt_out_handle_expr(os,t.ev, doc_writer);
		else doc_writer->out(os," ");
	};

	auto write_guards = [](ceps::docgen::Statemachine::transition& t, std::ostream& os, Doc_writer* doc_writer){
		if (!t.guards.size())
			doc_writer->out(os," ");		 
		else for(size_t i = 0; i != t.guards.size();++i){
			fmt_out_handle_expr(os,t.guards[i], doc_writer);
			if(i + 1 != t.guards.size()) {
				doc_writer->push_ctx();
				doc_writer->top().ignore_indent=true;
				doc_writer->top().eol=0;
				doc_writer->top().suffix="";
				doc_writer->out(os," && ");
				doc_writer->pop_ctx();
			}
		}
	};
	auto write_actions = [](ceps::docgen::Statemachine::transition& t, std::ostream& os, Doc_writer* doc_writer){
		if (!t.actions.size())
			doc_writer->out(os," ");		 
		else for(size_t i = 0; i != t.actions.size();++i){
			fmt_out_handle_expr(os,t.actions[i], doc_writer);
			if(i + 1 != t.actions.size()) {
				doc_writer->push_ctx();
				doc_writer->top().ignore_indent=true;
				doc_writer->top().eol=0;
				doc_writer->top().suffix="";
				doc_writer->out(os,"; ");
				doc_writer->pop_ctx();
			}
		}

	};
	auto write_visited = [](ceps::docgen::Statemachine::transition& t, std::ostream& os, Doc_writer* doc_writer){};

	std::vector<std::string> header_info {"From","To"};

	auto tbl_writer = doc_writer->get_table_writer(&os); 

	std::vector< void (*) (ceps::docgen::Statemachine::transition&, std::ostream& , Doc_writer* ) > tblf;
	if (visited_column) tblf.push_back(write_visited);

	tblf.push_back(write_from);
	tblf.push_back(write_to);
	if (event_column) 
		{tblf.push_back(write_events); header_info.push_back("Event");}
	if (guard_column) 
		{tblf.push_back(write_guards);header_info.push_back("Guard");}
	if (action_column) 
		{tblf.push_back(write_actions);header_info.push_back("Action");}
	doc_writer->push_ctx();
	doc_writer->top().eol = 0;
	doc_writer->top().suffix = "";

	tbl_writer->open_table();
	tbl_writer->open_row();
	for (auto hi : header_info){
		tbl_writer->open_cell(true);	
		doc_writer->out(os,hi);
		tbl_writer->close_cell();
	}
	tbl_writer->close_row();
	for(auto t : transitions){
		tbl_writer->open_row();
		for(auto f : tblf) {
			tbl_writer->open_cell(); f(t,os,doc_writer); tbl_writer->close_cell();
		}
		tbl_writer->close_row();
	}
	tbl_writer->close_table();
	doc_writer->pop_ctx();
}

void ceps::docgen::Statemachine::print_states(std::ostream& os, Doc_writer* doc_writer, bool states_on_single_line, bool print_coverage_statistics){
	if (doc_writer->supports_tables())
	{
		print_states_tabularized(os, doc_writer, states_on_single_line, print_coverage_statistics);
		return;
	}

	{
		auto eol_old = doc_writer->top().eol;
		doc_writer->push_ctx();
		fmt_out_layout_state_machine_states_keyword(doc_writer->top());
		doc_writer->top().eol = eol_old;
		doc_writer->top().suffix = ":";
		doc_writer->out(os,"States");
		doc_writer->pop_ctx();
	}
		

	++doc_writer->top().indent;
	{
		if (!states_on_single_line){
				doc_writer->push_ctx();
				doc_writer->top().eol=0;
				doc_writer->top().suffix="";
				doc_writer->out(os,"");
				doc_writer->pop_ctx();
			}			
			for(size_t i = 0; i!=states.size();++i){
				bool margin_annotation = false;
				if (print_coverage_statistics){
					for(auto idx: active_pointers_to_composite_ids_with_coverage_info){
						if (ctxt.composite_ids_with_coverage_info.size() <= (size_t) idx) break;
						if(ctxt.composite_ids_with_coverage_info[idx+1] == nullptr) continue;
						if(!is<Ast_node_kind::identifier>(ctxt.composite_ids_with_coverage_info[idx+1])) continue;
						if (ceps::ast::name(as_id_ref(ctxt.composite_ids_with_coverage_info[idx+1]))!=states[i]) continue;
						if (ctxt.composite_ids_with_coverage_info[idx+2] != nullptr || ctxt.composite_ids_with_coverage_info[idx+3] == nullptr 
						                                                            || !is<Ast_node_kind::int_literal>(ctxt.composite_ids_with_coverage_info[idx+3])) continue;
						margin_annotation = true;
						coverage_statistics.hits = value(as_int_ref(ctxt.composite_ids_with_coverage_info[idx+3]));												
						break;
					}										
				}
				{
					doc_writer->push_ctx();
					fmt_out_layout_state_name(doc_writer->top());
					if (states_on_single_line) doc_writer->top().ignore_indent = false; 
					doc_writer->out(os,states[i], (margin_annotation?this:nullptr));
					doc_writer->pop_ctx();
				}
				if (states_on_single_line){
					doc_writer->push_ctx();
					doc_writer->top().ignore_indent =true; 
					doc_writer->out(os,"");
					doc_writer->pop_ctx();
				}
				else if (i + 1 != states.size()) {
					doc_writer->push_ctx(); 
					doc_writer->top().eol=0;
					doc_writer->top().suffix="";
					doc_writer->top().ignore_indent = true; 
					doc_writer->out(os,", ");
					doc_writer->pop_ctx();
				}
			}
			{
				doc_writer->push_ctx();
				doc_writer->top().ignore_indent=true;
				doc_writer->top().suffix="";
				doc_writer->out(os,"");
				doc_writer->pop_ctx();
			}
		}
		--doc_writer->top().indent;
}

void ceps::docgen::Statemachine::print_states_tabularized(std::ostream& os, Doc_writer* doc_writer, bool states_on_single_line, bool print_coverage_statistics){
	auto tbl_writer = doc_writer->get_table_writer(&os); 
	std::vector<std::string> tbl_header {"State"};
	auto write_cell_visited = [](ceps::docgen::Statemachine* sm, std::string state_name, std::ostream& os, Doc_writer* doc_writer){
		auto visited = false;
		for(auto idx: sm->active_pointers_to_composite_ids_with_coverage_info){
			if(sm->ctxt.composite_ids_with_coverage_info.size() <= (size_t)idx) break;
			if(sm->ctxt.composite_ids_with_coverage_info[idx+1] == nullptr) continue;
			if(!is<Ast_node_kind::identifier>(sm->ctxt.composite_ids_with_coverage_info[idx+1])) continue;
			if (ceps::ast::name(as_id_ref(sm->ctxt.composite_ids_with_coverage_info[idx+1]))!=state_name) continue;
			if (sm->ctxt.composite_ids_with_coverage_info[idx+2] != nullptr || sm->ctxt.composite_ids_with_coverage_info[idx+3] == nullptr 
						                                                            || !is<Ast_node_kind::int_literal>(sm->ctxt.composite_ids_with_coverage_info[idx+3])) continue;
			auto coverage_statistics_hits = value(as_int_ref(sm->ctxt.composite_ids_with_coverage_info[idx+3]));
			visited = coverage_statistics_hits > 0;												
			break;			
		}
		if (visited) doc_writer->out(os,"&#10004;" );
	};

	auto write_cell_state = [](ceps::docgen::Statemachine*, std::string state_name, std::ostream& os, Doc_writer* doc_writer){doc_writer->out(os,state_name);};
	std::vector<void (*) (ceps::docgen::Statemachine*,std::string , std::ostream& , Doc_writer*)> tblf  {write_cell_state};
	if (print_coverage_statistics){
		tblf.push_back(write_cell_visited);
		tbl_header.push_back("Visited");
	}

	doc_writer->push_ctx();
	doc_writer->top().eol = 0;
	doc_writer->top().suffix = "";

	//Table starts here
	tbl_writer->open_table();
	tbl_writer->open_row();

	for (auto hi : tbl_header){
		tbl_writer->open_cell(true);	
		doc_writer->out(os,hi);
		tbl_writer->close_cell();
	}
	tbl_writer->close_row();
	for(auto s : states){
		tbl_writer->open_row();
		for(auto f : tblf) {
			tbl_writer->open_cell(); f(this,s,os,doc_writer); tbl_writer->close_cell();
		}
		tbl_writer->close_row();
	}
	tbl_writer->close_table();
	//Table ends here
	doc_writer->top().eol = 1;
	doc_writer->out(os,"");
	doc_writer->pop_ctx();
}

void ceps::docgen::Statemachine::print_transitions(std::ostream& os, Doc_writer* doc_writer){
		{
			auto eol_old = doc_writer->top().eol;
			doc_writer->push_ctx();
			fmt_out_layout_state_machine_transitions_keyword(doc_writer->top());
			doc_writer->top().eol = eol_old;
			doc_writer->top().suffix = ":";
			doc_writer->out(os,"Transitions");
			doc_writer->pop_ctx();
		}
		++doc_writer->top().indent;
		std::sort(transitions.begin(),transitions.end(),[](ceps::docgen::Statemachine::transition const & lhs, ceps::docgen::Statemachine::transition const & rhs){
			// strict weak order
			auto s1 =  default_text_representation(lhs.from);
			auto s2 =  default_text_representation(rhs.from);
			if (s1 == "Initial"){
			 if (s2 == "Initial") return default_text_representation(lhs.to) < default_text_representation(rhs.to);
			 else return true;
			}
			else if (s2 == "Initial") return false;
			if (s1 == s2) return default_text_representation(lhs.to) < default_text_representation(rhs.to);
			else return s1 < s2;
		});
		std::string last_from_state_name;
		auto max_from_len = 0;
		for(auto t : transitions){
			auto s = default_text_representation(t.from);
		 if (s.length() > (size_t)max_from_len) max_from_len = s.length();
		}

		if (doc_writer->supports_tables())
		 print_transitions_tabularized(os, doc_writer);
		else for(auto t : transitions){
			{
				doc_writer->push_ctx();
				doc_writer->top().eol=0;
				doc_writer->top().suffix="";
				doc_writer->out(os,"");
				doc_writer->pop_ctx();
			}

			auto st = default_text_representation(t.from);
			
			if (st.length() > 0 && last_from_state_name.length() > 0 && st != last_from_state_name){
				doc_writer->push_ctx();
				doc_writer->top().suffix="";
				doc_writer->out(os,"");
				doc_writer->top().eol = 0;
				doc_writer->top().prefix ="";
				doc_writer->out(os,"");
				doc_writer->pop_ctx();				
			}

			if (st != last_from_state_name) {
				fmt_out_handle_expr(os,t.from, doc_writer);
				doc_writer->push_ctx();
				doc_writer->top().suffix = doc_writer->top().prefix = "";
				doc_writer->top().eol = 0; 
				doc_writer->top().ignore_indent = true;
				doc_writer->top().normal_intensity =true;
				doc_writer->out(os, ([&](){std::string s = ""; 
				                           for(size_t i = 0; i < max_from_len - st.length(); ++i) s.append(" ");
										   return s;})());
				doc_writer->pop_ctx();
			}
			else {
				doc_writer->push_ctx();
				doc_writer->top().suffix = doc_writer->top().prefix = "";
				doc_writer->top().eol = 0; 
				doc_writer->top().ignore_indent = true;
				doc_writer->top().normal_intensity =true;
				doc_writer->out(os, ([&](){std::string s = ""; 
				                           for(size_t i = 0; i < (size_t) (max_from_len/2); ++i) s.append(" ");
										   s.append(".");
										   for(size_t i = 0; i < (size_t)(max_from_len - ( 1 +max_from_len/2)); ++i) s.append(" ");
										   return s;})());
				doc_writer->pop_ctx();
			}
			last_from_state_name = st;

			{
				doc_writer->push_ctx();
				doc_writer->top().ignore_indent=true;
				doc_writer->top().eol=0;
				doc_writer->top().suffix="";
				doc_writer->out(os," -");
				doc_writer->pop_ctx();
			}
			if (t.ev){
				fmt_out_handle_expr(os,t.ev, doc_writer);
			}
			if (t.guards.size()){
				{
					doc_writer->push_ctx();
					doc_writer->top().ignore_indent=true;
					doc_writer->top().eol=0;
					doc_writer->top().suffix="";
					doc_writer->top().set_text_foreground_color("state_machine.transition.guard.parentheses");
					doc_writer->out(os,"[");
					doc_writer->pop_ctx();
				}
				for(size_t i = 0; i != t.guards.size();++i){
					fmt_out_handle_expr(os,t.guards[i], doc_writer);
					if(i + 1 != t.guards.size()) {
						doc_writer->push_ctx();
						doc_writer->top().ignore_indent=true;
						doc_writer->top().eol=0;
						doc_writer->top().suffix="";
						doc_writer->out(os," && ");
						doc_writer->pop_ctx();
					}
				}
				{
					doc_writer->push_ctx();
					doc_writer->top().ignore_indent=true;
					doc_writer->top().eol=0;
					doc_writer->top().suffix="";
					doc_writer->top().set_text_foreground_color("state_machine.transition.guard.parentheses");
					doc_writer->out(os,"]");
					doc_writer->pop_ctx();
				}
			}
			if (t.actions.size()){
				{
					doc_writer->push_ctx();
					doc_writer->top().ignore_indent=true;
					doc_writer->top().eol=0;
					doc_writer->top().suffix="";
					doc_writer->out(os,"/");
					doc_writer->pop_ctx();
				}
				for(size_t i = 0; i != t.actions.size();++i){
					fmt_out_handle_expr(os,t.actions[i], doc_writer);
					{
						doc_writer->push_ctx();
						doc_writer->top().ignore_indent=true;
						doc_writer->top().eol=0;
						doc_writer->top().suffix="";
						doc_writer->out(os,"()");
						doc_writer->pop_ctx();
					}
					{
						doc_writer->push_ctx();
						doc_writer->top().ignore_indent=true;
						doc_writer->top().eol=0;
						doc_writer->top().suffix="";
						doc_writer->out(os,";");
						doc_writer->pop_ctx();
					}
				}
			}
			{
				doc_writer->push_ctx();
				doc_writer->top().ignore_indent=true;
				doc_writer->top().eol=0;
				doc_writer->top().suffix="";
				doc_writer->out(os, doc_writer->right_arrow() /*"-▶ " `➜` */);
				doc_writer->pop_ctx();
			}
			fmt_out_handle_expr(os,t.to, doc_writer);

			{
				doc_writer->push_ctx();
				doc_writer->top().ignore_indent=true;
				doc_writer->top().suffix="";
				doc_writer->top().eol=0;
				doc_writer->out(os,"");
				doc_writer->pop_ctx();
			}
		}
		--doc_writer->top().indent;
}

std::string ceps::docgen::Statemachine::compute_full_name(){
	if (parent == nullptr)
		return name;
	std::string nm = name;
	for(auto it = parent;it!=nullptr;it = it->parent)
		nm = it->name + "." + nm;
	return nm;
}

int ceps::docgen::Statemachine::compute_nesting_level(){
	auto r = 0;
	for(auto it = parent;it!=nullptr;it = it->parent)
		++r;
	return r;
}

void ceps::docgen::Statemachine::print(	std::ostream& os,
										Doc_writer* doc_writer,
										std::optional<std::set<std::string>> set_of_sms_to_print){
	
	if (set_of_sms_to_print){
		auto it = set_of_sms_to_print->find(compute_full_name());
		if (it == set_of_sms_to_print->end()) return;
	} 

	bool show_states_only = doc_writer->options.find("state-machines-show-only-states") != doc_writer->options.end();

	auto indent_old = doc_writer->top().indent;
	bool states_on_single_line = active_pointers_to_composite_ids_with_coverage_info.size() != 0;
	bool print_coverage_statistics = active_pointers_to_composite_ids_with_coverage_info.size() != 0;

	doc_writer->start_header(compute_nesting_level()+1, os);

	{
		doc_writer->push_ctx();
		fmt_out_layout_state_machine_keyword(doc_writer->top());
		doc_writer->out(os,"State Machine");
		doc_writer->pop_ctx();
	}
	{
		doc_writer->push_ctx();
		fmt_out_layout_macro_name(doc_writer->top());
		doc_writer->top().eol = 0;
		doc_writer->top().suffix = "";
		doc_writer->top().ignore_indent = true;
		doc_writer->out(os,compute_full_name());
		doc_writer->pop_ctx();
	}
	doc_writer->end_header(os);


	if (!parent && active_pointers_to_composite_ids_with_coverage_info.size())
	 	doc_writer->top().indent = MarginPrinter::left_margin;

	++doc_writer->top().indent;

	if (actions_vec.size() && !show_states_only){
		{
			auto eol_old = doc_writer->top().eol;
			doc_writer->push_ctx();
			fmt_out_layout_state_machine_actions_keyword(doc_writer->top());
			doc_writer->top().eol = eol_old;
			doc_writer->top().suffix = ":";
			doc_writer->out(os,"Actions");
			doc_writer->pop_ctx();
		}
		++doc_writer->top().indent;
	
		for(auto e: actions_vec)
		{
			{doc_writer->push_ctx();doc_writer->top().eol=0;doc_writer->top().suffix="";doc_writer->out(os,"");doc_writer->pop_ctx();}
			{doc_writer->push_ctx();fmt_out_layout_funcname(doc_writer->top());doc_writer->out(os,e);doc_writer->pop_ctx();}
			{doc_writer->push_ctx();doc_writer->top().ignore_indent=true;doc_writer->top().suffix=":";doc_writer->out(os,"");doc_writer->pop_ctx();}
			++doc_writer->top().indent;
			doc_writer->start_code_block(os);
			fmt_out_handle_children(os,action2body[e]->children(),doc_writer,true);
			doc_writer->end_code_block(os);
			--doc_writer->top().indent;
		}

	--doc_writer->top().indent;
	}

	if (states.size())
	 print_states(os,doc_writer,states_on_single_line, print_coverage_statistics);
	if (transitions.size() && !show_states_only)
		print_transitions(os, doc_writer);
	if (sub_machines.size()){	
		for(auto sm:sub_machines){
			{
				doc_writer->push_ctx();
				doc_writer->top().ignore_indent=true;
				doc_writer->top().suffix="";
				doc_writer->out(os,"");
				doc_writer->pop_ctx();
			}
			sm->print(os,doc_writer);
		}
	}
	--doc_writer->top().indent;
	{
		doc_writer->push_ctx();
		doc_writer->top().suffix="";
		doc_writer->out(os,"");
		doc_writer->pop_ctx();
	}
	doc_writer->top().indent = indent_old;
}

void ceps::docgen::Statemachine::build(){
	auto traverse_pred = [](Nodebase_ptr n) ->bool { return is<Ast_node_kind::stmts>(n) || is<Ast_node_kind::stmt>(n) || is<Ast_node_kind::expr>(n); };

	auto process_name = [&](Nodebase_ptr n)->bool {
	 	if (is<Ast_node_kind::identifier>(n)){
			name = ceps::ast::name(as_id_ref(n));
			return false;
		}
		return true;
	};

	auto process_transitions = [&](Nodebase_ptr n)->bool {
	 	if (is<Ast_node_kind::structdef>(n) && ceps::ast::name(as_struct_ref(n)) == "t"){
			 auto& ts{as_struct_ref(n)};
			 transition t{};
			 std::vector<Nodebase_ptr> v;
			 shallow_traverse_ex(ts.children(),[&](Nodebase_ptr n){v.push_back(n);return true;},traverse_pred);
			 if (v.size() < 2 || !is<Ast_node_kind::identifier>(v[0])|| !is<Ast_node_kind::identifier>(v[1])) return true;
			 t.from = v[0];t.to = v[1];

			 for(size_t i = 2; i != v.size(); ++i)
			 {
				 auto e = v[i];
				 if (is<Ast_node_kind::identifier>(e)){
					  auto r = ctxt.global_symbols.kind_of(ceps::ast::name(as_id_ref(e)));
					  if (r){
						  if( "Event" == r.value()) t.ev = e;
						  if( "Guard" == r.value()) t.guards.push_back(e);
					  } else if (actions.find(ceps::ast::name(as_id_ref(e))) != actions.end()){
						  t.actions.push_back(e);
					  }
				 } else if(is<Ast_node_kind::symbol>(e)){
					 if (kind(as_symbol_ref(e)) == "Event") t.ev = e;
					 if (kind(as_symbol_ref(e)) == "Guard") t.guards.push_back(e);
				 }
			 }
			 transitions.push_back(t);
			return true;
		}
		return true;
	};

	auto process_actions = [&](Nodebase_ptr n)->bool {
	 	if (is<Ast_node_kind::structdef>(n)){
			 auto& s{as_struct_ref(n)};
			 auto sname = ceps::ast::name(s);

			 if (sname == "Actions")
			 	shallow_traverse_ex(s.children(),
	                    			[&](Nodebase_ptr n)->bool{
										if (is<Ast_node_kind::structdef>(n)) {
											actions_vec.push_back(ceps::ast::name(as_struct_ref(n)));
											actions.insert(ceps::ast::name(as_struct_ref(n)));
											action2body[ceps::ast::name(as_struct_ref(n))] = as_struct_ptr(n);
										}
										return true;
									}, 
									traverse_pred);
			 else if (sname == "on_enter" || sname == "on_exit") {
				 actions_vec.push_back(sname);
				 actions.insert(sname);
				 action2body[sname] = as_struct_ptr(n);
			 }							 
			return true;
		}
		return true;
	};

	auto process_states = [&](Nodebase_ptr n)->bool {
	 	if (is<Ast_node_kind::structdef>(n)){
			 auto& s{as_struct_ref(n)};
			 auto sname = ceps::ast::name(s);
			 if (sname == "states")
			 	shallow_traverse_ex(s.children(),
	                    			[&](Nodebase_ptr n)->bool{
										if (is<Ast_node_kind::identifier>(n)) {
											states.push_back(ceps::ast::name(as_id_ref(n)));
										}
										return true;
									}, 
									traverse_pred);			  
			return true;
		}
		return true;
	};

	auto process_submachines = [&](Nodebase_ptr n)->bool {
	 	if (is<Ast_node_kind::structdef>(n)){
			 auto& s{as_struct_ref(n)};
			 auto sname = ceps::ast::name(s);
			 if (sname == "sm")
			  sub_machines.push_back(
				  //std::make_shared<Statemachine>(Statemachine{this,as_struct_ptr(n),ctxt,output_format_flags,symtab})
				  new Statemachine{this,as_struct_ptr(n),ctxt,output_format_flags,symtab}
				);
			return true;
		}
		return true;
	};

	//The action starts here

	shallow_traverse_ex(strct->children(),
	                    process_name, 
						traverse_pred);
	//Here we know the name of the state machine, i.e. member 'name' is valid
	if (parent == nullptr && ctxt.coverage_summary != nullptr && ctxt.composite_ids_with_coverage_info.size() != 0){
		//Compute the subset of indices relevant for this state machine (and its substates)
		for(size_t i = 0; i < ctxt.composite_ids_with_coverage_info.size(); ++i){
			if(ctxt.composite_ids_with_coverage_info[i] != nullptr) break;
			++i;
			if (nullptr == ctxt.composite_ids_with_coverage_info[i] || !is<Ast_node_kind::identifier>(ctxt.composite_ids_with_coverage_info[i])) break;
			if ( ceps::ast::name(as_id_ref(ctxt.composite_ids_with_coverage_info[i])) == name ){
				active_pointers_to_composite_ids_with_coverage_info.push_back(i);
			}
			for(;i < ctxt.composite_ids_with_coverage_info.size() && ctxt.composite_ids_with_coverage_info[i]!=nullptr; ++i);
			++i;

			if (ctxt.composite_ids_with_coverage_info[i] != nullptr && is<Ast_node_kind::int_literal>(ctxt.composite_ids_with_coverage_info[i])){
				auto val = value(as_int_ref(ctxt.composite_ids_with_coverage_info[i]));
				if (val > coverage_statistics.max_hits) coverage_statistics.max_hits = val;
			}

			
			for(;i < ctxt.composite_ids_with_coverage_info.size() && ctxt.composite_ids_with_coverage_info[i]!=nullptr; ++i);			
		}	
		if (active_pointers_to_composite_ids_with_coverage_info.size()) MarginPrinter::left_margin = 12;

	} else if (parent != nullptr && ctxt.coverage_summary != nullptr && ctxt.composite_ids_with_coverage_info.size() != 0){
		MarginPrinter::left_margin = parent->left_margin;
		for(size_t i = 0; i < parent->active_pointers_to_composite_ids_with_coverage_info.size(); ++i){
			auto idx = parent->active_pointers_to_composite_ids_with_coverage_info[i];
			if (ctxt.composite_ids_with_coverage_info.size() <= (size_t)idx + 1) break; //something is wrong
			if (ctxt.composite_ids_with_coverage_info[idx +1 ] == nullptr) continue; //end of composite id
			if (!is<Ast_node_kind::identifier>(ctxt.composite_ids_with_coverage_info[idx+1])) break; //something is wrong
			if (ceps::ast::name(as_id_ref(ctxt.composite_ids_with_coverage_info[idx+1])) == name) active_pointers_to_composite_ids_with_coverage_info.push_back(idx+1); //we have a composite id which includes this state machine
		}
		coverage_statistics = parent->coverage_statistics;
	}

	if (active_pointers_to_composite_ids_with_coverage_info.size()) coverage_statistics.hits_col_width = 4;

	shallow_traverse_ex(strct->children(),
	                    process_actions, 
						traverse_pred);						
	shallow_traverse_ex(strct->children(),
	                    process_transitions, 
						traverse_pred);
	shallow_traverse_ex(strct->children(),
	                    process_states, 
						traverse_pred);
	shallow_traverse_ex(strct->children(),
	                    process_submachines, 
						traverse_pred);				
}

