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


/*


ANSI

inside_schema.one_of_selector -> 228 
expr.func_call_target_is_id-> 229
expr.id -> 37
expr.double_literal -> 2
expr.double_literal -> 2
expr.string_literal -> 2
expr.unary_operator -> 2
expr.binary_operator -> 3
schema.outer_struct -> 214
schema.inner_struct -> 184
inner_struct -> 3
macro_name -> 4
schema.macro_name -> 4
keyword.macro -> 5
chema.keyword.macro -> 5
keyword.state_machine -> 5
schema.keyword.loop -> 5
keyword.loop -> 5
schema.keyword.loop_in -> 5
keyword.loop_in -> 5
schema.keyword.loop_var -> 6
keyword.loop_var -> 6
schema.loop.eol -> 6
loop.eol -> 6
schema.value_definition.eol -> 6
value_definition.eol -> 6
schema.if.eol -> 6
if.eol -> 6
schema.val_var -> 6
val_var -> 6
schema.keyword.val -> 5
keyword.val -> 5
val.arrow -> {}
schema.keyword.if -> 5
keyword.if -> 5
function.call.name -> 184


*/
namespace ceps{
	namespace docgen{
        using namespace ceps::ast;
        class MarginPrinter;

        struct color{
            int ansi_8bit_color = 255;
            int r = -1, g = -1, b = -1;
            color() = default;
            color(std::uint8_t ansi8bit):ansi_8bit_color{ansi8bit}{}
            color(std::uint8_t ansi8bit,std::uint8_t r, std::uint8_t g, std::uint8_t b):ansi_8bit_color{ansi8bit},r{r},g{g},b{b}{

            }
            std::string as_ansi_8bit_str() const {
                std::stringstream ss;
                ss << ansi_8bit_color;
                return ss.str();
            }

            std::string as_rgb_str(){
                std::stringstream ss;
                if (r < 16) ss << "0";
                ss << std::hex << r;
                if (g < 16) ss << "0";
                ss << std::hex << g;
                if (b < 16) ss << "0";
                ss << std::hex << b;
                return ss.str();
            }
            bool operator == (color const & rhs) const {
                return r == rhs.r && g == rhs.g && b ==rhs.b;
            }
            bool operator != (color const & rhs) const {
                return ! this->operator==(rhs);
            }

        }; 

        struct Theme{
            virtual color choose_color(std::string) = 0;
        };

        struct map_defined_theme:public Theme{
            std::map<std::string,color> m;
            color choose_color(std::string category) override{
                auto it = m.find(category);
                if (it != m.end()) return it->second;
                return color{};
            }

            map_defined_theme() = default;

            map_defined_theme(std::map<std::string,color> const & m ):m{m}{

            }
        };

        struct fmt_out_ctx {
            std::string text_foreground_color;


            void set_text_foreground_color(std::string v){text_foreground_color=v;}

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

            //std::string foreground_color_modifier;
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

        class fmt_out_ctx_stack{
            public:
            std::vector<fmt_out_ctx> fmt_stack;
            void push_ctx();
            void pop_ctx();
            fmt_out_ctx& top();
            fmt_out_ctx_stack();
        };

        class Doc_writer: public fmt_out_ctx_stack{
            protected:
            std::shared_ptr<Theme> theme;
            public:
            Doc_writer() = default;
            void set_theme(std::shared_ptr<Theme> new_theme){theme = new_theme;}
            std::shared_ptr<Theme> get_theme() const {return theme;}
            virtual void out(std::ostream& os, 
                             std::string s, 
							 MarginPrinter* mp = nullptr) = 0;
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
			Doc_writer* doc_writer;
			Comment(Struct const& strct, Doc_writer* doc_writer):strct{strct}, doc_writer{doc_writer}{
                this->doc_writer->push_ctx();
				this->doc_writer->top().comment_stmt_stack->clear();
			}
			void print(std::ostream& os) override;
            ~Comment() {doc_writer->pop_ctx();}
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
            Struct* strct; // The first state machine defining struct (sm-struct), there can be more
            std::string name; // name of the state machine, this is the very first identifier in the sm-struct
            std::vector<int> active_pointers_to_composite_ids_with_coverage_info;// p1|p2|...|p_n where p_i is an index into lookuptbls.composite_ids_with_coverage_info
            Statemachine* parent;
            struct {
                int hits = 0;
                int hits_col_width = 0;
                int max_hits = 0;
            } coverage_statistics;
			public:
            context& ctxt;
            std::vector<std::string> output_format_flags;
            ceps::parser_env::Symboltable* symtab;
			Statemachine(   Statemachine* parent,
                            Struct*  strct, 
                            context& ctxt,
                            std::vector<std::string> output_format_flags,
                            ceps::parser_env::Symboltable* symtab = nullptr
                        ):strct{strct}, parent{parent},ctxt{ctxt},output_format_flags{output_format_flags},symtab{symtab}{ 
				
                build();
			}
			void print(std::ostream& os, Doc_writer* doc_writer);
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

        void fmt_out_handle_children(std::ostream& os, std::vector<ceps::ast::Nodebase_ptr>& children, Doc_writer* doc_writer,bool ignore_macro_definitions);
        void fmt_out_handle_inner_struct(std::ostream& os, ceps::ast::Struct& strct, Doc_writer* doc_writer, bool ignore_macro_definitions);
        void fmt_out_handle_outer_struct(std::ostream& os, ceps::ast::Struct& strct, Doc_writer* doc_writer, bool ignore_macro_definitions);
        void fmt_out_handle_macro_definition(std::ostream& os, ceps::ast::Macrodef& macro, Doc_writer* doc_writer);
        void fmt_out_handle_valdef(std::ostream& os, ceps::ast::Valdef& valdef, Doc_writer* doc_writer);
        
        void fmt_out_handle_expr(std::ostream& os,Nodebase_ptr expr, Doc_writer* doc_writer,bool escape_strings= true, fmt_out_ctx ctx_base_string = {});
        void fmt_handle_node(std::ostream& os, ceps::ast::Nodebase_ptr n, Doc_writer* doc_writer,bool ignore_macro_definitions);
        void fmt_out_handle_loop(std::ostream& os, ceps::ast::Loop& loop, Doc_writer* doc_writer);
        void fmt_out_handle_let(std::ostream& os, Let& let, Doc_writer* doc_writer);
        void fmt_out_handle_ifelse(std::ostream& os, Ifelse& ifelse, Doc_writer* doc_writer);

        void fmt_out_layout_outer_strct(bool is_schema, fmt_out_ctx& ctx);
        void fmt_out_layout_inner_strct(fmt_out_ctx& ctx);
        void fmt_out_layout_macro_name(fmt_out_ctx& ctx);
        void fmt_out_layout_macro_keyword(fmt_out_ctx& ctx);

        void fmt_out_layout_state_machine_keyword(fmt_out_ctx& ctx);
        void fmt_out_layout_state_machine_actions_keyword(fmt_out_ctx& ctx);
        void fmt_out_layout_state_machine_states_keyword(fmt_out_ctx& ctx);
        void fmt_out_layout_state_machine_transitions_keyword(fmt_out_ctx& ctx);
        void fmt_out_layout_state_name(fmt_out_ctx& ctx);

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
        void print_comment(std::ostream& os, Doc_writer* doc_writer);


        std::string escape_ceps_string(std::string const & s);

	}
}


#endif