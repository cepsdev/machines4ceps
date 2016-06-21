#define _CRT_SECURE_NO_WARNINGS

#include "core/include/state_machine_simulation_core.hpp"
#include "core/include/base_defs.hpp"
#include <map>
#include <iostream>
#include <set>
#include <unordered_set>


using namespace ceps::ast;

static std::string sysstates_namespace = "systemstates";
static std::string guards_namespace = "guards";
static std::string global_functions_namespace = "globfuncs";
static std::string init_func = "void user_defined_init()";
static std::string builtin_funcs_namespace = "globfuncs";
static std::string smcore_singleton = "smcore_interface";



template<typename F, typename T> void traverse_sm(std::unordered_set<State_machine*>& m,State_machine* sm, T const & sms, F f){
	f(sm);

	for(auto state: sm->states()){
		if (!state->is_sm() || state->smp() == nullptr) continue;
		if (m.find(state->smp()) != m.end()) continue;
		m.insert(state->smp());
		traverse_sm(m,state->smp(),sms,f);
	}

	for(auto subsm: sm->children()){
		//assert(m.find(subsm) != m.end());
		if (m.find(subsm) != m.end()) continue;
		m.insert(subsm);
		traverse_sm(m,subsm,sms,f);
	}
}

template<typename F, typename T> void traverse_sms(T const & sms, F f){
	std::unordered_set<State_machine*> m;
	for(auto sm: sms){
	 if (m.find(sm) != m.end()) continue;
	 traverse_sm(m,sm,sms,f);
	 m.insert(sm);
	}
}
const std::string out_hpp_prefix = R"(
 static Ism4ceps_plugin_interface* smcore_interface; 
)";

const std::string out_hpp_systemstates_prefix = R"(

/*struct Variant{
  double dv_ = 0.0;
  int iv_ = 0;
  std::string sv_ ="";
  enum {Int,Double,String,Undefined} what_ = Undefined;
  Variant (double v):dv_{v},what_{Double}{}
  Variant (int v):iv_{v},what_{Int}{}
  Variant (std::string v):sv_{v},what_{String}{}
  Variant() = default;
  operator sm4ceps_plugin_int::Variant(){
   sm4ceps_plugin_int::Variant v;
   v.dv_ = dv_;v.iv_ = iv_;v.sv_ = sv_; 
   if (what_ == Int) v.what_ = sm4ceps_plugin_int::Variant::Int;
   else if (what_ == Double) v.what_ = sm4ceps_plugin_int::Variant::Double;
   else if (what_ == String) v.what_ = sm4ceps_plugin_int::Variant::String;
   else v.what_ = sm4ceps_plugin_int::Variant::Undefined;

   return v;
  }

 };*/

 using sm4ceps_plugin_int::Variant;
 using sm4ceps_plugin_int::ev;
 using sm4ceps_plugin_int::id;


 

 template<typename T> class State{
   T v_;
   bool changed_ = true;
   bool default_constructed_ = true;
 public:
   State() = default; 
   State(T const & v):v_{v},default_constructed_{false} {}
   State& operator = (State const & rhs){
     if (!changed_ && !default_constructed_) changed_ = v_ != rhs.v_;
     else if (default_constructed_) {default_constructed_ = false;changed_ = true;}
     v_ = rhs.v_;
     return *this;
   }
   State& operator = (T const & rhs){
     if (!changed_ && !default_constructed_) changed_ = v_ != rhs;
     else if (default_constructed_) {changed_ = true; default_constructed_=false;}
     v_ = rhs;
     return *this;
   }
   bool changed() {auto t = changed_;changed_=false;return t;}

   T& value() {return v_;}
   T value() const {return v_;}
 
  };

 std::ostream& operator << (std::ostream& o, State<int> & v){
  o << v.value();
  return o;
 }

 std::ostream& operator << (std::ostream& o, State<double> & v){
  o << v.value();
  return o;
 }

 double abs(Variant const &v){if (v.what_ == Variant::Double) return std::abs(v.dv_); return std::abs(v.iv_); }
 

 bool operator == (Variant const & lhs, std::string const & rhs) {return lhs.sv_ == rhs;}
 bool operator == (std::string const & lhs, Variant const & rhs) {return rhs.sv_ == lhs;}
 bool operator == (Variant const & lhs, int const & rhs) {return lhs.iv_ == rhs;}
 bool operator == (int const & lhs, Variant const & rhs) {return rhs.iv_ == lhs;}
 bool operator == (Variant const & lhs, double const & rhs) {return lhs.dv_ == rhs;}
 bool operator == (double const & lhs, Variant const & rhs) {return rhs.dv_ == lhs;}

 bool operator != (Variant const & lhs, std::string const & rhs) {return lhs.sv_ != rhs;}
 bool operator != (std::string const & lhs, Variant const & rhs) {return rhs.sv_ != lhs;}
 bool operator != (Variant const & lhs, int const & rhs) {return lhs.iv_ != rhs;}
 bool operator != (int const & lhs, Variant const & rhs) {return rhs.iv_ != lhs;}
 bool operator != (Variant const & lhs, double const & rhs) {return lhs.dv_ != rhs;}
 bool operator != (double const & lhs, Variant const & rhs) {return rhs.dv_ != lhs;}

 bool operator > (Variant const & lhs, std::string const & rhs) {return lhs.sv_ > rhs;}
 bool operator > (std::string const & lhs, Variant const & rhs) {return rhs.sv_ > lhs;}
 bool operator > (Variant const & lhs, int const & rhs) {return lhs.iv_ > rhs;}
 bool operator > (int const & lhs, Variant const & rhs) {return rhs.iv_ > lhs;}
 bool operator > (Variant const & lhs, double const & rhs) {return lhs.dv_ > rhs;}
 bool operator > (double const & lhs, Variant const & rhs) {return rhs.dv_ > lhs;}

 bool operator >= (Variant const & lhs, std::string const & rhs) {return lhs.sv_ >= rhs;}
 bool operator >= (std::string const & lhs, Variant const & rhs) {return rhs.sv_ >= lhs;}
 bool operator >= (Variant const & lhs, int const & rhs) {return lhs.iv_ >= rhs;}
 bool operator >= (int const & lhs, Variant const & rhs) {return rhs.iv_ >= lhs;}
 bool operator >= (Variant const & lhs, double const & rhs) {return lhs.dv_ >= rhs;}
 bool operator >= (double const & lhs, Variant const & rhs) {return rhs.dv_ >= lhs;}

 bool operator < (Variant const & lhs, std::string const & rhs) {return lhs.sv_ < rhs;}
 bool operator < (std::string const & lhs, Variant const & rhs) {return rhs.sv_ < lhs;}
 bool operator < (Variant const & lhs, int const & rhs) {return lhs.iv_ < rhs;}
 bool operator < (int const & lhs, Variant const & rhs) {return rhs.iv_ < lhs;}
 bool operator < (Variant const & lhs, double const & rhs) {return lhs.dv_ < rhs;}
 bool operator < (double const & lhs, Variant const & rhs) {return rhs.dv_ < lhs;}

 bool operator <= (Variant const & lhs, std::string const & rhs) {return lhs.sv_ <= rhs;}
 bool operator <= (std::string const & lhs, Variant const & rhs) {return rhs.sv_ <= lhs;}
 bool operator <= (Variant const & lhs, int const & rhs) {return lhs.iv_ <= rhs;}
 bool operator <= (int const & lhs, Variant const & rhs) {return rhs.iv_ <= lhs;}
 bool operator <= (Variant const & lhs, double const & rhs) {return lhs.dv_ <= rhs;}
 bool operator <= (double const & lhs, Variant const & rhs) {return rhs.dv_ <= lhs;}

 State<int>& set_value(State<int>& lhs, Variant const & rhs){lhs.value() = rhs.iv_; return lhs;}
 State<int>& set_value(State<int>& lhs, int rhs){lhs.value() = rhs; return lhs;}
 State<double>& set_value(State<double>& lhs, double rhs){lhs.value() = rhs; return lhs;}
 State<std::string>& set_value(State<std::string>& lhs, std::string rhs){lhs.value() = rhs; return lhs;}

 //void queue_event(std::string ev_name,std::initializer_list<Variant> vl = {});

)";





const std::string out_hpp_guards_prefix = R"(
 using Guard = bool(*)();
 using Guard_impl = bool ();
)";

const std::string out_hpp_global_functions_prefix = R"(
 extern bool in_state(std::initializer_list<systemstates::id>);
 extern void start_timer(double,sm4ceps_plugin_int::ev);
 extern void start_timer(double,sm4ceps_plugin_int::ev,sm4ceps_plugin_int::id);
 extern void start_periodic_timer(double,sm4ceps_plugin_int::ev);
 extern void start_periodic_timer(double,sm4ceps_plugin_int::ev,sm4ceps_plugin_int::id);
 extern void stop_timer(sm4ceps_plugin_int::id);
 size_t argc();
 sm4ceps_plugin_int::Variant argv(size_t);
 extern bool send(systemstates::id,systemstates::id);
)";

struct Indent{
	int indentation = 0;

	void print_indentation(std::ostream& out)
	{
		for(int i = 0; i < indentation; ++i)
			out << " ";
	}

	int indent_incr(){
		return ++indentation;
	}

	int indent_decr(){
		return --indentation;
	}
};


struct Type{
	enum {Int,String,Double,Struct,Undefined} t;
	std::string name;
};

