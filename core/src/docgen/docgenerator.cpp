
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
#include "core/include/docgen/docgenerator_docwriter_factory.hpp"
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

static void print_comment_impl(std::ostream& os,std::vector<Nodebase*> const & v, ceps::docgen::Doc_writer* doc_writer){
	doc_writer->push_ctx();
	doc_writer->top().ignore_comment_stmt_stack = true;
	doc_writer->top().faint_intensity = true;
	doc_writer->top().set_text_foreground_color(""); 
	doc_writer->top().italic = true;
	doc_writer->top().ignore_indent = true;
	doc_writer->top().normal_intensity = false;
	doc_writer->top().bold = false;

	for (auto n : v){		
		if (is<Ast_node_kind::stmts>(n))
			print_comment_impl(os, as_stmts_ref(n).children(), doc_writer);
		else ceps::docgen::fmt_out_handle_expr(os,n,doc_writer,false,doc_writer->top());		
	}
	doc_writer->pop_ctx();
}

void ceps::docgen::print_comment(std::ostream& os, Doc_writer* doc_writer){
	os <<  doc_writer->top().inline_comment_prefix;
	print_comment_impl(os, *doc_writer->top().comment_stmt_stack, doc_writer);
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

void ceps::docgen::fmt_out_handle_expr(std::ostream& os,Nodebase_ptr expr, Doc_writer* doc_writer,bool escape_strings, fmt_out_ctx ctx_base_string){
	if (is<Ast_node_kind::kind_def>(expr)) return;
	doc_writer->push_ctx();
	doc_writer->top().suffix = doc_writer->top().prefix = "";
	doc_writer->top().eol = 0; 
	doc_writer->top().ignore_indent = true;
	doc_writer->top().normal_intensity =true;
	if (is<Ast_node_kind::expr>(expr))
	 for(auto e : nlf_ptr(expr)->children())  
	  fmt_out_handle_expr(os,e, doc_writer, escape_strings, ctx_base_string);
	else if(is<Ast_node_kind::structdef>(expr)){
		fmt_out_handle_inner_struct(os, ceps::ast::as_struct_ref(expr), doc_writer, true );
	}
	else if (is<Ast_node_kind::binary_operator>(expr))
	{
		auto& bop{as_binop_ref(expr)};
		bool par{};
		if (op_val(bop) != "=") { par=true;doc_writer->out(os,"(");}

		if (op_val(bop) != "#") //comment operator
		{
			fmt_out_handle_expr(os,bop.left(), doc_writer, escape_strings, ctx_base_string);
			auto sop = op_val(bop);
			{
				doc_writer->push_ctx();
				doc_writer->top().set_text_foreground_color("expr.binary_operator");
				if (sop.length() > 1 || sop == ">" || sop == "<" || sop == "=") doc_writer->out(os," "+sop+" ");
					else doc_writer->out(os,sop);
				doc_writer->pop_ctx();
			}
		}
		fmt_out_handle_expr(os,bop.right(), doc_writer,escape_strings,ctx_base_string);
		if (par) doc_writer->out(os,")");
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
			doc_writer->push_ctx();
			doc_writer->top().set_text_foreground_color("expr.unary_operator"); 
	 		doc_writer->out(os,sop);
			doc_writer->pop_ctx();
		}
		fmt_out_handle_expr(os,uop.children()[0], doc_writer, escape_strings,ctx_base_string);
	} 
	else if (is<Ast_node_kind::string_literal>(expr)){
		{ auto local_ctx = ctx_base_string;
		  if (!escape_strings || !doc_writer->top().quote_string){
		    if (!escape_strings) doc_writer->out(os,value(as_string_ref(expr)));
			else doc_writer->out(os,value(as_string_ref(expr)));
		  } 
		  else {
			  doc_writer->push_ctx(); 
			  doc_writer->top().set_text_foreground_color("expr.string_literal"); 
			  doc_writer->out(os,"\""+escape_ceps_string(value(as_string_ref(expr)))+"\"");
			  doc_writer->pop_ctx();
		  } 
		}
	}		 
	else if (is<Ast_node_kind::int_literal>(expr)){
		std::stringstream ss;
		ss << value(as_int_ref(expr));
		{ 
			doc_writer->push_ctx();
			doc_writer->top().set_text_foreground_color("expr.int_literal");
			doc_writer->out(os,ss.str());
			doc_writer->pop_ctx();
		}
	} else if (is<Ast_node_kind::float_literal>(expr)){
		std::stringstream ss;
		ss << value(as_double_ref(expr));
		{
			doc_writer->push_ctx(); 
			doc_writer->top().set_text_foreground_color("expr.double_literal"); 
	    	doc_writer->out(os,ss.str());
			doc_writer->pop_ctx();
		}
	} else if (is<Ast_node_kind::identifier>(expr)){
		{
			doc_writer->push_ctx(); 
			doc_writer->top().set_text_foreground_color("expr.id");
			if (name(as_id_ref(expr)) == "Infinity") doc_writer->out(os,"∞");
			else doc_writer->out(os,name(as_id_ref(expr)));
			doc_writer->pop_ctx();
		}
	} else if (is<Ast_node_kind::symbol>(expr)){
		doc_writer->top().set_text_foreground_color("expr.symbol");
		doc_writer->out(os,name(as_symbol_ref(expr)));
	} else if (is<Ast_node_kind::func_call>(expr)){
		auto func_call = as_func_call_ref(expr);
	 	auto fcall_target = func_call_target(func_call);
		ceps::ast::Call_parameters& params = *dynamic_cast<ceps::ast::Call_parameters*>(func_call.children()[1]);
		std::vector<ceps::ast::Nodebase_ptr> args;
		if (params.children().size()) flatten_args(params.children()[0], args);
		if (is<Ast_node_kind::symbol>(fcall_target) && kind(as_symbol_ref(fcall_target)).substr(0,6) == "Docgen"){
			doc_writer->push_ctx();
			if (kind(as_symbol_ref(fcall_target)) == "DocgenWarning"){
				doc_writer->top().set_text_foreground_color("warn.sign");
				doc_writer->out(os,"⚠  ");
				doc_writer->top().set_text_foreground_color("warn.text");
			}

		 	if (args.size()){
		 		for(size_t i = 0; i != args.size();++i){
			 		fmt_out_handle_expr(os,args[i],doc_writer,false,ctx_base_string);
		 		}
			}
			doc_writer->pop_ctx();
			
		} else {
	 	
		 {
			doc_writer->push_ctx();
			doc_writer->top().set_text_foreground_color("expr.func_call_target_is_id");
			if (is_an_identifier(fcall_target)) doc_writer->out(os,name(as_id_ref(fcall_target)));
			else if (is<Ast_node_kind::symbol>(fcall_target)) doc_writer->out(os,name(as_symbol_ref(fcall_target)));

			doc_writer->pop_ctx();	 
		 }
		 doc_writer->out(os,"(");
		 if (args.size()){
		 	for(size_t i = 0; i != args.size();++i){
			 	fmt_out_handle_expr(os,args[i],doc_writer,escape_strings,ctx_base_string);
				if (i+1 < args.size())
			  	doc_writer->out(os,",");
		 	}
		 }
		 doc_writer->out(os,")");
		}
	}
	doc_writer->pop_ctx();
}

