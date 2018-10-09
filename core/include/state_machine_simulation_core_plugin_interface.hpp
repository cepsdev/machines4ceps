#ifndef INC_STATE_MACHINE_SIMULATION_CORE_PLUGIN_INTERFACE_HPP
#define INC_STATE_MACHINE_SIMULATION_CORE_PLUGIN_INTERFACE_HPP

#include <string>
#include <vector>
#include <cmath>
#include "ceps_all.hh"

class State_machine_simulation_core;

namespace sm4ceps_plugin_int{
 struct Variant{
  double dv_ = 0.0;
  int iv_ = 0;
  std::string sv_ ="";
  enum What {Int,Double,String,Undefined} what_ = Undefined;
  Variant (double v):dv_{v},what_{Double}{}
  Variant (int v):iv_{v},what_{Int}{}
  Variant (std::string v):sv_{v},what_{String}{}
  Variant() = default;
  Variant(Variant const&) = default;
  Variant& operator = (Variant const &) = default;
 };

 struct ev{
   std::string name_;
   std::vector<Variant> args_;
   ev() = default;
   ev(std::string name):name_(name){}
   ev(std::string name,std::vector<Variant> const & args):name_(name),args_(args){}

 };

 struct id{
   std::string name_;
   id(std::string name):name_(name){}
   id() = default;
 };

 struct xml_node_set{
	 void* xml_doc = nullptr;
	 void* xpath_node_set = nullptr;
	 xml_node_set& value() {return *this;}
 };


 //double abs(Variant const &v);


 bool operator == (Variant const & lhs, std::string const & rhs);
 bool operator == (std::string const & lhs, Variant const & rhs);
 bool operator == (Variant const & lhs, int const & rhs);
 bool operator == (int const & lhs, Variant const & rhs) ;
 bool operator == (Variant const & lhs, double const & rhs);
 bool operator == (double const & lhs, Variant const & rhs);

 bool operator != (Variant const & lhs, std::string const & rhs) ;
 bool operator != (std::string const & lhs, Variant const & rhs) ;
 bool operator != (Variant const & lhs, int const & rhs) ;
 bool operator != (int const & lhs, Variant const & rhs) ;
 bool operator != (Variant const & lhs, double const & rhs) ;
 bool operator != (double const & lhs, Variant const & rhs) ;

 bool operator > (Variant const & lhs, std::string const & rhs) ;
 bool operator > (std::string const & lhs, Variant const & rhs) ;
 bool operator > (Variant const & lhs, int const & rhs) ;
 bool operator > (int const & lhs, Variant const & rhs) ;
 bool operator > (Variant const & lhs, double const & rhs) ;
 bool operator > (double const & lhs, Variant const & rhs) ;

 bool operator >= (Variant const & lhs, std::string const & rhs);
 bool operator >= (std::string const & lhs, Variant const & rhs) ;
 bool operator >= (Variant const & lhs, int const & rhs) ;
 bool operator >= (int const & lhs, Variant const & rhs) ;
 bool operator >= (Variant const & lhs, double const & rhs);
 bool operator >= (double const & lhs, Variant const & rhs);

 bool operator < (Variant const & lhs, std::string const & rhs);
 bool operator < (std::string const & lhs, Variant const & rhs);
 bool operator < (Variant const & lhs, int const & rhs);
 bool operator < (int const & lhs, Variant const & rhs);
 bool operator < (Variant const & lhs, double const & rhs);
 bool operator < (double const & lhs, Variant const & rhs);

 bool operator <= (Variant const & lhs, std::string const & rhs);
 bool operator <= (std::string const & lhs, Variant const & rhs);
 bool operator <= (Variant const & lhs, int const & rhs);
 bool operator <= (int const & lhs, Variant const & rhs);
 bool operator <= (Variant const & lhs, double const & rhs);
 bool operator <= (double const & lhs, Variant const & rhs);

 using glob_handler_t = Variant (*)();

 class Framecontext{
   glob_handler_t handler = nullptr;
  public:
   virtual void update_sysstates() = 0;
   virtual void read_chunk(void*,size_t) = 0;
   virtual bool match_chunk(void*,size_t) = 0;
   glob_handler_t get_handler() {return handler;}
   void set_handler(glob_handler_t h) {handler = h;}
   virtual Framecontext* clone() = 0;
   virtual ~Framecontext(){}
 };

