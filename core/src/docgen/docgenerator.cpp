
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

std::string ceps::docgen::escape_ceps_string(std::string const & s){
    bool transform_necessary = false;
    for(std::size_t i = 0; i!=s.length();++i){
        auto ch = s[i];
        if (ch == '\n' || ch == '\t'|| ch == '\r' || ch == '"' || ch == '\\'){
            transform_necessary = true; break;
        }
    }
    if (!transform_necessary) return s;

    std::stringstream ss;
    for(std::size_t i = 0; i!=s.length();++i){
        char buffer[2] = {0};
        char ch = buffer[0] = s[i];
        if (ch == '\n') ss << "\\n";
        else if (ch == '\t') ss << "\\t";
        else if (ch == '\r' ) ss << "\\r";
        else if (ch == '"') ss << "\\\"";
        else if (ch == '\\') ss << "\\\\";
        else ss << buffer;
    }
    return ss.str();
}

// START ceps::docgen::symbol_info

void ceps::docgen::symbol_info::push_scope(){
	id2kind_maps.push_back({});
}

void ceps::docgen::symbol_info::pop_scope(){
	if (id2kind_maps.size() > 1) id2kind_maps.pop_back();
}

void ceps::docgen::symbol_info::reg_id_as(std::string id, std::string kind){
	id2kind_maps.back()[id] = kind;	
}

std::optional<std::string> ceps::docgen::symbol_info::kind_of(std::string id){
	for(size_t i = id2kind_maps.size(); i != 0; --i){
		auto& m = id2kind_maps[i-1];
		auto it = m.find(id);
		if (it != m.end()) return it->second;
	}
	return {};
}

// END ceps::docgen::symbol_info

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
		else ceps::docgen::fmt_out_handle_expr(os,n,ctx,false,local_ctx_string);		
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

void ceps::docgen::fmt_out_handle_expr(std::ostream& os,Nodebase_ptr expr, fmt_out_ctx ctx,bool escape_strings, fmt_out_ctx ctx_base_string){
	ctx.suffix = ctx.prefix = ctx.eol = ""; ctx.ignore_indent = true; ctx.normal_intensity =true;//ctx.foreground_color = "6";
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
				if (sop.length() > 1 || sop == ">" || sop == "<" || sop == "=") formatted_out(os," "+sop+" ",local_ctx);
					else formatted_out(os,sop,local_ctx);
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
	 		formatted_out(os,sop,local_ctx);
		}
		fmt_out_handle_expr(os,uop.children()[0], ctx,escape_strings,ctx_base_string);
	} 
	else if (is<Ast_node_kind::string_literal>(expr)){
		{ auto local_ctx = ctx_base_string;
		  if (!escape_strings || !ctx.quote_string){
		    if (!escape_strings) formatted_out(os,value(as_string_ref(expr)),local_ctx);
			else formatted_out(os,value(as_string_ref(expr)),ctx);
		  } 
		  else {
			  auto local_ctx{ctx}; local_ctx.foreground_color = "2"; 
			  formatted_out(os,"\""+escape_ceps_string(value(as_string_ref(expr)))+"\"",local_ctx);
		  } 
		}
	}		 
	else if (is<Ast_node_kind::int_literal>(expr)){
		std::stringstream ss;
		ss << value(as_int_ref(expr));
		{auto local_ctx{ctx}; local_ctx.foreground_color = "2"; 
	    formatted_out(os,ss.str(),local_ctx);}
	} else if (is<Ast_node_kind::float_literal>(expr)){
		std::stringstream ss;
		ss << value(as_double_ref(expr));
		{auto local_ctx{ctx}; local_ctx.foreground_color = "2"; 
	    formatted_out(os,ss.str(),local_ctx);}
	} else if (is<Ast_node_kind::identifier>(expr)){
		{auto local_ctx{ctx}; local_ctx.foreground_color = "37";
		if (name(as_id_ref(expr)) == "Infinity") formatted_out(os,"∞",local_ctx);
		else formatted_out(os,name(as_id_ref(expr)),local_ctx);}
	} else if (is<Ast_node_kind::symbol>(expr)){
		formatted_out(os,name(as_symbol_ref(expr)),ctx);
	} else if (is<Ast_node_kind::func_call>(expr)){
		auto func_call = as_func_call_ref(expr);
	 	auto fcall_target = func_call_target(func_call);
	 	if (is_an_identifier(fcall_target))
	 	{
		 {
			auto local_ctx{ctx};
			local_ctx.foreground_color ="229";
			formatted_out(os,name(as_id_ref(fcall_target)),local_ctx);	 
		 }
		 formatted_out(os,"(",ctx);
		 ceps::ast::Call_parameters& params = *dynamic_cast<ceps::ast::Call_parameters*>(func_call.children()[1]);
		 if (params.children().size()){
			std::vector<ceps::ast::Nodebase_ptr> args;
		 	flatten_args(params.children()[0], args);
		 	for(size_t i = 0; i != args.size();++i){
			 	fmt_out_handle_expr(os,args[i],ctx,escape_strings,ctx_base_string);
				if (i+1 < args.size())
			  	formatted_out(os,",",ctx);
		 	}
		 }
		 formatted_out(os,")",ctx);
		}
	}
}

