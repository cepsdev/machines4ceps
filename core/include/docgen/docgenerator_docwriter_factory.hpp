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


#ifndef INCMACHINES4CEPSCORE_DOCGENDOCWRITERFACTORYHPP
#define INCMACHINES4CEPSCORE_DOCGENDOCWRITERFACTORYHPP

#include "core/include/docgen/docgenerator.hpp"

namespace ceps{
	namespace docgen{
        using namespace ceps::ast;
        std::shared_ptr<Doc_writer> Doc_writer_factory(std::vector<std::string> output_format_flags);
    }
}

#endif
