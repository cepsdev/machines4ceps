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

#include "core/include/docgen/docgenerator_docwriter_markdown_jira_style.hpp"
#include <memory>
using namespace ceps::ast;

ceps::docgen::Doc_writer::eol_t ceps::docgen::Doc_writer_markdown_jira_style::eol() {return "\n"; };

void ceps::docgen::Doc_writer_markdown_jira_style::start(std::ostream& os) {}
void ceps::docgen::Doc_writer_markdown_jira_style::end(std::ostream& os) {}

bool ceps::docgen::Doc_writer_markdown_jira_style::handler_toplevel_struct( std::ostream& os,
																			std::vector<ceps::ast::Symbol*> toplevel_isolated_symbols,
                                          									ceps::ast::Struct& tplvl_struct) 
{
	return false;
}

ceps::docgen::Doc_writer_markdown_jira_style::Doc_writer_markdown_jira_style(std::vector<std::string> options):Doc_writer{options}{

}

void ceps::docgen::Doc_writer_markdown_jira_style::out(std::ostream& os, 
                             std::string s, 
							 MarginPrinter* mp) {
    auto& ctx = top(); 
	if(!ctx.ignore_indent) {
		if (mp != nullptr) 
            mp->print_left_margin(os,ctx);
		else for(int i = 0; i < ctx.indent; ++ i) 
                for(size_t j = 0; j < ctx.indent_str.size();++j) 
                 if (ctx.indent_str[j] == ' ') os << "&nbsp;";
                 else os << ctx.indent_str[j];
	}

	bool print_color_closing_tag = false;

	
	if (ctx.text_foreground_color.length()){
		auto col = theme->choose_color(ctx.text_foreground_color);
		if (col != color{}){
		 print_color_closing_tag = true;
		 os << "{color:#" << col.as_rgb_str() << "}";
		}
	} 
	
    if (s.size() + ctx.prefix.size()){ 
        if (ctx.underline) os << "+";
        if (ctx.italic) os << "_";
        if (ctx.bold) os << "*";
    }

	for(int i = 0; i < ctx.linebreaks_before;++i)
		os << eol(); 

	for(size_t i = 0; i < ctx.prefix.size();++i)
	 if (ctx.prefix[i] == '-') os << "\\-";
	 else os << ctx.prefix[i];
	
	for(size_t i = 0; i < s.size();++i)
	 if (s[i] == '-') os << "\\-";
	 else os << s[i];
	

    if (s.size() + ctx.prefix.size()){ 
        if (ctx.bold) os << "*";
        if (ctx.italic) os << "_";
        if (ctx.underline) os << "+";
    }

	if (ctx.info.size()){
		os << " (";
		for(size_t i = 0; i + 1 < ctx.info.size(); ++i)
		os << ctx.info[i] << ",";
		os << ctx.info[ctx.info.size()-1];
		os << ")";
		//if (ctx.text_foreground_color.size()) os << "\033[38;5;"<< ctx.text_foreground_color << "m";
	}
	os << ctx.suffix;
	if(print_color_closing_tag)
		 os << "{color}";

	if (ctx.eol && ctx.comment_stmt_stack->size() && !ctx.ignore_comment_stmt_stack){
		auto eol_temp = ctx.eol;
		ctx.eol = 0;
		ctx.suffix = "";
		print_comment(os,this);
		ctx.eol = eol_temp;
		ctx.comment_stmt_stack->clear();
	}
	for(auto i = 0; i < ctx.eol; ++i) os << eol();
}