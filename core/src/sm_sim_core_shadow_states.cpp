#include "core/include/state_machine_simulation_core.hpp"

#include "core/include/base_defs.hpp"
#include "core/include/sm_livelog_storage_utils.hpp"

#define DONT_PRINT_DEBUG
#define PRINT_LOG_SIM_LOOP

constexpr bool PRINT_DEBUG = true;

//static void invariant(std::string const &){}

template<typename T,typename Pred> static inline std::size_t collect_matching_indices(T& cont_dest,std::size_t n, Pred pred ){
 std::size_t hits = 0;
 for(std::size_t j = 0; j != n; ++j){
  if (!pred(j)) continue;
  ++hits;
  cont_dest.push_back(j);
 }
 return hits;
}

static inline bool operator <= (executionloop_context_t::transition_t const & lhs, executionloop_context_t::transition_t const & rhs){
 if ( (lhs.props & executionloop_context_t::TRANS_PROP_ABSTRACT) && lhs.ev <= 0 && lhs.script_guard.length() == 0) return true;
 if (lhs.ev == rhs.ev && lhs.script_guard == rhs.script_guard) return true;
 if (lhs.ev != rhs.ev) return false;
 if (lhs.script_guard.length() == 0) return true;
 return lhs.script_guard == rhs.script_guard;
}

static inline bool operator == (executionloop_context_t::transition_t const & lhs, executionloop_context_t::transition_t const & rhs){
	if (( (lhs.props & executionloop_context_t::TRANS_PROP_ABSTRACT) && lhs.ev <= 0 && lhs.script_guard.length() == 0)
		&& ( (rhs.props & executionloop_context_t::TRANS_PROP_ABSTRACT) && rhs.ev <= 0 && rhs.script_guard.length() == 0) )return true;
	return lhs.ev == rhs.ev && lhs.script_guard == rhs.script_guard;
}

static inline bool operator != (executionloop_context_t::transition_t const & lhs, executionloop_context_t::transition_t const & rhs){
	return !(lhs == rhs);
}
static inline bool operator < (executionloop_context_t::transition_t const & lhs, executionloop_context_t::transition_t const & rhs){
	return lhs <= rhs && lhs != rhs;
}

static inline bool operator > (executionloop_context_t::transition_t const & lhs, executionloop_context_t::transition_t const & rhs){
	return  rhs <= lhs  && lhs != rhs;
}

std::vector<int> State_machine_simulation_core::compute_compatible_transitions(executionloop_context_t::transition_t const & t){
 std::vector<int> r;
 auto shadow_from = executionloop_context().shadow_state[t.from];
 auto shadow_to = executionloop_context().shadow_state[t.to];

 std::vector<int> all_possible_transitions;
 collect_matching_indices(all_possible_transitions,executionloop_context().transitions.size(),
  [&](std::size_t i){
   return executionloop_context().transitions[i].from == shadow_from && executionloop_context().transitions[i].to == shadow_to;
  }
 );
 for (auto i : all_possible_transitions) {
  auto const & tt = executionloop_context().transitions[i];
  if (tt <= t) r.push_back(i);
 }
 return r;
}

static void err_no_candidates(State_machine_simulation_core* smp,executionloop_context_t::transition_t const & t ){
 auto shadow_from = smp->executionloop_context().shadow_state[t.from];
 auto shadow_to = smp->executionloop_context().shadow_state[t.to];
 std::stringstream ss;
 ss << "The transition " << smp->map_state_id_to_full_qualified_id[t.from] << " -> " << smp->map_state_id_to_full_qualified_id[t.to]<< " ";
 if (t.ev) ss << "triggered by event " << smp->executionloop_context().id_to_ev[t.ev] << " ";
 if (t.script_guard.length()) ss << "guarded by " << t.script_guard << " ";
 ss << "requires a compatible transition " << smp->map_state_id_to_full_qualified_id[shadow_from] << " -> " << smp->map_state_id_to_full_qualified_id[shadow_to];
 ss << ".";
 smp->fatal_(-1,ss.str());
}

static void err_ambiguous(State_machine_simulation_core* smp,executionloop_context_t::transition_t const & t ){
 auto shadow_from = smp->executionloop_context().shadow_state[t.from];
 auto shadow_to = smp->executionloop_context().shadow_state[t.to];
 std::stringstream ss;
 ss << "The transition " << smp->map_state_id_to_full_qualified_id[t.from] << " -> " << smp->map_state_id_to_full_qualified_id[t.to]<< " ";
 if (t.ev) ss << "triggered by event " << smp->executionloop_context().id_to_ev[t.ev] << " ";
 if (t.script_guard.length()) ss << "guarded by " << t.script_guard << " ";
 ss << " is ambiguous, there are multiple transitions of the form " << smp->map_state_id_to_full_qualified_id[shadow_from] << " -> " << smp->map_state_id_to_full_qualified_id[shadow_to];
 ss << ".";
 smp->fatal_(-1,ss.str());
}

void State_machine_simulation_core::compute_shadow_transitions(){
 std::vector<int> shadowed_transitions;
 if ( 0 == collect_matching_indices(shadowed_transitions,
		                            executionloop_context().transitions.size(),
									[&](std::size_t i) {return executionloop_context().shadow_state[executionloop_context().transitions[i].from] > 0 &&
											                   executionloop_context().shadow_state[executionloop_context().transitions[i].to] > 0;
                                                       }) ) return;
 for(auto st : shadowed_transitions){
  auto const & t = executionloop_context().transitions[st];
  auto candidates = compute_compatible_transitions(t);
  auto best_candidate = 0;
  if (candidates.size() == 0){
	  err_no_candidates(this,t);
  } else if (candidates.size() > 1) {
   std::sort(candidates.begin(), candidates.end(),[&](int i,int j){ return executionloop_context().transitions[i] < executionloop_context().transitions[j];  } );
   if (executionloop_context().transitions[candidates[candidates.size()-1]] > executionloop_context().transitions[candidates[candidates.size()-2]]){
	   best_candidate = candidates[candidates.size()-1];
   } else {
     err_ambiguous(this,t);
   }
  } else best_candidate = candidates[0];
  executionloop_context().shadow_transitions[st] = best_candidate;
 }//for

}
