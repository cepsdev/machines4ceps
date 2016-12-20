#ifndef INC_GENSM_HPP
#define INC_GENSM_HPP
#include "ceps_all.hh"

namespace sm4ceps{ namespace modelling { namespace gensm {
 struct sm{
  std::string id;
  std::vector<ceps::ast::ident> states;
  std::vector<std::tuple<std::string,std::string,ceps::ast::Nodebase_ptr >> transitions;
  ceps::ast::strct* header;

  sm(std::string id);
  ceps::ast::Nodeset ns();
  void add_state(std::string);
  void add_transition(std::string from, std::string to, ceps::ast::Nodebase_ptr g);
 };
}}}


#endif
