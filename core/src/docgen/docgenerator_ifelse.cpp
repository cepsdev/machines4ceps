
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

void ceps::docgen::fmt_out_handle_ifelse(std::ostream& os, Ifelse& ifelse, Doc_writer* doc_writer){
    //std::cerr << "*** " << ifelse << std::endl; 
    //std::cerr << "*** " << ifelse.children().size() << std::endl; 
	ceps::ast::Nodebase_ptr cond = ifelse.children()[0];
 	ceps::ast::Nodebase_ptr if_branch = nullptr,else_branch=nullptr;
	if (ifelse.children().size() > 1) if_branch = ifelse.children()[1];
	if (ifelse.children().size() > 2) else_branch = ifelse.children()[2];
	{
		doc_writer->push_ctx();
		fmt_out_layout_if_keyword(doc_writer->top());
		doc_writer->out(os,"if");
		doc_writer->pop_ctx();
	}

	fmt_out_handle_expr(os,cond,doc_writer);

	{
		doc_writer->push_ctx();
		fmt_out_layout_if_complete_line(doc_writer->top());
		doc_writer->out(os,"");
		doc_writer->pop_ctx();
	}

	if (if_branch) {
        ++doc_writer->top().indent;
		//std::cerr << "---------";
		doc_writer->start_line();
		fmt_handle_node(os,if_branch,doc_writer,false);
		doc_writer->out(os,"");
        --doc_writer->top().indent;
	}
	while (else_branch && is<Ast_node_kind::ifelse>(else_branch)){
        auto& else_branch_as_if = ceps::ast::as_ifelse_ref(else_branch);

        cond = else_branch_as_if.children()[0];
 	    if_branch = else_branch_as_if.children().size()  > 1 ? else_branch_as_if.children()[1] : nullptr;
        else_branch= else_branch_as_if.children().size()  > 2 ? else_branch_as_if.children()[2] : nullptr;

		{
			doc_writer->push_ctx();
			fmt_out_layout_if_keyword(doc_writer->top());
			doc_writer->out(os,"elif");
			doc_writer->pop_ctx();
		}
        fmt_out_handle_expr(os,cond,doc_writer);
		{
			doc_writer->push_ctx();
			fmt_out_layout_if_complete_line(doc_writer->top());
			doc_writer->out(os,"");
			doc_writer->pop_ctx();
		}
        if (if_branch) {
            ++doc_writer->top().indent;	
		    fmt_handle_node(os,if_branch,doc_writer,false);
            --doc_writer->top().indent;
	    }
		//fmt_out_handle_children(os,nlf_ptr(else_branch)->children(),doc_writer,true);
	}
    if (else_branch){
		{
			doc_writer->push_ctx();
			fmt_out_layout_if_keyword(doc_writer->top());
			doc_writer->out(os,"else");
			doc_writer->pop_ctx();
		}
		{
			doc_writer->push_ctx();
			fmt_out_layout_if_complete_line(doc_writer->top());
			doc_writer->out(os,"");
			doc_writer->pop_ctx();
		}
        ++doc_writer->top().indent;
    	fmt_handle_node(os,else_branch,doc_writer,false);	
        --doc_writer->top().indent;	
    }
	//--doc_writer->top().indent;
}
