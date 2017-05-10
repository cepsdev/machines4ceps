#ifndef INC_SM_EXECUTION_CTXT_HPP
#define INC_SM_EXECUTION_CTXT_HPP

#include <string>
#include <vector>
#include <map>
#include <unordered_set>
#include <limits>

class State_machine_simulation_core;

class executionloop_context_t{
public:

    //using state_rep_t = std::uint16_t;
    using state_rep_t = int;
    using state_present_rep_t = std::uint8_t;

	executionloop_context_t(){
		ev_sync_queue.resize(1024);
	}

	void push_ev_sync_queue(int ev_id){
		ev_sync_queue[ev_sync_queue_end] = ev_id;
		++ev_sync_queue_end;
		if (ev_sync_queue_end == ev_sync_queue.size()) ev_sync_queue_end = 0;
		if (ev_sync_queue_end == ev_sync_queue_start) {
			++ev_sync_queue_start;
			if (ev_sync_queue_start == ev_sync_queue.size()) ev_sync_queue_start = 0;
		}
	}

	int front_ev_sync_queue(){
		return ev_sync_queue[ev_sync_queue_start];
	}

	bool ev_sync_queue_empty() {
		return ev_sync_queue_start == ev_sync_queue_end;
	}

	bool pop_ev_sync_queue(){
		if (ev_sync_queue_empty()) return false;
		++ev_sync_queue_start;
		if (ev_sync_queue_start == ev_sync_queue.size())
			ev_sync_queue_start = 0;
		return true;
	}

	int get_parent(int state){
		return parent_vec[state];
	}

	void set_parent(int child, int state){
		if ((size_t)number_of_states > parent_vec.size())
			parent_vec.resize(number_of_states+1,0);
		parent_vec[child] = state;
	}

	void current_states_init_and_clear(){
		if (current_states.size() != (size_t)number_of_states+1) current_states.resize(number_of_states+1);
                std::memset(current_states.data(),0,(number_of_states+1)*sizeof(state_present_rep_t));
	}

	void set_inf(int state,unsigned int what,bool value){
		if ((size_t)number_of_states > inf_vec.size())
			inf_vec.resize(number_of_states+1,0);
		if (value) inf_vec[state] |=  1 << what;
		else inf_vec[state] &=  ~ (1 << what);
	}

	bool get_inf(int state, unsigned int what) const{
		unsigned int r = (inf_vec[state] & (1 << what));
		return r != 0;
	}

	void set_on_enter(int state,void(*fn)()){
		if ((size_t)number_of_states > on_enter.size())
			on_enter.resize(number_of_states+1,0);
		on_enter[state] = fn;
	}
	void set_on_exit(int state,void(*fn)()){
		if ((size_t)number_of_states > on_exit.size())
			on_exit.resize(number_of_states+1,0);
		on_exit[state] = fn;
	}

	void set_initial_state(int state,int initial){
		if ((size_t)number_of_states > initial_state.size())
			initial_state.resize(number_of_states+1,0);
		initial_state[state] = initial;
	}
	void set_final_state(int state,int final){
		if ((size_t)number_of_states > final_state.size())
			final_state.resize(number_of_states+1,0);
		final_state[state] = final;
	}

	void init_state_to_children(){
		state_to_children.resize(number_of_states+1);
	}

	bool is_sm(int state){
		return get_inf(state, SM);
	}

        void do_enter_impl(State_machine_simulation_core*,int sms,std::vector<executionloop_context_t::state_present_rep_t> const & v);
        void do_enter(State_machine_simulation_core*,int* sms,int n,std::vector<executionloop_context_t::state_present_rep_t> const & v);
        void do_exit_impl(State_machine_simulation_core* smc,int sms,std::vector<executionloop_context_t::state_present_rep_t> const & v);
        void do_exit(State_machine_simulation_core* smc,int* sms,int n,std::vector<executionloop_context_t::state_present_rep_t> const & v);

        void remove_children(int sms,std::vector<executionloop_context_t::state_present_rep_t> & v){
		auto child_idx = state_to_children[sms]+1;
                for(int child;(child = children[child_idx]);++child_idx){
			v[child] = 0;
			set_inf(child,VISITED,true);
			if (!is_sm(child)) continue;
			remove_children(child,v);
		}
	}

        bool empty(int sms,std::vector<executionloop_context_t::state_present_rep_t> const & v ){
	  auto child_idx = state_to_children[sms]+1;
	  bool active_sub_states = false;
          for(int child;(child=children[child_idx]);++child_idx){
		 if (v[child]){active_sub_states = true;break;}
	  }
	  if (active_sub_states) return false;
	  return true;
	}

