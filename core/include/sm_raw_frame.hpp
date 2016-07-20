#ifndef INC_SM_RAW_FRAME_INC
#define INC_SM_RAW_FRAME_INC

#include <string> 
#include <vector>
#include <map>
#include <algorithm>
#include <set> 
#include "ceps_all.hh"
#include "core/include/serialization.hpp"
class State_machine_simulation_core;

class Rawframe_generator{
protected:
	char* (*fn_gen_msg_native_)(size_t& ) = nullptr;
public:
	virtual bool readfrom_spec(ceps::ast::Nodeset const & spec) = 0;
	virtual void* gen_msg(State_machine_simulation_core*,size_t&) = 0;
	virtual size_t compute_size_of_msg(State_machine_simulation_core*,std::vector<std::string>,bool&) = 0;
	virtual bool read_msg(char* data,
			              size_t size,
						  State_machine_simulation_core*,
						  std::vector<std::string>,
						  std::vector<ceps::ast::Nodebase_ptr>&) = 0;
	virtual size_t header_length() = 0;
	void set_native_impl_gen_msg( char* (*fn)(size_t& ) ){
		fn_gen_msg_native_ = fn;
	}
	virtual ~Rawframe_generator() = default;
};

class Podframe_generator:public Rawframe_generator{
	ceps::ast::Nodeset spec_;
	size_t header_length_ = 0;
public:
	bool readfrom_spec(ceps::ast::Nodeset const & spec);
	ceps::ast::Nodeset const & spec() const {return spec_;}
	ceps::ast::Nodeset & spec() {return spec_;}
	void* gen_msg(State_machine_simulation_core*,size_t&);
	size_t compute_size_of_msg(State_machine_simulation_core*,std::vector<std::string>,bool&);
	bool read_msg(char* data,size_t size,State_machine_simulation_core*,std::vector<std::string>,std::vector<ceps::ast::Nodebase_ptr>&);
	size_t header_length();
};

#endif