static std::vector<ceps::ast::Symbol*> fetch_symbols_standing_alone(std::vector<ceps::ast::Nodebase_ptr> const & nodes){
	std::vector<ceps::ast::Symbol*> r;
	for (auto n : nodes)
	 if (ceps::ast::is_a_symbol(n))
	 	r.push_back(ceps::ast::as_symbol_ptr(n));
	return r;
}


void ceps::docgen::fmt_out_handle_loop(std::ostream& os, ceps::ast::Loop& loop, Doc_writer* doc_writer){
	{
		doc_writer->push_ctx();
		fmt_out_layout_loop_keyword(doc_writer->top());
		doc_writer->out(os,"for each");
		doc_writer->pop_ctx();
	}
	auto& loop_head =  as_loop_head_ref(loop.children()[0]);
	ceps::ast::Nodebase_ptr body = loop.children()[1];
	ceps::ast::Identifier& id  = ceps::ast::as_id_ref(loop_head.children()[0]);
	auto loop_expr = loop_head.children()[1];
	{
		doc_writer->push_ctx();
		fmt_out_layout_loop_variable(doc_writer->top());
		doc_writer->out(os,name(id));
		doc_writer->pop_ctx();
	}
	{
		doc_writer->push_ctx();
		fmt_out_layout_loop_in_keyword(doc_writer->top());
		doc_writer->out(os,"in");
		doc_writer->pop_ctx();
	}

	fmt_out_handle_expr(os,loop_expr,doc_writer);

	{
		doc_writer->push_ctx();
		fmt_out_layout_loop_complete_line(doc_writer->top());
		doc_writer->out(os,"");
		doc_writer->pop_ctx();
	}

	++doc_writer->top().indent;
	fmt_out_handle_children(os,nlf_ptr(body)->children(),doc_writer,true);
}

