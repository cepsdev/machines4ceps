
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


void ceps::docgen::fmt_out_layout_outer_strct(bool is_schema, ceps::docgen::fmt_out_ctx& ctx){
	if (is_schema) {
		ctx.underline = true;
		ctx.foreground_color = "214";
		ctx.suffix = ":";
		ctx.info.push_back("schema");
	} else return fmt_out_layout_inner_strct(ctx);
}

void ceps::docgen::fmt_out_layout_inner_strct(fmt_out_ctx& ctx){
	if (ctx.inside_schema) ctx.foreground_color = "184";
	else ctx.foreground_color = "3";
	ctx.suffix = ":";
	//ctx.info.push_back("schema");
}

void ceps::docgen::fmt_out_layout_macro_name(fmt_out_ctx& ctx){
	if (ctx.inside_schema) ctx.foreground_color = "4";
	else ctx.foreground_color = "4";
	ctx.bold = true;
	ctx.italic = true;
	ctx.suffix = ":";
	//ctx.eol = "\n\n";
}

void ceps::docgen::fmt_out_layout_macro_keyword(fmt_out_ctx& ctx){
	if (ctx.inside_schema) ctx.foreground_color = "5";
	else ctx.foreground_color = "5";
	ctx.bold = true;
	ctx.suffix = " ";
	ctx.prefix = "";
	ctx.eol = "";
}

void ceps::docgen::fmt_out_layout_state_machine_keyword(fmt_out_ctx& ctx){
	ctx.foreground_color = "5";
	ctx.bold = true;
	ctx.suffix = " ";
	ctx.prefix = "";
	ctx.eol = "";
}

void ceps::docgen::fmt_out_layout_loop_keyword(fmt_out_ctx& ctx){
	if (ctx.inside_schema) ctx.foreground_color = "5";
	else ctx.foreground_color = "5";
	ctx.bold = true;
	ctx.suffix = " ";
	ctx.prefix = "";
	ctx.eol = "";
}

void ceps::docgen::fmt_out_layout_loop_in_keyword(fmt_out_ctx& ctx){
	if (ctx.inside_schema) ctx.foreground_color = "5";
	else ctx.foreground_color = "5";
	ctx.bold = true;
	ctx.suffix = " ";
	ctx.prefix = "";
	ctx.eol = "";
	ctx.ignore_indent = true;
}

void ceps::docgen::fmt_out_layout_loop_variable(fmt_out_ctx& ctx){
	if (ctx.inside_schema) ctx.foreground_color = "6";
	else ctx.foreground_color = "6";
	ctx.italic = true;
	ctx.suffix = " ";
	ctx.prefix = "";
	ctx.eol = "";
	ctx.ignore_indent = true;
}


void ceps::docgen::fmt_out_layout_loop_complete_line(fmt_out_ctx& ctx){
	if (ctx.inside_schema) ctx.foreground_color = "6";
	else ctx.foreground_color = "6";
	ctx.bold = true;
	ctx.ignore_indent = true;
}

void ceps::docgen::fmt_out_layout_valdef_complete_line(fmt_out_ctx& ctx){
	if (ctx.inside_schema) ctx.foreground_color = "6";
	else ctx.foreground_color = "6";
	ctx.ignore_indent = true;
	ctx.bold = true;
}

void ceps::docgen::fmt_out_layout_if_complete_line(fmt_out_ctx& ctx){
	if (ctx.inside_schema) ctx.foreground_color = "6";
	else ctx.foreground_color = "6";
	ctx.bold = true;
	ctx.suffix = ":";
	ctx.ignore_indent = true;
}

void ceps::docgen::fmt_out_layout_val_var(fmt_out_ctx& ctx){
	if (ctx.inside_schema) ctx.foreground_color = "6";
	else ctx.foreground_color = "6";
	ctx.italic = true;
	ctx.suffix = " ";
	ctx.prefix = "";
	ctx.eol = "";
	ctx.ignore_indent = true;
}

void ceps::docgen::fmt_out_layout_val_keyword(fmt_out_ctx& ctx){
	if (ctx.inside_schema) ctx.foreground_color = "5";
	else ctx.foreground_color = "5";
	ctx.bold = true;
	ctx.suffix = " ";
	ctx.prefix = "";
	ctx.eol = "";
}

void ceps::docgen::fmt_out_layout_val_arrow(fmt_out_ctx& ctx){
	ctx.foreground_color = "";
	ctx.bold = true;
	ctx.suffix = " ";
	ctx.prefix = "";
	ctx.eol = "";
	ctx.ignore_indent = true;
}

void ceps::docgen::fmt_out_layout_if_keyword(fmt_out_ctx& ctx){
	if (ctx.inside_schema) ctx.foreground_color = "5";
	else ctx.foreground_color = "5";
	ctx.bold = true;
	ctx.suffix = " ";
	ctx.prefix = "";
	ctx.eol = "";
}

void ceps::docgen::fmt_out_layout_label(fmt_out_ctx& ctx){
	ctx.linebreaks_before = 1;
	ctx.suffix = "";
	ctx.eol = "\n\n";
	//ctx.underline = true;
	ctx.prefix = "📎 ";
	ctx.normal_intensity = true;
}

void ceps::docgen::fmt_out_layout_funcname(fmt_out_ctx& ctx){
	ctx.suffix = "";
	ctx.eol = "";
	ctx.prefix = "";
	ctx.normal_intensity = true;
	ctx.suffix = "";
	ctx.ignore_indent = true;
	ctx.foreground_color = "229";
}
