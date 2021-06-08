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

void ceps::docgen::Statemachine::print(	std::ostream& os,
										Doc_writer* doc_writer){
	auto indent_old = doc_writer->top().indent;
	if (!parent && active_pointers_to_composite_ids_with_coverage_info.size())
	 doc_writer->top().indent = MarginPrinter::left_margin;

	
	bool states_on_single_line = active_pointers_to_composite_ids_with_coverage_info.size() != 0;
	bool print_coverage_statistics = active_pointers_to_composite_ids_with_coverage_info.size() != 0;

	{
		doc_writer->push_ctx();
		fmt_out_layout_state_machine_keyword(doc_writer->top());
		doc_writer->out(os,"State Machine");
		doc_writer->pop_ctx();
	}
	{
		doc_writer->push_ctx();
		fmt_out_layout_macro_name(doc_writer->top());
		doc_writer->top().eol = "";
		doc_writer->top().suffix = "";
		doc_writer->top().ignore_indent = true;
		doc_writer->out(os,name);
		doc_writer->pop_ctx();
	}
	{
		auto eol_old = doc_writer->top().eol;
		doc_writer->push_ctx();
		fmt_out_layout_macro_keyword(doc_writer->top());
		doc_writer->top().eol = eol_old;
		doc_writer->top().suffix = ":";
		doc_writer->top().ignore_indent = true;
		doc_writer->out(os,"");
		doc_writer->pop_ctx();
	}
	++doc_writer->top().indent;
	if (actions_vec.size()){
		{
			auto eol_old = doc_writer->top().eol;
			doc_writer->push_ctx();
			fmt_out_layout_state_machine_keyword(doc_writer->top());
			doc_writer->top().eol = eol_old;
			doc_writer->top().suffix = ":";
			doc_writer->out(os,"Actions");
			doc_writer->pop_ctx();
		}
		++doc_writer->top().indent;
		for(auto e: actions_vec)
		{
			{doc_writer->push_ctx();doc_writer->top().eol="";doc_writer->top().suffix="";doc_writer->out(os,"");doc_writer->pop_ctx();}
			{doc_writer->push_ctx();fmt_out_layout_funcname(doc_writer->top());doc_writer->out(os,e);doc_writer->pop_ctx();}
			{doc_writer->push_ctx();doc_writer->top().ignore_indent=true;doc_writer->top().suffix=":";doc_writer->out(os,"");doc_writer->pop_ctx();}
			++doc_writer->top().indent;
			fmt_out_handle_children(os,action2body[e]->children(),doc_writer,true);
			--doc_writer->top().indent;
		}
	--doc_writer->top().indent;
	}
	if (states.size()){
		{
			auto eol_old = doc_writer->top().eol;
			doc_writer->push_ctx();
			fmt_out_layout_state_machine_keyword(doc_writer->top());
			doc_writer->top().eol = eol_old;
			doc_writer->top().suffix = ":";
			doc_writer->out(os,"States");
			doc_writer->pop_ctx();
		}
		++doc_writer->top().indent;
		{
			if (!states_on_single_line){
				doc_writer->push_ctx();
				doc_writer->top().eol="";
				doc_writer->top().suffix="";
				doc_writer->out(os,"");
				doc_writer->pop_ctx();
			}			
			for(size_t i = 0; i!=states.size();++i){
				bool margin_annotation = false;
				if (print_coverage_statistics){
					for(auto idx: active_pointers_to_composite_ids_with_coverage_info){
						if (ctxt.composite_ids_with_coverage_info.size() <= idx) break;
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
					fmt_out_layout_funcname(doc_writer->top());
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
					doc_writer->top().eol="";
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
	if (transitions.size()){
		{
			auto eol_old = doc_writer->top().eol;
			doc_writer->push_ctx();
			fmt_out_layout_state_machine_keyword(doc_writer->top());
			doc_writer->top().eol = eol_old;
			doc_writer->top().suffix = ":";
			doc_writer->out(os,"Transitions");
			doc_writer->pop_ctx();
		}
		++doc_writer->top().indent;
		for(auto t : transitions){
			{
				doc_writer->push_ctx();
				doc_writer->top().eol="";
				doc_writer->top().suffix="";
				doc_writer->out(os,"");
				doc_writer->pop_ctx();
			}
			fmt_out_handle_expr(os,t.from, doc_writer);
			{
				doc_writer->push_ctx();
				doc_writer->top().ignore_indent=true;
				doc_writer->top().eol="";
				doc_writer->top().suffix="";
				doc_writer->out(os," -");
				doc_writer->pop_ctx();
			}
			if (t.ev){
				//{auto local_ctx{ctx};local_ctx.ignore_indent=true;local_ctx.eol="";local_ctx.suffix="";formatted_out(os," ",local_ctx);}
				fmt_out_handle_expr(os,t.ev, doc_writer);
				//{auto local_ctx{ctx};local_ctx.ignore_indent=true;local_ctx.eol="";local_ctx.suffix="";formatted_out(os," ",local_ctx);}
			}
			if (t.guards.size()){
				{
					doc_writer->push_ctx();
					doc_writer->top().ignore_indent=true;
					doc_writer->top().eol="";
					doc_writer->top().suffix="";
					doc_writer->out(os,"[");
					doc_writer->pop_ctx();
				}
				for(size_t i = 0; i != t.guards.size();++i){
					fmt_out_handle_expr(os,t.guards[i], doc_writer);
					if(i + 1 != t.guards.size()) {
						doc_writer->push_ctx();
						doc_writer->top().ignore_indent=true;
						doc_writer->top().eol="";
						doc_writer->top().suffix="";
						doc_writer->out(os," && ");
						doc_writer->pop_ctx();
					}
					//{auto local_ctx{ctx};local_ctx.ignore_indent=true;local_ctx.eol="";local_ctx.suffix="";formatted_out(os,";",local_ctx);}
				}
				{
					doc_writer->push_ctx();
					doc_writer->top().ignore_indent=true;
					doc_writer->top().eol="";
					doc_writer->top().suffix="";
					doc_writer->out(os,"]");
					doc_writer->pop_ctx();
				}
			}
			if (t.actions.size()){
				{
					doc_writer->push_ctx();
					doc_writer->top().ignore_indent=true;
					doc_writer->top().eol="";
					doc_writer->top().suffix="";
					doc_writer->out(os,"/");
					doc_writer->pop_ctx();
				}
				for(size_t i = 0; i != t.actions.size();++i){
					fmt_out_handle_expr(os,t.actions[i], doc_writer);
					{
						doc_writer->push_ctx();
						doc_writer->top().ignore_indent=true;
						doc_writer->top().eol="";
						doc_writer->top().suffix="";
						doc_writer->out(os,"()");
						doc_writer->pop_ctx();
					}
					{
						doc_writer->push_ctx();
						doc_writer->top().ignore_indent=true;
						doc_writer->top().eol="";
						doc_writer->top().suffix="";
						doc_writer->out(os,";");
						doc_writer->pop_ctx();
					}
				}
				//fmt_out_handle_expr(os,t.ev, ctx);
				//{auto local_ctx{ctx};local_ctx.ignore_indent=true;local_ctx.eol="";local_ctx.suffix="";formatted_out(os," ",local_ctx);}
			}
			{
				doc_writer->push_ctx();
				doc_writer->top().ignore_indent=true;
				doc_writer->top().eol="";
				doc_writer->top().suffix="";
				doc_writer->out(os,"-▶ ");
				doc_writer->pop_ctx();
			}
			fmt_out_handle_expr(os,t.to, doc_writer);

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
				  std::make_shared<Statemachine>(Statemachine{this,as_struct_ptr(n),ctxt,output_format_flags,symtab})
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

