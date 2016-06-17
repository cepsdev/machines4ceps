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

const std::string out_hpp_systemstates_prefix = R"(

struct Variant{
  double dv_ = 0.0;
  int iv_ = 0;
  std::string sv_ ="";
  enum {Int,Double,String} what_;
  Variant (double v):dv_{v},what_{Double}{}
  Variant (int v):iv_{v},what_{Int}{}
  Variant (std::string v):sv_{v},what_{String}{}

 };

 std::ostream& operator << (std::ostream& o, Variant const & v){
  return o;
 }

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
   bool changed() const {auto t = changed_;changed_=false;return t;}

   T& value() {return v_;}
   T value() const {return v_;}
 
  };
 

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

 bool operator < (Variant const & lhs, std::string const & rhs) {return lhs.sv_ < rhs;}
 bool operator < (std::string const & lhs, Variant const & rhs) {return rhs.sv_ < lhs;}
 bool operator < (Variant const & lhs, int const & rhs) {return lhs.iv_ < rhs;}
 bool operator < (int const & lhs, Variant const & rhs) {return rhs.iv_ < lhs;}
 bool operator < (Variant const & lhs, double const & rhs) {return lhs.dv_ < rhs;}
 bool operator < (double const & lhs, Variant const & rhs) {return rhs.dv_ < lhs;}

 State<int>& set_value(State<int>& lhs, Variant const & rhs){lhs.value() = rhs.iv_; return lhs;}
 State<int>& set_value(State<int>& lhs, int rhs){lhs.value() = rhs; return lhs;}
 State<double>& set_value(State<double>& lhs, double rhs){lhs.value() = rhs; return lhs;}
 State<std::string>& set_value(State<std::string>& lhs, std::string rhs){lhs.value() = rhs; return lhs;}

 void queue_event(std::string ev_name,std::initializer_list<Variant> vl = {});

struct ev{
  std::string name_;
  ev(std::string name):name_(name){}
};

struct id{
  std::string name_;
  id(std::string name):name_(name){}
};

)";





const std::string out_hpp_guards_prefix = R"(
 using Guard = bool(*)();
 using Guard_impl = bool ();
)";

const std::string out_hpp_global_functions_prefix = R"(
 extern bool in_state(std::initializer_list<systemstates::id>);
 extern void start_timer(double,systemstates::ev);
 extern void start_timer(double,systemstates::ev,systemstates::id);
 extern void start_periodic_timer(double,systemstates::ev);
 extern void start_periodic_timer(double,systemstates::ev,systemstates::id);
 extern void stop_timer(systemstates::id);
 extern size_t argc();
 extern systemstates::Variant argv(size_t);
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
		<< "   GENERATED BY the sm4ceps C++ Generator VERSION 0.3 (c) Tomas Prerovsky <tomas.prerovsky@gmail.com>, ALL RIGHTS RESERVED. \n"
		<< "   Requires C++1y compatible compiler (use --std=c++11 for g++) \n"
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

void write_cpp_glob_func_decl(std::ostream& os,ceps::ast::Struct & func){
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
      os << ceps::ast::name(func);
      os << "(";
      for(size_t i = 0; i != typeparams.size(); ++i){
    	  os << typeparams[i]; os << " "; os << args[i];
    	  if (i+1 < typeparams.size()) os << ",";
      }
      os << ")";


	} else	os << "systemstates::Variant " << ceps::ast::name(func)  << "()";
}


std::string get_cpp_action_name(std::string prefix,State_machine::Transition::Action const & a){
	return prefix + "__action__" + a.id_;
}



void write_cpp_action_func_decl(std::ostream& os,std::string prefix,State_machine::Transition::Action const & a){
	std::vector<std::string> dummy;
	auto decl = cpp_templatized_decl(as_struct_ref(a.body_),dummy);

	if (decl.first.length() == 0){ os << "void " << get_cpp_action_name(prefix,a)  << "()";}
	else {os << decl.first << " " << "void " << get_cpp_action_name(prefix,a) << decl.second;}
}

