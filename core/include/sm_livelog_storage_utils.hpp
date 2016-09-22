#include "core/include/livelog/livelogger.hpp"
#include "core/include/sm_livelog_storage_ids.hpp"
#include "core/include/state_machine.hpp"
#include "core/include/sm_execution_ctxt.hpp"

#ifndef SM_LIVELOG_STORAGE_UTILS_INC
#define SM_LIVELOG_STORAGE_UTILS_INC

namespace sm4ceps{
  void livelog_write(livelog::Livelogger& live_logger,executionloop_context_t::states_t const &  states);
};

#endif
