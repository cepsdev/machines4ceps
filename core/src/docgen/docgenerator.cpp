
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
#include <memory>


using namespace ceps::ast;

struct fmt_out_ctx{
	bool inside_schema = false;

	bool bold                         = false;
	bool italic                       = false;
	bool underline                    = false;
	bool ignore_indent                = false;
	bool normal_intensity             = false;
	bool faint_intensity              = false;
	bool c_style_struct               = true;
	std::string inline_comment_prefix = " -- ";

	std::string foreground_color;
	std::string foreground_color_modifier;
	std::string suffix;
	std::string prefix;
	std::string eol                   ="\n";
	std::vector<std::string> info;
	std::vector<std::string> modifiers; 
    std::string indent_str            = "  ";
	int indent                        = 0;
	int linebreaks_before             = 0;
	ceps::parser_env::Symboltable* symtab = nullptr;
	bool ignore_comment_stmt_stack = false;
	std::shared_ptr<std::vector<Nodebase_ptr>> comment_stmt_stack;
};

static void fmt_out_handle_children(std::ostream& os, std::vector<ceps::ast::Nodebase_ptr>& children, fmt_out_ctx ctx);
static void fmt_out_handle_inner_struct(std::ostream& os, ceps::ast::Struct& strct, fmt_out_ctx ctx);
static void fmt_out_handle_macro_definition(std::ostream& os, ceps::ast::Macrodef& macro, fmt_out_ctx ctx);
static void fmt_out_handle_valdef(std::ostream& os, ceps::ast::Valdef& valdef, fmt_out_ctx ctx);
static void fmt_out(std::ostream& os, std::string s, fmt_out_ctx ctx);
static void fmt_out_layout_inner_strct(fmt_out_ctx& ctx);
static void fmt_out_handle_expr(std::ostream& os,Nodebase_ptr expr, fmt_out_ctx ctx,bool escape_strings= true, fmt_out_ctx ctx_base_string = {});