bool struct_contains_single_value(ceps::ast::Struct const& outer,Nodebase_ptr& v,Type& t){
 if(outer.children().size() != 1) return false;
 v = outer.children()[0];
 if (v->kind() == ceps::ast::Ast_node_kind::int_literal){
	 t = Type{Type::Int};
	 return true;
 } else if (v->kind() == ceps::ast::Ast_node_kind::float_literal){
	 t = Type{Type::Double};
	 return true;
 } else  if (v->kind() == ceps::ast::Ast_node_kind::string_literal){
	 t = Type{Type::String};
	 return true;
 }
 return false;
}

struct sm4ceps_struct{
	std::string name;
	Type t;
	int value_int;
	double value_double;
	std::string value_string;
	std::vector<sm4ceps_struct*> members;
	sm4ceps_struct* find_member(std::string name){
		for(auto m : members) if (m->name == name) return m;
		return nullptr;
	}
};

void write_cpp_value(std::ostream& os,Nodebase_ptr v,Type t){
	if (t.t == Type::Int) os << value(as_int_ref(v));
	else if (t.t == Type::Double) os << value(as_double_ref(v));
	else if (t.t == Type::String) os << "std::string{R\"(" << value(as_string_ref(v)) << ")\"}";
	else os << "{}";
}

void write_cpp_sm4ceps_struct_impl(Indent& indent,std::ostream& os,sm4ceps_struct* str){
  if (str->t.t != Type::Struct){
	  indent.print_indentation(os);
	  if (str->t.t == Type::Int) os << "State<int> ";
	  else if (str->t.t == Type::Double) os << "State<double> ";
	  else  os << "State<std::string> ";
	  os << str->name << " = ";
	  if (str->t.t == Type::Int) os << str->value_int;
	  else if (str->t.t == Type::Double) os << str->value_double;
	  else  os << str->value_string;
	  os << ";\n";
  }	else {
	  indent.print_indentation(os);
	  os << "struct{\n";
	  indent.indent_incr();
	  for(auto m:str->members)write_cpp_sm4ceps_struct_impl(indent,os,m);
	  indent.indent_decr();
	  indent.print_indentation(os);os << "} " << str->name << ";\n";
  }
}

void write_cpp_sm4ceps_struct(Indent& indent,std::ostream& os,sm4ceps_struct* str){
	indent.print_indentation(os);
	os<< "struct "<<str->name<<"{\n";
    indent.indent_incr();
	for(auto m:str->members)write_cpp_sm4ceps_struct_impl(indent,os,m);
	indent.indent_decr();
	indent.print_indentation(os);os << "};\n";
}


std::vector<sm4ceps_struct*> sm4ceps_structs;

sm4ceps_struct* find_sm4ceps_struct(std::string name){
	for(auto e: sm4ceps_structs) if (e->name == name) return e;
	return nullptr;
}

void add_sm4ceps_struct(sm4ceps_struct* s) { sm4ceps_structs.push_back(s); }
void add_sm4ceps_struct(sm4ceps_struct* s,std::string name) {
	s->name=name;
	for(auto& e: sm4ceps_structs){
		if (e->name == name)
		{
			e = s;
			return;
		}
	}
	sm4ceps_structs.push_back(s);
}


void add_sm4ceps_struct_impl(Struct& outer,sm4ceps_struct* parent) {
	for(auto child : outer.children()){
		if (child->kind() == Ast_node_kind::identifier){
			auto& current = as_id_ref(child);
			parent->members.push_back(new sm4ceps_struct{name(current),Type{Type::Int,name(current)},0} );
			continue;
		}
		if (child->kind() != Ast_node_kind::structdef) continue;
		auto& current = as_struct_ref(child);
		Nodebase_ptr v;
		Type t;
		if (struct_contains_single_value(current,v,t)){
			if (t.t == Type::Int) {parent->members.push_back(new sm4ceps_struct{name(current),Type{Type::Int,name(current)},value(as_int_ref(v))} );}
			else if (t.t == Type::Double) {parent->members.push_back(new sm4ceps_struct{name(current),Type{Type::Double,name(current)},0,value(as_double_ref(v))} );}
			else if (t.t == Type::String) {parent->members.push_back(new sm4ceps_struct{name(current),Type{Type::String,name(current)},0,0.0,value(as_string_ref(v))} );}
		} else {
		  auto new_parent = new sm4ceps_struct{name(current),Type{Type::Struct,name(current)}};
		  add_sm4ceps_struct_impl(current,new_parent);
		  parent->members.push_back(new_parent);
		}
	}
}

void add_sm4ceps_struct(Nodebase_ptr p) {
	if (p->kind() != Ast_node_kind::structdef) return;
	auto& outer = as_struct_ref(p);
	auto& n = name(outer);
	auto parent = new sm4ceps_struct{n,Type{Type::Struct,n}};
	add_sm4ceps_struct(parent);
	add_sm4ceps_struct_impl(outer,parent);
}



sm4ceps_struct* clone_sm4ceps_struct(sm4ceps_struct* s){

	auto s_cpy = new sm4ceps_struct{s->name,s->t,s->value_int,s->value_double,s->value_string};
	if (s->t.t != Type::Struct) return s_cpy;
	for(auto e: s->members){
		auto e_cpy = clone_sm4ceps_struct(e);
		s_cpy->members.push_back(e_cpy);
	}
	return s_cpy;
}

sm4ceps_struct* clone_sm4ceps_struct(std::string name){
	auto v = find_sm4ceps_struct(name);
	if (v == nullptr) return nullptr;
	return clone_sm4ceps_struct(v);
}

sm4ceps_struct* struct_assign(std::string compound_id, std::string struct_id){
	size_t n=0;
	sm4ceps_struct* parent = nullptr;
	sm4ceps_struct* current_substruct = parent;
	sm4ceps_struct* rhs = find_sm4ceps_struct(struct_id);
	if(rhs == nullptr) return nullptr;

	std::string base_id;
	//std::cout << "struct_assign:" <<compound_id << std::endl;
	for(;n<compound_id.length();){
	 auto nn = compound_id.find_first_of('.',n);
	 auto s = (nn == std::string::npos) ? compound_id.substr(n) : compound_id.substr(n,nn-n);
	 //std::cout << "s=" << s<<std::endl;
	 if (parent == nullptr){
		 base_id = s;
		 parent = current_substruct = find_sm4ceps_struct(s);
		 if (parent == nullptr){
			 auto cpy = clone_sm4ceps_struct(struct_id);
			 if (cpy == nullptr) return nullptr;
			 add_sm4ceps_struct(cpy,s);
			 return cpy;
		 } else {
			 parent = current_substruct = clone_sm4ceps_struct(base_id);
			 if (current_substruct == nullptr) return nullptr;
			 add_sm4ceps_struct(current_substruct,base_id/*+"___"+std::to_string(counter)*/);
		 }
	 } else {
		 current_substruct = current_substruct->find_member(s);
		 if (current_substruct  == nullptr) return nullptr;
	 }
	 if (nn == std::string::npos) break;
	 n = nn+1;
	}
	//Invariant current_substruct != nullptr
	auto cpy_rhs = clone_sm4ceps_struct(rhs);
	current_substruct->t.t = Type::Struct;
	for(auto m:current_substruct->members) delete m;
	current_substruct->members.clear();
	for(auto m:cpy_rhs->members) current_substruct->members.push_back(m);
	return parent;
}





void write_copyright_and_timestamp(std::ostream& out, std::string title,bool b){
	if(!b) return;
	time_t timer;time(&timer);tm * timeinfo;timeinfo = localtime(&timer);
	out
		<< "/* "<< title <<" " << std::endl
		<< "   CREATED " << asctime(timeinfo) << std::endl
		<< "   GENERATED BY the sm4ceps C++ Generator VERSION 0.50 (c) Tomas Prerovsky <tomas.prerovsky@gmail.com>, ALL RIGHTS RESERVED. \n"
		<< "   Requires C++1y compatible compiler (use --std=c++1y for g++) \n"
		<< "   BASED ON cepS "<< ceps::get_version_info() << std::endl
		<< "   THIS IS A GENERATED FILE. DO NOT MODIFY.\n*/\n"
		<< std::endl << std::endl;
}


struct sysstate{
	std::string name;
	int pos;
	Type type;
	bool operator < (sysstate const & rhs) const {return pos < rhs.pos;}
	bool operator == (sysstate const & rhs) const {return pos == rhs.pos;}
};



std::map<sysstate,Nodebase_ptr> systemstate_first_def; //systemstate name => first definition
std::map<std::string,std::pair<int,Nodebase_ptr> > struct_defs; //struct decl. => pos,definition
std::vector< std::pair<std::string,Nodebase_ptr>> guard_backpatch_vec;
std::map<Nodebase_ptr,int> guard_backpatch_idx;

static int guard_ctr = 0;

class Cppgenerator{
	std::map<std::string,sysstate> sysstates_;
	void write_cpp_expr_impl(State_machine_simulation_core* smp,
				                 Indent& indent,
								 std::ostream& os,
								 Nodebase_ptr p,
								 State_machine* cur_sm,std::vector<std::string>& parameters,
								 bool inside_func_param_list=false);
	bool write_cpp_stmt_impl(State_machine_simulation_core* smp,Indent& indent,std::ostream& os,Nodebase_ptr p,State_machine* cur_sm,std::vector<std::string>& parameters);
public:
	using sysstates_t = decltype(sysstates_);
	sysstates_t& sysstates() {return sysstates_;}
	sysstates_t const & sysstates() const {return sysstates_;}

	void write_cpp_expr(State_machine_simulation_core* smp,Indent& indent,std::ostream& os,Nodebase_ptr p,State_machine* cur_sm,std::vector<std::string>& parameters);
	bool write_cpp_stmt(State_machine_simulation_core* smp,Indent& indent,std::ostream& os,Nodebase_ptr p,State_machine* cur_sm,std::vector<std::string>& parameters);
};

