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


#pragma once

#include "core/include/docgen/docgenerator.hpp"

namespace ceps{
	namespace docgen{
        using namespace ceps::ast;
        class Doc_writer_markdown_minimal: public Doc_writer{
            private:
                std::shared_ptr<Doc_table_writer> tblwriter;
            public:
            Doc_writer_markdown_minimal() = delete;
            Doc_writer_markdown_minimal(std::vector<std::string> options);
            void out(std::ostream& os, 
                             std::string s, 
							 MarginPrinter* mp) override;
            bool handler_toplevel_struct(   std::ostream& os,
                                            std::vector<ceps::ast::Symbol*> toplevel_isolated_symbols,
                                            ceps::ast::Struct& tplvl_struct) override;
            void start(std::ostream& os) override; 
            void end(std::ostream& os) override; 
            eol_t eol() override;
            bool supports_tables() override {return true;}
            std::shared_ptr<Doc_table_writer> get_table_writer(std::ostream* os) override;
            void start_code_block(std::ostream& os) override;
            void end_code_block(std::ostream& os) override;
            void start_header(int lvl, std::ostream& os) override;
        };
    }
}