void ceps::docgen::formatted_out(std::ostream& os, 
                                 std::string s, 
								 fmt_out_ctx ctx,
								 MarginPrinter* mp){
	os << "\033[0m"; //reset
	if(!ctx.ignore_indent) {
		if (mp != nullptr) mp->print_left_margin(os,ctx);
		else for(int i = 0; i < ctx.indent; ++ i) os << ctx.indent_str;
	}
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


void ceps::docgen::fmt_out_handle_loop(std::ostream& os, ceps::ast::Loop& loop, fmt_out_ctx ctx){
	{
		auto local_ctx{ctx};
		fmt_out_layout_loop_keyword(local_ctx);
		formatted_out(os,"for each",local_ctx);
	}
	auto& loop_head =  as_loop_head_ref(loop.children()[0]);
	ceps::ast::Nodebase_ptr body = loop.children()[1];
	ceps::ast::Identifier& id  = ceps::ast::as_id_ref(loop_head.children()[0]);
	auto loop_expr = loop_head.children()[1];
	{
		auto local_ctx{ctx};
		fmt_out_layout_loop_variable(local_ctx);
		formatted_out(os,name(id),local_ctx);
	}
	{
		auto local_ctx{ctx};
		fmt_out_layout_loop_in_keyword(local_ctx);
		formatted_out(os,"in",local_ctx);
	}

	fmt_out_handle_expr(os,loop_expr,ctx);

	{
		auto local_ctx{ctx};
		fmt_out_layout_loop_complete_line(local_ctx);
		formatted_out(os,"",local_ctx);
	}

	++ctx.indent;
	fmt_out_handle_children(os,nlf_ptr(body)->children(),ctx,true);
}

void ceps::docgen::fmt_out_handle_valdef(std::ostream& os, Valdef& valdef, fmt_out_ctx ctx){
	auto lhs = name(valdef); 
	{
		auto local_ctx{ctx};
		fmt_out_layout_val_var(local_ctx);
		local_ctx.ignore_indent = false;
		formatted_out(os,lhs,local_ctx);
	}
	{
		auto local_ctx{ctx};
		fmt_out_layout_val_arrow(local_ctx);
		local_ctx.ignore_indent = true;
		formatted_out(os,":=",local_ctx);
	}

	fmt_out_handle_expr(os,valdef.children()[0],ctx);

	{
		auto local_ctx{ctx};
		fmt_out_layout_valdef_complete_line(local_ctx);
		formatted_out(os,"",local_ctx);
	}
}

void ceps::docgen::fmt_out_handle_let(std::ostream& os, Let& let, fmt_out_ctx ctx){
	auto lhs = name(let); 
	{
		auto local_ctx{ctx};
		fmt_out_layout_val_var(local_ctx);
		local_ctx.ignore_indent = false;
		formatted_out(os,lhs,local_ctx);
	}
	{
		auto local_ctx{ctx};
		fmt_out_layout_val_arrow(local_ctx);
		local_ctx.ignore_indent = true;
		formatted_out(os,"←",local_ctx);
	}

    //std::cout << *valdef.children()[0] << std::endl;
	fmt_out_handle_expr(os,let.children()[0],ctx);

	{
		auto local_ctx{ctx};
		fmt_out_layout_valdef_complete_line(local_ctx);
		formatted_out(os,"",local_ctx);
	}
}

void ceps::docgen::fmt_out_handle_ifelse(std::ostream& os, Ifelse& ifelse, fmt_out_ctx ctx){
	ceps::ast::Nodebase_ptr cond = ifelse.children()[0];
 	ceps::ast::Nodebase_ptr if_branch = nullptr,else_branch=nullptr;
	if (ifelse.children().size() > 1) if_branch = ifelse.children()[1];
	if (ifelse.children().size() > 2) else_branch = ifelse.children()[2];
	{
		auto local_ctx{ctx};
		fmt_out_layout_if_keyword(local_ctx);
		formatted_out(os,"if",local_ctx);
	}

	fmt_out_handle_expr(os,cond,ctx);

	{
		auto local_ctx{ctx};
		fmt_out_layout_if_complete_line(local_ctx);
		formatted_out(os,"",local_ctx);
	}

	++ctx.indent;
	if (if_branch) {
		//std::cout << *if_branch << std::endl;
		fmt_handle_node(os,if_branch,ctx,false);
	}
	if (else_branch){
		--ctx.indent;
		{
			auto local_ctx{ctx};
			fmt_out_layout_if_keyword(local_ctx);
			formatted_out(os,"else",local_ctx);
		}
		{
			auto local_ctx{ctx};
			fmt_out_layout_if_complete_line(local_ctx);
			formatted_out(os,"",local_ctx);
		}
		++ctx.indent;
		fmt_out_handle_children(os,nlf_ptr(else_branch)->children(),ctx,true);
	}
}

void ceps::docgen::fmt_handle_node(std::ostream& os, ceps::ast::Nodebase_ptr n, fmt_out_ctx ctx,bool ignore_macro_definitions){
	if (is<Ast_node_kind::loop>(n)){
		fmt_out_handle_loop(os,as_loop_ref(n),ctx);			
	} else if (is<Ast_node_kind::macro_definition>(n)) {
		if (!ignore_macro_definitions) fmt_out_handle_macro_definition(os,as_macrodef_ref(n),ctx);
	} else if (is<Ast_node_kind::valdef>(n)){
		fmt_out_handle_valdef(os, as_valdef_ref(n), ctx);
	} else if (is<Ast_node_kind::let>(n)){
		fmt_out_handle_let(os, as_let_ref(n), ctx);
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
			formatted_out(os,stitle,local_ctx);
		}

	} else if (is<Ast_node_kind::ifelse>(n)){
		fmt_out_handle_ifelse(os,as_ifelse_ref(n),ctx);
	} else if (!is<Ast_node_kind::stmts>(n) && !is<Ast_node_kind::scope>(n)){
		{auto local_ctx{ctx}; local_ctx.suffix = local_ctx.eol = "";formatted_out(os,"",local_ctx);}
		fmt_out_handle_expr(os, n, ctx);
		{auto local_ctx{ctx}; local_ctx.suffix = ""; local_ctx.ignore_indent = true; formatted_out(os,"",local_ctx);}
	} else if(is<Ast_node_kind::structdef>(n)){
		fmt_out_handle_inner_struct(os, ceps::ast::as_struct_ref(n), ctx,ignore_macro_definitions);
	} else if (auto inner = nlf_ptr(n)){
		fmt_out_handle_children(os, inner->children(), ctx,true);
	}
}