template<typename F> void for_all_nodes(State_machine_simulation_core* smp, Nodeset& ns,F f){
	int node_no = 0;
	std::set<std::string> util_already_seen;

	for(auto e: ns.nodes()) f(smp,e, node_no++,util_already_seen);
}

Type determine_primitive_type(Nodebase_ptr node){
	if (node->kind() == Ast_node_kind::int_literal) return Type{Type::Int};
	if (node->kind() == Ast_node_kind::float_literal) return Type{Type::Double};
	if (node->kind() == Ast_node_kind::string_literal) return Type{Type::String};

	return Type{Type::Undefined};
}

bool is_compound_id(std::string const & id){
	return id.find_first_of(".") != std::string::npos;
}

void store_struct_defs(State_machine_simulation_core* smp,Nodebase_ptr p,int node_no,std::set<std::string>& already_seen){
	if(p->kind() != Ast_node_kind::structdef) return;
	auto& sdef = ceps::ast::as_struct_ref(p);
	auto name_s = name(sdef);
	if(already_seen.find(name_s) != already_seen.end() ) return;
	struct_defs[name_s] = std::make_pair(node_no,p);
}

void store_first_state_assign(State_machine_simulation_core* smp,Nodebase_ptr p,int node_no,std::set<std::string>& already_seen){
	if (p->kind() == Ast_node_kind::symbol){
		auto kind_of_sym = kind(as_symbol_ref(p));
		if (kind_of_sym != "Systemstate" && kind_of_sym != "Systemparameter") return;
		auto lhs_id = name(as_symbol_ref(p));
		sysstate s{lhs_id,node_no,Type::Undefined};
		if (already_seen.find(lhs_id) != already_seen.end()) return;
		already_seen.insert(lhs_id);
		systemstate_first_def[s] = new ceps::ast::Int(0,ceps::ast::all_zero_unit(), nullptr, nullptr, nullptr);
		return;
	}
	if(!smp->is_assignment_op(p)) return;
	std::string lhs_id;
	auto& binop = as_binop_ref(p);
	if(!smp->is_assignment_to_state(binop, lhs_id)) return;
	if (is_compound_id(lhs_id)) return;
	sysstate s{lhs_id,node_no,Type::Undefined};
	if (already_seen.find(lhs_id) != already_seen.end()) return;
	already_seen.insert(lhs_id);
	systemstate_first_def[s] = binop.right();
}

void print_first_state_assigns(){
	for(auto e : systemstate_first_def){
		std::cout << e.first.name << " at " << e.first.pos << std::endl;
	}
}

void write_cpp_systemstate_type(std::ostream& os,Type t){
	if(t.t == Type::Int) os << "State<int>";
	else if(t.t == Type::Double) os << "State<double>";
	else if(t.t == Type::String) os << "State<std::string>";
	else if(t.t == Type::Struct) os << t.name;
	else os << "UNKNOWN_TYPE";
}

void write_cpp_systemstate_declaration(std::ostream& os,sysstate const & state){
	write_cpp_systemstate_type(os,state.type);
	os << " ";
	os << state.name;
}


std::pair<std::string,std::string> cpp_templatized_decl(ceps::ast::Struct & func,std::vector<std::string>& parameters){
	auto params = ceps::ast::Nodeset(func.children())["params"];
	if(params.size()){

	  std::vector<std::string> typeparams;
	  std::vector<std::string> args;
	  std::pair<std::string,std::string> r;

      for(size_t i = 0; i != params.nodes().size();++i){
    	  if (params.nodes()[i]->kind() != Ast_node_kind::identifier) continue;
    	  typeparams.push_back("T"+std::to_string(i));
    	  args.push_back(name(as_id_ref(params.nodes()[i])));
      }

      r.first+= "template<";
      for(size_t i = 0; i != typeparams.size(); ++i){
    	  r.first+= "typename "; r.first+= typeparams[i];
    	  if (i+1 < typeparams.size()) r.first+= ",";
      }
      r.first+= "> ";

      r.second+= "(";
      for(size_t i = 0; i != typeparams.size(); ++i){
    	  r.second += typeparams[i]; r.second += " "; r.second += args[i];
    	  if (i+1 < typeparams.size()) r.second += ",";
      }
      r.second += ")";
      parameters = args;
      return r;
	}
	return std::make_pair(std::string{},std::string{});
}

void write_cpp_glob_func_decl(std::ostream& os, std::string prefix, ceps::ast::Struct & func,std::vector<std::string>& parameters){
	auto params = ceps::ast::Nodeset(func.children())["params"];
	if(params.size()){
	  std::vector<std::string> typeparams;
	  std::vector<std::string> args;

      for(size_t i = 0; i != params.nodes().size();++i){
    	  if (params.nodes()[i]->kind() != Ast_node_kind::identifier) continue;
    	  typeparams.push_back("T"+std::to_string(i));
    	  args.push_back(name(as_id_ref(params.nodes()[i])));
      }
      os << "template<";
      for(size_t i = 0; i != typeparams.size(); ++i){
    	  os << "typename " << typeparams[i];
    	  if (i+1 < typeparams.size()) os << ",";
      }
      os << "> ";

      os << "systemstates::Variant ";
      os << prefix << ceps::ast::name(func);
      os << "(";
      for(size_t i = 0; i != typeparams.size(); ++i){
    	  os << typeparams[i]; os << " "; os << args[i];
    	  if (i+1 < typeparams.size()) os << ",";
      }
      os << ")";
      parameters = args;
	} else	os << "systemstates::Variant " << prefix << ceps::ast::name(func)  << "()";
}

void write_cpp_glob_func_decl(std::ostream& os, std::string prefix, ceps::ast::Struct & func){
	std::vector<std::string> parameters;
	write_cpp_glob_func_decl(os,prefix,func,parameters);
}


std::string get_cpp_action_name(std::string prefix,State_machine::Transition::Action const & a){
	return prefix + "__action__" + a.id_;
}


bool action_func_has_parameters(State_machine::Transition::Action const & a){
	auto params = ceps::ast::Nodeset(as_struct_ref(a.body_).children())["params"];
	return params.size() > 0;
}

void write_cpp_action_func_decl(std::ostream& os,std::string prefix,State_machine::Transition::Action const & a){
	std::vector<std::string> dummy;
	auto decl = cpp_templatized_decl(as_struct_ref(a.body_),dummy);

	if (decl.first.length() == 0){ os << "void " << get_cpp_action_name(prefix,a)  << "()";}
	else {os << decl.first << " " << "void " << get_cpp_action_name(prefix,a) << decl.second;}
}

void write_cpp_action_func_decl_full_name(std::ostream& os,
		std::string prefix,
		State_machine::Transition::Action const & a,
		std::vector<std::string>& parameters, bool print_return_type=true, bool print_params=true){
	auto decl = cpp_templatized_decl(as_struct_ref(a.body_),parameters);

	if (decl.first.length() == 0){
		os << ( print_return_type ? "void ":"")
		   << global_functions_namespace
		   << "::" << get_cpp_action_name(prefix,a)
		   << ( print_params ? "()":"");
	}
	else {
		os << decl.first
		   << " "
		   << ( print_return_type ? "void ":"")
		   << global_functions_namespace
		   << "::"
		   << get_cpp_action_name(prefix,a)
		   << ( print_params ? decl.second:"");}


}

void write_cpp_struct_decl_impl(Indent indent,std::ostream& os,ceps::ast::Struct const& outer){
	for(auto e: outer.children()){
		if(e->kind() != Ast_node_kind::structdef) continue;
		Nodebase_ptr v;Type t;
		if (struct_contains_single_value(as_struct_ref(e),v,t)){
			indent.print_indentation(os);write_cpp_systemstate_type(os,t);os << " " << name(as_struct_ref(e)) << " = ";
			write_cpp_value(os,v,t);os << ";\n";
		} else {
			indent.print_indentation(os);os << "struct {\n";
			indent.indent_incr();
			write_cpp_struct_decl_impl(indent,os,as_struct_ref(e));
			indent.indent_decr();
			indent.print_indentation(os);os << "}"<< name(as_struct_ref(e)) <<";\n";
		}
	}
}

std::string write_cpp_struct_decl(Indent indent,std::ostream& os,Nodebase_ptr p){
	std::string r;
	indent.print_indentation(os);os << "struct "<< (r = name(as_struct_ref(p))) <<" {\n";
	indent.indent_incr();
	write_cpp_struct_decl_impl(indent,os,as_struct_ref(p));
	indent.indent_decr();
	indent.print_indentation(os);os << "};\n";
	return r;
}

bool is_id_or_symbol(Nodebase_ptr p, std::string& n, std::string& k){
	if (p->kind() == Ast_node_kind::identifier) {n = name(as_id_ref(p));k = ""; return true;}
	if (p->kind() == Ast_node_kind::symbol) {n = name(as_symbol_ref(p));k = kind(as_symbol_ref(p)); return true;}
	return false;
}

bool is_id(Nodebase_ptr p, std::string & result, std::string& base_kind){
	std::string k,l;
	if (p->kind() == Ast_node_kind::binary_operator && op(as_binop_ref(p)) == '.'){
	 if (!is_id_or_symbol(as_binop_ref(p).right(),k,l)) return false;

	 if (!is_id(as_binop_ref(p).left(),result,base_kind)) return false;
	 result = result + "." + k;
	 return true;
	} else if (is_id_or_symbol(p,k,l)){ base_kind = l; result = k; return true; }
	return false;
}