void write_cpp_action_func_decl_full_name(std::ostream& os,std::string prefix,State_machine::Transition::Action const & a,std::vector<std::string>& parameters){
	auto decl = cpp_templatized_decl(as_struct_ref(a.body_),parameters);

	if (decl.first.length() == 0){ os << "void " << global_functions_namespace << "::" << get_cpp_action_name(prefix,a)  << "()";}
	else {os << decl.first << " " << "void " << global_functions_namespace << "::"<< get_cpp_action_name(prefix,a) << decl.second;}


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

void write_cpp_expr_impl(State_machine_simulation_core* smp,
		                 Indent& indent,
						 std::ostream& os,
						 Nodebase_ptr p,
						 State_machine* cur_sm,std::vector<std::string>& parameters,
						 bool inside_func_param_list=false){
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
			os << sysstates_namespace<<"::"<< compound_id<<".value()";
		} else if ("Event" == base_kind){
			if (inside_func_param_list) os << sysstates_namespace<<"::"<<  "ev{\""<< compound_id <<"\"}";
			else os <<  sysstates_namespace  <<"::queue_event" << "(\""<< compound_id <<"\")";
		} else if (compound_id == "s") {
			os << "1";
		} else {
			if (cur_sm != nullptr){
				for(auto a : cur_sm->actions_){
				 if (a.id_ != compound_id) continue;
				 state_rep_t srep(true,true,cur_sm,cur_sm->id());
				 auto fname = get_cpp_action_name(smp->get_fullqualified_id(srep,"__"),a);
				 os << fname << "()";
				 return;
				}
			}

			if (parameters.size()){//std::cout << "*******************" << std::endl;
				for(auto const & s : parameters )
				{
					if (s != compound_id) continue;
					os << s; return;
				}
			}
			os << sysstates_namespace<<"::"<< "id{\"" << compound_id << "\"}";
		}
	} else if (p->kind() == Ast_node_kind::binary_operator){
		auto& binop = as_binop_ref(p);
		bool print_closing_bracket = false;
		//if (binop.left()->kind() == Ast_node_kind::binary_operator || binop.right()->kind() == Ast_node_kind::binary_operator)
		{print_closing_bracket=true;os << "(";}
		write_cpp_expr_impl(smp,indent,os,binop.left(),cur_sm,parameters);
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
		write_cpp_expr_impl(smp,indent,os,binop.right(),cur_sm,parameters);
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
		else if (smp->is_global_event(name(id))){
			os <<  sysstates_namespace  <<"::queue_event" << "(\""<< name(id)<<"\",{";

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
		 write_cpp_expr_impl(smp,indent,os,unop.children()[0],cur_sm,parameters);
	} else os << "/* "<< *p << " ??*/";
}

void write_cpp_expr(State_machine_simulation_core* smp,Indent& indent,std::ostream& os,Nodebase_ptr p,State_machine* cur_sm,std::vector<std::string>& parameters){
	write_cpp_expr_impl(smp,indent,os,p,cur_sm,parameters,false);
}

