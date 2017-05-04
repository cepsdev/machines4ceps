#ifndef INC_STDDOC_IMPORT_HPP
#define INC_STDDOC_IMPORT_HPP
#include "ceps_all.hh"
#include "core/include/state_machine_simulation_core.hpp"

namespace sm4ceps {  namespace utils {
 ceps::ast::Nodeset make_stddoc(
     ceps::ast::Struct_ptr what,
     ceps::ast::Nodebase_ptr root,
     ceps::parser_env::Symbol* sym,
     ceps::parser_env::Symboltable & symtab,
     ceps::interpreter::Environment& env,
     ceps::ast::Nodebase_ptr ,
     ceps::ast::Nodebase_ptr
 );
} }


#endif
