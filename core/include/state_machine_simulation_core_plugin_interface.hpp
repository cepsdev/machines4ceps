#ifndef INC_STATE_MACHINE_SIMULATION_CORE_PLUGIN_INTERFACE_HPP
#define INC_STATE_MACHINE_SIMULATION_CORE_PLUGIN_INTERFACE_HPP

#include <string>
#include <vector>
#include <cmath>

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
   ev(std::string name):name_(name){}
   ev(std::string name,std::vector<Variant> const & args):name_(name),args_(args){}

 };

 struct id{
   std::string name_;
   id(std::string name):name_(name){}
   id() = default;
 };


 double abs(Variant const &v);


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


}
class Ism4ceps_plugin_interface{
public:
 virtual void queue_event(std::string ev_name,std::initializer_list<sm4ceps_plugin_int::Variant> vl = {}) = 0;
 virtual size_t argc() = 0;
 virtual sm4ceps_plugin_int::Variant argv(size_t) = 0;
 virtual void start_timer(double,sm4ceps_plugin_int::ev) = 0;
 virtual void start_timer(double,sm4ceps_plugin_int::ev,sm4ceps_plugin_int::id) = 0;
 virtual void start_periodic_timer(double,sm4ceps_plugin_int::ev) = 0;
 virtual void start_periodic_timer(double,sm4ceps_plugin_int::ev,sm4ceps_plugin_int::id) = 0;
 virtual void stop_timer(sm4ceps_plugin_int::id) = 0;

};

#endif
