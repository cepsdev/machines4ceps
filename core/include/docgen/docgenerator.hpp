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


#ifndef INCSM4CEPSCORE_DOCGENHPP
#define INCSM4CEPSCORE_DOCGENHPP

#include <string> 
#include <vector>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <set>
#include <unordered_set>
#include <chrono>
#include <thread>
#include <mutex>
#include <optional>
#include <condition_variable>
#include "ceps_all.hh"

namespace ceps{
	namespace docgen{
        using namespace ceps::ast;
        struct fmt_out_ctx{
            bool inside_schema = false;

            bool bold                         = false;
            bool italic                       = false;
            bool underline                    = false;
            bool ignore_indent                = false;
            bool normal_intensity             = false;
            bool faint_intensity              = false;
            bool c_style_struct               = true;
            bool quote_string                 = true;
            std::string inline_comment_prefix = " -- ";

            std::string foreground_color;
            std::string foreground_color_modifier;
            std::string suffix;
            std::string prefix;
            std::string eol                   ="\n";
            std::vector<std::string> info;
            std::vector<std::string> modifiers; 
            std::string indent_str            = " ";
            int indent                        = 0;
            int linebreaks_before             = 0;
            ceps::parser_env::Symboltable* symtab = nullptr;
            bool ignore_comment_stmt_stack = false;
            std::shared_ptr<std::vector<ceps::ast::Nodebase_ptr>> comment_stmt_stack;
        };

        struct symbol_info{
            std::vector<std::map<std::string,std::string>> id2kind_maps;
            void reg_id_as(std::string id, std::string kind);
            std::optional<std::string> kind_of(std::string id);
            void push_scope();
            void pop_scope();
            symbol_info() {id2kind_maps.push_back({});}
        };
        struct sm_info{
            struct state{
                std::string name;
                Nodebase_ptr parent; //nullptr for toplevel states
                std::vector<std::shared_ptr<state>> substates;
                std::vector<Nodebase_ptr> actions;
            };
        };

        struct context{
            symbol_info global_symbols;
            sm_info state_machines;
            ceps::ast::Struct* coverage_summary = nullptr;
            ceps::ast::Struct* covered_states = nullptr;
            ceps::ast::Struct* covered_states_visit_count = nullptr;
            std::vector<node_t> composite_ids_with_coverage_info; // a.b.c hits = 2 => NULL|POINTER TO a|POINTER TO b|POINTER TO c|NULL|POINTERR TO 2|NULL
        };

		struct Docelement{
			virtual void print(std::ostream& os) = 0;
		};
		struct Comment: public Docelement{
			private:
			void print_block(std::ostream& os,std::vector<ceps::ast::Nodebase_ptr> const &, bool outer = true);
			void print_section(std::ostream& os,std::vector<ceps::ast::Nodebase_ptr> const &, bool outer = true);
			void print_content(std::ostream& os, std::vector<ceps::ast::Nodebase_ptr> const &);
			public:
			Struct strct;
			fmt_out_ctx ctx;
			Comment(Struct const& strct, fmt_out_ctx const & ctx):strct{strct}, ctx{ctx}{ 
				this->ctx.comment_stmt_stack->clear();
			}
			void print(std::ostream& os) override;
		};

        class MarginPrinter{
            protected:
            int left_margin = 0;
            public:
            virtual void print_left_margin (std::ostream& os, fmt_out_ctx& ctx) = 0;
        };

		class Statemachine : public MarginPrinter{
			private:
            std::vector<std::string> states;
            std::set<std::string> actions;
            std::vector<std::string> actions_vec;
            std::map<std::string,Struct_ptr> action2body;
            std::vector<std::shared_ptr<Statemachine>> sub_machines;
            struct transition{
                Nodebase_ptr from = nullptr;
                Nodebase_ptr to = nullptr;
                std::vector<Nodebase_ptr> actions;
                Nodebase_ptr ev = nullptr;
                std::vector<Nodebase_ptr> guards;
            };
            std::vector<transition> transitions;
            void build();
            std::string name;
            std::vector<int> active_pointers_to_composite_ids_with_coverage_info;// p1|p2|...|p_n where p_i is an index into lookuptbls.composite_ids_with_coverage_info
            Statemachine* parent;
            struct {
                int hits = 0;
                int hits_col_width = 0;
                int max_hits = 0;
            } coverage_statistics;
			public:
			Struct strct;
            context& ctxt;
            std::vector<std::string> output_format_flags;
            ceps::parser_env::Symboltable* symtab;
			Statemachine(   Statemachine* parent,
                            Struct const& strct, 
                            context& ctxt,
                            std::vector<std::string> output_format_flags,
                            ceps::parser_env::Symboltable* symtab = nullptr
                        ):parent{parent},strct{strct}, ctxt{ctxt},output_format_flags{output_format_flags},symtab{symtab}{ 
				
                build();
			}
			void print(std::ostream& os, fmt_out_ctx& ctx);
            void print_left_margin (std::ostream& os, fmt_out_ctx& ctx) override ;
		};

