#include "core/include/state_machine_simulation_core_plugin_interface.hpp"

double sm4ceps_plugin_int::abs(sm4ceps_plugin_int::Variant const &v){if (v.what_ == Variant::Double) return std::abs(v.dv_); return std::abs(v.iv_); }


 bool sm4ceps_plugin_int::operator == (Variant const & lhs, std::string const & rhs) {return lhs.sv_ == rhs;}
 bool sm4ceps_plugin_int::operator == (std::string const & lhs, Variant const & rhs) {return rhs.sv_ == lhs;}
 bool sm4ceps_plugin_int::operator == (Variant const & lhs, int const & rhs) {return lhs.iv_ == rhs;}
 bool sm4ceps_plugin_int::operator == (int const & lhs, Variant const & rhs) {return rhs.iv_ == lhs;}
 bool sm4ceps_plugin_int::operator == (Variant const & lhs, double const & rhs) {return lhs.dv_ == rhs;}
 bool sm4ceps_plugin_int::operator == (double const & lhs, Variant const & rhs) {return rhs.dv_ == lhs;}

 bool sm4ceps_plugin_int::operator != (Variant const & lhs, std::string const & rhs) {return lhs.sv_ != rhs;}
 bool sm4ceps_plugin_int::operator != (std::string const & lhs, Variant const & rhs) {return rhs.sv_ != lhs;}
 bool sm4ceps_plugin_int::operator != (Variant const & lhs, int const & rhs) {return lhs.iv_ != rhs;}
 bool sm4ceps_plugin_int::operator != (int const & lhs, Variant const & rhs) {return rhs.iv_ != lhs;}
 bool sm4ceps_plugin_int::operator != (Variant const & lhs, double const & rhs) {return lhs.dv_ != rhs;}
 bool sm4ceps_plugin_int::operator != (double const & lhs, Variant const & rhs) {return rhs.dv_ != lhs;}

 bool sm4ceps_plugin_int::operator > (Variant const & lhs, std::string const & rhs) {return lhs.sv_ > rhs;}
 bool sm4ceps_plugin_int::operator > (std::string const & lhs, Variant const & rhs) {return rhs.sv_ > lhs;}
 bool sm4ceps_plugin_int::operator > (Variant const & lhs, int const & rhs) {return lhs.iv_ > rhs;}
 bool sm4ceps_plugin_int::operator > (int const & lhs, Variant const & rhs) {return rhs.iv_ > lhs;}
 bool sm4ceps_plugin_int::operator > (Variant const & lhs, double const & rhs) {return lhs.dv_ > rhs;}
 bool sm4ceps_plugin_int::operator > (double const & lhs, Variant const & rhs) {return rhs.dv_ > lhs;}

 bool sm4ceps_plugin_int::operator >= (Variant const & lhs, std::string const & rhs) {return lhs.sv_ >= rhs;}
 bool sm4ceps_plugin_int::operator >= (std::string const & lhs, Variant const & rhs) {return rhs.sv_ >= lhs;}
 bool sm4ceps_plugin_int::operator >= (Variant const & lhs, int const & rhs) {return lhs.iv_ >= rhs;}
 bool sm4ceps_plugin_int::operator >= (int const & lhs, Variant const & rhs) {return rhs.iv_ >= lhs;}
 bool sm4ceps_plugin_int::operator >= (Variant const & lhs, double const & rhs) {return lhs.dv_ >= rhs;}
 bool sm4ceps_plugin_int::operator >= (double const & lhs, Variant const & rhs) {return rhs.dv_ >= lhs;}

 bool sm4ceps_plugin_int::operator < (Variant const & lhs, std::string const & rhs) {return lhs.sv_ < rhs;}
 bool sm4ceps_plugin_int::operator < (std::string const & lhs, Variant const & rhs) {return rhs.sv_ < lhs;}
 bool sm4ceps_plugin_int::operator < (Variant const & lhs, int const & rhs) {return lhs.iv_ < rhs;}
 bool sm4ceps_plugin_int::operator < (int const & lhs, Variant const & rhs) {return rhs.iv_ < lhs;}
 bool sm4ceps_plugin_int::operator < (Variant const & lhs, double const & rhs) {return lhs.dv_ < rhs;}
 bool sm4ceps_plugin_int::operator < (double const & lhs, Variant const & rhs) {return rhs.dv_ < lhs;}

 bool sm4ceps_plugin_int::operator <= (Variant const & lhs, std::string const & rhs) {return lhs.sv_ <= rhs;}
 bool sm4ceps_plugin_int::operator <= (std::string const & lhs, Variant const & rhs) {return rhs.sv_ <= lhs;}
 bool sm4ceps_plugin_int::operator <= (Variant const & lhs, int const & rhs) {return lhs.iv_ <= rhs;}
 bool sm4ceps_plugin_int::operator <= (int const & lhs, Variant const & rhs) {return rhs.iv_ <= lhs;}
 bool sm4ceps_plugin_int::operator <= (Variant const & lhs, double const & rhs) {return lhs.dv_ <= rhs;}
 bool sm4ceps_plugin_int::operator <= (double const & lhs, Variant const & rhs) {return rhs.dv_ <= lhs;}


