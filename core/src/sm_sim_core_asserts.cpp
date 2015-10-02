#include "core/include/state_machine_simulation_core.hpp"

#ifdef __GNUC__
#define __funcname__ __PRETTY_FUNCTION__
#else
#define __funcname__ __FUNCSIG__
#endif
#define DEBUG_FUNC_PROLOGUE 	Debuglogger debuglog(__funcname__,this,this->print_debug_info_);
#define DEBUG (debuglog << "[DEBUG]", debuglog)


void State_machine_simulation_core::update_asserts(states_t const & reached_states)
{
	DEBUG_FUNC_PROLOGUE
	for(auto & ass:active_asserts_)
	{
		if (ass.type_ == ass.NEVER)
		{
			if (!ass.satisfied_) continue;
			DEBUG << "[CHECK_NOT_SATISFIED_ASSERT(NEVER CONDITION)]\n";
			state_rep_t never_state;
			for(auto const & s : reached_states)
			{
				bool match = false;
				for(auto const & ss : ass.states_)
				{
					if (ss == s){never_state=ss;match=true;break;}
				}
				if (match) ass.satisfied_ = false;
				if (match) break;
			}
			if (!ass.satisfied_){
			 std::stringstream ss;
			 ss << "\nExpected never to reach state " << get_fullqualified_id(never_state) <<", current states are:\n";
			 for(auto  s : reached_states)
			 {
				if (s == never_state) ss << " >" <<get_fullqualified_id(s) << "<" << "\n";
				else ss << " " << get_fullqualified_id(s) << "\n";
			 }
			 ss <<".";
			 fatal_(-1,"\nASSERTION not satisfied (ASSERT_NEVER_VISIT_STATES): "+ss.str());
			}

		} else {
		 if(ass.satisfied_) continue;
		 DEBUG << "[CHECK_NOT_SATISFIED_ASSERT]\n";
		 for(auto const & s : reached_states)
		 {
	 		bool match = false;
			for(auto const & ss : ass.states_)
			{
				if (ss == s){match=true;break;}
			}
			if (match) ass.satisfied_ = true;
			if (match) break;
		 }
		}
	}
}
