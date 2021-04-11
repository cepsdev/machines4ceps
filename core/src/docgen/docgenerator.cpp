
/*
Copyright 2014,2015,2016,2017,2018,2019,2020,2021 Tomas Prerovsky (cepsdev@hotmail.com).

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

using namespace ceps::ast;



struct fmt_out_ctx{
	bool inside_schema = false;

	bool bold = false;
	bool italic = false;
	bool underline = false;
	bool ignore_indent = false;
	bool normal_intensity = false;

	std::string foreground_color;
	std::string foreground_color_modifier;
	std::string suffix;
	std::string prefix;
	std::string eol="\n";
	std::vector<std::string> info;
	std::vector<std::string> modifiers; 
    std::string indent_str = "  ";
	int indent = 0;
	ceps::parser_env::Symboltable* symtab = nullptr;
};

static void fmt_out_handle_children(std::ostream& os, std::vector<ceps::ast::Nodebase_ptr>& children, fmt_out_ctx ctx);
static void fmt_out_handle_inner_struct(std::ostream& os, ceps::ast::Struct& strct, fmt_out_ctx ctx);
static void fmt_out_handle_macro_definition(std::ostream& os, ceps::ast::Macrodef& macro, fmt_out_ctx ctx);

static void fmt_out_layout_outer_strct(bool is_schema, fmt_out_ctx& ctx){
	if (is_schema) {
		ctx.underline = true;
		ctx.foreground_color = "214";
		ctx.suffix = ":";
		ctx.info.push_back("schema");
	}
}

static void fmt_out_layout_inner_strct(fmt_out_ctx& ctx){
	if (ctx.inside_schema) ctx.foreground_color = "184";
	else ctx.foreground_color = "25";
	ctx.suffix = ":";
	//ctx.info.push_back("schema");
}

static void fmt_out_layout_macro_name(fmt_out_ctx& ctx){
	if (ctx.inside_schema) ctx.foreground_color = "4";
	else ctx.foreground_color = "4";
	ctx.bold = true;
	ctx.italic = true;
	ctx.suffix = ":";
}

static void fmt_out_layout_macro_keyword(fmt_out_ctx& ctx){
	if (ctx.inside_schema) ctx.foreground_color = "5";
	else ctx.foreground_color = "5";
	ctx.bold = true;
	ctx.suffix = " ";
	ctx.prefix = "";
	ctx.eol = "";
}

static void fmt_out_layout_loop_keyword(fmt_out_ctx& ctx){
	if (ctx.inside_schema) ctx.foreground_color = "5";
	else ctx.foreground_color = "5";
	ctx.bold = true;
	ctx.suffix = " ";
	ctx.prefix = "";
	ctx.eol = "";
}

static void fmt_out_layout_loop_in_keyword(fmt_out_ctx& ctx){
	if (ctx.inside_schema) ctx.foreground_color = "5";
	else ctx.foreground_color = "5";
	ctx.bold = true;
	ctx.suffix = " ";
	ctx.prefix = "";
	ctx.eol = "";
	ctx.ignore_indent = true;
}

static void fmt_out_layout_loop_variable(fmt_out_ctx& ctx){
	if (ctx.inside_schema) ctx.foreground_color = "6";
	else ctx.foreground_color = "6";
	ctx.italic = true;
	ctx.suffix = " ";
	ctx.prefix = "";
	ctx.eol = "";
	ctx.ignore_indent = true;
}

static void fmt_out_layout_val_var(fmt_out_ctx& ctx){
	if (ctx.inside_schema) ctx.foreground_color = "6";
	else ctx.foreground_color = "6";
	ctx.italic = true;
	ctx.suffix = " ";
	ctx.prefix = "";
	ctx.eol = "";
	ctx.ignore_indent = true;
}

static void fmt_out_layout_val_keyword(fmt_out_ctx& ctx){
	if (ctx.inside_schema) ctx.foreground_color = "5";
	else ctx.foreground_color = "5";
	ctx.bold = true;
	ctx.suffix = " ";
	ctx.prefix = "";
	ctx.eol = "";
}

static void fmt_out_layout_label(fmt_out_ctx& ctx){
	ctx.suffix = "";
	ctx.underline = true;
	ctx.normal_intensity = true;
}





static void fmt_out(std::ostream& os, std::string s, fmt_out_ctx ctx){
 if(!ctx.ignore_indent) for(int i = 0; i < ctx.indent; ++ i) os << ctx.indent_str;
 os << "\033[0m"; //reset
 if (ctx.foreground_color.size()) os << "\033[38;5;"<< ctx.foreground_color << "m";
 if (ctx.underline) os << "\033[4m";
 if (ctx.italic) os << "\033[3m";
 if (ctx.bold) os << "\033[1m";
 if (ctx.normal_intensity) os << "\033[22m";

 os << ctx.prefix;
 os << s;
 if (ctx.info.size()){
    os << "\033[0m"; //reset
	os << "\033[2m";
	os << " (";
	for(size_t i = 0; i + 1 < ctx.info.size(); ++i)
	 os << ctx.info[i] << ",";
	os << ctx.info[ctx.info.size()-1];
	os << ")";
	os << "\033[0m"; //reset
    if (ctx.foreground_color.size()) os << "\033[38;5;"<< ctx.foreground_color << "m";
 }
 os << ctx.suffix;
 os << ctx.eol;
 os << "\033[0m"; //reset
}

static std::vector<ceps::ast::Symbol*> fetch_symbols_standing_alone(std::vector<ceps::ast::Nodebase_ptr> const & nodes){
	std::vector<ceps::ast::Symbol*> r;
	for (auto n : nodes)
	 if (ceps::ast::is_a_symbol(n))
	 	r.push_back(ceps::ast::as_symbol_ptr(n));
	return r;
}

static void fmt_out_handle_macro_definition(std::ostream& os, ceps::ast::Macrodef& macro, fmt_out_ctx ctx){
	if (ctx.symtab == nullptr) 
		return;
	auto symbol = ctx.symtab->lookup(name(macro),false,false,false);
	if (symbol == nullptr || symbol->category != ceps::parser_env::Symbol::MACRO)
		return;
	{
		auto local_ctx{ctx};
		fmt_out_layout_macro_keyword(local_ctx);
		fmt_out(os,"Macro",local_ctx);
	}
	{
		auto local_ctx{ctx};
		fmt_out_layout_macro_name(local_ctx);
		fmt_out(os,name(macro),local_ctx);
	}
	++ctx.indent;
	fmt_out_handle_children(os,as_stmts_ptr(static_cast<Nodebase_ptr>(symbol->payload))->children(),ctx);
}

static void fmt_out_handle_loop(std::ostream& os, ceps::ast::Loop& loop, fmt_out_ctx ctx){
	{
		auto local_ctx{ctx};
		fmt_out_layout_loop_keyword(local_ctx);
		fmt_out(os,"for each",local_ctx);
	}
	auto& loop_head =  as_loop_head_ref(loop.children()[0]);
	ceps::ast::Nodebase_ptr body = loop.children()[1];
	//std::cout << loop_head << std::endl;
	ceps::ast::Identifier& id  = ceps::ast::as_id_ref(loop_head.children()[0]);
	{
		auto local_ctx{ctx};
		fmt_out_layout_loop_variable(local_ctx);
		fmt_out(os,name(id),local_ctx);
	}
	{
		auto local_ctx{ctx};
		fmt_out_layout_loop_in_keyword(local_ctx);
		fmt_out(os,"in",local_ctx);
	}
	os << ":" << ctx.eol;
	++ctx.indent;
	fmt_out_handle_children(os,nlf_ptr(body)->children(),ctx);
}

static void fmt_handle_node(std::ostream& os, ceps::ast::Nodebase_ptr n, fmt_out_ctx ctx){
	if (is<Ast_node_kind::loop>(n)){
		fmt_out_handle_loop(os,as_loop_ref(n),ctx);			
	} else if (is<Ast_node_kind::macro_definition>(n)) {
		fmt_out_handle_macro_definition(os,as_macrodef_ref(n),ctx);
	} else if (is<Ast_node_kind::valdef>(n)){
		{
			auto local_ctx{ctx};
			fmt_out_layout_val_keyword(local_ctx);
			fmt_out(os,"val",local_ctx);
		}
		auto& valdef {as_valdef_ref(n)};
		auto lhs = name(valdef); 

		{
			auto local_ctx{ctx};
			fmt_out_layout_val_var(local_ctx);
			fmt_out(os,lhs,local_ctx);
		}

		{
			auto local_ctx{ctx};
			fmt_out_layout_val_keyword(local_ctx);
			local_ctx.ignore_indent = true;
			fmt_out(os,"=",local_ctx);
		}

		




		os <<  ctx.eol;
	} else if (is<Ast_node_kind::label>(n)){
		auto& label{ceps::ast::as_label_ref(n)};
		auto& attrs{ceps::ast::attributes(label)};
		std::stringstream title;
		std::string stitle;
		for(size_t i = 0; i != attrs.size(); i+=2){
			std::string what = value(ceps::ast::as_string_ref(attrs[i]));
			if (what != "title") continue;
			if (ceps::ast::is_a_string(attrs[i+1]))
				title << ceps::ast::value(ceps::ast::as_string_ref(attrs[i+1]));
		}
		if (title.str().length() == 0) stitle = ceps::ast::name(label);
		else stitle = title.str();
		{
			auto local_ctx{ctx};
			fmt_out_layout_label(local_ctx);
			fmt_out(os,stitle,local_ctx);
		}

	}
	else if (auto inner = nlf_ptr(n)){
			 fmt_out_handle_children(os, inner->children(), ctx);
	}
}

static void fmt_out_handle_children(std::ostream& os, std::vector<ceps::ast::Nodebase_ptr>& children, fmt_out_ctx ctx){
	for(auto n: children){
		if (is_a_struct(n)){
			auto& strct{ceps::ast::as_struct_ref(n)};
			fmt_out_handle_inner_struct(os,strct,ctx);
		} else if (is_a_string(n)){
			ctx.foreground_color = "78";
			fmt_out(os,"\""+value(as_string_ref(n))+"\"",ctx);
		} else if (is_an_identifier(n)){
			auto& id = as_id_ref(n);
			if("uint16" == name(id) || "uint32" == name(id) || "uint64" == name(id) || "uint8" == name(id) ||
			   "int16" == name(id) || "int32" == name(id) || "int64" == name(id) || "int8" == name(id)){
				ctx.foreground_color = "";
				fmt_out(os,name(id),ctx);
			}
		} else if (is_binop(n)){
			auto& opr = ceps::ast::as_binop_ref(n);
			if (op_val(opr) == ".."){
				auto left = opr.children()[0];
				auto right = opr.children()[1];
				auto is_int_rl = is_int(left);
				auto is_int_rr = is_int(right);

				if (is_int_rl.first && is_int_rr.first){
					ctx.foreground_color = "";
					std::stringstream s1; s1 << is_int_rl.second; 
					std::stringstream s2; s2 << is_int_rr.second;
					fmt_out(os,s1.str() + " .. " + s2.str(),ctx);
				}
			}
		} else fmt_handle_node(os, n, ctx);
	}
}

static void fmt_out_handle_inner_struct(std::ostream& os, ceps::ast::Struct& strct, fmt_out_ctx ctx){		    
	{
		auto lctx{ctx};
		fmt_out_layout_inner_strct(lctx);
		if (ceps::ast::name(strct) == "one_of"){
			lctx.italic = true;
			lctx.suffix = "";
			if (lctx.inside_schema) lctx.foreground_color = "228";
			fmt_out(os,"one of",lctx);
		} else fmt_out(os,ceps::ast::name(strct),lctx);
	}
	++ctx.indent;
	fmt_out_handle_children(os,strct.children(),ctx);
}

static void fmt_out_handle_outer_struct(std::ostream& os, ceps::ast::Struct& strct, fmt_out_ctx ctx){
	auto v = fetch_symbols_standing_alone(strct.children());
	auto is_schema = v.end() != std::find_if(v.begin(),v.end(),[](ceps::ast::Symbol* p){ return ceps::ast::kind(*p) == "Schema"; });
	ctx.inside_schema = is_schema;
	{
		auto local_ctx{ctx};
		fmt_out_layout_outer_strct(is_schema,local_ctx);
		fmt_out(os,ceps::ast::name(strct),local_ctx);
	}
	++ctx.indent;
	for(auto n: strct.children()){
		if (is<Ast_node_kind::structdef>(n)){
			fmt_out_handle_inner_struct(os,ceps::ast::as_struct_ref(n),ctx);
		}
	}
}

void fmt_out(std::ostream& os, std::vector<ceps::ast::Nodebase_ptr> const & ns,ceps::parser_env::Symboltable* symtab){
	using namespace ceps::ast;
	fmt_out_ctx ctx;
	ctx.symtab = symtab;
	for(auto n : ns){
		if (is<Ast_node_kind::structdef>(n)){
			auto&  current_struct{as_struct_ref(n)};
			fmt_out_handle_outer_struct(os,current_struct,ctx);
		} else fmt_handle_node(os,n,ctx);
	}
}