void ceps::docgen::fmt_out_handle_valdef(std::ostream& os, Valdef& valdef, Doc_writer* doc_writer){
	auto lhs = name(valdef); 
	{
		doc_writer->push_ctx();
		fmt_out_layout_val_var(doc_writer->top());
		doc_writer->top().ignore_indent = false;
		doc_writer->out(os,lhs);
		doc_writer->pop_ctx();
	}
	{
		doc_writer->push_ctx();
		fmt_out_layout_val_arrow(doc_writer->top());
		doc_writer->top().ignore_indent = true;
		doc_writer->out(os,":=");
		doc_writer->pop_ctx();
	}
	fmt_out_handle_expr(os,valdef.children()[0],doc_writer);
	{
		doc_writer->push_ctx();
		fmt_out_layout_valdef_complete_line(doc_writer->top());
		doc_writer->out(os,"");
		doc_writer->pop_ctx();
	}
}

void ceps::docgen::fmt_out_handle_let(std::ostream& os, Let& let, Doc_writer* doc_writer){
	auto lhs = name(let); 
	{
		doc_writer->push_ctx();
		doc_writer->top().ignore_indent = false;
		doc_writer->out(os,lhs);
		doc_writer->pop_ctx();
	}
	{
		doc_writer->push_ctx();
		fmt_out_layout_val_arrow(doc_writer->top());
		doc_writer->top().ignore_indent = true;
		doc_writer->out(os,"←");
		doc_writer->pop_ctx();
	}
	fmt_out_handle_expr(os,let.children()[0],doc_writer);
	{
		doc_writer->push_ctx();
		fmt_out_layout_valdef_complete_line(doc_writer->top());
		doc_writer->out(os,"");
		doc_writer->pop_ctx();
	}
}

void ceps::docgen::fmt_handle_node(
	std::ostream& os, ceps::ast::Nodebase_ptr n,
	Doc_writer* doc_writer,
	bool ignore_macro_definitions, int default_eol ){
	if (is<Ast_node_kind::loop>(n)){
		fmt_out_handle_loop(os,as_loop_ref(n),doc_writer);			
	} else if (is<Ast_node_kind::macro_definition>(n)) {
		if (!ignore_macro_definitions) fmt_out_handle_macro_definition(os,as_macrodef_ref(n),doc_writer);
	} else if (is<Ast_node_kind::valdef>(n)){
		fmt_out_handle_valdef(os, as_valdef_ref(n), doc_writer);
	} else if (is<Ast_node_kind::let>(n)){
		fmt_out_handle_let(os, as_let_ref(n), doc_writer);
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
			doc_writer->push_ctx();
			fmt_out_layout_label(doc_writer->top());
			doc_writer->out(os,stitle);
			doc_writer->pop_ctx();
		}

	} else if (is<Ast_node_kind::ifelse>(n)){
		fmt_out_handle_ifelse(os,as_ifelse_ref(n),doc_writer);
 	} else if(is<Ast_node_kind::structdef>(n)){
		fmt_out_handle_inner_struct(os, ceps::ast::as_struct_ref(n), doc_writer,ignore_macro_definitions);
	} else if (!is<Ast_node_kind::stmts>(n) && !is<Ast_node_kind::scope>(n)){
		fmt_out_handle_expr(os, n, doc_writer); 
	} else if (auto inner = nlf_ptr(n)){
		fmt_out_handle_children(os, inner->children(), doc_writer,true);
	}
}