static void flatten_args(ceps::ast::Nodebase_ptr r, std::vector<ceps::ast::Nodebase_ptr>& v, char op_val = ',')
{
	using namespace ceps::ast;
	if (r == nullptr) return;
	if (r->kind() == ceps::ast::Ast_node_kind::binary_operator && op(as_binop_ref(r)) ==  op_val)
	{
		auto& t = as_binop_ref(r);
		flatten_args(t.left(),v,op_val);
		flatten_args(t.right(),v,op_val);
		return;
	}
	v.push_back(r);
}





void Cppgenerator::write_cpp_expr_impl(State_machine_simulation_core* smp,
		                 Indent& indent,
						 std::ostream& os,
						 Nodebase_ptr p,
						 State_machine* cur_sm,std::vector<std::string>& parameters,
						 bool inside_func_param_list){
	std::string compound_id;
	std::string base_kind;

	if (p->kind() == Ast_node_kind::int_literal)
		os << value(as_int_ref(p));
	else if (p->kind() == Ast_node_kind::float_literal)
		os << value(as_double_ref(p));
	else if (p->kind() == Ast_node_kind::string_literal)
		os << "std::string{R\"("<< value(as_string_ref(p)) << ")\"}";
	else if (is_id(p,compound_id,base_kind)){
		if ("Guard" == base_kind){
			os << guards_namespace <<"::"<< compound_id << "()";
		}else if ("Systemstate" == base_kind || "Systemparameter" == base_kind ){
			std::string base_id = compound_id;
			auto dot_pos = base_id.find_first_of('.');
			if (dot_pos == std::string::npos && sysstates()[base_id].type.t == Type::Struct)
				os << sysstates_namespace<<"::"<< compound_id;
			else os << sysstates_namespace<<"::"<< compound_id<<".value()";
		} else if ("Event" == base_kind){
			if (inside_func_param_list) os << sysstates_namespace<<"::"<<  "ev{\""<< compound_id <<"\"}";
			else os <<  smcore_singleton  <<"->queue_event" << "(\""<< compound_id <<"\")";
		} else if (compound_id == "s") {
			os << "1";
		} else {
			std::string base_id = compound_id;
			auto dot_pos = base_id.find_first_of('.');
			bool is_compound = dot_pos != std::string::npos;
			if (is_compound ) base_id = compound_id.substr(0,dot_pos);

			if (cur_sm != nullptr){
				for(auto a : cur_sm->actions_){
				 if (a.id_ != compound_id) continue;
				 state_rep_t srep(true,true,cur_sm,cur_sm->id());
				 auto fname = get_cpp_action_name(smp->get_fullqualified_id(srep,"__"),a);
				 os << fname << "()";
				 return;
				}
			}

			if (parameters.size()){
				for(auto const & s : parameters )
				{
					if (s != base_id) continue;
					os << compound_id; if (is_compound) os << ".value()"; return;
				}
			}
			os << sysstates_namespace<<"::"<< "id{\"" << compound_id << "\"}";
		}
	} else if (p->kind() == Ast_node_kind::binary_operator){
		auto& binop = as_binop_ref(p);
		bool print_closing_bracket = false;
		//if (binop.left()->kind() == Ast_node_kind::binary_operator || binop.right()->kind() == Ast_node_kind::binary_operator)
		{print_closing_bracket=true;os << "(";}
		write_cpp_expr_impl(smp,indent,os,binop.left(),cur_sm,parameters,inside_func_param_list);
		if (op(binop) == '=') os << " == ";
		else if (op(binop) == ceps::Cepsparser::token::REL_OP_EQ) os << " == ";
		else if (op(binop) == ceps::Cepsparser::token::REL_OP_NEQ) os << " != ";
		else if (op(binop) == ceps::Cepsparser::token::REL_OP_GT) os << " > ";
		else if (op(binop) == ceps::Cepsparser::token::REL_OP_LT) os << " < ";
		else if (op(binop) == ceps::Cepsparser::token::REL_OP_GT_EQ) os << " >= ";
		else if (op(binop) == ceps::Cepsparser::token::REL_OP_LT_EQ) os << " <= ";
		else if (op(binop) == '+') os << " + ";
		else if (op(binop) == '&') os << " && ";
		else if (op(binop) == '|') os << " || ";
		else if (op(binop) == '-') os << " - ";
		else if (op(binop) == '*') os << " * ";
		else if (op(binop) == '/') os << " / ";
		else if (op(binop) == '^') os << " ^ ";
		else os << " Unknown_Operator ";
		write_cpp_expr_impl(smp,indent,os,binop.right(),cur_sm,parameters,inside_func_param_list);
		if (print_closing_bracket) os << ")";
	} else if (p->kind() == Ast_node_kind::func_call){
		ceps::ast::Func_call& func_call = *dynamic_cast<ceps::ast::Func_call*>(p);
	    ceps::ast::Identifier& id = *dynamic_cast<ceps::ast::Identifier*>(func_call.children()[0]);
 	    ceps::ast::Call_parameters& params = *dynamic_cast<ceps::ast::Call_parameters*>(func_call.children()[1]);
		std::vector<ceps::ast::Nodebase_ptr> args;
		if (params.children().size()) flatten_args(params.children()[0], args);

		//indent.print_indentation(os);
		if (cur_sm != nullptr){
			for(auto a : cur_sm->actions_){
				if (a.id_ != name(id)) continue;
				state_rep_t srep(true,true,cur_sm,cur_sm->id());
				auto fname = get_cpp_action_name(smp->get_fullqualified_id(srep,"__"),a);
				os <<  fname << "(";
				for(size_t i = 0; i != args.size();++i){
					write_cpp_expr_impl(smp,indent,os,args[i],cur_sm,parameters,true);
					if (i+1<args.size()) os << " , ";
				}
				os << ")";
				return;
			}
		}

		if ("truncate" == name(id))os << "((int)";
		else if ("changed" == name(id)){
			os << sysstates_namespace << "::" << value(as_string_ref(args[0])) << ".changed()";return;
		}
		else if (smp->is_global_event(name(id))){
			if (!inside_func_param_list) os <<  smcore_singleton  <<"->queue_event" << "(\""<< name(id)<<"\",{";
			else os << sysstates_namespace<<"::"<<  "ev(\""<< name(id) <<"\", {";

			for(size_t i = 0; i != args.size();++i){
				os << sysstates_namespace << "::Variant{";
				write_cpp_expr_impl(smp,indent,os,args[i],cur_sm,parameters,true);
				os << "}";
				if (i+1<args.size()) os << " , ";
			}
			os << "})";
			return;
		} else if ("in_state" == name(id)){
			os <<  builtin_funcs_namespace  <<"::in_state" << "({";
			for(size_t i = 0; i != args.size();++i){
				write_cpp_expr_impl(smp,indent,os,args[i],cur_sm,parameters,true);
				if (i+1<args.size()) os << " , ";
			}
			os << "})";
			return;
		}
		else if ("print" == name(id)){
			os << "std::cout";
			for(size_t i = 0; i != args.size();++i){
				os << "<<";write_cpp_expr_impl(smp,indent,os,args[i],cur_sm,parameters,true);
			}
			return;
		}
		else os << name(id) << "(";

		for(size_t i = 0; i != args.size();++i){
			write_cpp_expr_impl(smp,indent,os,args[i],cur_sm,parameters,true);
			if (i + 1 != args.size()) os << " , ";
		}
		os << ")";

	} else if (p->kind() == Ast_node_kind::unary_operator){
		 ceps::ast::Unary_operator& unop = *dynamic_cast<ceps::ast::Unary_operator*>(p);
		 if (op(unop) == '-') os << "-";
		 else if (op(unop) == '!') os << "!";
		 write_cpp_expr_impl(smp,indent,os,unop.children()[0],cur_sm,parameters,inside_func_param_list);
	} else os << "/* "<< *p << " ??*/";
}

void Cppgenerator::write_cpp_expr(State_machine_simulation_core* smp,Indent& indent,std::ostream& os,Nodebase_ptr p,State_machine* cur_sm,std::vector<std::string>& parameters){
	write_cpp_expr_impl(smp,indent,os,p,cur_sm,parameters,false);
}

bool Cppgenerator::write_cpp_stmt_impl(State_machine_simulation_core* smp,Indent& indent,std::ostream& os,Nodebase_ptr p,State_machine* cur_sm,std::vector<std::string>& parameters){
	if(smp->is_assignment_op(p)){
		std::string lhs_id;
		auto& binop = as_binop_ref(p);
		if(smp->is_assignment_to_state(binop, lhs_id)) {
         //if (is_compound_id(lhs_id))
	     if (binop.right()->kind() != Ast_node_kind::identifier) {
	    	 indent.print_indentation(os); os <<"set_value("<< sysstates_namespace << "::" << lhs_id <<" , ";
	    	 write_cpp_expr(smp,indent,os,binop.right(),cur_sm,parameters);
	    	 os << ")";
	    	 return true;
	     }
	     else return false;//typedef
		} else if (smp->is_assignment_to_guard(binop)) {
		 indent.print_indentation(os); os << guards_namespace << "::" << name(as_symbol_ref(binop.left())) <<" = ";
		 os << guards_namespace << "::" <<  guard_backpatch_vec[guard_backpatch_idx[binop.right()]].first;
		 return true;
		}
	} else if (p->kind() == ceps::ast::Ast_node_kind::ifelse) {
		auto ifelse = ceps::ast::as_ifelse_ptr(p);
		ceps::ast::Nodebase_ptr cond = ifelse->children()[0];
		indent.print_indentation(os);
		os << "if (";
		write_cpp_expr(smp,indent,os,cond,cur_sm,parameters);
		os << ") {\n";
		indent.indent_incr();
		if (ifelse->children().size() > 1) {
			if (write_cpp_stmt_impl(smp,indent,os,ifelse->children()[1],cur_sm,parameters)) os << ";\n";
		}
		indent.indent_decr();
		indent.print_indentation(os);os << "}";
		if (ifelse->children().size() > 2) {
		 os << " else ";
		 bool else_block_is_if = ifelse->children()[2]->kind() == ceps::ast::Ast_node_kind::ifelse;
		 //if (else_block_is_if)  std::cout << "!!!!!";
		 if (!else_block_is_if){ os << "{\n";indent.indent_incr();}
		 if(write_cpp_stmt_impl(smp,indent,os,ifelse->children()[2],cur_sm,parameters)) os << ";\n";
		 if (!else_block_is_if){
			 indent.indent_decr();
		 	 indent.print_indentation(os);os << "}\n";
		 }
		} else os << "\n";
		return false;
	} else if (p->kind() == Ast_node_kind::scope) {
		auto scp = ceps::ast::nlf_ptr(p);
		for (auto pp : scp->children()){
			if (write_cpp_stmt_impl(smp,indent,os,pp,cur_sm,parameters)) os << ";\n";
		}
		return false;
	} else {indent.print_indentation(os);write_cpp_expr(smp,indent,os,p,cur_sm,parameters);}
	return true;
}

