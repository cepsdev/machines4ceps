
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

void ceps::docgen::fmt_out_handle_macro_definition(std::ostream& os, ceps::ast::Macrodef& macro, Doc_writer* doc_writer){
	if (doc_writer->top().symtab == nullptr) 
		return;
	auto symbol = doc_writer->top().symtab->lookup(name(macro),false,false,false);
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
		doc_writer->push_ctx();
		fmt_out_layout_macro_keyword(doc_writer->top());
		doc_writer->out(os,sinitial.length() > 0 ? sinitial : "Macro");
		doc_writer->pop_ctx();
	}
	{
		doc_writer->push_ctx();
		fmt_out_layout_macro_name(doc_writer->top());
		doc_writer->out(os,stitle.length() > 0 ? stitle : name(macro));
		doc_writer->pop_ctx();
	}
	++doc_writer->top().indent;
	fmt_out_handle_children(os,as_stmts_ptr(static_cast<Nodebase_ptr>(symbol->payload))->children(),doc_writer,true);
}
