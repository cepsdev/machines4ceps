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


#ifndef INCMACHINES4CEPSCORE_DOCGENDOCWRITERMARKDOWNGITHUBSTYLEHPP
#define INCMACHINES4CEPSCORE_DOCGENDOCWRITERMARKDOWNGITHUBSTYLEHPP

#include "core/include/docgen/docgenerator.hpp"

namespace ceps{
	namespace docgen{
        using namespace ceps::ast;

        class Doc_writer_markdown_github_style: public Doc_writer{
            public:
            Doc_writer_markdown_github_style() = delete;
            Doc_writer_markdown_github_style(std::vector<std::string> options);
            void out(std::ostream& os, 
                             std::string s, 
							 MarginPrinter* mp) override;
            bool handler_toplevel_struct(   std::ostream& os,
                                            std::vector<ceps::ast::Symbol*> toplevel_isolated_symbols,
                                            ceps::ast::Struct& tplvl_struct) override;
            void start(std::ostream& os) override; 
            void end(std::ostream& os) override; 
            eol_t eol() override;
            symbol_t right_arrow() override;
            void start_code_block(std::ostream& os) override;
            void end_code_block(std::ostream& os) override;
            void start_comment_block(std::ostream& os) override;
            void end_comment_block(std::ostream& os) override;
            bool no_nesting() override;
            void start_header(int lvl, std::ostream& os) override;

        };
    }
}

#endif
