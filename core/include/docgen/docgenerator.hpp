/*
Copyright 2014,2015,2016,2017,2018,2019,2020,2021 Tomas Prerovsky (cepsdev@hotmail.com).

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
            std::string indent_str            = "  ";
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

		struct Statemachine: public Docelement{
			private:
            std::vector<std::string> states;
            std::set<std::string> actions;
            std::vector<std::string> actions_vec;
            std::map<std::string,Struct_ptr> action2body;
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
			public:
			Struct strct;
			fmt_out_ctx ctx;
            context& ctxt;
            std::vector<std::string> output_format_flags;
            ceps::parser_env::Symboltable* symtab;
			Statemachine(   Struct const& strct, 
                            fmt_out_ctx const & ctx,
                            context& ctxt,
                            std::vector<std::string> output_format_flags,
                            ceps::parser_env::Symboltable* symtab = nullptr
                        ):strct{strct}, ctx{ctx}, ctxt{ctxt},output_format_flags{output_format_flags},symtab{symtab}{ 
				this->ctx.comment_stmt_stack->clear();
                build();
			}
			void print(std::ostream& os) override;
		};

        //Entry point for Terminal output
        void fmt_out(   std::ostream& os, 
                        std::vector<ceps::ast::Nodebase_ptr> const & ns, 
                        context& ctxt,
                        std::vector<std::string> output_format_flags,
                        ceps::parser_env::Symboltable* symtab = nullptr);

        template <typename T> 
            bool shallow_traverse(std::vector<ceps::ast::Nodebase_ptr> const & ns, T f){
	            for(auto n : ns){
		            if(is<Ast_node_kind::stmts>(n) || is<Ast_node_kind::stmt>(n)) {
                        if(!shallow_traverse(nlf_ptr(n)->children(),f)) return false;
                    } else if (!f(n)) return false;
	            }
                return true;
            }
        template <typename T1, typename T2> 
            bool shallow_traverse_ex(std::vector<ceps::ast::Nodebase_ptr> const & ns, T1 f, T2 p){
	            for(auto n : ns){
		            if(p(n)) {
                        if(!shallow_traverse_ex(nlf_ptr(n)->children(),f,p)) return false;
                    } else if (!f(n)) return false;
	            }
                return true;
            }
	}
}


#endif