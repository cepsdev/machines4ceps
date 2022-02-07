/*
Copyright 2022 Tomas Prerovsky (cepsdev@hotmail.com).

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

#include "core/include/docgen/docgenerator_docwriter_markdown_minimal.hpp"
#include <memory>
using namespace ceps::ast;

/////////
// Table writer
/////////

class Table_writer_markdown_minimal: public ceps::docgen::Doc_table_writer {
	ceps::docgen::Doc_writer* parent;
	std::ostream* os;
	int rows_written {};
	int cells_written {};

    public:
		Table_writer_markdown_minimal(ceps::docgen::Doc_writer* parent, std::ostream* os) : parent{parent}, os{os} {}
    	void open_table() override;
        void close_table() override;
        void open_row() override;
        void close_row() override;
        void open_cell( bool header = false, unsigned int vert_span = 0, unsigned int horz_span = 0) override;
		void write_cell(std::string title) override;
        void close_cell() override;
		virtual ~Table_writer_markdown_minimal() {}
};

void Table_writer_markdown_minimal::write_cell(std::string title){
	
}

void Table_writer_markdown_minimal::open_table(){
	rows_written = 0;
}

void Table_writer_markdown_minimal::close_table(){
	rows_written = 0;
}

void Table_writer_markdown_minimal::open_row(){
	cells_written = 0;
	*os << " | ";
}
void Table_writer_markdown_minimal::close_row() {
	*os << "\n";
	if (rows_written == 0)
	 if (cells_written)
	 	{
			 for(int i = 0; i < cells_written; ++i) *os << "|---";
			 *os << "|\n";
		} 
	++rows_written;
}

void Table_writer_markdown_minimal::open_cell( bool header, unsigned int vert_span, unsigned int horz_span){
	++cells_written;
}

void Table_writer_markdown_minimal::close_cell(){
	*os << "|";	
}

////////
/// Doc_writer_markdown_minimal
///////

std::shared_ptr<ceps::docgen::Doc_table_writer> ceps::docgen::Doc_writer_markdown_minimal::get_table_writer(std::ostream* os) {
	if (!tblwriter)
		tblwriter = std::make_shared<Table_writer_markdown_minimal>(this,os);
	return tblwriter;
}

ceps::docgen::Doc_writer::eol_t ceps::docgen::Doc_writer_markdown_minimal::eol() {return "\n"; };

void ceps::docgen::Doc_writer_markdown_minimal::start(std::ostream& os) {}
void ceps::docgen::Doc_writer_markdown_minimal::end(std::ostream& os) {}

void ceps::docgen::Doc_writer_markdown_minimal::start_code_block(std::ostream& os){
	os << "\n```\n";
}

void ceps::docgen::Doc_writer_markdown_minimal::end_code_block(std::ostream& os){
	os << "\n```\n";
}



bool ceps::docgen::Doc_writer_markdown_minimal::handler_toplevel_struct( std::ostream& os,
																		 std::vector<ceps::ast::Symbol*> toplevel_isolated_symbols,
                                          								 ceps::ast::Struct& tplvl_struct) 
{
	return false;
}

ceps::docgen::Doc_writer_markdown_minimal::Doc_writer_markdown_minimal(std::vector<std::string> options):Doc_writer{options}{

}


void ceps::docgen::Doc_writer_markdown_minimal::out(std::ostream& os, 
                             std::string s, 
							 MarginPrinter* mp) {
    auto& ctx = top(); 
	if (ctx.heading && ctx.heading_level){
		os << "\n";
		for(auto i = 0; i != ctx.heading_level;++i)
		 os << "#";
		os << " ";
	}
	if(!ctx.ignore_indent && !ctx.heading) {
		if (mp != nullptr) 
            mp->print_left_margin(os,ctx);
		/*else for(int i = 0; i < ctx.indent; ++ i) 
                for(size_t j = 0; j < ctx.indent_str.size();++j) 
                 if (ctx.indent_str[j] == ' ') os << "&nbsp;";
                 else os << ctx.indent_str[j];*/
	}

	bool print_color_closing_tag = false;

	if (ctx.text_foreground_color.length()){
		auto col = theme->choose_color(ctx.text_foreground_color);
		if (col != color{}){
		 print_color_closing_tag = true;
		 //os << "{color:#" << col.as_rgb_str() << "}";
		}
	} 
	
    if (s.size() + ctx.prefix.size()){ 
        if (ctx.underline) os << "+";
        if (ctx.italic) os << "*";
        if (ctx.bold) os << "__";
    }

	for(int i = 0; i < ctx.linebreaks_before;++i)
		os << eol(); 

	 os << ctx.prefix;

	if (ctx.badge) os << "`";
	
	os << s;
	
    if (s.size() + ctx.prefix.size()){ 
        if (ctx.bold) os << "__";
        if (ctx.italic) os << "*";
        if (ctx.underline) os << "+";
		os << " ";
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
		 //os << "{color}"
		 ;
	if (ctx.badge) os << "`";

	if (ctx.eol && ctx.comment_stmt_stack->size() && !ctx.ignore_comment_stmt_stack){
		auto eol_temp = ctx.eol;
		ctx.eol = 0;
		ctx.suffix = "";
		print_comment(os,this);
		ctx.eol = eol_temp;
		ctx.comment_stmt_stack->clear();
	}

	if (!ctx.heading) {for(auto i = 0; i < ctx.eol;++i) os << "\n\n";}
	else if (ctx.eol) os << "\n\n"; 
}