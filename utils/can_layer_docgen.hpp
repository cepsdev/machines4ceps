#ifndef INC_CAN_LAXER_DOCGEN_HPP
#define INC_CAN_LAXER_DOCGEN_HPP
#include "ceps_all.hh"
#include "core/include/state_machine_simulation_core.hpp"
#include "core/include/cmdline_utils.hpp"
#include <iostream>
namespace sm4ceps {  namespace utils {

 void dump_asciidoc_canlayer_doc(std::ostream&,State_machine_simulation_core*);
 void dump_stddoc_canlayer_doc(Result_process_cmd_line const &, std::ostream&,State_machine_simulation_core*);

} }


#endif
