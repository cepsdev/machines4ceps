
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




void ceps::docgen::Statemachine::print(	std::ostream& os,
										fmt_out_ctx& ctx){
	{
		auto local_ctx{ctx};
		fmt_out_layout_state_machine_keyword(local_ctx);
		formatted_out(os,"State Machine",local_ctx);
	}
	{
		auto local_ctx{ctx};
		fmt_out_layout_macro_name(local_ctx);
		local_ctx.eol = "";
		local_ctx.suffix = "";
		local_ctx.ignore_indent = true;
		formatted_out(os,name,local_ctx);
	}
	{
		auto local_ctx{ctx};
		fmt_out_layout_macro_keyword(local_ctx);
		local_ctx.eol = ctx.eol;
		local_ctx.suffix = ":";
		local_ctx.ignore_indent = true;
		formatted_out(os,"",local_ctx);
	}
	++ctx.indent;
	if (actions_vec.size()){
		{
			auto local_ctx{ctx};
			fmt_out_layout_state_machine_keyword(local_ctx);
			local_ctx.eol = ctx.eol;
			local_ctx.suffix = ":";
			formatted_out(os,"Actions",local_ctx);
		}
		++ctx.indent;
		for(auto e: actions_vec)
		{
			{auto local_ctx{ctx};local_ctx.eol="";local_ctx.suffix="";formatted_out(os,"",local_ctx);}
			{auto local_ctx{ctx};fmt_out_layout_funcname(local_ctx);formatted_out(os,e,local_ctx);}
			{auto local_ctx{ctx};local_ctx.ignore_indent=true;local_ctx.suffix=":";formatted_out(os,"",local_ctx);}
			++ctx.indent;
			fmt_out_handle_children(os,action2body[e]->children(),ctx,true);
			--ctx.indent;
		}
	--ctx.indent;
	}
	if (states.size()){
		{
			auto local_ctx{ctx};
			fmt_out_layout_state_machine_keyword(local_ctx);
			local_ctx.eol = ctx.eol;
			local_ctx.suffix = ":";
			formatted_out(os,"States",local_ctx);
		}
		++ctx.indent;
		{
			{auto local_ctx{ctx};local_ctx.eol="";local_ctx.suffix="";formatted_out(os,"",local_ctx);}			
			for(size_t i = 0; i!=states.size();++i){
				{auto local_ctx{ctx};fmt_out_layout_funcname(local_ctx);formatted_out(os,states[i],local_ctx);}
				if (i + 1 != states.size()) {auto local_ctx{ctx};local_ctx.eol="";local_ctx.suffix="";local_ctx.ignore_indent = true; formatted_out(os,", ",local_ctx);}
			}
			{auto local_ctx{ctx};local_ctx.ignore_indent=true;local_ctx.suffix="";formatted_out(os,"",local_ctx);}
		}
		--ctx.indent;
	}
	if (transitions.size()){
		{
			auto local_ctx{ctx};
			fmt_out_layout_state_machine_keyword(local_ctx);
			local_ctx.eol = ctx.eol;
			local_ctx.suffix = ":";
			formatted_out(os,"Transitions",local_ctx);
		}
		++ctx.indent;
		for(auto& t : transitions){
			{auto local_ctx{ctx};local_ctx.eol="";local_ctx.suffix="";formatted_out(os,"",local_ctx);}
			fmt_out_handle_expr(os,t.from, ctx);
			{auto local_ctx{ctx};local_ctx.ignore_indent=true;local_ctx.eol="";local_ctx.suffix="";formatted_out(os," -",local_ctx);}
			if (t.ev){
				//{auto local_ctx{ctx};local_ctx.ignore_indent=true;local_ctx.eol="";local_ctx.suffix="";formatted_out(os," ",local_ctx);}
				fmt_out_handle_expr(os,t.ev, ctx);
				//{auto local_ctx{ctx};local_ctx.ignore_indent=true;local_ctx.eol="";local_ctx.suffix="";formatted_out(os," ",local_ctx);}
			}
			if (t.guards.size()){
				{auto local_ctx{ctx};local_ctx.ignore_indent=true;local_ctx.eol="";local_ctx.suffix="";formatted_out(os,"[",local_ctx);}
				for(size_t i = 0; i != t.guards.size();++i){
					fmt_out_handle_expr(os,t.guards[i], ctx);
					if(i + 1 != t.guards.size()) {auto local_ctx{ctx};local_ctx.ignore_indent=true;local_ctx.eol="";local_ctx.suffix="";formatted_out(os," && ",local_ctx);}
					//{auto local_ctx{ctx};local_ctx.ignore_indent=true;local_ctx.eol="";local_ctx.suffix="";formatted_out(os,";",local_ctx);}
				}
				{auto local_ctx{ctx};local_ctx.ignore_indent=true;local_ctx.eol="";local_ctx.suffix="";formatted_out(os,"]",local_ctx);}
			}
			if (t.actions.size()){
				{auto local_ctx{ctx};local_ctx.ignore_indent=true;local_ctx.eol="";local_ctx.suffix="";formatted_out(os,"/",local_ctx);}
				for(size_t i = 0; i != t.actions.size();++i){
					fmt_out_handle_expr(os,t.actions[i], ctx);
					{auto local_ctx{ctx};local_ctx.ignore_indent=true;local_ctx.eol="";local_ctx.suffix="";formatted_out(os,"()",local_ctx);}
					{auto local_ctx{ctx};local_ctx.ignore_indent=true;local_ctx.eol="";local_ctx.suffix="";formatted_out(os,";",local_ctx);}
				}
				//fmt_out_handle_expr(os,t.ev, ctx);
				//{auto local_ctx{ctx};local_ctx.ignore_indent=true;local_ctx.eol="";local_ctx.suffix="";formatted_out(os," ",local_ctx);}
			}
			{auto local_ctx{ctx};local_ctx.ignore_indent=true;local_ctx.eol="";local_ctx.suffix="";formatted_out(os,"-▶ ",local_ctx);}
			fmt_out_handle_expr(os,t.to, ctx);

			{auto local_ctx{ctx};local_ctx.ignore_indent=true;local_ctx.suffix="";formatted_out(os,"",local_ctx);}
		}
		--ctx.indent;
	}
	if (sub_machines.size()){	
		for(auto sm:sub_machines){
			{auto local_ctx{ctx};local_ctx.ignore_indent=true;local_ctx.suffix="";formatted_out(os,"",local_ctx);}
			sm->print(os,ctx);
		}
	}
	--ctx.indent;
	{auto local_ctx{ctx};local_ctx.suffix="";formatted_out(os,"",local_ctx);}
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
			  sub_machines.push_back(std::make_shared<Statemachine>(Statemachine{s,ctxt,output_format_flags,symtab}));
			return true;
		}
		return true;
	};


	shallow_traverse_ex(strct.children(),
	                    process_name, 
						traverse_pred);
	shallow_traverse_ex(strct.children(),
	                    process_actions, 
						traverse_pred);						
	shallow_traverse_ex(strct.children(),
	                    process_transitions, 
						traverse_pred);
	shallow_traverse_ex(strct.children(),
	                    process_states, 
						traverse_pred);
	shallow_traverse_ex(strct.children(),
	                    process_submachines, 
						traverse_pred);											
}
// ceps::docgen::Statemachine END