bool Cppgenerator::write_cpp_stmt(State_machine_simulation_core* smp,Indent& indent,std::ostream& os,Nodebase_ptr p,State_machine* cur_sm,std::vector<std::string>& parameters){
	return write_cpp_stmt_impl(smp,indent,os,p,cur_sm,parameters);
}

void process_guard_rhs(std::set<std::string>& named_guards,State_machine_simulation_core* smc,std::map<std::string, ceps::ast::Nodebase_ptr> const & all_guards,Nodeset& globals,Nodeset& global_functions)
{
	int number_of_guards_in_globals = 0;


	//Step #1 : Collect all rhs' in globals
	auto store_guard_rhs_in_guard_assignment = [&](State_machine_simulation_core* smp,Nodebase_ptr p,int,std::set<std::string>& seen){
		if(!smp->is_assignment_op(p)) return;
		auto& binop = as_binop_ref(p);
		if(!smp->is_assignment_to_guard(binop)) return;
		guard_backpatch_vec.push_back(std::make_pair("guard_impl_"+std::to_string(++number_of_guards_in_globals),binop.right()));
		guard_backpatch_idx[binop.right()] = guard_backpatch_vec.size()-1;
		named_guards.insert(name(as_symbol_ref(binop.left())));
	 };

	for_all_nodes(smc,globals,store_guard_rhs_in_guard_assignment);

	//Step #2 : Collect remaining guard rhs' in all_guards (anonymous guards)
	for(auto & g : all_guards){
		if (named_guards.find(g.first) != named_guards.end()) continue;
		guard_backpatch_vec.push_back(std::make_pair("guard_impl_"+std::to_string(++number_of_guards_in_globals),g.second));
		guard_backpatch_idx[g.second] = guard_backpatch_vec.size()-1;
	}
	//Step #3 : Collect remaining guard rhs' in all global functions
	for(auto & f : global_functions.nodes()){
		auto ns = Nodeset(as_struct_ref(f).children() );
		for_all_nodes(smc,ns,store_guard_rhs_in_guard_assignment);
	}
	//Step #4 : Collect guard rhs' in all actions

	{
		std::vector<State_machine*> smsv;
		for(auto sm : State_machine::statemachines) smsv.push_back(sm.second);
		State_machine_simulation_core* smp = smc;
		traverse_sms(smsv,[smp,store_guard_rhs_in_guard_assignment,smc](State_machine* cur_sm){
			state_rep_t srep(true,true,cur_sm,cur_sm->id());
			for(auto & a: cur_sm->actions_){
					auto & func_body = as_struct_ref(a.body_);
					auto ns = Nodeset(func_body.children() );
					for_all_nodes(smc,ns,store_guard_rhs_in_guard_assignment);
				}
			});
	}

}

std::string replace_all(std::string str, const std::string& from, const std::string& to) {
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
    }
    return str;
}


// FRAMES




struct raw_frame_t{
	struct raw_frame_entry{
		int width;
		std::string base_type;
		std::string name;
		ceps::ast::Nodebase_ptr in;
		ceps::ast::Nodebase_ptr out;
	};
	std::vector<raw_frame_entry>;
};


size_t build_raw_frame(size_t & header_length,std::vector<ceps::ast::Nodebase_ptr> pattern,
		            size_t data_size,raw_frame_t& raw_frame,size_t bit_offs,bool in,
		            size_t bit_width=sizeof(std::int64_t)*8,
		            bool signed_value = true,
		            bool write_data = true,
		            bool host_byte_order = true
		            );