void ceps::docgen::fmt_out_handle_children(std::ostream& os, std::vector<ceps::ast::Nodebase_ptr>& children, fmt_out_ctx ctx, bool ignore_macro_definitions){
	for(auto n: children){
		if (is_a_struct(n)){
			auto& strct{ceps::ast::as_struct_ref(n)};
			fmt_out_handle_inner_struct(os,strct,ctx,ignore_macro_definitions);
		} else fmt_handle_node(os, n, ctx,ignore_macro_definitions);
	}
}


//Comment

void ceps::docgen::Comment::print_section(std::ostream& os,std::vector<Nodebase_ptr> const & v, bool outer){
	for(auto e:v){
		if (is<Ast_node_kind::structdef>(e) && name(as_struct_ref(e)) == "title" ){
			{
				auto local_ctx{ctx};
				local_ctx.eol = "";
				local_ctx.suffix = "";
				formatted_out(os,"",local_ctx);
				local_ctx.bold = true;			
				local_ctx.quote_string = false;
				local_ctx.ignore_indent = true;
				for(auto ee: as_struct_ref(e).children()){
					fmt_handle_node(os, ee, local_ctx,true);
				}
			}			
		} else if (is<Ast_node_kind::stmts>(e)){
			print_section(os,as_stmts_ref(e).children(),false);
		}
	}
	if(!outer) return;
	{auto local_ctx{ctx}; local_ctx.ignore_indent = true; formatted_out(os,"",local_ctx);}
	++ctx.indent;
	{		
		print_content(os, v);
	}
	--ctx.indent;
}

