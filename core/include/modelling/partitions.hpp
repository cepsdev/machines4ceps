#ifndef INC_PARTITIONS_HPP
#define INC_PARTITIONS_HPP
#include "ceps_all.hh"

namespace sm4ceps {  namespace modelling {

 ceps::ast::Nodeset standard_value_partition_sm(
  				ceps::ast::Struct_ptr,
  				ceps::ast::Nodebase_ptr,
 				ceps::parser_env::Symbol* ,
  				ceps::parser_env::Symboltable &,
  				ceps::interpreter::Environment& ,
  				ceps::ast::Nodebase_ptr ,
  				ceps::ast::Nodebase_ptr );

} }


#endif
