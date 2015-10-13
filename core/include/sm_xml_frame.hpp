#ifndef INC_SM_XML_FRAME_INC
#define INC_SM_XML_FRAME_INC

#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <set>
#include "ceps_all.hh"
#include "core/include/serialization.hpp"
#include "core/include/sm_raw_frame.hpp"



class Xmlframe_generator:public Rawframe_generator{
	ceps::ast::Nodeset spec_;
public:
	virtual bool readfrom_spec(ceps::ast::Nodeset const & spec);
	ceps::ast::Nodeset const & spec() const {return spec_;}
	ceps::ast::Nodeset & spec() {return spec_;}
	virtual void* gen_msg(State_machine_simulation_core*,size_t&);
	virtual size_t compute_size_of_msg(State_machine_simulation_core*,std::vector<std::string>,bool&);
	virtual bool read_msg(char* data,size_t size,State_machine_simulation_core*,std::vector<std::string>,std::vector<ceps::ast::Nodebase_ptr>&);
};




#endif
