#include "core/include/livelog/livelogger.hpp"
#include "core/include/sm_livelog_storage_ids.hpp"
#include "core/include/sm_livelog_storage_utils.hpp"

void sm4ceps::livelog_write(livelog::Livelogger& live_logger,executionloop_context_t::states_t const &  states){
 std::size_t len = 0;
 for(auto const & s : states) if (s != 0) len+=sizeof(std::uint32_t);
 live_logger.write_ext(sm4ceps::STORAGE_WHAT_CURRENT_STATES,[&](char * data){
	 std::uint32_t counter = 0;
	 for(auto const & s : states){
		 if (s != 0) {*((std::uint32_t*)data) = counter; data += sizeof(std::uint32_t);} ++counter;
	 }
  },len);
}
