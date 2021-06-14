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

#include "core/include/docgen/docgenerator_theme_factory.hpp"
#include <stdexcept>


using namespace ceps::ast;


 std::shared_ptr<ceps::docgen::Theme> ceps::docgen::Theme_factory(std::vector<std::string> output_format_flags){

     return          std::make_shared<ceps::docgen::map_defined_theme> (ceps::docgen::map_defined_theme{


/*inside_schema.one_of_selector -> 228 
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
function.call.name -> 184*/

            { 
                { "keyword.state_machine",color{5}},
                { "state_machine.state_name",color{148}},
                { "keyword.state_machine_states", color{144}},
                { "keyword.state_machine_transitions", color{144}},
                { "keyword.state_machine_actions", color{144}}


            }



         });
}