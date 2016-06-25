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
 void start_periodic_timer(double t,sm4ceps_plugin_int::Variant (*fp)(),sm4ceps_plugin_int::id id_);
 void start_periodic_timer(double t,sm4ceps_plugin_int::Variant (*fp)());
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


struct raw_frame_t{
	struct raw_frame_entry{
		int width = 0;
		std::string base_type = "";
		std::string name = "";
		ceps::ast::Nodebase_ptr expr = nullptr;
	};
	std::vector<raw_frame_entry> entries_in;
	std::vector<raw_frame_entry> entries_out;
	std::string name;
	size_t header_length = 0;
	size_t len;
	std::string create_basetype(int width, bool signd){
		if (width <= 8 && signd) return "char";
		if (width <= 8 && !signd) return "unsigned char";
		if (width <= 16 && signd) return "short";
		if (width <= 16 && !signd) return "unsigned short";
		if (width <= 32 && signd) return "int";
		if (width <= 32 && !signd) return "unsigned int";
		if (width <= 64 && signd) return "long long";
		if (width <= 64 && !signd) return "unsigned long long";
		return "int";
	}
	void create_any(int width, bool signd,bool in,bool out){
		if (!in && !out) return;
		if (in && out){
			create_any(width, signd,true,false);
			create_any(width, signd,false,true);
			return;
		}
		std::vector<raw_frame_entry>* pentries= in ? &entries_in:&entries_out;
		pentries->push_back(raw_frame_entry{width,create_basetype(width,signd),"pos_"+std::to_string(pentries->size()+1),nullptr});
	}
	void create_entry(ceps::ast::Nodebase_ptr expr,int width, bool signd,bool in,bool out){
		if (!in && !out) return;
		if (in && out){
			create_entry(expr,width, signd,true,false);
			create_entry(expr,width, signd,false,true);
			return;
		}
		std::vector<raw_frame_entry>* pentries= in ? &entries_in:&entries_out;
		pentries->push_back(raw_frame_entry{width,create_basetype(width,signd),"pos_"+std::to_string(pentries->size()+1),expr});
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





void write_copyright_and_timestamp(std::ostream& out, std::string title,bool b,Result_process_cmd_line const& result_cmd_line){
	if(!b) return;
	time_t timer;time(&timer);tm * timeinfo;timeinfo = localtime(&timer);
	out
		<< "/* "<< title <<" " << std::endl
		<< "   CREATED " << asctime(timeinfo) << std::endl
		<< "   GENERATED BY the sm4ceps C++ Generator VERSION 0.50 (c) Tomas Prerovsky <tomas.prerovsky@gmail.com>, ALL RIGHTS RESERVED. \n"
		<< "   Requires C++1y compatible compiler (use --std=c++1y for g++) \n"
		<< "   BASED ON cepS "<< ceps::get_version_info() << "\n\n"

		<< "   Input files (relative paths):\n";
	for( auto const & f: result_cmd_line.definition_file_rel_paths) {
	out << "      "<< f << "\n";
	}

	out << "\n";
	out	<< "   THIS IS A GENERATED FILE.\n\n"
		<< "   *** DO NOT MODIFY. ***\n*/\n"
		<< std::endl << std::endl;
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











// ******************* CPPGENERATOR



class Cppgenerator{
	std::map<std::string,sysstate> sysstates_;
	std::vector<raw_frame_t> raw_frames_;
	std::set<std::string> glob_funcs_;

	void write_cpp_expr_impl(State_machine_simulation_core* smp,
				                 Indent& indent,
								 std::ostream& os,
								 Nodebase_ptr p,
								 State_machine* cur_sm,std::vector<std::string>& parameters,
								 bool inside_func_param_list=false);
	bool write_cpp_stmt_impl(State_machine_simulation_core* smp,Indent& indent,std::ostream& os,Nodebase_ptr p,State_machine* cur_sm,std::vector<std::string>& parameters);
	bool write_raw_frame_declaration_impl(State_machine_simulation_core* smp,Indent& indent,std::ostream& os,std::vector<raw_frame_t::raw_frame_entry> const & frame_entries);
    template<typename F> void traverse_inorder_expr(Nodebase_ptr p,F f){
    	if (p == nullptr) return;
    	std::string compound_id,base_kind;
    	if (p->kind() == Ast_node_kind::binary_operator){
    		auto& binop = as_binop_ref(p);
    		traverse_inorder_expr(binop.left(),f);
    		f(p);
    		traverse_inorder_expr(binop.right(),f);
    	} else if (p->kind() == Ast_node_kind::func_call){
    		f(p);
    		ceps::ast::Func_call& func_call = *dynamic_cast<ceps::ast::Func_call*>(p);
    	    ceps::ast::Identifier& id = *dynamic_cast<ceps::ast::Identifier*>(func_call.children()[0]);
     	    ceps::ast::Call_parameters& params = *dynamic_cast<ceps::ast::Call_parameters*>(func_call.children()[1]);
    		std::vector<ceps::ast::Nodebase_ptr> args;
    		if (params.children().size()) flatten_args(params.children()[0], args);
    		for(size_t i = 0; i != args.size();++i)
    			traverse_inorder_expr(args[i],f);
    	} else if (p->kind() == Ast_node_kind::unary_operator){
    		 ceps::ast::Unary_operator& unop = *dynamic_cast<ceps::ast::Unary_operator*>(p);
    		 f(p);
    		 traverse_inorder_expr(unop.children()[0],f);
    	} else f(p);
    }
public:
	using sysstates_t = decltype(sysstates_);
	using raw_frames_t = decltype(raw_frames_);

	sysstates_t& sysstates() {return sysstates_;}
	sysstates_t const & sysstates() const {return sysstates_;}

	raw_frames_t& raw_frames() {return raw_frames_;}
	raw_frames_t const & raw_frames() const {return raw_frames_;}
	decltype(glob_funcs_)& glob_funcs(){return glob_funcs_;}
	void write_cpp_expr(State_machine_simulation_core* smp,Indent& indent,std::ostream& os,Nodebase_ptr p,State_machine* cur_sm,std::vector<std::string>& parameters);
	bool write_cpp_stmt(State_machine_simulation_core* smp,Indent& indent,std::ostream& os,Nodebase_ptr p,State_machine* cur_sm,std::vector<std::string>& parameters);
	bool write_raw_frame_declaration(State_machine_simulation_core* smp,Indent& indent,std::ostream& os,raw_frame_t const & raw_frame);
	raw_frames_t::iterator  find_raw_frame(std::string id){
		for(auto i = raw_frames().begin(); i != raw_frames().end();++i)
			if (id == i->name) return i;
		return raw_frames().end();
	}
	void write_raw_frame_send(State_machine_simulation_core* smp,Indent& indent,std::ostream& os,raw_frame_t const & raw_frame,std::string channel_id,
			 State_machine* cur_sm,std::vector<std::string>& parameters);
	std::vector<std::string> compute_all_sysstates_frame_depends_on(raw_frame_t const & raw_frame, bool in){
		std::vector<raw_frame_t::raw_frame_entry> const * pentry;

		std::set<std::string> result_set;
		if (in) pentry = &raw_frame.entries_in; else pentry = &raw_frame.entries_out;
		for(auto const & entry : *pentry){
			if (entry.expr == nullptr) continue;
			traverse_inorder_expr(entry.expr,[&result_set](Nodebase_ptr p){
				std::string compound_id,base_kind;
				if (!is_id(p,compound_id,base_kind)) return;
				if ("Systemstate" != base_kind && "Systemparameter" != base_kind ) return;
				result_set.insert(compound_id);
			} );
		}
		return std::vector<std::string>{result_set.begin(),result_set.end()};
	}
	void write_frame_context_definitions(State_machine_simulation_core* smp,Indent& indent,std::ostream& os){
		for(auto const & raw_frame : raw_frames()){
			auto s = compute_all_sysstates_frame_depends_on(raw_frame,true);
			indent.print_indentation(os);
			os << "struct " << raw_frame.name << "_in_ctxt_t : public sm4ceps_plugin_int::Framecontext "<<  "{\n";
			indent.indent_incr();
			for(auto & e : s){
				indent.print_indentation(os);
				os << "decltype(systemstates::"<< e << ") "<< e << "_";
				os << ";\n";
			}
			indent.print_indentation(os);os << "void update_sysstates();\n";
			indent.print_indentation(os);os << "void read_chunk(void*,size_t);\n";
			indent.print_indentation(os);os << "bool match_chunk(void*,size_t);\n";
			indent.print_indentation(os);os << "void init();\n";
			indent.print_indentation(os);os << "Framecontext* clone();\n";
			indent.print_indentation(os);os << "virtual ~"<< raw_frame.name << "_in_ctxt_t" <<"();\n";



			indent.indent_decr();
			indent.print_indentation(os);os << "};\n";
			indent.print_indentation(os);os <<"extern "<< raw_frame.name << "_in_ctxt_t " << raw_frame.name <<"_in_ctxt;\n";
		}
	}

	void write_frame_context_method_definitions(State_machine_simulation_core* smp,Indent& indent,std::ostream& os){
		std::vector<std::string> params;
		for(auto const & raw_frame : raw_frames()){
		  auto s = compute_all_sysstates_frame_depends_on(raw_frame,true);
		  indent.print_indentation(os);
		  os << "\n";
		  os << R"(
//Frame Context Definitions

)";
		  os << "void systemstates::" << raw_frame.name << "_in_ctxt_t::update_sysstates(){\n";
			indent.indent_incr();
			for(auto & e : s){
				indent.print_indentation(os);
				os << "systemstates::"<< e << " = " <<   e << "_";
				os << ";\n";
			}
			indent.indent_decr();
			indent.print_indentation(os);os << "}\n";

            os << "void systemstates::" << raw_frame.name << "_in_ctxt_t::read_chunk(void* chunk,size_t){\n";
			indent.indent_incr();
			indent.print_indentation(os);
			os << "raw_frm_dcls::"<< raw_frame.name << "_in& in = *((" << "raw_frm_dcls::"<< raw_frame.name << "_in" <<"*)chunk);\n";
			for(auto & e : raw_frame.entries_in){
				if (e.expr == nullptr) continue;
				std::string cid,base_kind;
				if (!is_id(e.expr,cid,base_kind)) continue;
				if (base_kind != "Systemstate" && base_kind != "Systemparameter") continue;
				indent.print_indentation(os);
				if (e.width > 1) os << cid<<"_ = " << " in." << e.name << ";\n";
				else os << cid<<"_ = (" << " in." << e.name<<"==0?0:1 )" << ";\n";

			}
			indent.indent_decr();
			indent.print_indentation(os);os << "}\n";

			os << "bool systemstates::" << raw_frame.name << "_in_ctxt_t::match_chunk(void* chunk,size_t chunk_size){\n";
			indent.indent_incr();
			indent.print_indentation(os);
			os << "if(chunk_size != " << raw_frame.len << ") return false;\n";
			indent.print_indentation(os);
			os << "raw_frm_dcls::"<< raw_frame.name << "_in& in = *((" << "raw_frm_dcls::"<< raw_frame.name << "_in" <<"*)chunk);\n";
			for(auto & e : raw_frame.entries_in){
				if (e.expr == nullptr) continue;
				std::string cid,base_kind;
				if (is_id(e.expr,cid,base_kind)) continue;
				indent.print_indentation(os);
				os << "if (";
				this->write_cpp_expr(smp,indent,os,e.expr,nullptr,params);
				os <<" != in."<< e.name<<") return false;\n";
			}
			indent.print_indentation(os);
			os << "return true;\n";
			indent.indent_decr();
			indent.print_indentation(os);os << "}\n";

	        os << "void systemstates::" << raw_frame.name << "_in_ctxt_t::init(){\n";
			indent.indent_incr();
			auto ss = compute_all_sysstates_frame_depends_on(raw_frame,true);
			for(auto & e : ss){
				indent.print_indentation(os);
				os << e <<"_ " << " = systemstates::"<< e << ";\n";
			}
			indent.indent_decr();
		    indent.print_indentation(os);os << "}\n";

		    os << "sm4ceps_plugin_int::Framecontext*  systemstates::" << raw_frame.name << "_in_ctxt_t::clone(){\n";
		  	indent.indent_incr();
		  	indent.print_indentation(os);
		  	os << "return new "<< raw_frame.name << "_in_ctxt_t(*this);\n";
  			indent.indent_decr();
  		    indent.print_indentation(os);os << "}\n";

		    os << " systemstates::" << raw_frame.name << "_in_ctxt_t::~"<<raw_frame.name << "_in_ctxt_t" <<" (){\n";
		  	indent.indent_incr();
		  	indent.print_indentation(os);
		  	indent.indent_decr();
  		    indent.print_indentation(os);os << "}\n";

			indent.print_indentation(os);os <<"systemstates::"<< raw_frame.name << "_in_ctxt_t systemstates::" << raw_frame.name <<"_in_ctxt;\n";
		}
		os << "\n\n";
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
		} else	{
			glob_funcs().insert(ceps::ast::name(func));
			os << "systemstates::Variant " << prefix << ceps::ast::name(func)  << "()";
		}
	}

	void write_cpp_glob_func_decl(std::ostream& os, std::string prefix, ceps::ast::Struct & func){
		std::vector<std::string> parameters;
		write_cpp_glob_func_decl(os,prefix,func,parameters);
	}
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

			if (glob_funcs().find(compound_id) != glob_funcs().end() )
				os << compound_id;
			else os << sysstates_namespace<<"::"<< "id{\"" << compound_id << "\"}";
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
		} else if ("send" == name(id)){
			auto raw_frame_it = this->find_raw_frame(name(as_id_ref(args[0])));
			if (raw_frame_it == raw_frames().end())
				smp->fatal_(-1,name(as_id_ref(args[0]))+": No raw frame definition with that id found.");
			this->write_raw_frame_send(smp,indent,os,*raw_frame_it,name(as_id_ref(args[1])),cur_sm,parameters);
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

void Cppgenerator::write_raw_frame_send(State_machine_simulation_core* smp,
		Indent& indent,
		std::ostream& os,
		raw_frame_t const & raw_frame,
		std::string channel_id,
		State_machine* cur_sm,std::vector<std::string>& parameters){

	os << "{\n";
	indent.indent_incr();
	indent.print_indentation(os);
	os << "raw_frm_dcls::"<<raw_frame.name<< "_out out = {0};\n";
	for (auto const & entry: raw_frame.entries_out){
		if (entry.expr == nullptr) continue;
		indent.print_indentation(os);
		os << "out." << entry.name << " = ";
		this->write_cpp_expr(smp,indent,os,entry.expr,cur_sm,parameters);
		if (entry.width == 1) os << " != 0";
		os << ";\n";
	}
	indent.print_indentation(os);
	os << "smcore_interface->send_raw_frame(&out,sizeof(out),"<<raw_frame.header_length<<",\""<<channel_id <<"\");\n";
	indent.indent_decr();
	indent.print_indentation(os);os << "}";
}

bool Cppgenerator::write_raw_frame_declaration_impl(State_machine_simulation_core* smp,
		Indent& indent,
		std::ostream& os,
		std::vector<raw_frame_t::raw_frame_entry> const & frame_entries){

	for(auto const& entry : frame_entries){
		indent.print_indentation(os);
		os << entry.base_type << " " << entry.name << ":"<< entry.width;
		os << ";\n";
	}
	return true;
}

bool Cppgenerator::write_raw_frame_declaration(State_machine_simulation_core* smp,Indent& indent,std::ostream& os,raw_frame_t const & raw_frame){

	if (raw_frame.entries_in.size()){
		indent.print_indentation(os);os << "struct " << raw_frame.name << "_in" << "{\n";
		indent.indent_incr();
		write_raw_frame_declaration_impl(smp,indent,os,raw_frame.entries_in);
		indent.indent_decr();
		indent.print_indentation(os);os << "}__attribute__ ((__packed__));\n";
	}
	if (raw_frame.entries_out.size()){
		indent.print_indentation(os);os << "struct " << raw_frame.name << "_out" << "{\n";
		indent.indent_incr();
		write_raw_frame_declaration_impl(smp,indent,os,raw_frame.entries_out);
		indent.indent_decr();
		indent.print_indentation(os);os << "}__attribute__ ((__packed__));\n";
	}

	return true;
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





std::map<std::string, std::pair<int,bool> > map_descr_to_inttype =
 {
   {std::string{"byte"},{8,true}},{std::string{"int8"},{8,true}},{std::string{"int16"},{16,true}},{std::string{"int32"},{32,true}},{std::string{"int64"},{64,true}},
   {std::string{"bit"},{1,true}},{std::string{"int4"},{4,true}},{std::string{"int2"},{2,true}},{std::string{"int3"},{3,true}},{std::string{"int5"},{5,true}},
   {std::string{"int6"},{6,true}},{std::string{"int7"},{7,true}},{std::string{"int9"},{9,true}},{std::string{"int10"},{10,true}},{std::string{"int11"},{11,true}},
   {std::string{"int12"},{12,true}},{std::string{"int13"},{13,true}},{std::string{"int14"},{14,true}},{std::string{"int15"},{15,true}},{std::string{"int17"},{17,true}},
   {std::string{"int18"},{18,true}},{std::string{"int19"},{19,true}},{std::string{"int20"},{20,true}},{std::string{"int21"},{21,true}},{std::string{"int22"},{22,true}},
   {std::string{"int23"},{23,true}},{std::string{"int24"},{24,true}},{std::string{"int25"},{25,true}},{std::string{"int26"},{26,true}},{std::string{"int27"},{27,true}},
   {std::string{"int28"},{28,true}},{std::string{"int29"},{29,true}},{std::string{"int30"},{30,true}},{std::string{"int31"},{31,true}},{std::string{"longlong"},{64,true}},
   {std::string{"ubyte"},{8,false}},{std::string{"uint8"},{8,false}},{std::string{"uint16"},{16,false}},{std::string{"uint32"},{32,false}},{std::string{"int64"},{64,true}},
   {std::string{"ubit"},{1,false}},{std::string{"uint4"},{4,false}},{std::string{"uint2"},{2,false}},{std::string{"uint3"},{3,false}},{std::string{"uint5"},{5,false}},
   {std::string{"uint6"},{6,false}},{std::string{"uint7"},{7,false}},{std::string{"uint9"},{9,false}},{std::string{"uint10"},{10,false}},{std::string{"uint11"},{11,false}},
   {std::string{"uint12"},{12,false}},{std::string{"uint13"},{13,false}},{std::string{"uint14"},{14,false}},{std::string{"uint15"},{15,false}},{std::string{"uint17"},{17,false}},
   {std::string{"uint18"},{18,false}},{std::string{"uint19"},{19,false}},{std::string{"uint20"},{20,false}},{std::string{"uint21"},{21,false}},{std::string{"uint22"},{22,false}},
   {std::string{"uint23"},{23,false}},{std::string{"uint24"},{24,false}},{std::string{"uint25"},{25,false}},{std::string{"uint26"},{26,false}},{std::string{"uint27"},{27,false}},
   {std::string{"uint28"},{28,false}},{std::string{"uint29"},{29,false}},{std::string{"uint30"},{30,false}},{std::string{"uint31"},{31,false}},{std::string{"ulonglong"},{64,false}}
 };


struct Framebuilder{

	State_machine_simulation_core* smc;


size_t build_raw_frame( size_t & header_length,
		                ceps::ast::Nodebase_ptr p,
		                size_t data_size,
						raw_frame_t& raw_frame,
						size_t bit_offs,
						bool in,bool out,
		                size_t bit_width=sizeof(std::int64_t)*8,
		                bool signed_value = true,
		                bool write_data = true,
		                bool host_byte_order = true) {
	using namespace ceps::ast;
	if (p == nullptr) return 0;
	if (p->kind() == ceps::ast::Ast_node_kind::structdef){
		auto& st = ceps::ast::as_struct_ref(p);
		auto& nm = ceps::ast::name(st);
		auto type_id = map_descr_to_inttype.find(nm);
		if (type_id != map_descr_to_inttype.end()){
			return build_raw_frame(header_length,st.children(), data_size,raw_frame, bit_offs, in,out, type_id->second.first,type_id->second.second,write_data,host_byte_order);
		} else 	if (nm == "in"){
			build_raw_frame(header_length,st.children(), data_size,raw_frame, bit_offs,true,false,  bit_width,signed_value,write_data,host_byte_order);
			//We assume there is always a twin out
			return 0;
		}else 	if (nm == "out"){
			return build_raw_frame(header_length,st.children(), data_size,raw_frame, bit_offs,false,true,  bit_width,signed_value,write_data,host_byte_order);
		} else {
			return build_raw_frame(header_length,st.children(), data_size,raw_frame, bit_offs,in,out, bit_width,signed_value,write_data,host_byte_order);
		}
	}
	else if (p->kind() == ceps::ast::Ast_node_kind::identifier){
		auto& ident = ceps::ast::as_id_ref(p);
		if (ceps::ast::name(ident) == "any") {
			if (write_data){
				raw_frame.create_any(bit_width,signed_value,in,out);
			}
			return bit_width;
		}
		else if (ceps::ast::name(ident) == "__current_frame_size") {
			if (write_data){
				auto temp_node = new ceps::ast::Int(data_size,ceps::ast::all_zero_unit(), nullptr, nullptr, nullptr);
				raw_frame.create_entry(temp_node,bit_width,signed_value,in,out);
			}
			return bit_width;
		} else {
			if (write_data) raw_frame.create_entry(p,bit_width,signed_value,in,out);
			return bit_width;
		};
	}	else if (p->kind() == ceps::ast::Ast_node_kind::float_literal) {
		 smc->fatal_(-1,"Floating point numbers not supported in raw frames.");
	}	else if (p->kind() == ceps::ast::Ast_node_kind::string_literal){
		 smc->fatal_(-1,"Strings not supported in raw frames.");
	}	else {
		if (write_data) raw_frame.create_entry(p,bit_width,signed_value,in,out);
		return bit_width;
	}
	return 0;
}

size_t build_raw_frame(size_t & header_length,std::vector<ceps::ast::Nodebase_ptr> pattern,
		            size_t data_size,raw_frame_t& raw_frame,size_t bit_offs,bool in,bool out,
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
		auto rr = build_raw_frame(header_length,p,data_size,raw_frame,bit_offs,in,out,bit_width,signed_value,write_data,host_byte_order);
		if (header) {header_length = rr; }
		r+=rr;
		bit_offs += rr;
	}
	return r;
}
};


void handle_frames(Cppgenerator& cppgenerator,State_machine_simulation_core* smc,ceps::Ceps_Environment& ceps_env,
		  ceps::ast::Nodeset& universe,
		  std::map<std::string, ceps::ast::Nodebase_ptr> const & all_guards,
		  ceps::ast::Nodeset& frames,Result_process_cmd_line const& result_cmd_line)
{
 using namespace ceps::ast;
 if (frames.size() == 0) return;
 //Preludes
 std::string out_cpp;
 std::string out_hpp;

 std::ofstream o_cpp{out_cpp = "out_frames.cpp"};
 std::ofstream o_hpp{out_hpp = "out_frames.hpp"};

 write_copyright_and_timestamp(o_cpp,out_cpp,true,result_cmd_line);
 write_copyright_and_timestamp(o_hpp,out_hpp,true,result_cmd_line);
 Indent indent_hpp;Indent indent_cpp;
 o_hpp << "namespace raw_frm_dcls{\n";
 indent_hpp.indent_incr();
 Framebuilder framebuilder{smc};

 for (auto frame_ : frames.nodes()){
	 auto & frame = as_struct_ref(frame_);
	 raw_frame_t raw_frame;
	 raw_frame.name = Nodeset(frame.children())["id"].as_str();
	 size_t data_size = framebuilder.build_raw_frame(
			            raw_frame.header_length,
						Nodeset(frame.children())["data"].nodes(),
	 		            0,raw_frame,0,true,true,
	 		            sizeof(std::int64_t)*8,
	 		            true,
	 		            false,
	 		            true
	 		            );
	 raw_frame.len = data_size = data_size / 8 + (data_size % 8 ? 1 : 0);
	 framebuilder.build_raw_frame(
			 	 	 	    raw_frame.header_length,
	 						Nodeset(frame.children())["data"].nodes(),
	 	 		            data_size,raw_frame,0,true,true,
	 	 		            sizeof(std::int64_t)*8,
	 	 		            true,
	 	 		            true,
	 	 		            true
	 	 		            );
	 cppgenerator.raw_frames().push_back(raw_frame);
 }

 for (auto& frame : cppgenerator.raw_frames()){
	 cppgenerator.write_raw_frame_declaration(smc,indent_hpp,o_hpp,frame);o_hpp << "\n";
 }

 indent_hpp.indent_decr();
 o_hpp << "}\n";
}

void State_machine_simulation_core::do_generate_cpp_code(ceps::Ceps_Environment& ceps_env,
													  ceps::ast::Nodeset& universe,
													  std::map<std::string, ceps::ast::Nodebase_ptr> const & all_guards,Result_process_cmd_line const& result_cmd_line){
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

	write_copyright_and_timestamp(o_cpp,out_cpp,true,result_cmd_line);
	write_copyright_and_timestamp(o_hpp,out_hpp,true,result_cmd_line);

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
	if (raw_frames.size()) o_hpp << "#include\"out_frames.hpp\"\n";
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
	o_cpp <<"void init_frame_ctxts();\n";
	o_cpp << "\n\n" <<  init_func << "{\n";
	indent_cpp.indent_incr();
	for_all_nodes(this,globals,[&](State_machine_simulation_core*,Nodebase_ptr p,int,std::set<std::string>& seen) {
		std::vector<std::string> dummy;
		if (cppgenerator.write_cpp_stmt(this,indent_cpp,o_cpp,p,nullptr,dummy)) o_cpp << ";\n";
	});
	indent_hpp.indent_decr();
	indent_cpp.print_indentation(o_cpp);o_cpp <<"init_frame_ctxts();\n";
	o_cpp << "}\n";

	for(auto & state_entry : sys_states){
		indent_hpp.print_indentation(o_hpp);write_cpp_systemstate_declaration(o_hpp,state_entry);o_hpp << ";\n";
		indent_cpp.print_indentation(o_cpp);o_cpp << sysstates_namespace<<"::";write_cpp_systemstate_declaration(o_cpp,state_entry);
		cppgenerator.sysstates()[state_entry.name] = state_entry;
		//if (state_entry.type.t != Type::Struct){o_cpp << " = "; write_cpp_expr(indent_cpp,o_cpp,systemstate_first_def[state_entry]);}
		//else o_cpp << "{}";
		o_cpp << ";\n";
	}

	handle_frames(cppgenerator,this,ceps_env,
				  universe,
				  all_guards,
				  raw_frames,result_cmd_line);
	o_hpp << "\n";
	cppgenerator.write_frame_context_definitions(this,indent_hpp,o_hpp);

	o_cpp << "\n\n";
	indent_cpp.print_indentation(o_cpp);
	o_cpp << "void init_frame_ctxts(){\n";
	indent_cpp.indent_incr();
	for(auto & r : cppgenerator.raw_frames()){
		indent_cpp.print_indentation(o_cpp);o_cpp << "systemstates::"<< r.name <<"_in_ctxt.init();\n";
	}
	indent_cpp.indent_decr();
	indent_cpp.print_indentation(o_cpp);
	o_cpp << "}\n";

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
		cppgenerator.write_cpp_glob_func_decl(o_hpp,"",str);
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
			cppgenerator.write_cpp_glob_func_decl(o_cpp,global_functions_namespace+"::",str,parameters);
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

	cppgenerator.write_frame_context_method_definitions(this,indent_cpp,o_cpp);


    //Handle receiver of raw messages
	auto receiver = universe[all{"receiver"}];
	if(receiver.size()){
		for(auto r_:receiver.nodes()){
			auto r = Nodeset(as_struct_ref(r_).children());
			std::string recv_id = r["id"].as_str();
			auto on_msgs_ = r[all{"on_msg"}];
			if (on_msgs_.size() == 0) continue;
			for(auto on_msg_: on_msgs_.nodes()){
			 auto on_msg = Nodeset(as_struct_ref(on_msg_).children());
			 auto frame_ids = on_msg["frame_id"];
			 if (frame_ids.size() != 1) continue;
			 std::string frame_id = name(as_id_ref(frame_ids.nodes()[0]));
			 auto handler = on_msg["handler"];
			 for (auto h : handler.nodes()){
				 if (h->kind() != Ast_node_kind::identifier) continue;
				 init_plugin_content << "systemstates::"<< frame_id <<"_in_ctxt.set_handler("
				 << "&globfuncs::"<< name(as_id_ref(h)) << ");\n";
				 init_plugin_content << "smcore_interface->register_frame_ctxt(&"
				 << "systemstates::"<< frame_id <<"_in_ctxt,\"" << recv_id <<"\");\n";
			 }
			}
		}
	}

	//Register global functions without parameters

	for(auto const & e : cppgenerator.glob_funcs())
		init_plugin_content << "smcore_interface->register_global_function(\""<<e<<"\",globfuncs::"<<e<<");\n";


	init_plugin_content << "}\n";

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
void globfuncs::start_periodic_timer(double t,sm4ceps_plugin_int::Variant (*fp)(),sm4ceps_plugin_int::id id_){smcore_interface->start_periodic_timer(t,fp,id_);}
void globfuncs::start_periodic_timer(double t,sm4ceps_plugin_int::Variant (*fp)()){smcore_interface->start_periodic_timer(t,fp);}
void globfuncs::stop_timer(sm4ceps_plugin_int::id id_){smcore_interface->stop_timer(id_);}
bool in_state(std::initializer_list<systemstates::id> state_ids){return smcore_interface->in_state(state_ids);}
)~";



}