size_t build_raw_frame( size_t & header_length,  ceps::ast::Nodebase_ptr p,
		               size_t data_size,raw_frame_t& raw_frame, size_t bit_offs,bool in,
		               size_t bit_width=sizeof(std::int64_t)*8,
		               bool signed_value = true,
		               bool write_data = true,
		               bool host_byte_order = true) {
	using namespace ceps::ast;
	//unsigned char * data = (unsigned char *) data_;
	if (p == nullptr) return 0;
	if (p->kind() == ceps::ast::Ast_node_kind::structdef){
		auto& st = ceps::ast::as_struct_ref(p);
		auto& nm = ceps::ast::name(st);
		if (nm == "in"){
			return build_raw_frame(header_length,st.children(), data_size,raw_frame, bit_offs, true, bit_width,signed_value,write_data,host_byte_order);
		} else if (nm == "out"){
			return build_raw_frame(header_length,st.children(), data_size,raw_frame, bit_offs, false, bit_width,signed_value,write_data,host_byte_order);
		} else if (nm == "byte" || nm == "int8"){
			return build_raw_frame(header_length,st.children(), data_size,raw_frame, bit_offs, in, 8,true,write_data,host_byte_order);
		} else if (nm == "ubyte" || nm == "uint8"){
			return build_raw_frame(header_length,st.children(), data_size,raw_frame, bit_offs, in, 8,false,write_data,host_byte_order);
		} else if (nm == "ushort" || nm == "uint16"){
			return build_raw_frame(header_length,st.children(), data_size,raw_frame, bit_offs, in, 16,false,write_data,host_byte_order);
		} else if (nm == "short" || nm == "int16"){
			return build_raw_frame(header_length,st.children(), data_size,raw_frame, bit_offs, in, 16,true,write_data,host_byte_order);
		} else if (nm == "uint" || nm == "uint32"){
			return build_raw_frame(header_length,st.children(),data_size,data,bit_offs,32,false,write_data,host_byte_order);
		} else if (nm == "int" || nm == "int32") {
			return build_raw_frame(header_length,st.children(),data_size,data,bit_offs,32,true,write_data,host_byte_order);
		} else if (nm == "ulonglong" || nm == "uint64") {
			return fill_raw_chunk(header_length,st.children(),data_size,data,bit_offs,64,false,write_data,host_byte_order);
		} else if (nm == "longlong" || nm == "int64") {
			return fill_raw_chunk(header_length,st.children(),data_size,data,bit_offs,64,true,write_data,host_byte_order);
		} else if (nm == "uint31"){
			return fill_raw_chunk(header_length,st.children(),data_size,data,bit_offs,31,false,write_data,host_byte_order);
		} else if (nm == "uint30"){
			return fill_raw_chunk(header_length,st.children(),data_size,data,bit_offs,30,false,write_data,host_byte_order);
		} else if (nm == "uint29"){
			return fill_raw_chunk(header_length,st.children(),data_size,data,bit_offs,29,false,write_data,host_byte_order);
		}else if (nm == "uint28"){
			return fill_raw_chunk(header_length,st.children(),data_size,data,bit_offs,28,false,write_data,host_byte_order);
		} else if (nm == "uint27"){
			return fill_raw_chunk(header_length,st.children(),data_size,data,bit_offs,27,false,write_data,host_byte_order);
		} else if (nm == "uint26"){
			return fill_raw_chunk(header_length,st.children(),data_size,data,bit_offs,26,false,write_data,host_byte_order);
		} else if (nm == "uint25"){
			return fill_raw_chunk(header_length,st.children(),data_size,data,bit_offs,25,false,write_data,host_byte_order);
		} else if (nm == "int31"){
			return fill_raw_chunk(header_length,st.children(),data_size,data,bit_offs,31,true,write_data,host_byte_order);
		} else if (nm == "int30"){
			return fill_raw_chunk(header_length,st.children(),data_size,data,bit_offs,30,true,write_data,host_byte_order);
		} else if (nm == "int29"){
			return fill_raw_chunk(header_length,st.children(),data_size,data,bit_offs,29,true,write_data,host_byte_order);
		}else if (nm == "int28"){
			return fill_raw_chunk(header_length,st.children(),data_size,data,bit_offs,28,true,write_data,host_byte_order);
		} else if (nm == "int27"){
			return fill_raw_chunk(header_length,st.children(),data_size,data,bit_offs,27,true,write_data,host_byte_order);
		} else if (nm == "int26"){
			return fill_raw_chunk(header_length,st.children(),data_size,data,bit_offs,26,true,write_data,host_byte_order);
		} else if (nm == "int25"){
			return fill_raw_chunk(header_length,st.children(),data_size,data,bit_offs,25,true,write_data,host_byte_order);
		} else if (nm == "uint24"){
			return fill_raw_chunk(header_length,st.children(),data_size,data,bit_offs,24,false,write_data,host_byte_order);
		} else if (nm == "int24"){
			return fill_raw_chunk(header_length,st.children(),data_size,data,bit_offs,24,true,write_data,host_byte_order);
		}else if (nm == "uint23"){
			return fill_raw_chunk(header_length,st.children(),data_size,data,bit_offs,23,false,write_data,host_byte_order);
		} else if (nm == "int23"){
			return fill_raw_chunk(header_length,st.children(),data_size,data,bit_offs,23,true,write_data,host_byte_order);
		}else if (nm == "uint22"){
			return fill_raw_chunk(header_length,st.children(),data_size,data,bit_offs,22,false,write_data,host_byte_order);
		} else if (nm == "int22"){
			return fill_raw_chunk(header_length,st.children(),data_size,data,bit_offs,22,true,write_data,host_byte_order);
		}else if (nm == "uint21"){
			return fill_raw_chunk(header_length,st.children(),data_size,data,bit_offs,21,false,write_data,host_byte_order);
		} else if (nm == "int21"){
			return fill_raw_chunk(header_length,st.children(),data_size,data,bit_offs,21,true,write_data,host_byte_order);
		}else if (nm == "uint20"){
			return fill_raw_chunk(header_length,st.children(),data_size,data,bit_offs,20,false,write_data,host_byte_order);
		} else if (nm == "int20"){
			return fill_raw_chunk(header_length,st.children(),data_size,data,bit_offs,20,true,write_data,host_byte_order);
		}else if (nm == "uint19"){
			return fill_raw_chunk(header_length,st.children(),data_size,data,bit_offs,19,false,write_data,host_byte_order);
		} else if (nm == "int19"){
			return fill_raw_chunk(header_length,st.children(),data_size,data,bit_offs,19,true,write_data,host_byte_order);
		}else if (nm == "uint18"){
			return fill_raw_chunk(header_length,st.children(),data_size,data,bit_offs,18,false,write_data,host_byte_order);
		} else if (nm == "int18"){
			return fill_raw_chunk(header_length,st.children(),data_size,data,bit_offs,18,true,write_data,host_byte_order);
		}else if (nm == "uint17"){
			return fill_raw_chunk(header_length,st.children(),data_size,data,bit_offs,17,false,write_data,host_byte_order);
		} else if (nm == "int17"){
			return fill_raw_chunk(header_length,st.children(),data_size,data,bit_offs,17,true,write_data,host_byte_order);
		}else if (nm == "uint15"){
			return fill_raw_chunk(header_length,st.children(),data_size,data,bit_offs,15,false,write_data,host_byte_order);
		} else if (nm == "int15"){
			return fill_raw_chunk(header_length,st.children(),data_size,data,bit_offs,15,true,write_data,host_byte_order);
		} else if (nm == "uint14"){
			return fill_raw_chunk(header_length,st.children(),data_size,data,bit_offs,14,false,write_data,host_byte_order);
		} else if (nm == "int14"){
			return fill_raw_chunk(header_length,st.children(),data_size,data,bit_offs,14,true,write_data,host_byte_order);
		}else if (nm == "uint13"){
			return fill_raw_chunk(header_length,st.children(),data_size,data,bit_offs,13,false,write_data,host_byte_order);
		} else if (nm == "int13"){
			return fill_raw_chunk(header_length,st.children(),data_size,data,bit_offs,13,true,write_data,host_byte_order);
		}else if (nm == "uint12"){
			return fill_raw_chunk(header_length,st.children(),data_size,data,bit_offs,12,false,write_data,host_byte_order);
		} else if (nm == "int12"){
			return fill_raw_chunk(header_length,st.children(),data_size,data,bit_offs,12,true,write_data,host_byte_order);
		}else if (nm == "uint11"){
			return fill_raw_chunk(header_length,st.children(),data_size,data,bit_offs,11,false,write_data,host_byte_order);
		} else if (nm == "int11"){
			return fill_raw_chunk(header_length,st.children(),data_size,data,bit_offs,11,true,write_data,host_byte_order);
		} else if (nm == "uint10"){
			return fill_raw_chunk(header_length,st.children(),data_size,data,bit_offs,10,false,write_data,host_byte_order);
		} else if (nm == "int10"){
			return fill_raw_chunk(header_length,st.children(),data_size,data,bit_offs,10,true,write_data,host_byte_order);
		}else if (nm == "uint9"){
			return fill_raw_chunk(header_length,st.children(),data_size,data,bit_offs,9,false,write_data,host_byte_order);
		} else if (nm == "int9"){
			return fill_raw_chunk(header_length,st.children(),data_size,data,bit_offs,9,true,write_data,host_byte_order);
		}else if (nm == "uint7"){
			return fill_raw_chunk(header_length,st.children(),data_size,data,bit_offs,7,false,write_data,host_byte_order);
		} else if (nm == "int7"){
			return fill_raw_chunk(header_length,st.children(),data_size,data,bit_offs,7,true,write_data,host_byte_order);
		} else if (nm == "uint6"){
			return fill_raw_chunk(header_length,st.children(),data_size,data,bit_offs,6,false,write_data,host_byte_order);
		} else if (nm == "int6"){
			return fill_raw_chunk(header_length,st.children(),data_size,data,bit_offs,6,true,write_data,host_byte_order);
		} else if (nm == "uint5"){
			return fill_raw_chunk(header_length,st.children(),data_size,data,bit_offs,5,false,write_data,host_byte_order);
		} else if (nm == "int5"){
			return fill_raw_chunk(header_length,st.children(),data_size,data,bit_offs,8,true,write_data,host_byte_order);
		} else if (nm == "uint4"){
			return fill_raw_chunk(header_length,st.children(),data_size,data,bit_offs,4,false,write_data,host_byte_order);
		} else if (nm == "int4"){
			return fill_raw_chunk(header_length,st.children(),data_size,data,bit_offs,4,true,write_data,host_byte_order);
		} else if (nm == "uint3"){
			return fill_raw_chunk(header_length,st.children(),data_size,data,bit_offs,3,false,write_data,host_byte_order);
		} else if (nm == "int3"){
			return fill_raw_chunk(header_length,st.children(),data_size,data,bit_offs,3,true,write_data,host_byte_order);
		} else if (nm == "uint2"){
			return fill_raw_chunk(header_length,st.children(),data_size,data,bit_offs,2,false,write_data,host_byte_order);
		} else if (nm == "int2"){
			return fill_raw_chunk(header_length,st.children(),data_size,data,bit_offs,2,true,write_data,host_byte_order);
		} else if (nm == "bit"){
			return fill_raw_chunk(header_length,st.children(),data_size,data,bit_offs,1,true,write_data,host_byte_order);
		} else {
			return fill_raw_chunk(header_length,st.children(), data_size,data, bit_offs, bit_width,signed_value,write_data,host_byte_order);
		}
	}
	else if (p->kind() == ceps::ast::Ast_node_kind::identifier){
		auto& ident = ceps::ast::as_id_ref(p);
		if (ceps::ast::name(ident) == "any") return bit_width;
		else if (ceps::ast::name(ident) == "__current_frame_size") {
			auto temp_node = ceps::ast::Int(data_size,ceps::ast::all_zero_unit(), nullptr, nullptr, nullptr);
			return fill_raw_chunk(header_length,smc,&temp_node,data_size, data, bit_offs, bit_width,signed_value,write_data,host_byte_order);
		} else smc->fatal_(-1,std::string("Raw frame:  Unknown identifier '")+ceps::ast::name(ident)+"'");
	}else if (p->kind() == ceps::ast::Ast_node_kind::int_literal){
		if (write_data){
			std::uint64_t v = (std::uint64_t)value(as_int_ref(p));
			if (bit_width < 64){
			   if (bit_width == 1){ v =  (v ? 1 : 0) ;  }
			   else if (bit_width == 4) v &= 0xFLL;
			   else if (bit_width == 8) v &= 0xFFLL;
			   else if (bit_width == 12) v &= 0xFFFLL;
			   else if (bit_width == 16) v &= 0xFFFFLL;
			   else if (bit_width == 20) v &= 0xFFFFFLL;
			   else if (bit_width == 24) v &= 0xFFFFFFLL;
			   else if (bit_width == 28) v &= 0xFFFFFFFLL;
			   else if (bit_width == 32) v &= 0xFFFFFFFFLL;
			   else{
				   std::uint64_t w=1;
				   for(size_t i = 0; i < bit_width;++i) w |= (w << 1);
				   v &= w;
			   }
			}
			int bits_written = 0;
			if (bit_offs % 8) {
				unsigned short o = bit_offs % 8;
				unsigned short d = 8 - o ; // d is the number of bits left in the byte to write
				unsigned char w = *(data + bit_offs/8);
				if (d > bit_width) d = (unsigned short) bit_width;
				unsigned char c1=1;for(int i = 0; i < d;++i)c1 |= (c1 << 1);
				unsigned char c2=1;for(int i = 0; i < o-1;++i)c2 |= (c2 << 1);

				w = (w & c2) | ( ((unsigned char)v & c1) << o );
				*( (unsigned char*)( data + bit_offs/8)) = w;
				bits_written = d;bit_offs+=d;
			}
			if (bits_written) v = v >> bits_written;

			//INVARIANT: bit_offs points to a byte address
			if (bits_written < (int) bit_width){
				for(; bit_width-bits_written>=8;bit_offs+=8,bits_written+=8){
					*( (unsigned char*)data+bit_offs/8) = (unsigned char) v;
				    v = v >> 8;
				}
			}
			//INVARIANT: bit_offs points to a byte address
			//INVARIANT: bit_width - bits_written < 8
			if (bits_written < (int)bit_width){
				if ((int)bit_width-bits_written  == 1){
					unsigned char & target = *( (unsigned char*)(data+ bit_offs/8) );
					if (v & 1)  target |= 1;
					else target &= 0xFE;
				} else
				{
					unsigned char & target = *( (unsigned char*)(data+ bit_offs/8) );
					unsigned char c = 1;
					for(int i = 0; i < (int)bit_width-bits_written-1; ++i ) c |= c << 1;
					target = c & v;
				}
			}
		}
		return bit_width;
	}
	else if (p->kind() == ceps::ast::Ast_node_kind::float_literal) {
		 smc->fatal_(-1,"Floating point numbers not supported in raw frames.");
	}
	else if (p->kind() == ceps::ast::Ast_node_kind::string_literal){
		 std::string s = ceps::ast::value(ceps::ast::as_string_ref(p));
		 int corr = 0;
		 if (bit_offs % 8) corr = 8 - (bit_offs % 8);
		 bit_offs += corr;
		 if (write_data) memcpy((data+bit_offs/8),s.c_str(),s.length());
		 return s.length()*8+corr;
	}
	else {std::stringstream ss; ss << *p; smc->fatal_(-1,"Serialization of raw frame: Illformed expression:"+ss.str());}
	return 0;
}