void ceps::docgen::fmt_out_handle_children( std::ostream& os, 
											std::vector<ceps::ast::Nodebase_ptr>& children, 
											Doc_writer* doc_writer, 
											bool ignore_macro_definitions,
											int default_eol){
	for(auto n: children){
		if (is_a_struct(n)){
			auto& strct{ceps::ast::as_struct_ref(n)};
			fmt_out_handle_inner_struct(os,strct,doc_writer,ignore_macro_definitions);
		} else if (is<Ast_node_kind::stmts>(n)){
			auto inner = nlf_ptr(n);
			for(auto n: inner->children())
				fmt_handle_node(os, n, doc_writer,ignore_macro_definitions,default_eol);
		}  else fmt_handle_node(os, n, doc_writer,ignore_macro_definitions,default_eol);
	}
}

//Comment

void ceps::docgen::Comment::print_section(std::ostream& os,std::vector<Nodebase_ptr> const & v, bool outer){
	for(auto e:v){
		if (is<Ast_node_kind::structdef>(e) && name(as_struct_ref(e)) == "title" ){
			{
				doc_writer->push_ctx();
				doc_writer->top().eol = 0;
				doc_writer->top().suffix = "";
				doc_writer->out(os,"");
				doc_writer->top().bold = true;			
				doc_writer->top().quote_string = false;
				doc_writer->top().ignore_indent = true;
				for(auto ee: as_struct_ref(e).children()){
					fmt_handle_node(os, ee, doc_writer,true);
				}
				doc_writer->pop_ctx();
			}			
		} else if (is<Ast_node_kind::stmts>(e)){
			print_section(os,as_stmts_ref(e).children(),false);
		}
	}
	if(!outer) return;
	{
		doc_writer->push_ctx(); 
		doc_writer->top().ignore_indent = true; 
		doc_writer->out(os,"");
		doc_writer->pop_ctx();
	}
	++doc_writer->top().indent;
	{		
		print_content(os, v);
	}
	--doc_writer->top().indent;
}

void ceps::docgen::Comment::print_block(std::ostream& os,std::vector<Nodebase_ptr> const & v, bool outer){	
	++doc_writer->top().indent;
	{
		doc_writer->push_ctx();
		doc_writer->top().eol = 0; 
		doc_writer->out(os,"");
		doc_writer->pop_ctx();
	}
	print_content(os, v);
	--doc_writer->top().indent;
	doc_writer->out(os,"");
}

void ceps::docgen::Comment::print_content(std::ostream& os, std::vector<Nodebase_ptr> const &v){
	for(auto e:v){
		if (is<Ast_node_kind::structdef>(e) && name(as_struct_ref(e)) == "section" )
			print_section(os,as_struct_ref(e).children());
	    else if (is<Ast_node_kind::structdef>(e) && name(as_struct_ref(e)) == "block" )
			print_block(os,as_struct_ref(e).children());
		else if (is<Ast_node_kind::structdef>(e) && name(as_struct_ref(e)) != "title" ){
			doc_writer->out(os,"");
			fmt_out_handle_inner_struct(os,as_struct_ref(e),doc_writer,true);			
		} else if (is<Ast_node_kind::stmts>(e))
			print_content(os,as_stmts_ref(e).children());
		else {
			doc_writer->push_ctx();
			doc_writer->top().eol = 0;
			doc_writer->top().suffix = "";
			doc_writer->top().quote_string = false;
			doc_writer->top().ignore_indent = true;
			fmt_handle_node(os, e, doc_writer,true);
			doc_writer->pop_ctx();
		}
	}
}

void ceps::docgen::Comment::print(std::ostream& os){
	print_content(os,strct.children());
}

static bool only_primitives (std::vector<ceps::ast::node_t> v, size_t& count){
	for(auto e: v){
		if (is<Ast_node_kind::stmts>(e) || is<Ast_node_kind::expr>(e)){
			auto inner = nlf_ptr(e);
			if (!only_primitives(inner->children(), count)) return false;
		} else if (!(	is<Ast_node_kind::int_literal>(e) || 
					is<Ast_node_kind::string_literal>(e) || 
					is<Ast_node_kind::float_literal>(e) || 
					is<Ast_node_kind::long_literal>(e) || 
					is<Ast_node_kind::unsigned_long_literal>(e) || 
					is<Ast_node_kind::symbol>(e) || 
					is<Ast_node_kind::identifier>(e) ) ) {
						return false;
		} else ++count;		
	}
	return true;
}

