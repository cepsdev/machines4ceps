#ifndef INC_COVER_PATH_HPP
#define INC_COVER_PATH_HPP
#include "ceps_all.hh"

namespace sm4ceps {  namespace modelling {

 ceps::ast::Nodeset cover_path(
  				ceps::ast::Struct_ptr,
  				ceps::ast::Nodebase_ptr,
 				ceps::parser_env::Symbol* ,
  				ceps::parser_env::Symboltable &,
  				ceps::interpreter::Environment& ,
  				ceps::ast::Nodebase_ptr ,
  				ceps::ast::Nodebase_ptr );

} }


#endif
