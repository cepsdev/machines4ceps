#include "utils/can_layer_docgen.hpp"
#include <string>

#define INVARIANT(x)

struct cldocgen_state{
	std::string name;
	std::vector<ceps::ast::Nodebase_ptr> constraints;
	ceps::ast::Nodebase_ptr default_value;
};


static bool is_assignment(ceps::ast::Nodebase_ptr p){
	if (p == nullptr) return false;
	if (p->kind() != ceps::ast::Ast_node_kind::binary_operator) return false;
	if (ceps::ast::op(ceps::ast::as_binop_ref(p)) != '=') return false;
	return true;
}

static bool is_symbol(ceps::ast::Nodebase_ptr p, std::string& name, std::string& kind){
	if (p == nullptr) return false;
	if (p->kind() != ceps::ast::Ast_node_kind::symbol) return false;
	name = ceps::ast::name(ceps::ast::as_symbol_ref(p));
	kind = ceps::ast::kind(ceps::ast::as_symbol_ref(p));
	return true;
}

static bool get_one_and_only_symbol(ceps::ast::Nodebase_ptr p, std::string& name, std::string& kind){
	if (p == nullptr) return false;
	if (p->kind() == ceps::ast::Ast_node_kind::symbol)
	{
	 name = ceps::ast::name(ceps::ast::as_symbol_ref(p));
	 kind = ceps::ast::kind(ceps::ast::as_symbol_ref(p));
	 return true;
	}

	if (p->kind() == ceps::ast::Ast_node_kind::int_literal || p->kind() == ceps::ast::Ast_node_kind::float_literal || p->kind() == ceps::ast::Ast_node_kind::string_literal)
		return false;

	if (p->kind() == ceps::ast::Ast_node_kind::binary_operator) {
      auto & oper = ceps::ast::as_binop_ref(p);
      std::string name1;
      std::string kind1;
      std::string name2;
      std::string kind2;
      bool r1 = get_one_and_only_symbol(oper.left(),name1,kind1);
      bool r2 = get_one_and_only_symbol(oper.right(),name2,kind2);
      if (!(r1 || r2)) return false;
      if (! (r1 && r2) ) {
    	  if (r1) {name=name1;kind=kind1;return true;}
    	  else {name=name2;kind=kind2;return true;}
      }
      if (name1 != name2 || kind1 != kind2 ) return false;
      name = name1; kind = kind1;
      return true;
	}

	return false;
}

static std::string binop2str(int ch){
	if (ch <= 255){
		std::stringstream ss;
		ss << (char) ch;
		return ss.str();
	}

	return "?";
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


static std::string expr2asciimath(ceps::ast::Nodebase_ptr p){
	if (p == nullptr) return {};
	std::stringstream ss;
	if (p->kind() == ceps::ast::Ast_node_kind::float_literal)
	{
		ss << ceps::ast::value(ceps::ast::as_double_ref(p));
		return ss.str();
	}
	else if (p->kind() == ceps::ast::Ast_node_kind::int_literal)
	{
		ss << ceps::ast::value(ceps::ast::as_int_ref(p));
		return ss.str();
	}
	else if (p->kind() == ceps::ast::Ast_node_kind::string_literal)
	{
		ss << "'" << ceps::ast::value(ceps::ast::as_string_ref(p)) << "'";
		return ss.str();
	}
	else if (p->kind() == ceps::ast::Ast_node_kind::binary_operator){
		auto & bop = ceps::ast::as_binop_ref(p);

		return "("+ expr2asciimath(bop.left()) + binop2str(ceps::ast::op(bop)) + expr2asciimath(bop.right()) + ")";
	} else if (p->kind() == ceps::ast::Ast_node_kind::func_call){
		ceps::ast::Func_call& func_call = *dynamic_cast<ceps::ast::Func_call*>(p);
	    ceps::ast::Identifier& id = *dynamic_cast<ceps::ast::Identifier*>(func_call.children()[0]);
 	    ceps::ast::Call_parameters& params = *dynamic_cast<ceps::ast::Call_parameters*>(func_call.children()[1]);
		std::vector<ceps::ast::Nodebase_ptr> args;
		if (params.children().size()) flatten_args(params.children()[0], args);
		std::string r =ceps::ast::name(id) + "(";

		r+=")";
		return r;
	}

    return {};
}


void sm4ceps::utils::dump_asciidoc_canlayer_doc(std::ostream& os,State_machine_simulation_core* smc){
 using namespace ceps::ast;
 auto & ns = smc->current_universe();
 std::map<std::string,cldocgen_state> states;
 auto frames = ns[all{"frame"}];
 auto encodings = ns[all{"encoding"}];
 auto constraints = ns["constraints"];
 auto globals = ns["Globals"];
 for(auto e : globals.nodes()){
  if (!is_assignment(e)) continue;
  std::string lhs_name,lhs_kind;
  if (!get_one_and_only_symbol(as_binop_ref(e).left(), lhs_name,lhs_kind)) continue;
  if (lhs_kind != "Systemstate") continue;
  cldocgen_state state; state.name = lhs_name;state.default_value = as_binop_ref(e).right();
  states[state.name] = state;
 }

 for(auto & expr : constraints){


 }


 os << ":stem:\n";
 os << "= CAN Layer \n\n\n";
 os << "== Signals \n\n\n";
 for(auto const & s : states){
   os << "=== "<<s.first<<"\n\n";
   std::string def_val_expr = expr2asciimath(s.second.default_value);
   os << "**Default value = ** \n";
   if (def_val_expr.length() > 0) os << "stem:["<< def_val_expr <<"]\n\n";
   else os << "\nWARNING: Not available.\n\n";
 }
 os << "== Frames \n\n\n";
 //os << "[asciimath]\n++++\n gag <= 2.0 * x\n++++\n";
}