 class Executioncontext{
 public:
     virtual void run(State_machine_simulation_core* ctxt) = 0;
 };


}

typedef ceps::ast::Nodebase_ptr (*smcore_plugin_fn_t)(ceps::ast::Call_parameters* params);

class Ism4ceps_plugin_interface{
public:
 virtual void queue_event(std::string ev_name,std::initializer_list<sm4ceps_plugin_int::Variant> vl = {}) = 0;
 virtual void sync_queue_event(int ev_id) = 0;
 virtual size_t argc() = 0;
 virtual sm4ceps_plugin_int::Variant argv(size_t) = 0;
 virtual void start_timer(double,sm4ceps_plugin_int::ev) = 0;
 virtual void start_timer(double,sm4ceps_plugin_int::ev,sm4ceps_plugin_int::id) = 0;
 virtual void start_periodic_timer(double,sm4ceps_plugin_int::ev) = 0;
 virtual void start_periodic_timer(double,sm4ceps_plugin_int::ev,sm4ceps_plugin_int::id) = 0;
 virtual void start_periodic_timer(double,sm4ceps_plugin_int::Variant (*fp)()) = 0;
 virtual void start_periodic_timer(double,sm4ceps_plugin_int::Variant (*fp)(),sm4ceps_plugin_int::id) = 0;
 virtual void stop_timer(sm4ceps_plugin_int::id) = 0;
 virtual void send_raw_frame(void*,size_t,size_t,std::string const &) = 0;
 virtual void register_frame_ctxt(sm4ceps_plugin_int::Framecontext* ctxt, std::string receiver_id) = 0;
 virtual bool in_state(std::initializer_list<sm4ceps_plugin_int::id> state_ids) = 0;
 virtual void register_global_function(std::string name,sm4ceps_plugin_int::Variant (*fp)()) = 0;
 virtual void x_path(sm4ceps_plugin_int::xml_node_set&,std::string path) = 0;
 virtual int as_int(sm4ceps_plugin_int::xml_node_set&) = 0;
 virtual double as_double(sm4ceps_plugin_int::xml_node_set&) = 0;
 virtual std::string as_string(sm4ceps_plugin_int::xml_node_set&) = 0;
 virtual bool empty(sm4ceps_plugin_int::xml_node_set&) = 0;
 virtual bool register_raw_frame_generator_framectxt(std::string,sm4ceps_plugin_int::Framecontext*) = 0;
 virtual void drop_all_sms() = 0;
 virtual void* create_sm(std::string name, std::string full_name,int depth, int order) = 0;
 virtual void* get_sm(std::string name) = 0;
 virtual bool sm_set_parent(void * sm, void * parent) = 0;
 virtual bool sm_add_child(void* sm, void * child) = 0;
 virtual bool sm_set_misc_attributes(void* sm, bool is_thread, bool contains_threads, bool complete, bool join, bool idx) = 0;
 virtual void sm_add_state(void* sm, std::string id, bool is_sm, void* smp, void* parent,bool unresolved,bool idx ) = 0;
 virtual void sm_add_transition(void* sm,int slot, std::string guard, void * orig_parent) = 0;
 virtual void sm_transition_set_from(void* sm,int slot, std::string id , bool is_sm ,void * smp, void * parent, bool unresolved, int idx) = 0;
 virtual void sm_transition_set_to(void* sm,int slot, std::string id , bool is_sm ,void * smp, void * parent, bool unresolved, int idx) = 0;
 virtual void sm_transition_add_ev(void* sm,int slot,std::string id, int idx) = 0;
 virtual void sm_transition_add_action(void* sm,int slot,std::string id, void* assoc_sm) = 0;
 virtual void sm_transition_add_action(void* sm,std::string id, void* assoc_sm) = 0;
 virtual void sm_set_join_state(void* sm, std::string id, bool is_sm, void* smp, void* parent,bool unresolved,bool idx ) = 0;
 virtual void sm_add_ref_to_sm_at_least_one_transition_was_moved_to(void* sm, void* sm_to) = 0;
 virtual void reg_ceps_plugin(std::string name, smcore_plugin_fn_t)=0;
};

#endif