size_t build_raw_frame(size_t & header_length,std::vector<ceps::ast::Nodebase_ptr> pattern,
		            size_t data_size,raw_frame_t& raw_frame,size_t bit_offs,bool in,
		            size_t bit_width=sizeof(std::int64_t)*8,
		            bool signed_value = true,
		            bool write_data = true,
		            bool host_byte_order = true
		            ){
	size_t r=0;
	for(auto p : pattern){

		bool header = false;
		if (p->kind() == ceps::ast::Ast_node_kind::structdef && !write_data){
				auto& st = ceps::ast::as_struct_ref(p);
				auto& nm = ceps::ast::name(st);
				if (nm == "header") {header = true;}
		}
		auto rr = fill_raw_chunk(header_length,smc,p,data_size,data,bit_offs,bit_width,signed_value,write_data,host_byte_order);
		if (header) {header_length = rr; }
		r+=rr;
		bit_offs += rr;
	}
	return r;
}


void handle_frames(ceps::Ceps_Environment& ceps_env,
		  ceps::ast::Nodeset& universe,
		  std::map<std::string, ceps::ast::Nodebase_ptr> const & all_guards,
		  ceps::ast::Nodeset& frames)
{
 using namespace ceps::ast;
 if (frames.size() == 0) return;
 //Preludes
 std::string out_cpp;
 std::string out_hpp;

 std::ofstream o_cpp{out_cpp = "out_frames.cpp"};
 std::ofstream o_hpp{out_hpp = "out_frames.hpp"};

 write_copyright_and_timestamp(o_cpp,out_cpp,true);
 write_copyright_and_timestamp(o_hpp,out_hpp,true);
 Indent indent_hpp;Indent indent_cpp;
 o_hpp << "namespace raw_frm_dcls{\n";
 indent_hpp.indent_incr();
 for (auto frame_ : frames.nodes()){
	 auto & frame = as_struct_ref(frame_);
	 indent_hpp.print_indentation(o_hpp);
	 o_hpp << "struct " << Nodeset(frame.children())["id"].as_str() << "{\n";
	 indent_hpp.print_indentation(o_hpp);o_hpp << "};\n";
 }
 indent_hpp.indent_decr();
 o_hpp << "}\n";
}

