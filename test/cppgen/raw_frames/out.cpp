/* out.cpp 
   CREATED Fri Jun 24 17:33:01 2016

   GENERATED BY the sm4ceps C++ Generator VERSION 0.50 (c) Tomas Prerovsky <tomas.prerovsky@gmail.com>, ALL RIGHTS RESERVED. 
   Requires C++1y compatible compiler (use --std=c++1y for g++) 
   BASED ON cepS VERSION 1.1 (Jun  1 2016) BUILT WITH GCC 5.2.1 20151010 on GNU/LINUX 64BIT (C) BY THE AUTHORS OF ceps (ceps is hosted at github: https://github.com/cepsdev/ceps.git) 

   Input files (relative paths):
      raw_frame_ex_1_basedefs.ceps
      raw_frame_ex_1_receiver.ceps

   THIS IS A GENERATED FILE.

   *** DO NOT MODIFY. ***
*/




#include "out.hpp"

void init_frame_ctxts();


void user_defined_init(){
 set_value(systemstates::s1 , 0);
 set_value(systemstates::extbg_luftdruck_pneumatiktank_out , 0);
 set_value(systemstates::extbg_luftdruck_pneumatiktank_in , 0);
 set_value(systemstates::extbg_luftdruck_stuetze_links_out , 0);
 set_value(systemstates::extbg_luftdruck_stuetze_links_in , 0);
 set_value(systemstates::extbg_luftdruck_stuetze_rechts_out , 0);
 set_value(systemstates::extbg_luftdruck_stuetze_rechts_in , 0);
 set_value(systemstates::extbg_zustand_mastantrieb_stoerung_kommunikation_out , 0);
 set_value(systemstates::extbg_zustand_mastantrieb_stoerung_kommunikation_in , 0);
 set_value(systemstates::extbg_zustand_mastantrieb_interner_fehler_out , 0);
 set_value(systemstates::extbg_zustand_mastantrieb_interner_fehler_in , 0);
 set_value(systemstates::extbg_zustand_horizontierantriebe_stoerung_kommunikation_out , 0);
 set_value(systemstates::extbg_zustand_horizontierantriebe_stoerung_kommunikation_in , 0);
 set_value(systemstates::extbg_zustand_antrieb_x_ok_out , 0);
 set_value(systemstates::extbg_zustand_antrieb_x_ok_in , 0);
 set_value(systemstates::extbg_zustand_antrieb_y_ok_out , 0);
 set_value(systemstates::extbg_zustand_antrieb_y_ok_in , 0);
 set_value(systemstates::extbg_zustand_talin_daten_gueltig_out , 0);
 set_value(systemstates::extbg_zustand_talin_daten_gueltig_in , 0);
 set_value(systemstates::extbg_zustand_antrieb_x_interner_fehler_out , 0);
 set_value(systemstates::extbg_zustand_antrieb_x_interner_fehler_in , 0);
 set_value(systemstates::extbg_zustand_antrieb_y_interner_fehler_out , 0);
 set_value(systemstates::extbg_zustand_antrieb_y_interner_fehler_in , 0);
 set_value(systemstates::extbg_zustand_heckverteiler_stoerung_out , 0);
 set_value(systemstates::extbg_zustand_heckverteiler_stoerung_in , 0);
 set_value(systemstates::extbg_zustand_heckverteiler_ok_out , 0);
 set_value(systemstates::extbg_zustand_heckverteiler_ok_in , 0);
 set_value(systemstates::extbg_kommunikation_ivenet_ok_out , 0);
 set_value(systemstates::extbg_kommunikation_ivenet_ok_in , 0);
 init_frame_ctxts();
}
 systemstates::State<int> s1;
 systemstates::State<int> extbg_luftdruck_pneumatiktank_out;
 systemstates::State<int> extbg_luftdruck_pneumatiktank_in;
 systemstates::State<int> extbg_luftdruck_stuetze_links_out;
 systemstates::State<int> extbg_luftdruck_stuetze_links_in;
 systemstates::State<int> extbg_luftdruck_stuetze_rechts_out;
 systemstates::State<int> extbg_luftdruck_stuetze_rechts_in;
 systemstates::State<int> extbg_zustand_mastantrieb_stoerung_kommunikation_out;
 systemstates::State<int> extbg_zustand_mastantrieb_stoerung_kommunikation_in;
 systemstates::State<int> extbg_zustand_mastantrieb_interner_fehler_out;
 systemstates::State<int> extbg_zustand_mastantrieb_interner_fehler_in;
 systemstates::State<int> extbg_zustand_horizontierantriebe_stoerung_kommunikation_out;
 systemstates::State<int> extbg_zustand_horizontierantriebe_stoerung_kommunikation_in;
 systemstates::State<int> extbg_zustand_antrieb_x_ok_out;
 systemstates::State<int> extbg_zustand_antrieb_x_ok_in;
 systemstates::State<int> extbg_zustand_antrieb_y_ok_out;
 systemstates::State<int> extbg_zustand_antrieb_y_ok_in;
 systemstates::State<int> extbg_zustand_talin_daten_gueltig_out;
 systemstates::State<int> extbg_zustand_talin_daten_gueltig_in;
 systemstates::State<int> extbg_zustand_antrieb_x_interner_fehler_out;
 systemstates::State<int> extbg_zustand_antrieb_x_interner_fehler_in;
 systemstates::State<int> extbg_zustand_antrieb_y_interner_fehler_out;
 systemstates::State<int> extbg_zustand_antrieb_y_interner_fehler_in;
 systemstates::State<int> extbg_zustand_heckverteiler_stoerung_out;
 systemstates::State<int> extbg_zustand_heckverteiler_stoerung_in;
 systemstates::State<int> extbg_zustand_heckverteiler_ok_out;
 systemstates::State<int> extbg_zustand_heckverteiler_ok_in;
 systemstates::State<int> extbg_kommunikation_ivenet_ok_out;
 systemstates::State<int> extbg_kommunikation_ivenet_ok_in;


 void init_frame_ctxts(){
  systemstates::frame_zustand_komponenten_in_ctxt.init();
 }
 void globfuncs::S1__action__a1(){
 }
 void globfuncs::S1__action__a2(){
 }
 systemstates::Variant globfuncs::handler_frame_zustand_komponenten(){
  std::cout<<systemstates::s1.value()<<std::string{R"(
)"};
return systemstates::Variant{};
}
 

