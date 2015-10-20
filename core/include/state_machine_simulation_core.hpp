#ifndef INC_CEPS_STATE_MACHINE_CORE_HPP
#define INC_CEPS_STATE_MACHINE_CORE_HPP

#include <string> 
#include <vector>
#include <map>
#include <algorithm>
#include <set> 
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>


#include "ceps_all.hh"
#include "core/include/state_machine.hpp"
#include "core/include/cmdline_utils.hpp"
#include "core/include/sm_ev_comm_layer.hpp"
#include "core/include/sm_raw_frame.hpp"

template<typename T, typename Q> class threadsafe_queue{
	Q data_;
	mutable std::mutex m_;
	std::condition_variable cv_;
public:
	void push(T const & elem){
		std::lock_guard<std::mutex> lk(m_);
		data_.push(elem);
		cv_.notify_one();
	}

	void wait_and_pop(T & elem){
		std::unique_lock<std::mutex> lk(m_);
		cv_.wait(lk, [this]{return !data_.empty(); });
		elem = data_.front();
		data_.pop();
	}

	void wait_for_data(){
		std::unique_lock<std::mutex> lk(m_);
		cv_.wait(lk, [this]{return !data_.empty(); });
	}

	Q& data() {return data_;}
	std::mutex& data_mutex() const {return m_;}
};

class State_machine_simulation_core
{
	typedef std::chrono::steady_clock clock_type;

	ceps::ast::Nodeset*	current_universe_ = nullptr;
	ceps::Ceps_Environment * ceps_env_current_ = nullptr;
	std::map<std::string, ceps::ast::Nodebase_ptr> global_guards;
	std::map<std::string, ceps::ast::Nodebase_ptr> global_states;
	using states_t = std::vector<state_rep_t>;
	bool quiet_mode_= false;
	bool shutdown_threads_ = false;
	mutable std::recursive_mutex states_mutex_;
	states_t current_states_;

	std::map<std::string,ceps::ast::Nodebase_ptr> global_funcs_;
	std::map<std::string,Rawframe_generator*> frame_generators_;

public:

	size_t timeout_ = 0;



	std::map<std::string, ceps::ast::Nodebase_ptr>& global_systemstates(){return global_states;}

	std::map<std::string,ceps::ast::Nodebase_ptr>& global_funcs() {return global_funcs_;}
	std::map<std::string,ceps::ast::Nodebase_ptr> const & global_funcs() const {return global_funcs_;}

	std::map<std::string, Rawframe_generator*>&  frame_generators() {return frame_generators_;}
	std::map<std::string, Rawframe_generator*> const &  frame_generators() const {return frame_generators_;}

	std::recursive_mutex& states_mutex() {return states_mutex_;}

	states_t & current_states(){return current_states_;}
	bool shutdown(){return shutdown_threads_;}
	void lock_global_states() const {states_mutex_.lock();}
	void unlock_global_states() const {states_mutex_.unlock();}
	std::map<std::string, ceps::ast::Nodebase_ptr> & get_global_states() {return global_states;}
	bool& quiet_mode(){return quiet_mode_;}
	bool quiet_mode() const {return quiet_mode_;}
	struct event_t {
		std::string id_;
		clock_type::duration delta_time_ = {};

		std::string timer_id_;
		bool already_sent_to_out_queues_ = false;
		std::vector<ceps::ast::Nodebase_ptr> payload_;
		bool periodic_ = false;
		clock_type::duration delta_time_orig_ = {};
		event_t() = default;
		event_t(std::string const & id,std::vector<ceps::ast::Nodebase_ptr> const & payload = {}):id_(id),payload_(payload) {}
		event_t(std::string const & id,
				clock_type::duration delta_time,
				std::string timer_id = "",
				bool periodic = false)
		 :id_(id),delta_time_(delta_time),timer_id_(timer_id),periodic_(periodic),delta_time_orig_(delta_time) {}
		bool operator < (event_t const & rhs) const
		{
			return delta_time_ > rhs.delta_time_;
		}
		bool is_epsilon() const {return id_ == "";}
		event_t& operator = (event_rep_t const & rhs) { id_=rhs.sid_;payload_=rhs.payload_;return *this;}
	};