        //Entry point for Terminal output
        void fmt_out(   std::ostream& os, 
                        std::vector<ceps::ast::Nodebase_ptr> const & ns, 
                        context& ctxt,
                        std::vector<std::string> output_format_flags,
                        bool ignore_macro_definitions,
                        ceps::parser_env::Symboltable* symtab = nullptr);

        //Helper routines

        void fmt_out_handle_children(std::ostream& os, std::vector<ceps::ast::Nodebase_ptr>& children, fmt_out_ctx ctx,bool ignore_macro_definitions);
        void fmt_out_handle_inner_struct(std::ostream& os, ceps::ast::Struct& strct, fmt_out_ctx ctx, bool ignore_macro_definitions);
        void fmt_out_handle_outer_struct(std::ostream& os, ceps::ast::Struct& strct, fmt_out_ctx ctx, bool ignore_macro_definitions);
        void fmt_out_handle_macro_definition(std::ostream& os, ceps::ast::Macrodef& macro, fmt_out_ctx ctx);
        void fmt_out_handle_valdef(std::ostream& os, ceps::ast::Valdef& valdef, fmt_out_ctx ctx);
        void formatted_out(std::ostream& os, std::string s, fmt_out_ctx ctx, MarginPrinter* mp = nullptr);
        void fmt_out_handle_expr(std::ostream& os,Nodebase_ptr expr, fmt_out_ctx ctx,bool escape_strings= true, fmt_out_ctx ctx_base_string = {});
        void fmt_handle_node(std::ostream& os, ceps::ast::Nodebase_ptr n, fmt_out_ctx ctx,bool ignore_macro_definitions);
        void fmt_out_handle_loop(std::ostream& os, ceps::ast::Loop& loop, fmt_out_ctx ctx);
        void fmt_out_handle_let(std::ostream& os, Let& let, fmt_out_ctx ctx);
        void fmt_out_handle_ifelse(std::ostream& os, Ifelse& ifelse, fmt_out_ctx ctx);

        void fmt_out_layout_outer_strct(bool is_schema, fmt_out_ctx& ctx);
        void fmt_out_layout_inner_strct(fmt_out_ctx& ctx);
        void fmt_out_layout_macro_name(fmt_out_ctx& ctx);
        void fmt_out_layout_macro_keyword(fmt_out_ctx& ctx);
        void fmt_out_layout_state_machine_keyword(fmt_out_ctx& ctx);
        void fmt_out_layout_loop_keyword(fmt_out_ctx& ctx);
        void fmt_out_layout_loop_in_keyword(fmt_out_ctx& ctx);
        void fmt_out_layout_loop_variable(fmt_out_ctx& ctx);
        void fmt_out_layout_loop_complete_line(fmt_out_ctx& ctx);
        void fmt_out_layout_valdef_complete_line(fmt_out_ctx& ctx);
        void fmt_out_layout_if_complete_line(fmt_out_ctx& ctx);
        void fmt_out_layout_val_var(fmt_out_ctx& ctx);
        void fmt_out_layout_val_keyword(fmt_out_ctx& ctx);
        void fmt_out_layout_val_arrow(fmt_out_ctx& ctx);
        void fmt_out_layout_if_keyword(fmt_out_ctx& ctx);
        void fmt_out_layout_label(fmt_out_ctx& ctx);
        void fmt_out_layout_funcname(fmt_out_ctx& ctx);


        std::string escape_ceps_string(std::string const & s);

	}
}


#endif