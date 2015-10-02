#ifndef INC_CEPS_STATE_MACHINE_HPP
#define INC_CEPS_STATE_MACHINE_HPP

#include <string> 
#include <vector>
#include <map>
#include <algorithm>
#include <set> 
#include "ceps_all.hh"


class State_machine;
struct state_rep_t {
	bool valid_ = false;
	bool is_sm_ = false;
	State_machine* smp_ = nullptr;
	std::string sid_;

	bool tag1_;

	bool valid() const {return valid_;}
	state_rep_t() = default;
	state_rep_t(bool valid, bool is_sm,State_machine* smp, std::string const & sid) : valid_(valid),is_sm_(is_sm),smp_(smp),sid_(sid)
	{ assert(!valid || is_sm || smp != nullptr);}
	bool initial() const {return sid_ == "initial" || sid_ == "Initial";}
	bool final() const {return sid_ == "final" || sid_ == "Final";}

	bool operator < (state_rep_t const & rhs) const
	{
		if ( (size_t) smp_ == (size_t) rhs.smp_ )
		{
			return sid_ < rhs.sid_;
		}
		return (size_t) smp_ < (size_t) rhs.smp_;
	}

	bool operator ==  (state_rep_t const & rhs) const
	{
		return smp_ == rhs.smp_ && sid_ == rhs.sid_;
	}

	State_machine* containing_sm() const;

	bool& tag1() {return tag1_;}
};

struct State_machine
{
	
 
  std::string const & id() const {return id_;}
  std::string & id() {return id_;}

  std::vector<State_machine*> const & children() const {return children_;}
  std::vector<State_machine*> & children()  {return children_;}
  void add_child(State_machine* smp){
	  if(smp == nullptr) return;
	  for(auto s: children()) if (s == smp) return;
	  children().push_back(smp);
  }

  bool has_child(State_machine* smp) const
  {
	  if (smp == nullptr) return false;
	  if (children().size() == 0) return false;
	  for(auto p:children()) if (p == smp) return true;
	  return false;
  }

  State_machine* get_sub_machine_by_name(std::string const & name) const
  {
	  for(auto p : children())
	  {
		  //std::cerr << "###" << p->id() << "\n";
		  if (p->id() == name) return p;
	  }
	  return nullptr;
  }

  State_machine* parent() const {return parent_;}
  State_machine* & parent() {return parent_;}

  State_machine(int order,std::string id, State_machine* parent, int depth)
  :order_(order),id_(id),parent_(parent),depth_(depth){}

  bool operator == (State_machine const & rhs) const {return order_ == rhs.order_;}
  bool operator < (State_machine const & rhs) const {return order_ < rhs.order_;}
  bool operator > (State_machine const & rhs) const {return order_ > rhs.order_;}

  struct State{
	std::string id_;
    bool is_sm_ = false;
    State_machine* smp_ = nullptr;

    State_machine* parent_ = nullptr;
    std::vector<std::string> q_id_;
    bool unresolved_ = false;


    State() = default;
    State(State const &) = default;
    State(std::string const & id):id_(id),is_sm_(false){}
    State(State_machine* smp):id_(std::string{}),is_sm_(true),smp_(smp){}

    State(std::vector<std::string> const & q_id):q_id_(q_id),unresolved_(true){}

    bool is_initial() const {return id_ == "initial" || id_ == "Initial" ;}
    bool is_final() const {return id_ == "final" || id_ == "Final";}

    std::string const & id() const {if (!is_sm_ || smp_ == nullptr) return id_; return smp_->id();}
    std::string & id() {if (!is_sm_ || smp_ == nullptr) return id_; return smp_->id();}
    State_machine* smp() const {return smp_;}
    State_machine* & smp() {return smp_;}

    bool is_foreign(State_machine* smp) const {if (parent_ == nullptr) return false; return smp != parent_;}
    State_machine* parent() const {return parent_;}
    State_machine*& parent() {return parent_;}

    bool unresolved() const { return unresolved_;}
    bool& unresolved()  { return unresolved_;}

    std::vector<std::string> const & q_id() const {return q_id_;}
    std::vector<std::string>  & q_id() {return q_id_;}

    bool operator  == (State const & rhs) const
    {
      return id() == rhs.id();
    }
    bool operator  < (State const & rhs) const
    {
      return id() < rhs.id();
    }
    bool operator  > (State const & rhs) const
    {
      return id() > rhs.id();
    }
  };

  struct Transition{
    State from_,to_;
    using Nonleafbase_ptr = ceps::ast::Nonleafbase* ;
    std::string guard_ ;

    Transition() = default;
    Transition(State from , State to):from_(from),to_(to){}
    State const & from() const {return from_;}
    State & from() {return from_;}
    State const & to() const {return to_;}
    State & to() {return to_;}

    std::string& guard() {return guard_;}
    std::string guard()const {return guard_;}
    bool has_guard() const {return guard().length() != 0;}


    struct Event
    {
    	std::string id_;
    	Event() = default;
    	Event(std::string id): id_(id) {}
    	std::string const & id() const {return id_;}
    	std::string & id() {return id_;}
    	bool operator == (Event const & rhs) const {return id_ == rhs.id_;}
    	bool operator < (Event const & rhs) const {return id_ < rhs.id_;}
    };