void ceps::docgen::fmt_out_handle_inner_struct(std::ostream& os, ceps::ast::Struct& strct, Doc_writer* doc_writer, bool ignore_macro_definitions ){		    
	bool lbrace = false;
	auto& nm{name(strct)};

	if (nm == "comment"){
		ceps::docgen::Comment comment{strct,doc_writer};
		comment.print(os);		
		return;
	}
	
	size_t primitives = 0;
	bool data_section  = only_primitives(children(strct), primitives);
	bool print_eol {};
	{
		doc_writer->push_ctx();
		fmt_out_layout_inner_strct(doc_writer->top());
		if (nm == "comment_stmt"){
			doc_writer->top().comment_stmt_stack->insert(std::end(*doc_writer->top().comment_stmt_stack), std::begin(strct.children()), std::end(strct.children()) );	
			return;		
		} else if (nm == "one_of"){
			doc_writer->top().italic = true;
			doc_writer->top().suffix = "";
			if (doc_writer->top().inside_schema) doc_writer->top().set_text_foreground_color("inside_schema.one_of_selector");
			doc_writer->out(os,"one of");
		} else {
			if (primitives <= 1 && data_section){
				doc_writer->top().suffix = "{";
				doc_writer->out(os,ceps::ast::name(strct)); 			
			} else {
				doc_writer->top().suffix = "{";
				doc_writer->out(os,ceps::ast::name(strct));
			}
			lbrace = true;
		}
		doc_writer->pop_ctx();
	}
	if (! (data_section && primitives <= 1 ) ) ++doc_writer->top().indent;
	
	print_eol= !data_section || primitives > 1;
	auto default_eol = data_section ? (primitives > 1, 0) : 1 ;
	doc_writer->start_line();
	doc_writer->push_ctx();
	doc_writer->top().eol = 0;

	for(auto n: children(strct)){
		if (is_a_struct(n)){
			auto& strct{ceps::ast::as_struct_ref(n)};
			fmt_out_handle_inner_struct(os,strct,doc_writer,ignore_macro_definitions);
		} else if (is<Ast_node_kind::stmts>(n)){
			auto inner = nlf_ptr(n);
			for(auto n: inner->children()){
				fmt_handle_node(os, n, doc_writer,ignore_macro_definitions,default_eol);
				if (default_eol == 0) doc_writer->out(os," ");
			}
		}  else {
			fmt_handle_node(os, n, doc_writer,ignore_macro_definitions,default_eol);
			if (default_eol == 0) doc_writer->out(os," ");
		}
	}
	doc_writer->pop_ctx();
	
	
	if (print_eol) doc_writer->out(os,"");
	if (lbrace){
		if (! (data_section && primitives <= 1 ) ) --doc_writer->top().indent;
		doc_writer->push_ctx();
		fmt_out_layout_inner_strct(doc_writer->top());
		doc_writer->top().suffix = "}";
		doc_writer->out(os,"");
		doc_writer->pop_ctx();
	}
}

