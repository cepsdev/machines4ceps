#include "core/include/livelog/livelogger.hpp"
#ifndef SM_LIVELOG_STORAGE_IDS_INC
#define SM_LIVELOG_STORAGE_IDS_INC

namespace sm4ceps{
  constexpr int START_SM4CEPS_STORAGE_IDS = livelog::START_USER_DEFINED_STORAGE_IDS + 0xA00;
  constexpr int STORAGE_IDX2FQS = START_SM4CEPS_STORAGE_IDS; // Mapping numeric state id -> full qualified state 
  constexpr int STORAGE_IDX2FQS_FLUSH = START_SM4CEPS_STORAGE_IDS+1;

  constexpr livelog::Livelogger::Storage::what_t STORAGE_WHAT_EVENT = 3;
  constexpr livelog::Livelogger::Storage::what_t STORAGE_WHAT_CURRENT_STATES = 4;
  constexpr livelog::Livelogger::Storage::what_t STORAGE_WHAT_CONSOLE = 5;
  constexpr livelog::Livelogger::Storage::what_t STORAGE_WHAT_INT32_TO_STRING_MAP = 6;
};

#endif
