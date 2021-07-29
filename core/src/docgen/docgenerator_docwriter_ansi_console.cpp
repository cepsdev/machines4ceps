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

#include "core/include/docgen/docgenerator_docwriter_ansi_console.hpp"
#include <memory>
using namespace ceps::ast;

void ceps::docgen::Doc_writer_ansi_console::start(std::ostream& os) {}
void ceps::docgen::Doc_writer_ansi_console::end(std::ostream& os) {}


bool ceps::docgen::Doc_writer_ansi_console::handler_toplevel_struct( 
																	std::ostream& os, 
																	std::vector<ceps::ast::Symbol*> toplevel_isolated_symbols,
                                          							ceps::ast::Struct& tplvl_struct) 
{
	return false;
}

void ceps::docgen::Doc_writer_ansi_console::out(std::ostream& os, 
                             std::string s, 
							 MarginPrinter* mp) {

    auto& ctx = top(); 
    os << "\033[0m"; //reset
	if(!ctx.ignore_indent) {
		if (mp != nullptr) mp->print_left_margin(os,ctx);
		else for(int i = 0; i < ctx.indent; ++ i) os << ctx.indent_str;
	}
	os << "\033[0m"; //reset
	
	if (ctx.text_foreground_color.length()) os << "\033[38;5;"<< theme->choose_color(ctx.text_foreground_color).as_ansi_8bit_str() << "m";
	
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
		
		if (ctx.text_foreground_color.size()) os << "\033[38;5;"<< theme->choose_color(ctx.text_foreground_color).as_ansi_8bit_str() << "m";
	}
	os << ctx.suffix;
	if (ctx.eol.length() && ctx.comment_stmt_stack->size() && !ctx.ignore_comment_stmt_stack){
		os << "\033[0m";
		os << "\033[2m";
		os << "\033[3m";
		std::string eol_temp = ctx.eol;
		ctx.eol = "";
		ctx.suffix = "";
		print_comment(os,this);
		ctx.eol = eol_temp;
		ctx.comment_stmt_stack->clear();
	}
	os << "\033[0m"; //reset
	os << ctx.eol; 

}