	typedef ceps::ast::Nodebase_ptr (*smcore_plugin_fn_t)(ceps::ast::Call_parameters* params);

	void register_plugin_fn(std::string const & id, smcore_plugin_fn_t fn);

	typedef void (*global_event_call_back_fn)(event_t);
	global_event_call_back_fn global_event_call_back_fn_ = nullptr;
	void set_global_event_call_back(global_event_call_back_fn fn){global_event_call_back_fn_ = fn;}



private:

	std::map<std::string,smcore_plugin_fn_t> name_to_smcore_plugin_fn;

	using main_event_queue_t = threadsafe_queue<event_t, std::priority_queue<event_t> >;
	using out_event_queue_t  = threadsafe_queue<event_t, std::queue<event_t> >;
	using out_event_queues_t = std::vector<out_event_queue_t*>;

	mutable std::mutex out_event_queues_m_;
	main_event_queue_t main_event_queue_;
	out_event_queues_t out_event_queues_;


	struct Log{
		std::ostream* os_p = nullptr;
		State_machine_simulation_core* parent_ = nullptr;
		Log() = default;
		Log(State_machine_simulation_core* parent):parent_(parent){}
		std::mutex m_;
		template <typename T> Log& operator << (T const & v)
		{
			if (parent_->quiet_mode()) return *this;
			std::lock_guard<std::mutex> print_lock(m_);
			if (os_p) { *os_p << v;}
			return *this;
		}
	};

	Log std_log_;
public:
	using type_definitions_t = std::map<std::string, ceps::ast::Struct_ptr>;
	type_definitions_t type_definitions_;
	type_definitions_t const & type_definitions() const {return type_definitions_;}
	type_definitions_t & type_definitions() {return type_definitions_;}
private:



	void process_statemachine_helper_handle_transitions(
						State_machine* current_statemachine,
						std::vector<State_machine::Transition>& trans,
						std::string id,
						ceps::ast::Nodeset& transitions,int& guard_ctr);

	void process_statemachine(	ceps::ast::Nodeset& sm_definition,
								std::string prefix,
								State_machine* parent,
								int depth,
								int thread_ctr,
								bool is_thread = false);
	state_rep_t resolve_state_qualified_id(ceps::ast::Nodebase_ptr p, State_machine* parent);
	event_rep_t resolve_event_qualified_id(ceps::ast::Nodebase_ptr p, State_machine* parent);
	void process_simulation(ceps::ast::Nodeset& sim,ceps::Ceps_Environment& ceps_env,ceps::ast::Nodeset& universe);
	void eval_guard_assign(ceps::ast::Binary_operator & root);
	void eval_state_assign(ceps::ast::Binary_operator & root,std::string const &);
	void add(states_t& states, state_rep_t s);
	std::string get_fullqualified_id(state_rep_t const & s);
	std::string get_full_qualified_id(State_machine::State const& s);
	void print_info(states_t& states_from, states_t& states_to,std::set<state_rep_t>const & new_states_triggered_set,std::set<state_rep_t> const& );
	void print_info(states_t const& states);
	void trans_hull_of_containment_rel(states_t& states_in,states_t& states_out);
	bool eval_to_bool(ceps::ast::Nodebase_ptr p);
	bool compute_successor_states_kernel_under_event(event_rep_t ev,
													 states_t& states,
													 std::map<state_rep_t,state_rep_t>& pred,
													 states_t& states_without_transition,
													 ceps::Ceps_Environment& ceps_env,
													 ceps::ast::Nodeset universe,
													 std::map<state_rep_t,std::vector<State_machine::Transition::Action> >& associated_actions);

public:
	bool is_assignment_op(ceps::ast::Nodebase_ptr n);
	bool is_assignment_to_guard(ceps::ast::Binary_operator & node);
	bool is_assignment_to_state(ceps::ast::Binary_operator & node,std::string& lhs_id);

	Log& log(){return std_log_;}


	ceps::ast::Nodebase_ptr ceps_interface_eval_func(State_machine* active_smp,std::string const & , ceps::ast::Call_parameters*);
private:
	bool (*on_empty_event_queue_handler_)(std::string&) =  nullptr;
	bool (*on_queued_event_handler_)(std::string const &)  = nullptr;
	bool (*step_handler_ )() = nullptr;