bool write_cpp_stmt_impl(State_machine_simulation_core* smp,Indent& indent,std::ostream& os,Nodebase_ptr p,State_machine* cur_sm,std::vector<std::string>& parameters){
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
		 os << " else {\n";
		 indent.indent_incr();
		 if(write_cpp_stmt_impl(smp,indent,os,ifelse->children()[2],cur_sm,parameters)) os << ";\n";
		 indent.indent_decr();
		 indent.print_indentation(os);os << "}\n";
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

bool write_cpp_stmt(State_machine_simulation_core* smp,Indent& indent,std::ostream& os,Nodebase_ptr p,State_machine* cur_sm,std::vector<std::string>& parameters){
	return write_cpp_stmt_impl(smp,indent,os,p,cur_sm,parameters);
}


void State_machine_simulation_core::do_generate_cpp_code(ceps::Ceps_Environment& ceps_env,
													  ceps::ast::Nodeset& universe){
	DEBUG_FUNC_PROLOGUE


	auto globals = universe["Globals"];
	auto struct_defs_ns = universe["typedef"];
	auto global_functions = universe["global_functions"];
	auto post_proc = universe["post_event_processing"];
	auto sym = ceps_env.get_global_symboltable();
	guard_ctr = 0;

	ceps::ast::Struct dummy_strct{"post_event_processing"};
	dummy_strct.children_ = post_proc.nodes_;

	if (post_proc.size()) global_functions.nodes_.insert(global_functions.nodes_.end(),&dummy_strct );

	for_all_nodes(this,struct_defs_ns,store_struct_defs);
	for_all_nodes(this,globals,store_first_state_assign);
	int number_of_guards_in_globals = 0;
	for_all_nodes(this,globals,[&](State_machine_simulation_core* smp,Nodebase_ptr p,int,std::set<std::string>& seen){
		if(!smp->is_assignment_op(p)) return;
		auto& binop = as_binop_ref(p);
		if(!smp->is_assignment_to_guard(binop)) return;
		guard_backpatch_vec.push_back(std::make_pair("guard_impl_"+std::to_string(++number_of_guards_in_globals),binop.right()));
		guard_backpatch_idx[binop.right()] = guard_backpatch_vec.size()-1;
	 }
	);

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

	write_copyright_and_timestamp(o_cpp,out_cpp,true);
	write_copyright_and_timestamp(o_hpp,out_hpp,true);

	Indent indent_hpp;Indent indent_cpp;

	o_hpp << "\n\n#include<iostream>\n#include<string>\n#include<algorithm>\n#include<map>\n#include<vector>\n\n";
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
		if (write_cpp_stmt(this,indent_cpp,o_cpp,p,nullptr,dummy)) o_cpp << ";\n";
	});
	indent_hpp.indent_decr();
	o_cpp << "}\n";

	for(auto & state_entry : sys_states){
		indent_hpp.print_indentation(o_hpp);write_cpp_systemstate_declaration(o_hpp,state_entry);o_hpp << ";\n";
		indent_cpp.print_indentation(o_cpp);o_cpp << sysstates_namespace<<"::";write_cpp_systemstate_declaration(o_cpp,state_entry);
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
	});
	o_hpp << "\n\n";
	for(auto& e: guard_backpatch_vec){
		indent_hpp.print_indentation(o_hpp);o_hpp << "extern Guard_impl " << e.first << ";\n";
 	}
	indent_hpp.indent_decr();o_hpp<<"\n}\n";

	//Global functions section

	o_hpp << "namespace "<< global_functions_namespace <<"{\n";
	o_hpp << out_hpp_global_functions_prefix << "\n";
	indent_hpp.indent_incr();

	for_all_nodes(this,global_functions,[&](State_machine_simulation_core*,Nodebase_ptr p,int,std::set<std::string>& seen) {
		if(p->kind() != Ast_node_kind::structdef) return;
		auto& str = as_struct_ref(p);
		indent_hpp.print_indentation(o_hpp);
		write_cpp_glob_func_decl(o_hpp,str);
		o_hpp << ";\n";
	});


	{
		//print state machine action declarations
		std::vector<State_machine*> smsv;
		for(auto sm : State_machine::statemachines) smsv.push_back(sm.second);
		State_machine_simulation_core* smp = this;
		traverse_sms(smsv,[smp,&o_hpp,&indent_hpp](State_machine* cur_sm){
			state_rep_t srep(true,true,cur_sm,cur_sm->id());
			for(auto & a: cur_sm->actions_){
				indent_hpp.print_indentation(o_hpp);
				write_cpp_action_func_decl(o_hpp,smp->get_fullqualified_id(srep,"__"),a);
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
		write_cpp_expr(this,indent_cpp,o_cpp,guard_rhs,nullptr,parameters);
		o_cpp << ";\n";
		indent_cpp.indent_decr();
		indent_cpp.print_indentation(o_cpp);o_cpp << "}\n";
	}



	//Write definitions of actions
		{
			std::vector<State_machine*> smsv;
			for(auto sm : State_machine::statemachines) smsv.push_back(sm.second);
			State_machine_simulation_core* smp = this;
			traverse_sms(smsv,[smp,&o_cpp,&indent_cpp](State_machine* cur_sm){
				state_rep_t srep(true,true,cur_sm,cur_sm->id());
				for(auto & a: cur_sm->actions_){
					indent_cpp.print_indentation(o_cpp);
					std::vector<std::string> parameters;
					write_cpp_action_func_decl_full_name(o_cpp,smp->get_fullqualified_id(srep,"__"),a,parameters);
					o_cpp << "{\n";indent_cpp.indent_incr();

					auto & func_body = as_struct_ref(a.body_);
					for(auto p : func_body.children()){
						if(write_cpp_stmt(smp,indent_cpp,o_cpp,p,cur_sm,parameters)) o_cpp << ";\n";}

					indent_cpp.indent_decr();indent_cpp.print_indentation(o_cpp);o_cpp << "}\n";
				}
			});
		}

}


