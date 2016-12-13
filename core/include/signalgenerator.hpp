#ifndef INC_SIGNALGENERATOR_HPP
#define INC_SIGNALGENERATOR_HPP
#include "ceps_all.hh"

namespace sm4ceps{ namespace datasources{
 class Signalgenerator{
	bool native_ = false;
	double delta_ = 0.1;
	bool loop_ = true;
	double stop_time_ = -1.0;
    std::vector<double> values_;
 public:
    bool& native() {return native_;}
    double& delta() {return delta_;}
    bool& loop() {return loop_;}
    double& stop_time() {return stop_time_;}
    std::vector<double>& values(){return values_;}
 };
}}
#endif
