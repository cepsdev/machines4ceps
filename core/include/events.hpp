#ifndef INC_SM4CEPS_EVENTS_INC
#define INC_SM4CEPS_EVENTS_INC

#include <string> 
#include <vector>
#include <map>
#include <algorithm>
#include <set>
#include "ceps_all.hh"
#include <cstring>

namespace sm4ceps{

 class Eventfactory;

 class Event{
	 int id_= -1;
	 int class_id_ = -1;
	 std::vector<ceps::ast::Nodebase_ptr> payload_;
	 int set_id(int id) {auto t = id_; id_=id; return t;}
	 int set_class_id(int id) {auto t = class_id_; class_id_=id; return t;}
 public:
	 int id() const {return id_;}
	 int class_id() const {return class_id_;}
	 std::vector<ceps::ast::Nodebase_ptr>& payload() {return payload_;};
	 friend class Eventfactory;
 };

 class Eventfactory{
	 char * name_pool_ = nullptr;
	 std::size_t name_pool_size_ = 0;
	 int name_pool_reserved_ = 0;
	 int find_name(char const * ev_name) {
		 int i=0;
		 for(;(size_t)i < name_pool_size_ && strcmp(name_pool_+i,ev_name) ; i+=strlen(name_pool_+i)+1);
		 return i;
	 }
	 bool valid_name_idx(int i) {return i>= 0 && (size_t)i < name_pool_size_; }
 public:
	 class name_pool_exhausted{};
	 class ev_class_unknown{};

	 Eventfactory(){name_pool_size_= 1024*32; name_pool_ = new char[name_pool_size_]; }
	 explicit Eventfactory(std::size_t name_pool_size) {name_pool_size_ = name_pool_size; }

	 int reg_event_class(char const * ev_name){
		 if (name_pool_ == nullptr) throw name_pool_exhausted();
		 auto i = find_name(ev_name);
		 if((size_t)i < name_pool_size_) return i;
		 //INVARIANT: ev_name not yet registered
		 auto l = strlen(ev_name);
		 if (name_pool_reserved_+ l + 1 > name_pool_size_ ) throw name_pool_exhausted();
		 memcpy(name_pool_+name_pool_size_,ev_name,l+1);
		 auto t = name_pool_size_;
		 name_pool_size_+=l+1;
		 return t;
	 }
	 int reg_event_class(std::string const & ev_name){return reg_event_class(ev_name.c_str());}

	 Event gen_ev(std::string const & ev_class, bool create_class_entry = true) {
		 Event e;
		 auto class_id = find_name(ev_class.c_str());
		 if (!valid_name_idx(class_id)) { if (!create_class_entry) throw ev_class_unknown();
		 else class_id = reg_event_class(ev_class);}
		 e.set_class_id(class_id);
		 return std::move(e);
	 }

 };

}



#endif
