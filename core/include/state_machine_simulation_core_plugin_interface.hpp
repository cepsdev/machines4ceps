#ifndef INC_STATE_MACHINE_SIMULATION_CORE_PLUGIN_INTERFACE_HPP
#define INC_STATE_MACHINE_SIMULATION_CORE_PLUGIN_INTERFACE_HPP

#include <string>
namespace sm4ceps_plugin_int{
 struct Variant{
  double dv_ = 0.0;
  int iv_ = 0;
  std::string sv_ ="";
  enum {Int,Double,String,Undefined} what_ = Undefined;
  Variant (double v):dv_{v},what_{Double}{}
  Variant (int v):iv_{v},what_{Int}{}
  Variant (std::string v):sv_{v},what_{String}{}
  Variant() = default;

 };
}
class Ism4ceps_plugin_interface{
public:
 virtual void queue_event(std::string ev_name,std::initializer_list<sm4ceps_plugin_int::Variant> vl = {}) = 0;
};

#endif
