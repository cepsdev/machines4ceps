
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
		ctx.set_text_foreground_color("schema.outer_struct");  
		ctx.suffix = ":";
		ctx.info.push_back("schema");
	} else return fmt_out_layout_inner_strct(ctx);
}

void ceps::docgen::fmt_out_layout_inner_strct(fmt_out_ctx& ctx){
	if (ctx.inside_schema) ctx.set_text_foreground_color("schema.inner_struct");
	else ctx.set_text_foreground_color("inner_struct");
	ctx.suffix = ":";
	//ctx.info.push_back("schema");
}

void ceps::docgen::fmt_out_layout_macro_name(fmt_out_ctx& ctx){
	if (ctx.inside_schema) ctx.set_text_foreground_color("schema.macro_name");
	else ctx.set_text_foreground_color("macro_name");
	ctx.bold = true;
	ctx.italic = true;
	ctx.suffix = ":";
	//ctx.eol = "\n\n";
}

void ceps::docgen::fmt_out_layout_macro_keyword(fmt_out_ctx& ctx){
	if (ctx.inside_schema) ctx.set_text_foreground_color("schema.keyword.macro");
	else ctx.set_text_foreground_color("keyword.macro");
	ctx.bold = true;
	ctx.suffix = " ";
	ctx.prefix = "";
	ctx.eol = "";
}



void ceps::docgen::fmt_out_layout_loop_keyword(fmt_out_ctx& ctx){
	if (ctx.inside_schema) ctx.set_text_foreground_color("schema.keyword.loop");
	else ctx.set_text_foreground_color("keyword.loop");
	ctx.bold = true;
	ctx.suffix = " ";
	ctx.prefix = "";
	ctx.eol = "";
}

void ceps::docgen::fmt_out_layout_loop_in_keyword(fmt_out_ctx& ctx){
	if (ctx.inside_schema) ctx.set_text_foreground_color("schema.keyword.loop_in");
	else ctx.set_text_foreground_color("keyword.loop_in");
	ctx.bold = true;
	ctx.suffix = " ";
	ctx.prefix = "";
	ctx.eol = "";
	ctx.ignore_indent = true;
}

void ceps::docgen::fmt_out_layout_loop_variable(fmt_out_ctx& ctx){
	if (ctx.inside_schema) ctx.set_text_foreground_color("schema.keyword.loop_var");
	else ctx.set_text_foreground_color("keyword.loop_var");
	ctx.italic = true;
	ctx.suffix = " ";
	ctx.prefix = "";
	ctx.eol = "";
	ctx.ignore_indent = true;
}


void ceps::docgen::fmt_out_layout_loop_complete_line(fmt_out_ctx& ctx){
	if (ctx.inside_schema) ctx.set_text_foreground_color("schema.loop.eol");
	else ctx.set_text_foreground_color("loop.eol");
	ctx.bold = true;
	ctx.ignore_indent = true;
}

void ceps::docgen::fmt_out_layout_valdef_complete_line(fmt_out_ctx& ctx){
	if (ctx.inside_schema) ctx.set_text_foreground_color("schema.value_definition.eol");
	else ctx.set_text_foreground_color("value_definition.eol");
	ctx.ignore_indent = true;
	ctx.bold = true;
}

void ceps::docgen::fmt_out_layout_if_complete_line(fmt_out_ctx& ctx){
	if (ctx.inside_schema) ctx.set_text_foreground_color("schema.if.eol");
	else ctx.set_text_foreground_color("if.eol");
	ctx.bold = true;
	ctx.suffix = ":";
	ctx.ignore_indent = true;
}

void ceps::docgen::fmt_out_layout_val_var(fmt_out_ctx& ctx){
	if (ctx.inside_schema) ctx.set_text_foreground_color("schema.val_var");
	else ctx.set_text_foreground_color("val_var");
	ctx.italic = true;
	ctx.suffix = " ";
	ctx.prefix = "";
	ctx.eol = "";
	ctx.ignore_indent = true;
}

void ceps::docgen::fmt_out_layout_val_keyword(fmt_out_ctx& ctx){
	if (ctx.inside_schema) ctx.set_text_foreground_color("schema.keyword.val");
	else ctx.set_text_foreground_color("keyword.val");
	ctx.bold = true;
	ctx.suffix = " ";
	ctx.prefix = "";
	ctx.eol = "";
}

void ceps::docgen::fmt_out_layout_val_arrow(fmt_out_ctx& ctx){
	ctx.set_text_foreground_color("val.arrow");
	ctx.bold = true;
	ctx.suffix = " ";
	ctx.prefix = "";
	ctx.eol = "";
	ctx.ignore_indent = true;
}

void ceps::docgen::fmt_out_layout_if_keyword(fmt_out_ctx& ctx){
	if (ctx.inside_schema) ctx.set_text_foreground_color("schema.keyword.if");
	else ctx.set_text_foreground_color("keyword.if");
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
	ctx.set_text_foreground_color("function.call.name");
}


void ceps::docgen::fmt_out_layout_state_machine_keyword(fmt_out_ctx& ctx){
	ctx.set_text_foreground_color("keyword.state_machine");
	ctx.bold = true;
	ctx.suffix = " ";
	ctx.prefix = "";
	ctx.eol = "";
}

void ceps::docgen::fmt_out_layout_state_machine_actions_keyword(fmt_out_ctx& ctx){
	ctx.set_text_foreground_color("keyword.state_machine_actions");
	ctx.bold = true;
	ctx.suffix = " ";
	ctx.prefix = "";
	ctx.eol = "";
}
void ceps::docgen::fmt_out_layout_state_machine_states_keyword(fmt_out_ctx& ctx){
	ctx.set_text_foreground_color("keyword.state_machine_states");
	ctx.bold = true;
	ctx.suffix = " ";
	ctx.prefix = "";
	ctx.eol = "";
}
void ceps::docgen::fmt_out_layout_state_machine_transitions_keyword(fmt_out_ctx& ctx){
	ctx.set_text_foreground_color("keyword.state_machine_transitions");
	ctx.bold = true;
	ctx.suffix = " ";
	ctx.prefix = "";
	ctx.eol = "";
}

void ceps::docgen::fmt_out_layout_state_name(fmt_out_ctx& ctx){
	ctx.set_text_foreground_color("state_machine.state_name");
	ctx.bold = true;
	ctx.suffix = "";
	ctx.ignore_indent = true;
	ctx.prefix = "";
	ctx.eol = "";
}
