/*
Copyright 2021,22 Tomas Prerovsky (cepsdev@hotmail.com).

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

#include "core/include/docgen/docgenerator_docwriter_factory.hpp"
#include "core/include/docgen/docgenerator_theme_factory.hpp"
#include "core/include/docgen/docgenerator_docwriter_ansi_console.hpp"
#include "core/include/docgen/docgenerator_docwriter_markdown_jira_style.hpp"
#include "core/include/docgen/docgenerator_docwriter_markdown_github_style.hpp"
#include "core/include/docgen/docgenerator_docwriter_html5.hpp"
#include "core/include/docgen/docgenerator_docwriter_markdown_minimal.hpp"
#include <stdexcept>

using namespace ceps::ast;

std::shared_ptr<ceps::docgen::Doc_writer> ceps::docgen::Doc_writer_factory(std::vector<std::string> output_format_flags){
    std::shared_ptr<ceps::docgen::Doc_writer> r;

    for(auto e : output_format_flags)
     if (e == "ansi") 
        r = std::make_shared<Doc_writer_ansi_console>(Doc_writer_ansi_console{output_format_flags});
     else if (e == "markdown_jira" || e == "markdown_jira_style") 
        r = std::make_shared<Doc_writer_markdown_jira_style>(Doc_writer_markdown_jira_style{output_format_flags});
     else if (e == "markdown_github" || e == "markdown_github_style") 
        r = std::make_shared<Doc_writer_markdown_github_style>(Doc_writer_markdown_github_style{output_format_flags});
      else if (e == "markdown" || e == "markdown_minimal") 
        r = std::make_shared<Doc_writer_markdown_minimal>(Doc_writer_markdown_minimal{output_format_flags});        
     else if (e == "html" || e == "html5") 
        r = std::make_shared<Doc_writer_html5>(Doc_writer_html5{output_format_flags});     
     if (!r)  
        r = std::make_shared<Doc_writer_ansi_console>(Doc_writer_ansi_console{output_format_flags});
     r -> set_theme(ceps::docgen::Theme_factory(output_format_flags));
     return r;
}