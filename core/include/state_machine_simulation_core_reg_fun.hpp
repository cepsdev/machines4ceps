#ifndef INC_STATE_MACHINE_SIMULATION_CORE_REG_FUN_HPP
#define INC_STATE_MACHINE_SIMULATION_CORE_REG_FUN_HPP

#include <string>
#include "core/include/state_machine_simulation_core_plugin_interface.hpp"

class IUserdefined_function_registry{
public:
	virtual void regfn(std::string name, int(*fn) ()) = 0;
	virtual void regfn(std::string name, double(*fn) ()) = 0;
	virtual void regfn(std::string name, int (*fn) (int) ) = 0;
	virtual void regfn(std::string name, double (*fn) (int)) = 0;
	virtual void regfn(std::string name, int (*fn) (double)) = 0;
	virtual void regfn(std::string name, double (*fn) (double)) = 0;
	virtual void regfn(std::string name, int(*fn) (int,int)) = 0;
	virtual void regfn(std::string name, double(*fn) (int,int)) = 0;
	virtual void regfn(std::string name, int(*fn) (double,int)) = 0;
	virtual void regfn(std::string name, double(*fn) (double, int)) = 0;
	virtual void regfn(std::string name, int(*fn) (int, double)) = 0;
	virtual void regfn(std::string name, double(*fn) (int, double)) = 0;
	virtual void regfn(std::string name, int(*fn) (double, double)) = 0;
	virtual void regfn(std::string name, double(*fn) (double, double)) = 0;
	virtual void regfn(std::string name, int(*fn) (std::string)) = 0;
	virtual void regfn(std::string name, std::string(*fn) (std::string)) = 0;
	virtual void regfn(std::string name, int(*fn) (int,int,int,int,int,int) ) = 0;
	virtual bool register_action(std::string state_machine_id,std::string action, void(*fn)()) = 0;
	virtual bool register_global_init(void(*fn)()) = 0;
	virtual bool register_guard(std::string,bool(**)()) = 0;
	virtual bool register_raw_frame_generator_gen_msg(std::string,char* (*)(size_t& )) = 0;
	virtual Ism4ceps_plugin_interface* get_plugin_interface() = 0;
};

#endif