//Frame Context Definitions

void systemstates::frame_zustand_komponenten_in_ctxt_t::update_sysstates(){
  systemstates::extbg_kommunikation_ivenet_ok_in = extbg_kommunikation_ivenet_ok_in_;
  systemstates::extbg_luftdruck_stuetze_links_in = extbg_luftdruck_stuetze_links_in_;
  systemstates::extbg_luftdruck_stuetze_rechts_in = extbg_luftdruck_stuetze_rechts_in_;
  systemstates::extbg_zustand_antrieb_x_interner_fehler_in = extbg_zustand_antrieb_x_interner_fehler_in_;
  systemstates::extbg_zustand_antrieb_x_ok_in = extbg_zustand_antrieb_x_ok_in_;
  systemstates::extbg_zustand_antrieb_y_interner_fehler_in = extbg_zustand_antrieb_y_interner_fehler_in_;
  systemstates::extbg_zustand_antrieb_y_ok_in = extbg_zustand_antrieb_y_ok_in_;
  systemstates::extbg_zustand_heckverteiler_ok_in = extbg_zustand_heckverteiler_ok_in_;
  systemstates::extbg_zustand_mastantrieb_interner_fehler_in = extbg_zustand_mastantrieb_interner_fehler_in_;
  systemstates::extbg_zustand_mastantrieb_stoerung_kommunikation_in = extbg_zustand_mastantrieb_stoerung_kommunikation_in_;
  systemstates::extbg_zustand_talin_daten_gueltig_in = extbg_zustand_talin_daten_gueltig_in_;
  systemstates::s1 = s1_;
 }
void systemstates::frame_zustand_komponenten_in_ctxt_t::read_chunk(void* chunk,size_t){
  raw_frm_dcls::frame_zustand_komponenten_in& in = *((raw_frm_dcls::frame_zustand_komponenten_in*)chunk);
  s1_ =  in.pos_4;
  extbg_luftdruck_stuetze_links_in_ =  in.pos_5;
  extbg_luftdruck_stuetze_rechts_in_ =  in.pos_6;
  extbg_zustand_mastantrieb_stoerung_kommunikation_in_ = ( in.pos_7==0?0:1 );
  extbg_zustand_mastantrieb_interner_fehler_in_ = ( in.pos_14==0?0:1 );
  extbg_zustand_antrieb_x_ok_in_ = ( in.pos_15==0?0:1 );
  extbg_zustand_antrieb_y_ok_in_ = ( in.pos_16==0?0:1 );
  extbg_zustand_talin_daten_gueltig_in_ = ( in.pos_17==0?0:1 );
  extbg_zustand_antrieb_x_interner_fehler_in_ = ( in.pos_21==0?0:1 );
  extbg_zustand_antrieb_y_interner_fehler_in_ = ( in.pos_22==0?0:1 );
  extbg_zustand_heckverteiler_ok_in_ = ( in.pos_23==0?0:1 );
  extbg_kommunikation_ivenet_ok_in_ = ( in.pos_31==0?0:1 );
 }
