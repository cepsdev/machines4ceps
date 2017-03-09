#ifndef INC_FIBEX_IMPORT_HPP
#define INC_FIBEX_IMPORT_HPP
#include "ceps_all.hh"
#include "core/include/state_machine_simulation_core.hpp"

namespace sm4ceps {  namespace utils {

 ceps::ast::Nodeset import_fibex(
		        State_machine_simulation_core*,
  				ceps::ast::Struct_ptr,
  				ceps::parser_env::Symboltable &,
  				ceps::interpreter::Environment&
  				);

} }


#endif
