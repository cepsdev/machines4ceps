/*
Copyright 2015, cepsdev (cepsdev@hotmail.com).
All rights reserved.
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.
    * Neither the name of Google Inc. nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#include "core/include/state_machine.hpp"
#include "core/include/state_machine_simulation_core.hpp"

static State_machine_simulation_core* sim_core = nullptr;

ceps::ast::Nodebase_ptr double_value(ceps::ast::Call_parameters* params)
{
  using namespace ceps::ast;
  if(params->children().size() != 1) sim_core->fatal_(-1,"Function 'double_value' expects exactly one argument");
  auto const & args =  params->children();
  if (args[0]->kind() == Ast_node_kind::int_literal)
    return new Int( value(as_int_ref(args[0])) *2 /*scalar part*/,unit(as_int_ref(args[0])) /*SI unit part*/ );
  else if (args[0]->kind() == Ast_node_kind::float_literal)
    return new Double( value(as_double_ref(args[0])) *2.0 /*scalar part*/,unit(as_int_ref(args[0])) /*SI unit part*/ ); 
   else if (args[0]->kind() == Ast_node_kind::string_literal)
    return new String( value(as_string_ref(args[0])) + value(as_string_ref(args[0])) /*scalar part*/); 
   
  return nullptr; //Case if argument is not of any kind listed above
}

extern "C" void init_plugin(State_machine_simulation_core* smc, 
			     void (*register_plugin)(State_machine_simulation_core*,std::string const&,ceps::ast::Nodebase_ptr (ceps::ast::Call_parameters* )))
{
  sim_core = smc;
  register_plugin(smc,"double_value", double_value);  
}