	bool resolve_imports(State_machine & );
	void guards_in_expr(ceps::ast::Nodebase_ptr  expr, std::set<std::string> & v);
	ceps::ast::Nodebase_ptr unfold(ceps::ast::Nodebase_ptr expr,
								   std::map<std::string, ceps::ast::Nodebase_ptr>& guard_to_interpretation,
								   std::set<std::string>& path,states_t const & states);
	bool eval_guard(ceps::Ceps_Environment& ceps_env,std::string const & guard_name,std::vector<state_rep_t> const& current_states );
	bool contains_sm_func_calls(ceps::ast::Nodebase_ptr  expr);
	void update_asserts(states_t const & reached_states);

	int timed_events_active_ = 0;
	void enqueue_event(event_t ev,bool update_out_queues = false);
	clock_type clock_;
	clock_type::time_point base_time_point_;

	struct assert_t
	{
		enum type{ALWAYS,NEVER,EVENTUALLY,ASSERT};
		enum quantifier{NONE,ALL,AT_LEAST_ONE,EXACTLY_ONE};
		type type_;
		quantifier quantifier_;
		std::string id_;
		std::string description_;
		bool satisfied_ = false;
		states_t states_;
		std::vector<assert_t> triggered_asserts_;
		std::vector<ceps::ast::Nodebase_ptr> guards_;
	};
	std::vector<assert_t> active_asserts_;
	std::map<State_machine*,std::string> sm_to_id_;

	bool running_as_node_ = false;


	void process_event_from_remote(nmp_header,char* data);
	void exec_action_timer(std::vector<ceps::ast::Nodebase_ptr>& args,bool);
	bool is_global_event(std::string const & ev_name);


	struct event_triggered_sender_t{
		std::string event_id_;
		std::string frame_id_;
		Rawframe_generator* frame_gen_;
		threadsafe_queue< std::pair<char*,size_t>, std::queue<std::pair<char*,size_t> >>* frame_queue_;
		std::thread* thread_;
	};
	std::vector<event_triggered_sender_t>event_triggered_sender_;

public:
	std::vector<event_triggered_sender_t>& event_triggered_sender(){return event_triggered_sender_;};
	bool running_as_node() const {return running_as_node_;}
	bool& running_as_node() {return running_as_node_;}
	bool print_debug_info_ = false;
	static int SM_COUNTER;

	main_event_queue_t& main_event_queue() {return main_event_queue_;}

	out_event_queue_t* out_event_queue(int idx) {
		std::lock_guard<std::mutex> lk(out_event_queues_m_);
		if (idx >= 0 && (size_t)idx >= out_event_queues_.size()) return nullptr; return out_event_queues_[(size_t)idx];
	}
	int allocate_out_event_queue(){
		std::lock_guard<std::mutex> lk(out_event_queues_m_);
		out_event_queues_.push_back(new out_event_queue_t);
		return out_event_queues_.size()-1;
	}


	void (*fatal_)(int, std::string const & ) = nullptr;
	void (*warn_)(int, std::string const & ) = nullptr;

	std::pair<bool,std::string> get_qualified_id(State_machine* sm) {
		auto it = sm_to_id_.find(sm);
		if (it == sm_to_id_.end()) return std::make_pair(false,"");
		return std::make_pair(true,it->second);
	}
	void set_qualified_id(State_machine* sm,std::string id){sm_to_id_[sm] = id;}
	void start_processing_init_script(ceps::ast::Nodeset& sim,int& pos,states_t states);


	State_machine_simulation_core(std::string const & prelude = {})
	{
		ceps_env_current_ = new ceps::Ceps_Environment (prelude);
		current_universe_ = new ceps::ast::Nodeset();
		std_log_.parent_ = this;
	}

	void reset_universe() {current_universe_ = new ceps::ast::Nodeset();}

	void set_log_stream(std::ostream* os_p) {std_log_.os_p = os_p; }
	ceps::ast::Nodeset& current_universe() const {return *current_universe_;}
	ceps::Ceps_Environment& ceps_env_current() const {return *ceps_env_current_;}

