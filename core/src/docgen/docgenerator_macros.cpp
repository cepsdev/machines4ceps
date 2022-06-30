
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

void ceps::docgen::fmt_out_handle_macro_definition(std::ostream& os, 
	ceps::ast::Macrodef& macro, Doc_writer* doc_writer){
	if (doc_writer->top().symtab == nullptr) 
		return;	

	auto const & attrs = attributes(macro);
	std::stringstream title;
	std::stringstream initial;

	std::string stitle;
	std::string sinitial;
	bool attr_initial_set = false;
	
	for(size_t i = 0; i != attrs.size(); i+=2){
		std::string what = value(ceps::ast::as_string_ref(attrs[i]));
		auto where = &title;
		if (what == "initial" || what == "type" ) {attr_initial_set = true; where = &initial;}
		
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
		doc_writer->push_ctx();
		doc_writer->top().eol= 1;
		doc_writer->out(os,"");	
		doc_writer->top().eol= 0;
		doc_writer->start_line();		
		fmt_out_layout_macro_keyword(doc_writer->top());
		auto output = attr_initial_set ? sinitial : "Macro";
		if (output.length()) doc_writer->out(os,output);
		doc_writer->pop_ctx();
	}
	{
		doc_writer->push_ctx();
		fmt_out_layout_macro_name(doc_writer->top());
		doc_writer->top().eol= 1;
		doc_writer->out(os,stitle.length() > 0 ? stitle : name(macro));
		doc_writer->pop_ctx();
	}
	++doc_writer->top().indent;

	for(auto n: as_stmts_ptr(body(macro))->children()){
		doc_writer->start_line();
		if (is_a_struct(n)){
			auto& strct{ceps::ast::as_struct_ref(n)};
			fmt_out_handle_inner_struct(os,strct,doc_writer,false);
		} else if (is<Ast_node_kind::stmts>(n)){
			auto inner = nlf_ptr(n);
			for(auto n: inner->children())
				fmt_handle_node(os, n, doc_writer,false,1);
		}  else fmt_handle_node(os, n, doc_writer,false,1);
	}
}