void ceps::docgen::fmt_out_handle_outer_struct(	std::ostream& os, 
												ceps::ast::Struct& strct, 
												Doc_writer* doc_writer,
												bool ignore_macro_definitions){
	//Fetch top level statements which are comprised of a single symbol only
	auto v = fetch_symbols_standing_alone(strct.children());
	//Let the doc writer handle special cases, e.g. CAN Frames 
	if (doc_writer->handler_toplevel_struct(os,v,strct)) return;

	auto is_schema = v.end() != std::find_if(v.begin(),v.end(),[](ceps::ast::Symbol* p){ return ceps::ast::kind(*p) == "Schema"; });
	if (!is_schema) {
		fmt_out_handle_inner_struct(os,strct,doc_writer,ignore_macro_definitions);
		return;
	}
	doc_writer->top().inside_schema = is_schema;
	if (is_schema) doc_writer->top().c_style_struct = false;
	{
		doc_writer->push_ctx();
		fmt_out_layout_outer_strct(is_schema,doc_writer->top());
		doc_writer->out(os,ceps::ast::name(strct));
		doc_writer->pop_ctx();
	}
	++doc_writer->top().indent;
	for(auto n: strct.children()){
		if (is<Ast_node_kind::structdef>(n)){
			fmt_out_handle_inner_struct(os,ceps::ast::as_struct_ref(n),doc_writer, ignore_macro_definitions);
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

template <typename F> struct cleanup{
	F f;
	cleanup(F f): f{f}{}
	~cleanup(){f();}
};

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

	// Setup respective docwriter (determined by the format flags)
	std::shared_ptr<ceps::docgen::Doc_writer> doc_writer = Doc_writer_factory(output_format_flags);
	doc_writer->start(os);
	cleanup onexit{[&doc_writer,&os]{doc_writer->end(os);}};

	doc_writer->top().symtab = symtab; 

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
			if (name(current_struct) == "sm"){
				Statemachine sm{ nullptr,
				                 as_struct_ptr(n),
				                 lookuptbls,
								 output_format_flags,
								 symtab};
				sm.print(os,doc_writer.get());
			} else if (name(current_struct) == "Simulation" || name(current_struct) == "simulation"){
				Simulation sim{ as_struct_ptr(n),
				                lookuptbls,
								output_format_flags,
								symtab};
				sim.print(os,doc_writer.get());
			} else if (name(current_struct) == "docgen_parameters"){
				for(auto p : current_struct.children() ){
					if (is<Ast_node_kind::binary_operator>(p)){
						auto& binop = as_binop_ref(p);
						if (op_val(binop) != "=") continue;
						if (is<Ast_node_kind::symbol>(children(binop)[0]) && "Docgenparam" == kind(as_symbol_ref(children(binop)[0]))) {
							int intval = 0;
							if (is<Ast_node_kind::int_literal>(children(binop)[1]))
							 	intval = value(as_int_ref(children(binop)[1]));
							if (name(as_symbol_ref(children(binop)[0])) == "heading_level")
								doc_writer->top().heading_level = intval;
						}
					}					
				}
			}
			else fmt_out_handle_outer_struct(os,current_struct,doc_writer.get(),ignore_macro_definitions);
		} else if (is<Ast_node_kind::kind_def>(n)) {
			auto& kd{as_kinddef_ref(n)};
			auto k = kind(kd);
			for(auto id: kd.children()) lookuptbls.global_symbols.reg_id_as(name(as_id_ref(id)),k);
		} else fmt_handle_node(os,n,doc_writer.get(),ignore_macro_definitions);
		return true;
	});
	doc_writer->out(os,"");
}

///// Simulation

std::string flatten_sms_id(ceps::ast::node_t n){
	if (is<Ast_node_kind::binary_operator>(n))
	 	return flatten_sms_id( children(as_binop_ref(n))[0] ) + "." + flatten_sms_id( children(as_binop_ref(n))[1] );
	if(is<Ast_node_kind::identifier>(n))
		return name(as_id_ref(n));
	return "";
}
				
void ceps::docgen::Simulation::build(){
	bool start_directive_seen{};
	shallow_traverse(this->strct->children(), [&,this](node_t n) -> bool{
		if(!n) return true;
		if (is<Ast_node_kind::structdef>(n) && name(as_struct_ref(n)) == "title" ){
			this->title = children(as_struct_ref(n));
		}
		else if (is<Ast_node_kind::structdef>(n) && name(as_struct_ref(n)) == "Start" ){
			start_directive_seen = true;
			auto& v = children(as_struct_ref(n));
			for (auto p : v) if (is<Ast_node_kind::identifier>(p)) sms.push_back(name(as_id_ref(p)));
			else if (is<Ast_node_kind::binary_operator>(p)) sms.push_back(flatten_sms_id(p));
		} else if (start_directive_seen) steps.push_back(n);
		else steps_before_start_directive.push_back(n);
		return true;
	});
}