	void set_fatal_error_handler( void (*fatal)(int, std::string const & )   ) {fatal_ = fatal;}
	void set_non_fatal_error_handler( void (*warn)(int, std::string const & )   ) {warn_ = warn;}


	void set_on_empty_event_queue_handler( bool (*handler)(std::string&) ) {on_empty_event_queue_handler_ = handler;}
	void set_on_queued_event_handler( bool (*handler)(std::string const &) ) {on_queued_event_handler_ = handler;}
	void set_step_handler (bool (*handler) ()) {step_handler_ = handler ;}


	static const int ERR_FILE_OPEN_FAILED = 0;
	static const int ERR_CEPS_PARSER      = 1;
	static const int WARN_XML_PROPERTIES_MISSING_PREFIX_DEFINITION = 100;
	static const int WARN_CANNOT_FIND_TEMPLATE = 101;
	static const int WARN_NO_INVOCATION_MAPPING_AND_NO_TABLE_DEF_FOUND = 103;
	static const int WARN_TESTCASE_ID_ALREADY_DEFINED = 104;
	static const int WARN_TESTCASE_EMPTY = 105;
	static const int WARN_NO_STATEMACHINES = 106;

	/*
	Iterates through a vector of file names, evaluates each file with ceps, appends resulting node set to the 'universe'.
	Node set resulting from evaluation is supplemented with internal variables (like paths).
	*/
	void process_files(	std::vector<std::string> const & file_names,
						std::string& last_file_processed);
	void processs_content(State_machine **entry_machine = nullptr);
	void leave_sm(State_machine* smp,states_t & states,std::set<State_machine*>& sms_exited,std::vector<State_machine*>& on_exit_seq);
	void enter_sm(	bool triggered_by_immediate_child_state, /*true <=> state which triggered enter is a pure state => No automatic trigger of Initial*/
					State_machine* smp,
					std::set<State_machine*>& sms_entered,
					std::vector<State_machine*>& on_enter_seq,
					std::set<state_rep_t>& new_states_set,
					std::vector<State_machine::Transition::Action>& on_enter_sm_derived_action_list,
					states_t const& current_states);
	ceps::ast::Nodebase_ptr execute_action_seq(State_machine* containing_smp,ceps::ast::Nodebase_ptr ac_seq);
	bool fetch_event(event_rep_t& ev,ceps::ast::Nodeset& sim,int& pos,states_t& states,
			bool& states_updated, std::vector<State_machine*>& on_enter_seq,bool ignore_handler = true, bool ignore_ev_queue =  false,bool exit_if_start_found = false);
	void simulate(ceps::ast::Nodeset sim,states_t& states_in,ceps::Ceps_Environment& ceps_env,ceps::ast::Nodeset& universe);

	bool print_debug_info(bool b) {bool t = print_debug_info_; print_debug_info_ = b;return t;}
	bool resolve_q_id(State_machine* smp, std::vector<std::string> const & q_id, State_machine::State & s);




	std::map<std::string, ceps::ast::Nodebase_ptr>&  guards(){return global_guards;}
	std::map<std::string, ceps::ast::Nodebase_ptr>&  states(){return global_states;}

	event_t current_event_;
	event_t& current_event() {return current_event_;}

	friend void run_state_machine_simulation(State_machine_simulation_core* smc,Result_process_cmd_line const& result_cmd_line);
private:
	std::map<std::string,int> registered_sockets_;
	std::recursive_mutex registered_sockets_mtx_;
public:
	std::recursive_mutex& get_reg_sock_mtx(){return registered_sockets_mtx_;}
	std::map<std::string,int>& get_reg_socks(){return registered_sockets_;}

};

struct ceps_interface_eval_func_callback_ctxt_t{
	State_machine_simulation_core* smc;
	State_machine* active_smp;
};

bool state_machine_sim_core_default_stepping();
void init_state_machine_simulation(int argc, char ** argv,State_machine_simulation_core* smc,Result_process_cmd_line& result_cmd_line);
void run_state_machine_simulation(State_machine_simulation_core* smc,Result_process_cmd_line const& result_cmd_line);
void state_machine_simulation_fatal(int code, std::string const & msg );
void state_machine_simulation_warn(int code, std::string const & msg);


#endif