static void fmt_out_layout_outer_strct(bool is_schema, fmt_out_ctx& ctx){
	if (is_schema) {
		ctx.underline = true;
		ctx.foreground_color = "214";
		ctx.suffix = ":";
		ctx.info.push_back("schema");
	} else return fmt_out_layout_inner_strct(ctx);
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
	//ctx.eol = "\n\n";
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


static void fmt_out_layout_loop_complete_line(fmt_out_ctx& ctx){
	if (ctx.inside_schema) ctx.foreground_color = "6";
	else ctx.foreground_color = "6";
	ctx.bold = true;
	ctx.ignore_indent = true;
}

static void fmt_out_layout_valdef_complete_line(fmt_out_ctx& ctx){
	if (ctx.inside_schema) ctx.foreground_color = "6";
	else ctx.foreground_color = "6";
	ctx.ignore_indent = true;
	ctx.bold = true;
}

static void fmt_out_layout_if_complete_line(fmt_out_ctx& ctx){
	if (ctx.inside_schema) ctx.foreground_color = "6";
	else ctx.foreground_color = "6";
	ctx.bold = true;
	ctx.suffix = ":";
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

static void fmt_out_layout_val_arrow(fmt_out_ctx& ctx){
	ctx.foreground_color = "";
	ctx.bold = true;
	ctx.suffix = " ";
	ctx.prefix = "";
	ctx.eol = "";
	ctx.ignore_indent = true;
}

static void fmt_out_layout_if_keyword(fmt_out_ctx& ctx){
	if (ctx.inside_schema) ctx.foreground_color = "5";
	else ctx.foreground_color = "5";
	ctx.bold = true;
	ctx.suffix = " ";
	ctx.prefix = "";
	ctx.eol = "";
}

static void fmt_out_layout_label(fmt_out_ctx& ctx){
	ctx.linebreaks_before = 1;
	ctx.suffix = "";
	ctx.eol = "\n\n";
	//ctx.underline = true;
	ctx.prefix = "📎 ";
	ctx.normal_intensity = true;
}

static void print_comment_impl(std::ostream& os,std::vector<Nodebase*> const & v, fmt_out_ctx const & ctx){
	auto local_ctx_string{ctx};
	local_ctx_string.ignore_comment_stmt_stack = true;
	local_ctx_string.faint_intensity = true;
	local_ctx_string.foreground_color = "";
	local_ctx_string.italic = true;
	local_ctx_string.ignore_indent = true;
	local_ctx_string.normal_intensity = false;
	local_ctx_string.bold = false;

	for ( auto n :v){		
		if (is<Ast_node_kind::stmts>(n))
			print_comment_impl(os, as_stmts_ref(n).children(), ctx);
		else fmt_out_handle_expr(os,n,ctx,false,local_ctx_string);		
	}
}

static void print_comment(std::ostream& os, fmt_out_ctx const & ctx){
	os <<  ctx.inline_comment_prefix;
	print_comment_impl(os, *ctx.comment_stmt_stack, ctx);
}

static void flatten_args(ceps::ast::Nodebase_ptr r, std::vector<ceps::ast::Nodebase_ptr>& v, char op_val = ',')
{
	using namespace ceps::ast;
	if (r == nullptr) return;
	if (r->kind() == ceps::ast::Ast_node_kind::binary_operator && op(as_binop_ref(r)) ==  op_val)
	{
		auto& t = as_binop_ref(r);
		flatten_args(t.left(),v,op_val);
		flatten_args(t.right(),v,op_val);
		return;
	}
	v.push_back(r);
}

static void fmt_out_handle_expr(std::ostream& os,Nodebase_ptr expr, fmt_out_ctx ctx,bool escape_strings, fmt_out_ctx ctx_base_string){
	ctx.suffix = ctx.prefix = ctx.eol = ""; ctx.ignore_indent = true; ctx.foreground_color = "6";
	if (is<Ast_node_kind::expr>(expr))
	 for(auto e : nlf_ptr(expr)->children())  
	  fmt_out_handle_expr(os,e, ctx,escape_strings,ctx_base_string);
	else if (is<Ast_node_kind::binary_operator>(expr))
	{
		auto& bop{as_binop_ref(expr)};
		if (op_val(bop) != "#") //comment operator
		{
			fmt_out_handle_expr(os,bop.left(), ctx,escape_strings,ctx_base_string);
			auto sop = op_val(bop);
			{
				auto local_ctx{ctx};
				local_ctx.foreground_color ="3";
				if (sop.length() > 1 || sop == ">" || sop == "<" || sop == "=") fmt_out(os," "+sop+" ",local_ctx);
					else fmt_out(os,sop,local_ctx);
			}
		}
		fmt_out_handle_expr(os,bop.right(), ctx,escape_strings,ctx_base_string);
	} 
	else if (is<Ast_node_kind::unary_operator>(expr))
	{
		auto& uop{as_unary_op_ref(expr)};
		std::string sop = "";
		if (op(uop) == '!') sop = "!";
		else if (op(uop) == '-') sop = "-";
		else if (op(uop) == '~') sop = "~";
		else if ( (unsigned short)op(uop) <= 255) sop.push_back(op(uop));
		{
			auto local_ctx{ctx};
			local_ctx.foreground_color = "3";
	 		fmt_out(os,sop,local_ctx);
		}
		fmt_out_handle_expr(os,uop.children()[0], ctx,escape_strings,ctx_base_string);
	} 
	else if (is<Ast_node_kind::string_literal>(expr)){
		{ auto local_ctx = ctx_base_string;
		  if (escape_strings) fmt_out(os,"\""+value(as_string_ref(expr))+"\"",ctx); 
		  else fmt_out(os,value(as_string_ref(expr)),local_ctx);
		}
	}		 
	else if (is<Ast_node_kind::int_literal>(expr)){
		std::stringstream ss;
		ss << value(as_int_ref(expr));
	    fmt_out(os,ss.str(),ctx);		 
	} else if (is<Ast_node_kind::float_literal>(expr)){
		std::stringstream ss;
		ss << value(as_double_ref(expr));
		fmt_out(os,ss.str(),ctx);
	} else if (is<Ast_node_kind::identifier>(expr)){
		if (name(as_id_ref(expr)) == "Infinity") fmt_out(os,"∞",ctx);
		else fmt_out(os,name(as_id_ref(expr)),ctx);
	} else if (is<Ast_node_kind::symbol>(expr)){
		fmt_out(os,name(as_symbol_ref(expr)),ctx);
	} else if (is<Ast_node_kind::func_call>(expr)){
		auto func_call = as_func_call_ref(expr);
	 	auto fcall_target = func_call_target(func_call);
	 	if (is_an_identifier(fcall_target))
	 	{
		 //std::cout << func_call << std::endl;
		 //ceps::ast::Identifier& id = as_id_ref(fcall_target);
         //fmt_out_handle_expr(os,fcall_target,ctx);
		 {
			auto local_ctx{ctx};
			local_ctx.foreground_color ="229";
			fmt_out(os,name(as_id_ref(fcall_target)),local_ctx);	 
		 }
		 fmt_out(os,"(",ctx);
		 ceps::ast::Call_parameters& params = *dynamic_cast<ceps::ast::Call_parameters*>(func_call.children()[1]);
		 if (params.children().size()){
			std::vector<ceps::ast::Nodebase_ptr> args;
		 	flatten_args(params.children()[0], args);
		 	for(size_t i = 0; i != args.size();++i){
			 	fmt_out_handle_expr(os,args[i],ctx,escape_strings,ctx_base_string);
				if (i+1 < args.size())
			  	fmt_out(os,",",ctx);
		 	}
		 }
		 fmt_out(os,")",ctx);
		}
	}
}

static void fmt_out(std::ostream& os, std::string s, fmt_out_ctx ctx){
 if(!ctx.ignore_indent) for(int i = 0; i < ctx.indent; ++ i) os << ctx.indent_str;
 os << "\033[0m"; //reset
 if (ctx.foreground_color.size()) os << "\033[38;5;"<< ctx.foreground_color << "m";
 if (ctx.underline) os << "\033[4m";
 if (ctx.italic) os << "\033[3m";
 if (ctx.bold) os << "\033[1m";
 if (ctx.normal_intensity) os << "\033[22m";
 if (ctx.faint_intensity) os << "\033[2m";
 for(int i = 0; i < ctx.linebreaks_before;++i) os << ctx.eol; 

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
 if (ctx.eol.length() && ctx.comment_stmt_stack->size() && !ctx.ignore_comment_stmt_stack){
	 os << "\033[0m";
	 os << "\033[2m";
	 os << "\033[3m";
	 std::string eol_temp = ctx.eol;
	 ctx.eol = "";
	 ctx.suffix = "";
     print_comment(os,ctx);
	 ctx.eol = eol_temp;
	 ctx.comment_stmt_stack->clear();
 }
 os << "\033[0m"; //reset
 os << ctx.eol; 
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

	auto const & attrs = attributes(macro);
	std::stringstream title;
	std::stringstream initial;

	std::string stitle;
	std::string sinitial;
	
	for(size_t i = 0; i != attrs.size(); i+=2){
		std::string what = value(ceps::ast::as_string_ref(attrs[i]));
		auto where = &title;
		if (what == "initial") where = &initial;
		
		if (ceps::ast::is_a_string(attrs[i+1]))
			*where << ceps::ast::value(ceps::ast::as_string_ref(attrs[i+1]));
		else if (ceps::ast::is_int(attrs[i+1]).first)
			*where << ceps::ast::value(ceps::ast::as_int_ref(attrs[i+1]));
		else if (is<Ast_node_kind::float_literal>(attrs[i+1]))
			*where << ceps::ast::value(ceps::ast::as_double_ref(attrs[i+1]));
	}
	stitle = title.str();
	sinitial = initial.str();

	{
		auto local_ctx{ctx};
		fmt_out_layout_macro_keyword(local_ctx);

		fmt_out(os,sinitial.length() > 0 ? sinitial : "Macro",local_ctx);
	}
	{
		auto local_ctx{ctx};
		fmt_out_layout_macro_name(local_ctx);
		fmt_out(os,stitle.length() > 0 ? stitle : name(macro),local_ctx);
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
	//std::cout << ">>>> " << loop_head.children().size() << std::endl;
	//std::cout << *loop_head.children()[1] << std::endl;
	ceps::ast::Identifier& id  = ceps::ast::as_id_ref(loop_head.children()[0]);
	auto loop_expr = loop_head.children()[1];
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

	fmt_out_handle_expr(os,loop_expr,ctx);

	{
		auto local_ctx{ctx};
		fmt_out_layout_loop_complete_line(local_ctx);
		fmt_out(os,"",local_ctx);
	}

	++ctx.indent;
	fmt_out_handle_children(os,nlf_ptr(body)->children(),ctx);
}

static void fmt_out_handle_valdef(std::ostream& os, Valdef& valdef, fmt_out_ctx ctx){
	/*{
		auto local_ctx{ctx};
		fmt_out_layout_val_keyword(local_ctx);
		fmt_out(os,"val",local_ctx);
	}*/
	auto lhs = name(valdef); 
	{
		auto local_ctx{ctx};
		fmt_out_layout_val_var(local_ctx);
		local_ctx.ignore_indent = false;
		fmt_out(os,lhs,local_ctx);
	}
	{
		auto local_ctx{ctx};
		fmt_out_layout_val_arrow(local_ctx);
		local_ctx.ignore_indent = true;
		fmt_out(os,"←",local_ctx);
	}

    //std::cout << *valdef.children()[0] << std::endl;
	fmt_out_handle_expr(os,valdef.children()[0],ctx);

	{
		auto local_ctx{ctx};
		fmt_out_layout_valdef_complete_line(local_ctx);
		fmt_out(os,"",local_ctx);
	}
}

static void fmt_out_handle_ifelse(std::ostream& os, Ifelse& ifelse, fmt_out_ctx ctx){
	ceps::ast::Nodebase_ptr cond = ifelse.children()[0];
 	ceps::ast::Nodebase_ptr if_branch = nullptr,else_branch=nullptr;
	if (ifelse.children().size() > 1) if_branch = ifelse.children()[1];
	if (ifelse.children().size() > 2) else_branch = ifelse.children()[2];
	{
		auto local_ctx{ctx};
		fmt_out_layout_if_keyword(local_ctx);
		fmt_out(os,"if",local_ctx);
	}

	fmt_out_handle_expr(os,cond,ctx);

	{
		auto local_ctx{ctx};
		fmt_out_layout_if_complete_line(local_ctx);
		fmt_out(os,"",local_ctx);
	}

	++ctx.indent;
	if (if_branch) fmt_out_handle_children(os,nlf_ptr(if_branch)->children(),ctx);
	if (else_branch){
		--ctx.indent;
		{
			auto local_ctx{ctx};
			fmt_out_layout_if_keyword(local_ctx);
			fmt_out(os,"else",local_ctx);
		}
		{
			auto local_ctx{ctx};
			fmt_out_layout_if_complete_line(local_ctx);
			fmt_out(os,"",local_ctx);
		}
		++ctx.indent;
		fmt_out_handle_children(os,nlf_ptr(else_branch)->children(),ctx);
	}
}

static void fmt_handle_node(std::ostream& os, ceps::ast::Nodebase_ptr n, fmt_out_ctx ctx){
	if (is<Ast_node_kind::loop>(n)){
		fmt_out_handle_loop(os,as_loop_ref(n),ctx);			
	} else if (is<Ast_node_kind::macro_definition>(n)) {
		fmt_out_handle_macro_definition(os,as_macrodef_ref(n),ctx);
	} else if (is<Ast_node_kind::valdef>(n)){
		fmt_out_handle_valdef(os, as_valdef_ref(n), ctx);
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

	} else if (is<Ast_node_kind::ifelse>(n)){
		fmt_out_handle_ifelse(os,as_ifelse_ref(n),ctx);
	} else if (!is<Ast_node_kind::stmts>(n)){
		{auto local_ctx{ctx}; local_ctx.suffix = local_ctx.eol = "";fmt_out(os,"",local_ctx);}
		fmt_out_handle_expr(os, n, ctx);
		{auto local_ctx{ctx}; local_ctx.suffix = ""; local_ctx.ignore_indent = true; fmt_out(os,"",local_ctx);}
	} else if (auto inner = nlf_ptr(n)){
		fmt_out_handle_children(os, inner->children(), ctx);
	}
}

static void fmt_out_handle_children(std::ostream& os, std::vector<ceps::ast::Nodebase_ptr>& children, fmt_out_ctx ctx){
	for(auto n: children){
		if (is_a_struct(n)){
			auto& strct{ceps::ast::as_struct_ref(n)};
			fmt_out_handle_inner_struct(os,strct,ctx);
		} else fmt_handle_node(os, n, ctx);
	}
}

static void fmt_out_handle_inner_struct(std::ostream& os, ceps::ast::Struct& strct, fmt_out_ctx ctx){		    
	bool lbrace = false;

	{
		auto lctx{ctx};
		fmt_out_layout_inner_strct(lctx);
		auto& nm{name(strct)};

		if (nm == "comment_stmt"){
			ctx.comment_stmt_stack->insert(std::end(*ctx.comment_stmt_stack), std::begin(strct.children()), std::end(strct.children()) );	
			return;		
		} else if (nm == "one_of"){
			lctx.italic = true;
			lctx.suffix = "";
			if (lctx.inside_schema) lctx.foreground_color = "228";
			fmt_out(os,"one of",lctx);
		} else {
			lctx.suffix = "{";
			fmt_out(os,ceps::ast::name(strct),lctx);
			lbrace = true;
		}
	}
	++ctx.indent;
	fmt_out_handle_children(os,strct.children(),ctx);
	if (lbrace){
		--ctx.indent;
		auto lctx{ctx};
		fmt_out_layout_inner_strct(lctx);
		lctx.suffix = "}";
		fmt_out(os,"",lctx);
	}
}

static void fmt_out_handle_outer_struct(std::ostream& os, ceps::ast::Struct& strct, fmt_out_ctx ctx){
	auto v = fetch_symbols_standing_alone(strct.children());
	auto is_schema = v.end() != std::find_if(v.begin(),v.end(),[](ceps::ast::Symbol* p){ return ceps::ast::kind(*p) == "Schema"; });
	if (!is_schema) {
		fmt_out_handle_inner_struct(os,strct,ctx);
		return;
	}
	ctx.inside_schema = is_schema;
	if (is_schema) ctx.c_style_struct = false;
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
	ctx.comment_stmt_stack = std::make_shared<std::vector<ceps::ast::Nodebase_ptr>>(std::vector<ceps::ast::Nodebase_ptr>{});
	ctx.symtab = symtab;
	for(auto n : ns){
		if (is<Ast_node_kind::structdef>(n)){
			auto&  current_struct{as_struct_ref(n)};
			fmt_out_handle_outer_struct(os,current_struct,ctx);
		} else fmt_handle_node(os,n,ctx);
	}
}