bool systemstates::frame_zustand_komponenten_in_ctxt_t::match_chunk(void* chunk,size_t chunk_size){
  if(chunk_size != 9) return false;
  raw_frm_dcls::frame_zustand_komponenten_in& in = *((raw_frm_dcls::frame_zustand_komponenten_in*)chunk);
  if (1 != in.pos_1) return false;
  if (0 != in.pos_2) return false;
  if (9 != in.pos_3) return false;
  return true;
 }
void systemstates::frame_zustand_komponenten_in_ctxt_t::init(){
  extbg_kommunikation_ivenet_ok_in_  = systemstates::extbg_kommunikation_ivenet_ok_in;
  extbg_luftdruck_stuetze_links_in_  = systemstates::extbg_luftdruck_stuetze_links_in;
  extbg_luftdruck_stuetze_rechts_in_  = systemstates::extbg_luftdruck_stuetze_rechts_in;
  extbg_zustand_antrieb_x_interner_fehler_in_  = systemstates::extbg_zustand_antrieb_x_interner_fehler_in;
  extbg_zustand_antrieb_x_ok_in_  = systemstates::extbg_zustand_antrieb_x_ok_in;
  extbg_zustand_antrieb_y_interner_fehler_in_  = systemstates::extbg_zustand_antrieb_y_interner_fehler_in;
  extbg_zustand_antrieb_y_ok_in_  = systemstates::extbg_zustand_antrieb_y_ok_in;
  extbg_zustand_heckverteiler_ok_in_  = systemstates::extbg_zustand_heckverteiler_ok_in;
  extbg_zustand_mastantrieb_interner_fehler_in_  = systemstates::extbg_zustand_mastantrieb_interner_fehler_in;
  extbg_zustand_mastantrieb_stoerung_kommunikation_in_  = systemstates::extbg_zustand_mastantrieb_stoerung_kommunikation_in;
  extbg_zustand_talin_daten_gueltig_in_  = systemstates::extbg_zustand_talin_daten_gueltig_in;
  s1_  = systemstates::s1;
 }
sm4ceps_plugin_int::Framecontext*  systemstates::frame_zustand_komponenten_in_ctxt_t::clone(){
  return new frame_zustand_komponenten_in_ctxt_t(*this);
 }
 systemstates::frame_zustand_komponenten_in_ctxt_t::~frame_zustand_komponenten_in_ctxt_t (){
   }
 systemstates::frame_zustand_komponenten_in_ctxt_t systemstates::frame_zustand_komponenten_in_ctxt;


extern "C" void init_plugin(IUserdefined_function_registry* smc){
smcore_interface = smc->get_plugin_interface();
smc->register_global_init(user_defined_init);
smc->register_action("S1","a1", globfuncs::S1__action__a1);
smc->register_action("S1","a2", globfuncs::S1__action__a2);
systemstates::frame_zustand_komponenten_in_ctxt.set_handler(&globfuncs::handler_frame_zustand_komponenten);
smcore_interface->register_frame_ctxt(&systemstates::frame_zustand_komponenten_in_ctxt,"channel_can_1_in");
}

std::ostream& operator << (std::ostream& o, systemstates::Variant const & v)
{
 if (v.what_ == systemstates::Variant::Int)
  o << v.iv_;
 else if (v.what_ == systemstates::Variant::Double)
  o << std::to_string(v.dv_);
 else if (v.what_ == systemstates::Variant::String)
  o << v.sv_;
 else
  o << "(Undefined)";
 return o;
}

size_t globfuncs::argc(){
 return smcore_interface->argc();
}sm4ceps_plugin_int::Variant globfuncs::argv(size_t j){
 return smcore_interface->argv(j);
}
void globfuncs::start_timer(double t,sm4ceps_plugin_int::ev ev_){ smcore_interface->start_timer(t,ev_); }
void globfuncs::start_timer(double t,sm4ceps_plugin_int::ev ev_,sm4ceps_plugin_int::id id_){smcore_interface->start_timer(t,ev_,id_);}
void globfuncs::start_periodic_timer(double t ,sm4ceps_plugin_int::ev ev_){smcore_interface->start_periodic_timer(t,ev_);}
void globfuncs::start_periodic_timer(double t,sm4ceps_plugin_int::ev ev_,sm4ceps_plugin_int::id id_){smcore_interface->start_periodic_timer(t,ev_,id_);}
void globfuncs::stop_timer(sm4ceps_plugin_int::id id_){smcore_interface->stop_timer(id_);}