void ceps::docgen::Comment::print_block(std::ostream& os,std::vector<Nodebase_ptr> const & v, bool outer){	
	++ctx.indent;
	{auto local_ctx{ctx}; local_ctx.eol = ""; formatted_out(os,"",local_ctx);}
	print_content(os, v);
	--ctx.indent;
	formatted_out(os,"",ctx);
}

void ceps::docgen::Comment::print_content(std::ostream& os, std::vector<Nodebase_ptr> const &v){
	for(auto e:v){
		if (is<Ast_node_kind::structdef>(e) && name(as_struct_ref(e)) == "section" )
			print_section(os,as_struct_ref(e).children());
	    else if (is<Ast_node_kind::structdef>(e) && name(as_struct_ref(e)) == "block" )
			print_block(os,as_struct_ref(e).children());
		else if (is<Ast_node_kind::structdef>(e) && name(as_struct_ref(e)) != "title" ){
			formatted_out(os,"",ctx);fmt_out_handle_inner_struct(os,as_struct_ref(e),ctx,true);			
		} else if (is<Ast_node_kind::stmts>(e))
			print_content(os,as_stmts_ref(e).children());
		else {
			auto local_ctx{ctx};
			local_ctx.eol = "";
			local_ctx.suffix = "";
			local_ctx.quote_string = false;
			local_ctx.ignore_indent = true;
			fmt_handle_node(os, e, local_ctx,true);
		}
	}
}

void ceps::docgen::Comment::print(std::ostream& os){
	print_content(os,strct.children());
}


void ceps::docgen::fmt_out_handle_inner_struct(std::ostream& os, ceps::ast::Struct& strct, fmt_out_ctx ctx, bool ignore_macro_definitions ){		    
	bool lbrace = false;
	auto& nm{name(strct)};

	if (nm == "comment"){
		ceps::docgen::Comment comment{strct,ctx};
		comment.print(os);		
		return;
	}

	{
		auto lctx{ctx};
		fmt_out_layout_inner_strct(lctx);
		if (nm == "comment_stmt"){
			ctx.comment_stmt_stack->insert(std::end(*ctx.comment_stmt_stack), std::begin(strct.children()), std::end(strct.children()) );	
			return;		
		} else if (nm == "one_of"){
			lctx.italic = true;
			lctx.suffix = "";
			if (lctx.inside_schema) lctx.foreground_color = "228";
			formatted_out(os,"one of",lctx);
		} else {
			lctx.suffix = "{";
			formatted_out(os,ceps::ast::name(strct),lctx);
			lbrace = true;
		}
	}
	++ctx.indent;
	fmt_out_handle_children(os,strct.children(),ctx,ignore_macro_definitions);
	if (lbrace){
		--ctx.indent;
		auto lctx{ctx};
		fmt_out_layout_inner_strct(lctx);
		lctx.suffix = "}";
		formatted_out(os,"",lctx);
	}
}

void ceps::docgen::fmt_out_handle_outer_struct(std::ostream& os, ceps::ast::Struct& strct, fmt_out_ctx ctx,bool ignore_macro_definitions){
	auto v = fetch_symbols_standing_alone(strct.children());
	auto is_schema = v.end() != std::find_if(v.begin(),v.end(),[](ceps::ast::Symbol* p){ return ceps::ast::kind(*p) == "Schema"; });
	if (!is_schema) {
		fmt_out_handle_inner_struct(os,strct,ctx,ignore_macro_definitions);
		return;
	}
	ctx.inside_schema = is_schema;
	if (is_schema) ctx.c_style_struct = false;
	{
		auto local_ctx{ctx};
		fmt_out_layout_outer_strct(is_schema,local_ctx);
		formatted_out(os,ceps::ast::name(strct),local_ctx);
	}
	++ctx.indent;
	for(auto n: strct.children()){
		if (is<Ast_node_kind::structdef>(n)){
			fmt_out_handle_inner_struct(os,ceps::ast::as_struct_ref(n),ctx, ignore_macro_definitions);
		}
	}
}