void State_machine_simulation_core::do_generate_cpp_code(ceps::Ceps_Environment& ceps_env,
													  ceps::ast::Nodeset& universe,
													  std::map<std::string, ceps::ast::Nodebase_ptr> const & all_guards){
	DEBUG_FUNC_PROLOGUE

	Cppgenerator cppgenerator;


	auto globals = universe["Globals"];
	auto struct_defs_ns = universe["typedef"];
	auto global_functions = universe["global_functions"];
	auto post_proc = universe["post_event_processing"];
	auto raw_frames = universe[all{"frame"}];

	auto sym = ceps_env.get_global_symboltable();
	guard_ctr = 0;

	ceps::ast::Struct dummy_strct{"post_event_processing"};
	dummy_strct.children_ = post_proc.nodes_;

	if (post_proc.size()) global_functions.nodes_.insert(global_functions.nodes_.end(),&dummy_strct );

	for_all_nodes(this,struct_defs_ns,store_struct_defs);
	for_all_nodes(this,globals,store_first_state_assign);

	//Collect defining right hand side of guards
	std::set<std::string> named_guards; //remember guards with names
	process_guard_rhs(named_guards,this,all_guards,globals,global_functions);

	//print_first_state_assigns();

	std::vector<std::pair<int,Nodebase_ptr>> struct_decls;
	for(auto e : struct_defs) struct_decls.push_back(e.second);
	std::sort(struct_decls.begin(),struct_decls.end(),[](std::pair<int,Nodebase_ptr> const& lhs, std::pair<int,Nodebase_ptr> const& rhs){
		return std::get<0>(lhs) < std::get<0>(rhs);
	}  );

	for(auto s:struct_decls){
		add_sm4ceps_struct(s.second);
	}

	std::vector<sysstate> sys_states;
	std::vector<sysstate> sys_states_which_are_structs;

	for(auto & state_entry : systemstate_first_def){
		bool non_primitive_types_found = false;
		auto type = determine_primitive_type(state_entry.second);
		if (type.t == Type::Undefined) non_primitive_types_found=true;
		sysstate s = state_entry.first;
		s.type = type;
		if (non_primitive_types_found)sys_states_which_are_structs.push_back(s); else sys_states.push_back(s);
	}
	for (auto & s : sys_states_which_are_structs){
		//std::cout << "Determining type of " << s.name << std::endl;
		for_all_nodes(this,globals,[&](State_machine_simulation_core* smp,Nodebase_ptr p,int,std::set<std::string>& seen) {
			if(!smp->is_assignment_op(p)) return;
			std::string lhs_id;
			auto& binop = as_binop_ref(p);
			if(!smp->is_assignment_to_state(binop, lhs_id)) return;
			if (lhs_id != s.name && s.name+"." != lhs_id.substr(0,s.name.length()+1)) return;
			std::string rhs_id;std::string base_kind;
			if (!is_id(binop.right(), rhs_id, base_kind)) return;
			//std::cout << "Seen the assignment "<< lhs_id << " = " << rhs_id << std::endl;
			auto struct_type= struct_assign(lhs_id, rhs_id);
			if (struct_type == nullptr){
				smp->warn_(-1,"Couldn't deduce type of "+s.name+" assume int.");
				s.type.t = Type::Int;
				return;
			}
			Indent indent;
			//write_cpp_sm4ceps_struct(indent,std::cout,struct_type);
			s.type.name = struct_type->name;
			s.type.t = Type::Struct;
		});
		//std::cout << "---->Type of " << s.name << " is " << s.type.name << std::endl;
		if (s.type.t == Type::Undefined){
			warn_(-1,"Couldn't deduce type of "+s.name+" assume int.");
			s.type.t = Type::Int;
		}
		sys_states.push_back(s);
	}


	//Write files

	//Preludes
	std::string out_cpp;
	std::string out_hpp;

	std::ofstream o_cpp{out_cpp = "out.cpp"};
	std::ofstream o_hpp{out_hpp = "out.hpp"};
	std::stringstream init_plugin_content;
	init_plugin_content << "extern \"C\" void init_plugin(IUserdefined_function_registry* smc){\n";
	init_plugin_content << "smcore_interface = smc->get_plugin_interface();\n";
	init_plugin_content << "smc->register_global_init(user_defined_init);\n";

	write_copyright_and_timestamp(o_cpp,out_cpp,true);
	write_copyright_and_timestamp(o_hpp,out_hpp,true);

	Indent indent_hpp;Indent indent_cpp;

	o_hpp << R"(
#include <iostream>
#include <string>
#include <algorithm>
#include <map>
#include <vector>
#include <cstdlib>
#include "core/include/state_machine_simulation_core_reg_fun.hpp"
#include "core/include/state_machine_simulation_core_plugin_interface.hpp"
#include "user_defined.hpp"
)";
	o_hpp << out_hpp_prefix << "\n";

	o_cpp << "\n\n#include \""<< out_hpp <<"\"\n\n";

	//Systemstates section
	o_hpp << "namespace "<< sysstates_namespace <<"{\n";
	o_hpp << out_hpp_systemstates_prefix << "\n";

	indent_hpp.indent_incr();

	std::set<std::string> struct_declarations_already_written;
	for(auto & struct_decl: struct_decls){
		auto struct_name = write_cpp_struct_decl(indent_hpp,o_hpp,std::get<1>(struct_decl));o_hpp << "\n";
		struct_declarations_already_written.insert(struct_name);
	}

	for (auto & s : sys_states){


		if (s.type.t != Type::Struct) continue;
		std::string struct_name;
		if (s.name == s.type.name) struct_name = s.name + "__type"; else struct_name = s.type.name;
		if ( struct_declarations_already_written.end() != struct_declarations_already_written.find(struct_name)) continue;
		struct_declarations_already_written.insert(struct_name);
		auto struct_type=find_sm4ceps_struct(s.type.name);
		if (struct_type == nullptr ) struct_type=find_sm4ceps_struct(struct_name);
		if (struct_type == nullptr ) fatal_(-1,"Couldn't find struct declaration for systemstate '"+s.name+"' .");
		struct_type->name = s.type.name = struct_name;
		write_cpp_sm4ceps_struct(indent_hpp,o_hpp,struct_type);
	}

	//Write init
	o_cpp << "\n\n" <<  init_func << "{\n";
	indent_cpp.indent_incr();
	for_all_nodes(this,globals,[&](State_machine_simulation_core*,Nodebase_ptr p,int,std::set<std::string>& seen) {
		std::vector<std::string> dummy;
		if (cppgenerator.write_cpp_stmt(this,indent_cpp,o_cpp,p,nullptr,dummy)) o_cpp << ";\n";
	});
	indent_hpp.indent_decr();
	o_cpp << "}\n";

	for(auto & state_entry : sys_states){
		indent_hpp.print_indentation(o_hpp);write_cpp_systemstate_declaration(o_hpp,state_entry);o_hpp << ";\n";
		indent_cpp.print_indentation(o_cpp);o_cpp << sysstates_namespace<<"::";write_cpp_systemstate_declaration(o_cpp,state_entry);
		cppgenerator.sysstates()[state_entry.name] = state_entry;
		//if (state_entry.type.t != Type::Struct){o_cpp << " = "; write_cpp_expr(indent_cpp,o_cpp,systemstate_first_def[state_entry]);}
		//else o_cpp << "{}";
		o_cpp << ";\n";
	}
	indent_hpp.indent_decr();o_hpp<<"\n}\n";

	//Guards section
	o_hpp << "namespace "<< guards_namespace <<"{\n";
	o_hpp << out_hpp_guards_prefix << "\n";
	indent_hpp.indent_incr();

	for_all_nodes(this,globals,[&](State_machine_simulation_core*,Nodebase_ptr p,int,std::set<std::string>& seen) {
		if(!is_assignment_op(p)) return;
		auto& binop = as_binop_ref(p);
		if(!is_assignment_to_guard(binop)) return;
		auto& guard = as_symbol_ref(binop.left());
		if (seen.find(name(guard)) != seen.end()) return;
		seen.insert(name(guard));
		indent_hpp.print_indentation(o_hpp);o_hpp << "extern Guard " << name(guard) << ";\n";
		indent_cpp.print_indentation(o_cpp);o_cpp << guards_namespace<<"::" << "Guard " <<guards_namespace<<"::"<< name(guard) << " = nullptr;\n";
		init_plugin_content << "smc->register_guard(\""<<name(guard)<<"\",&"<<guards_namespace<<"::"<< name(guard) << ");\n";
	});

	for(auto & g : all_guards){
		if (named_guards.find(g.first) != named_guards.end()) continue;
		indent_hpp.print_indentation(o_hpp);
		o_hpp << "extern Guard " << replace_all(g.first,".","_D_") << ";\n";
		indent_cpp.print_indentation(o_cpp);
		o_cpp << guards_namespace<<"::" << "Guard " <<guards_namespace<<"::"<< replace_all(g.first,".","_D_") << " = "
			  << guards_namespace<<"::"<< guard_backpatch_vec[guard_backpatch_idx[g.second]].first <<";\n";
		init_plugin_content << "smc->register_guard(\""<<g.first<<"\",&"<<guards_namespace<<"::"<< replace_all(g.first,".","_D_") << ");\n";
	}
	o_hpp << "\n\n";
	for(auto& e: guard_backpatch_vec){
		indent_hpp.print_indentation(o_hpp);o_hpp << "extern Guard_impl " << e.first << ";\n";
 	}
	indent_hpp.indent_decr();o_hpp<<"\n}\n";

	//START: Global functions section

	o_hpp << "namespace "<< global_functions_namespace <<"{\n";
	o_hpp << out_hpp_global_functions_prefix << "\n";
	indent_hpp.indent_incr();

	for_all_nodes(this,global_functions,[&](State_machine_simulation_core*,Nodebase_ptr p,int,std::set<std::string>& seen) {
		if(p->kind() != Ast_node_kind::structdef) return;
		auto& str = as_struct_ref(p);
		indent_hpp.print_indentation(o_hpp);
		write_cpp_glob_func_decl(o_hpp,"",str);
		o_hpp << ";\n";
	});



	{
		//print state machine action declarations
		std::vector<State_machine*> smsv;
		for(auto sm : State_machine::statemachines) smsv.push_back(sm.second);
		State_machine_simulation_core* smp = this;
		traverse_sms(smsv,[smp,&o_hpp,&indent_hpp,&init_plugin_content](State_machine* cur_sm){
			state_rep_t srep(true,true,cur_sm,cur_sm->id());
			for(auto & a: cur_sm->actions_){
				bool register_action = !action_func_has_parameters(a);
				indent_hpp.print_indentation(o_hpp);
				write_cpp_action_func_decl(o_hpp,smp->get_fullqualified_id(srep,"__"),a);
				if (register_action){
					init_plugin_content << "smc->register_action(\""<<smp->get_fullqualified_id(srep,".") <<"\",\""<<a.id_ <<"\", ";
					std::vector<std::string> parameters;
				    write_cpp_action_func_decl_full_name(init_plugin_content,smp->get_fullqualified_id(srep,"__"),a,parameters,false,false);
				    init_plugin_content<<");\n";
				}
				o_hpp << ";\n";
				//write_cpp_action_func_decl(std::cout,smp->get_fullqualified_id(srep,"__"),a);std::cout << "\n";
			//std::cout << "'" << smp->get_fullqualified_id(srep,"__")<< "'" << std::endl;
			}
		});
	}


	indent_hpp.indent_decr();o_hpp<<"\n}\n";



	//Write Guard implementations
	for(auto e : guard_backpatch_vec){
		auto guard_rhs = e.second;
		indent_cpp.print_indentation(o_cpp);
		o_cpp << "bool "<< guards_namespace<< "::" << e.first << "(){\n";
		indent_cpp.indent_incr();
		indent_cpp.print_indentation(o_cpp);
		o_cpp << "return ";
		std::vector<std::string> parameters;
		cppgenerator.write_cpp_expr(this,indent_cpp,o_cpp,guard_rhs,nullptr,parameters);
		o_cpp << ";\n";
		indent_cpp.indent_decr();
		indent_cpp.print_indentation(o_cpp);o_cpp << "}\n";
	}



	//Write definitions of actions
	{
		std::vector<State_machine*> smsv;
		for(auto sm : State_machine::statemachines) smsv.push_back(sm.second);
		State_machine_simulation_core* smp = this;
		traverse_sms(smsv,[smp,&o_cpp,&indent_cpp,&cppgenerator](State_machine* cur_sm){
			state_rep_t srep(true,true,cur_sm,cur_sm->id());
			for(auto & a: cur_sm->actions_){
				indent_cpp.print_indentation(o_cpp);
				std::vector<std::string> parameters;
				write_cpp_action_func_decl_full_name(o_cpp,smp->get_fullqualified_id(srep,"__"),a,parameters);
				o_cpp << "{\n";indent_cpp.indent_incr();
				auto & func_body = as_struct_ref(a.body_);
				for(auto p : func_body.children()){
					if(cppgenerator.write_cpp_stmt(smp,indent_cpp,o_cpp,p,cur_sm,parameters)) o_cpp << ";\n";}
					indent_cpp.indent_decr();indent_cpp.print_indentation(o_cpp);o_cpp << "}\n";
			}
		});
	}

	{
		State_machine_simulation_core* smp = this;

		for_all_nodes(this,global_functions,[&](State_machine_simulation_core*,Nodebase_ptr p,int,std::set<std::string>& seen) {
			if(p->kind() != Ast_node_kind::structdef) return;
			auto& str = as_struct_ref(p);
			indent_cpp.print_indentation(o_cpp);
			std::vector<std::string> parameters;
			write_cpp_glob_func_decl(o_cpp,global_functions_namespace+"::",str,parameters);
			o_cpp << "{\n";
			indent_cpp.indent_incr();
			auto & func_body = as_struct_ref(p);
			for(auto p : func_body.children()){
				if(cppgenerator.write_cpp_stmt(smp,indent_cpp,o_cpp,p,nullptr,parameters)) o_cpp << ";\n";}
			o_cpp << "return systemstates::Variant{};\n";
			indent_cpp.indent_decr();
			o_cpp << "}\n";
	   });
	}


	init_plugin_content << "\n}";

	o_cpp << init_plugin_content.str();

	o_hpp << "std::ostream& operator << (std::ostream& o, systemstates::Variant const & v);\n";
	o_cpp << R"~(
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

)~";

o_cpp << "size_t "<<global_functions_namespace << "::argc()"<< R"~({
 return smcore_interface->argc();
})~";

o_cpp << "sm4ceps_plugin_int::Variant "<<global_functions_namespace << "::argv(size_t j)"<< R"~({
 return smcore_interface->argv(j);
})~";

o_cpp << R"~(
void globfuncs::start_timer(double t,sm4ceps_plugin_int::ev ev_){ smcore_interface->start_timer(t,ev_); }
void globfuncs::start_timer(double t,sm4ceps_plugin_int::ev ev_,sm4ceps_plugin_int::id id_){smcore_interface->start_timer(t,ev_,id_);}
void globfuncs::start_periodic_timer(double t ,sm4ceps_plugin_int::ev ev_){smcore_interface->start_periodic_timer(t,ev_);}
void globfuncs::start_periodic_timer(double t,sm4ceps_plugin_int::ev ev_,sm4ceps_plugin_int::id id_){smcore_interface->start_periodic_timer(t,ev_,id_);}
void globfuncs::stop_timer(sm4ceps_plugin_int::id id_){smcore_interface->stop_timer(id_);}
)~";


handle_frames(ceps_env,
		  universe,
		  all_guards,
		  raw_frames);
}