	void set_assoc_sm(int state,State_machine* smp){
		if ((size_t)number_of_states > assoc_sm.size())
			assoc_sm.resize(number_of_states+1,0);
		assoc_sm[state] = smp;
	}

	State_machine* get_assoc_sm(int state) const{
		if ((size_t)number_of_states > assoc_sm.size())
			assoc_sm.resize(number_of_states+1,0);
		return assoc_sm[state];
	}


	void set_join_state(int state,int join){
		if ((size_t)number_of_states > join_states.size())
			join_states.resize(number_of_states+1,0);
		join_states[state] = join;
	}

	int get_join_state(int state){
		if ((size_t)number_of_states > join_states.size())
			join_states.resize(number_of_states+1,0);
		return join_states[state];
	}

	int number_of_states = 0;
	int current_ev_id = 0;
	std::map<std::string,int> ev_to_id;
	std::map<int,std::string> id_to_ev;
	class transition_t{
		public:
		unsigned int props = 0;
		int root_sms = 0; //root state machine, i.e. the very top level sms which transitively contains this transition
		int smp = 0,from = 0, to = 0, ev = 0;int rel_idx = -1;
        bool native = true;
		bool(**guard)() = nullptr;
		void(* a1)() = nullptr;
		void(* a2)() = nullptr;
		void(* a3)() = nullptr;

		std::string script_guard;
        //void* guard_script = nullptr;
        void* a1_script = nullptr;
        void* a2_script = nullptr;
        void* a3_script = nullptr;

		transition_t() = default;
		bool start() const {return smp != 0 && from == 0 && to == 0;}
	};

	using states_t = std::vector<state_present_rep_t>;

	std::vector<transition_t> transitions;
	std::vector<int> shadow_transitions;
    std::vector<state_present_rep_t> current_states;
    std::vector<int> shadow_state;
	std::unordered_map<int,int> state_to_first_transition;
	std::vector<int> ev_sync_queue;
	std::vector<int> parent_vec;
	std::vector<int> inf_vec;
	std::vector<void(*)()> on_enter;
	std::vector<void(*)()> on_exit;
	std::vector<int> initial_state;
	std::vector<int> final_state;
	std::vector<int> children;
	std::vector<int> state_to_children;
	std::vector<int> join_states;
	std::map<std::string,int> state_id_to_idx;
	std::map<int,std::string> idx_to_state_id;
	std::unordered_set<int> exported_events;
	mutable std::vector<State_machine*> assoc_sm;
	std::vector<int> triggered_shadow_transitions;

	static constexpr unsigned int INIT = 0;
	static constexpr unsigned int FINAL = 1;
	static constexpr unsigned int SM = 2;
	static constexpr unsigned int EXIT = 3;
	static constexpr unsigned int ENTER = 4;
	static constexpr unsigned int VISITED = 5;
	static constexpr unsigned int VISITING = 6;
	static constexpr unsigned int THREAD = 7;
	static constexpr unsigned int JOIN = 8;
	static constexpr unsigned int IN_THREAD = 9;
	static constexpr unsigned int REGION = 10;

	static constexpr unsigned int TRANS_PROP_ABSTRACT = 1;

	size_t ev_sync_queue_start = 0;
	size_t ev_sync_queue_end = 0;

	size_t start_of_covering_states = std::numeric_limits<size_t>::max();
	bool start_of_covering_states_valid() const { return start_of_covering_states != std::numeric_limits<size_t>::max(); }
	size_t start_of_covering_transitions = std::numeric_limits<size_t>::max();
	bool start_of_covering_transitions_valid() const { return start_of_covering_transitions != std::numeric_limits<size_t>::max(); }

	std::vector<int> coverage_state_table;
	std::vector<int> coverage_transitions_table;

	void init_coverage_structures(){
		if (start_of_covering_states_valid()) coverage_state_table.resize((size_t)number_of_states - start_of_covering_states+1);
		if (start_of_covering_transitions_valid()) coverage_transitions_table.resize(transitions.size()-start_of_covering_transitions);
	}

	void visit_state(size_t idx){
		if (idx < start_of_covering_states) return;
		++coverage_state_table[idx - start_of_covering_states];
	}
	void visit_transition(size_t idx){
		if (idx < start_of_covering_transitions) return;
		++coverage_transitions_table[idx - start_of_covering_transitions];
	}


};




#endif