    struct Action
    {
    	std::string id_;
    	State_machine* associated_sm_ = nullptr;
    	bool valid() const {return id_.length() > 0;}
    	ceps::ast::Nodebase_ptr body_ = nullptr;
    	Action() = default;
    	Action(std::string id): id_(id),body_(nullptr) {}
    	Action(std::string id,ceps::ast::Nodebase_ptr body): id_(id),body_(body) {}
    	Action(State_machine* smp,std::string id,ceps::ast::Nodebase_ptr body): id_(id),associated_sm_(smp),body_(body) {}

    	ceps::ast::Nodebase_ptr & body() {return body_;}
    	ceps::ast::Nodebase_ptr  body() const {return body_;}

    	std::string const & id() const {return id_;}
    	std::string & id() {return id_;}
    	bool operator == (Action const & rhs) const {return id_ == rhs.id_;}
    	bool operator < (Action const & rhs) const {return id_ < rhs.id_;}
     };

    std::set<Event> events_;
    std::vector<Action> action_;

    std::set<Event>& events() {return events_;}
    std::set<Event> const & events() const {return events_;}
    std::vector<Action>& actions() {return action_;}
    std::vector<Action> const & actions() const {return action_;}

     bool can_take(std::string ev_id) const
     {
    	 if(events_.size() == 0) return true;
    	 return events_.find(Event(ev_id)) != events_.end();
     }
  };

  bool has_initial_state() const { for(auto const & s: states) if (s.is_initial()) return true; return false;}
  State get_initial_state() const { for(auto const & s: states) if (s.is_initial()) return s; return State();}


  void insert_state(State s)
  {
	s.is_sm_ = false;
	s.smp_ = this;
    states.insert(s);
  }

  bool lookup(State& s)
  {

    auto it = states.find(s);
    if (it != states.end()) {s = *it; return true;}
    for(auto &t : children_)
     if (t->id() == s.id()) {s.smp() = t;s.is_sm_ = true;return true;}
    if (parent_!=nullptr)
     return parent_->lookup(s);
    auto it_glob = statemachines.find(s.id());
    if (it_glob != statemachines.end()) { s.smp() = it_glob->second;s.is_sm_ = true;return true;}
    return false;
  }


  std::set<Transition::Event>& events() {return events_;}
  std::set<Transition::Event> const & events() const {return events_;}
  std::set<Transition::Action>& actions() {return actions_;}
  std::set<Transition::Action> const & actions() const {return actions_;}




  using unresolve_import_t = std::tuple<std::string /*name of SM which is to be imported.*/ ,
    	  	  std::string /*alias under which imported SM is named inside this machine*/,
    	  	  State_machine * /*pointer to SM corresponding to first tuple entry*/>;



  std::vector< unresolve_import_t >& unresolved_imports() {return unresolved_imports_;}

  bool & definition_complete() {return complete_;}




  template <typename T> bool lookup_info(T& s,std::set<T> const & cont)
  {
    if (s.id().length() == 0) {return false;}
    auto it = cont.find(s);
    if (it != cont.end()) {s = *it; return true;}
    if (parent_!=nullptr)
     return parent_->lookup_info(s,cont);

    return false;
  }


  void map_state(State& s,std::map<State_machine*,State_machine*>& cloned_sms,
		  std::map<std::string,State_machine*>& id_to_sm,int& counter,std::string const & id,
		  bool (*resolve) (State_machine &, void*) , void * context )
  {
	  if (cloned_sms.find(s.smp_) != cloned_sms.end()){ s.smp_=cloned_sms[s.smp_];}
  	  else {
 	  	auto smp = new State_machine(counter++,s.smp_->id_,this,depth_+1);
	  	smp->clone_from(s.smp(),counter,id,resolve,context);
	  	cloned_sms[s.smp()] = smp;
	  	id_to_sm[s.id()] = smp;
	  	children_.push_back(smp);
	  	s.smp_ = smp;
  	  }
  }

  void clone_from(State_machine* rhs,int & counter,std::string const & , bool (*) (State_machine &, void*) , void * context);

  bool is_thread() const {return is_thread_;}
  bool& is_thread() {return is_thread_;}
  bool contains_threads() const {return contains_threads_;}
  bool& contains_threads() {return contains_threads_;}

  static std::map<std::string,State_machine*> statemachines;
  bool is_thread_ = false;
  bool contains_threads_=false;
  bool complete_ = true;
  int order_;
  std::string id_;

  State_machine* parent_;
  int depth_ = 1;

  std::vector<State_machine*> children_;

  std::vector<Transition> const & transitions() const {return transitions_;}
  std::vector<Transition> & transitions() {return transitions_;}

  std::set<Transition::Action> actions_;
  std::set<Transition::Event> events_;
  std::vector< unresolve_import_t > unresolved_imports_;
  std::vector<std::vector<Transition>> threads_;
  std::vector<Transition> transitions_;
  std::set<State> states;

  bool join_ = false;
  State join_state_;

  bool& join() {return join_;}
  bool join() const {return join_;}
  State const & join_state() const {return join_state_;}
  State & join_state() {return join_state_;}


  decltype(threads_)& threads() {return threads_;}
  decltype(threads_) const & threads() const {return threads_;}

};



struct event_rep_t {
	bool valid_ = false;
	State_machine* smp_ = nullptr;
	std::string sid_;
	std::vector<ceps::ast::Nodebase_ptr> payload_;
	bool valid() const {return valid_;}
	bool epsilon() const {return !valid_;}


	event_rep_t() = default;
	event_rep_t(bool valid, State_machine* smp, std::string const & sid) : valid_(valid),smp_(smp),sid_(sid)
	{}
};



#endif