int flatten_composite_id(node_t root,std::vector<node_t>& dest){
	if (root == nullptr) return 0;
	if (is<Ast_node_kind::identifier>(root)) { dest.push_back(root); return 1; }
	if (is<Ast_node_kind::binary_operator>(root)){
		auto& oper = as_binop_ref(root);
		if (op_val(oper) != ".") return -1;
		auto lhs = flatten_composite_id(oper.left(), dest); if (lhs < 0) return lhs;
		auto rhs = flatten_composite_id(oper.right(), dest); if (rhs < 0) return rhs;
		return lhs + rhs;
	}
	return -1;
}

//
// Enry point for formatted console output of:
//  - unevaluated (raw) ceps trees
//  - evaluated ceps trees which includes evaluated ceps trees in different stages (e.g. post processing)
//
void ceps::docgen::fmt_out(	std::ostream& os, 
							std::vector<ceps::ast::Nodebase_ptr> const & ns,
							context& lookuptbls,
							std::vector<std::string> output_format_flags,
							bool ignore_macro_definitions,
							ceps::parser_env::Symboltable* symtab)
{
	using namespace ceps::ast;
	fmt_out_ctx ctx;
	ctx.comment_stmt_stack = std::make_shared<std::vector<ceps::ast::Nodebase_ptr>>(std::vector<ceps::ast::Nodebase_ptr>{});
	ctx.symtab = symtab;

	// Preprocess:
	// - Find coverage summary (if it exists)
	if (lookuptbls.coverage_summary == nullptr) shallow_traverse(ns, [&](node_t n) -> bool{
		if (is<Ast_node_kind::structdef>(n)){
			auto&  strct{as_struct_ref(n)};
			auto v = fetch_symbols_standing_alone(strct.children());
			auto is_coverage_report = v.end() != std::find_if(v.begin(),v.end(),[](ceps::ast::Symbol* p){ return ceps::ast::kind(*p) == "@@coverage_summary"; });
			if (is_coverage_report){
				lookuptbls.coverage_summary = as_struct_ptr(n);
				//find coverage struct
				lookuptbls.covered_states = as_struct_ptr(get_node_by_path({"coverage","state_coverage","covered_states"},strct.children()));
				lookuptbls.covered_states_visit_count = as_struct_ptr(get_node_by_path({"coverage","state_coverage","covered_states_visit_count"},strct.children()));				
				return false;
			}
		}
		return true;
	} );

	if (lookuptbls.coverage_summary != nullptr && lookuptbls.composite_ids_with_coverage_info.size() == 0){
		//compute lookup tables for coverage info 
		//lookuptbls.covered_states
		std::vector<node_t> composite_ids;
		for(size_t i = 0; i < lookuptbls.covered_states->children().size(); ++i){
			auto l = composite_ids.size(); 
			composite_ids.push_back(nullptr);
			if ( 0 > flatten_composite_id(lookuptbls.covered_states->children()[i],composite_ids)){
				composite_ids.erase(composite_ids.begin()+l,composite_ids.end());
			} else {
				composite_ids.push_back(nullptr);
				if (lookuptbls.covered_states_visit_count->children().size() > i) 
				 composite_ids.push_back(lookuptbls.covered_states_visit_count->children()[i]);
				composite_ids.push_back(nullptr);
			}
		}
		lookuptbls.composite_ids_with_coverage_info = std::move(composite_ids);
	}
	
	shallow_traverse(ns, [&](Nodebase_ptr n) -> bool{
		if (is<Ast_node_kind::structdef>(n)){
			if (lookuptbls.coverage_summary == n) return true;
			auto&  current_struct{as_struct_ref(n)};
			if (name(current_struct) == "Simulation") return true;

			if (name(current_struct) == "sm"){
				Statemachine sm{ nullptr,
				                 as_struct_ptr(n),
				                 lookuptbls,
								 output_format_flags,
								 symtab};
				sm.print(os,ctx);
			}
			else fmt_out_handle_outer_struct(os,current_struct,ctx,ignore_macro_definitions);
		} else if (is<Ast_node_kind::kind_def>(n)) {
			auto& kd{as_kinddef_ref(n)};
			auto k = kind(kd);
			for(auto id: kd.children()) lookuptbls.global_symbols.reg_id_as(name(as_id_ref(id)),k);
		} else fmt_handle_node(os,n,ctx,ignore_macro_definitions);
		return true;
	});
}