void ceps::docgen::Simulation::print(std::ostream& os, Doc_writer* doc_writer){
	//title first
	auto handle_step = [&](node_t step, node_t next_step){
		doc_writer->push_ctx();
		doc_writer->top().eol = 0;doc_writer->top().prefix=doc_writer->top().suffix="";
		doc_writer->start_code_block(os);
		if (is<Ast_node_kind::symbol>(step) && "Event" == kind(as_symbol_ref(step)) ){
			doc_writer->out(os,"Trigger Event");
			doc_writer->top().ignore_indent = true;

			doc_writer->out(os," ");
			doc_writer->out(os, name(as_symbol_ref(step)));
			doc_writer->out(os,".");
			doc_writer->top().ignore_indent = false;			
		} else {
			doc_writer->top().eol = 0;doc_writer->top().prefix=doc_writer->top().suffix="";
			fmt_handle_node(os,step,doc_writer,true);
		}
		if (next_step && is<Ast_node_kind::structdef>(next_step) && name(as_struct_ref(next_step)) == "comment"){
			auto& v = children(as_struct_ref(next_step));
			if (v.size()){
				doc_writer->start_comment_block(os);				
				for (auto n:v){
					if (is<Ast_node_kind::string_literal>(n)) os << value(as_string_ref(n));
					else if (is<Ast_node_kind::identifier>(n)) os << name(as_id_ref(n));
					else fmt_handle_node(os,n,doc_writer,true);
				}
				doc_writer->end_comment_block(os);
			}
		}
		doc_writer->end_code_block(os);
		doc_writer->pop_ctx();
	};


	doc_writer->push_ctx();
	++doc_writer->top().heading_level;
	doc_writer->top().heading = true;
	doc_writer->top().eol = 1;

	std::stringstream ss;
	if (!title.size()) ss << "[Simulation] ";
	for(auto n:title){
		if (is<Ast_node_kind::string_literal>(n)) ss << value(as_string_ref(n));
		else if (is<Ast_node_kind::identifier>(n)) ss << name(as_id_ref(n));
	}
	doc_writer->out(os,ss.str());
	
	++doc_writer->top().indent;

	++doc_writer->top().heading_level;
	doc_writer->out(os,"Steps");
	--doc_writer->top().heading_level;
	doc_writer->top().heading = false;
	--doc_writer->top().heading_level;

	auto foreach_step = [&](std::vector<node_t> v){
		for(size_t i = 0; i < v.size();++i){
			node_t step = v[i];
			node_t next_step = i + 1 < v.size() ? v[i+1] : nullptr; 
			handle_step( step, next_step );
			if (next_step && is<Ast_node_kind::structdef>(next_step) && name(as_struct_ref(next_step)) == "comment" ){
				++i;		
			}
		}
	};
	foreach_step(steps_before_start_directive);

	if (sms.size()){
		doc_writer->push_ctx();
		doc_writer->top().eol=0;doc_writer->top().prefix=doc_writer->top().suffix="";
		doc_writer->start_code_block(os);
		doc_writer->out(os,"Start");
		doc_writer->top().ignore_indent = true;
		if (sms.size() == 1)
			doc_writer->out(os," state machine ");
		else
		 	doc_writer->out(os," state machines ");
		for(size_t i = 0; i < sms.size();++i){
			doc_writer->out(os,sms[i]);
			if (i + 1 < sms.size())
				doc_writer->out(os,", ");					
		}
		doc_writer->out(os,".");		
		doc_writer->end_code_block(os);
		doc_writer->pop_ctx();				
	}

	foreach_step(steps);
	--doc_writer->top().indent;
	doc_writer->pop_ctx();
}


////// fmt_out_ctx_stack

ceps::docgen::fmt_out_ctx_stack::fmt_out_ctx_stack(std::vector<std::string> options){
	for(auto e: options)
	{
		if (e.substr(0,11) == "doc-option-")
		 this->options[e.substr(11)] = "";
	}
	push_ctx();
	top().comment_stmt_stack = std::make_shared<std::vector<ceps::ast::Nodebase_ptr>>(std::vector<ceps::ast::Nodebase_ptr>{});
}

void ceps::docgen::fmt_out_ctx_stack::push_ctx(){
	if (fmt_stack.size()) fmt_stack.push_back(fmt_stack.back());
	else fmt_stack.push_back(fmt_out_ctx{});
}

ceps::docgen::fmt_out_ctx& ceps::docgen::fmt_out_ctx_stack::top(){
	return fmt_stack.back();
}

void ceps::docgen::fmt_out_ctx_stack::pop_ctx(){
	if (fmt_stack.size() > 1) fmt_stack.pop_back();	
}