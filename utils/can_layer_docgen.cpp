#include "utils/can_layer_docgen.hpp"
#include <string>

#define INVARIANT(x)

struct cldocgen_state{
	std::string name;
	std::vector<ceps::ast::Nodebase_ptr> constraints;
	ceps::ast::Nodebase_ptr default_value;
	std::vector<ceps::ast::Nodebase_ptr> encodings;
	std::vector< std::vector<std::pair<std::string,std::string>>> encodings_other_vars;
};

struct cldocgen_frame_row{
	std::string width;
	std::string signal;
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

static bool get_symbols(ceps::ast::Nodebase_ptr p, std::set<std::pair<std::string,std::string>> & symbols){
	if (p == nullptr) return false;
	if (p->kind() == ceps::ast::Ast_node_kind::symbol)
	{
	 symbols.insert(
			 std::make_pair(ceps::ast::name(ceps::ast::as_symbol_ref(p)),ceps::ast::kind(ceps::ast::as_symbol_ref(p)) ));
	 return true;
	}

	if (p->kind() == ceps::ast::Ast_node_kind::int_literal || p->kind() == ceps::ast::Ast_node_kind::float_literal || p->kind() == ceps::ast::Ast_node_kind::string_literal)
		return false;

	if (p->kind() == ceps::ast::Ast_node_kind::binary_operator) {
      auto & oper = ceps::ast::as_binop_ref(p);
      bool r1 = get_symbols(oper.left(),symbols);
      bool r2 = get_symbols(oper.right(),symbols);
      return r1 || r2;
	}

	if (p->kind() == ceps::ast::Ast_node_kind::func_call){
	  ceps::ast::Func_call& func_call = *dynamic_cast<ceps::ast::Func_call*>(p);
	  ceps::ast::Identifier& id = *dynamic_cast<ceps::ast::Identifier*>(func_call.children()[0]);
	  ceps::ast::Call_parameters& params = *dynamic_cast<ceps::ast::Call_parameters*>(func_call.children()[1]);
	  std::vector<ceps::ast::Nodebase_ptr> args;
	  if (params.children().size()) flatten_args(params.children()[0], args);
	  else return false;
	  bool r = false;
	  for(auto e : args)
		  r = r || get_symbols(e,symbols);
	}

	return false;
}


static std::map<int,int> precedence;

static bool is_zero(ceps::ast::Nodebase_ptr p){
	if (p->kind() == ceps::ast::Ast_node_kind::float_literal && ceps::ast::value(ceps::ast::as_double_ref(p)) == 0.0) return true;
	if (p->kind() == ceps::ast::Ast_node_kind::int_literal && ceps::ast::value(ceps::ast::as_int_ref(p)) == 0) return true;

    return false;
}

static std::string expr2asciimath(ceps::ast::Nodebase_ptr p, ceps::ast::Nodebase_ptr parent){
	if (p == nullptr) return {};
	std::stringstream ss;
	if (p->kind() == ceps::ast::Ast_node_kind::float_literal)
	{
		ss << " " << ceps::ast::value(ceps::ast::as_double_ref(p)) << " ";
		if (ceps::ast::value(ceps::ast::as_double_ref(p)) < 0.0) return "(" + ss.str() + ")";
		return ss.str();
	}
	else if (p->kind() == ceps::ast::Ast_node_kind::int_literal)
	{
		ss << " "<< ceps::ast::value(ceps::ast::as_int_ref(p))<< " ";
		return ss.str();
	}
	else if (p->kind() == ceps::ast::Ast_node_kind::string_literal)
	{
		ss << "'" << ceps::ast::value(ceps::ast::as_string_ref(p)) << "'";
		return ss.str();
	}
	else if (p->kind() == ceps::ast::Ast_node_kind::binary_operator){
		auto & bop = ceps::ast::as_binop_ref(p);
                auto oop = ceps::ast::op(bop);
		auto l = bop.left();
		auto r = bop.right();
		bool l_iszero = is_zero(l);
		bool r_iszero = is_zero(r);

		if (l_iszero && r_iszero) return "0";
		if (r_iszero && oop == '+') return expr2asciimath(l,p);

		if (parent == nullptr || parent->kind() != ceps::ast::Ast_node_kind::binary_operator)
			return expr2asciimath(bop.left(),p) + binop2str(ceps::ast::op(bop)) + expr2asciimath(bop.right(),p);
		if(parent->kind() == ceps::ast::Ast_node_kind::binary_operator && ceps::ast::op(ceps::ast::as_binop_ref(parent)) == '=')
			return expr2asciimath(bop.left(),p) + binop2str(ceps::ast::op(bop)) + expr2asciimath(bop.right(),p);
		if(parent->kind() == ceps::ast::Ast_node_kind::binary_operator && ceps::ast::op(ceps::ast::as_binop_ref(parent)) == '+' && op(bop) == '*')
					return expr2asciimath(bop.left(),p) + binop2str(ceps::ast::op(bop)) + expr2asciimath(bop.right(),p);

		return "("+ expr2asciimath(bop.left(),p) + binop2str(ceps::ast::op(bop)) + expr2asciimath(bop.right(),p) + ")";
	} else if (p->kind() == ceps::ast::Ast_node_kind::func_call){
		ceps::ast::Func_call& func_call = *dynamic_cast<ceps::ast::Func_call*>(p);
	    ceps::ast::Identifier& id = *dynamic_cast<ceps::ast::Identifier*>(func_call.children()[0]);
 	    ceps::ast::Call_parameters& params = *dynamic_cast<ceps::ast::Call_parameters*>(func_call.children()[1]);
		std::vector<ceps::ast::Nodebase_ptr> args;
		if (params.children().size()) flatten_args(params.children()[0], args);
		std::string r =ceps::ast::name(id) + "(";
        if (args.size()) {
        	for(int i = 0; i!=args.size()-1;++i) r += expr2asciimath(args[i],p) + ",";
        	r += expr2asciimath(args[args.size()-1],p);
        }
		r+=")";
		return r;
	} else if (p->kind() == ceps::ast::Ast_node_kind::symbol){
		return "\""+ceps::ast::name(ceps::ast::as_symbol_ref(p))+"\"";
	}

    return {};
}

static void build_frame_rows(ceps::ast::Nodeset ns,std::vector<cldocgen_frame_row>& rows){
 for (auto e : ns.nodes()){
	 if (e->kind() != ceps::ast::Ast_node_kind::structdef) continue;
	 auto & s = ceps::ast::as_struct_ref(e);
	 if (s.children().size() == 0) continue;
	 cldocgen_frame_row r;
	 r.width = ceps::ast::name(s);
	 auto ch = s.children()[0];
	 if (ch->kind() == ceps::ast::Ast_node_kind::identifier) r.signal = "unused";
	 else if (ch->kind() == ceps::ast::Ast_node_kind::symbol) r.signal = ceps::ast::name(ceps::ast::as_symbol_ref(ch));
	 rows.push_back(r);
 }
}


void sm4ceps::utils::dump_asciidoc_canlayer_doc(std::ostream& os,State_machine_simulation_core* smc){
 using namespace ceps::ast;
 auto & ns = smc->current_universe();
 std::map<std::string,cldocgen_state> states;
 auto frames = ns[all{"frame"}]; auto encodings = ns[all{"encoding"}]; auto constraints = ns["constraints"]; auto globals = ns["Globals"];
 for(auto e : globals.nodes()){
  if (!is_assignment(e)) continue;
  std::string lhs_name,lhs_kind;
  if (!get_one_and_only_symbol(as_binop_ref(e).left(), lhs_name,lhs_kind)) continue;
  if (lhs_kind != "Systemstate") continue;
  cldocgen_state state; state.name = lhs_name;state.default_value = as_binop_ref(e).right();
  states[state.name] = state;
 }

 for(auto e : constraints.nodes()){
	 std::set<std::pair<std::string,std::string>> symbols;
	 if (!get_symbols(e,symbols)) continue;
     for (auto const & sym : symbols)
    	 if (sym.second == "Systemstate") states[sym.first].constraints.push_back(e);
 }

 for (auto enc_: encodings){
	 auto enc = enc_["encoding"];
	 for (auto e : enc.nodes()){
		 if (!is_assignment(e)) continue;
		 std::set<std::pair<std::string,std::string>> symbols;
	     if (!get_symbols(e, symbols)) continue;
	     for(auto s: symbols) {
	    	 auto it = states.find(s.first);
	    	 if (it == states.end()) continue;
	    	 it->second.encodings.push_back(e);
	    	 std::vector<std::pair<std::string,std::string>> vv;
	    	 for(auto ss: symbols) {if (ss.first == it->first) continue; vv.push_back(ss);}
	    	 it->second.encodings_other_vars.push_back(vv);
	     }

	 }
 }

 os << ":stem:\n";os << "= CAN Layer \n\n\n"; os << "== Signals \n\n\n";
 for(auto const & s : states){
   os << "=== "<<"[["<< s.first <<"]]"<<s.first<<"\n\n";  std::string def_val_expr = expr2asciimath(s.second.default_value,nullptr);
   os << "**Default value : **\n\n";
   if (def_val_expr.length() > 0) os << "stem:["<< def_val_expr <<"]\n\n";
   else os << "\nWARNING: Not available.\n\n";
   if (s.second.constraints.size()){
	   os << "**Constraints : **\n\n";
	   for (auto c : s.second.constraints ){
		   auto expr = expr2asciimath(c,nullptr);
		   os << "stem:["<< expr << "]\n\n";
	   }
   }
   if (s.second.encodings.size()){
    os << "==== Encodings\n";
    os << "|===\n";
    for(int i = 0; i != s.second.encodings.size();++i){
    	auto & e = s.second.encodings[i];
    	auto expr = expr2asciimath(e,nullptr);
    	os << "|";
    	for(auto ss : s.second.encodings_other_vars[i]){
    		os <<"*"<< ss.first<< "* is a " <<ss.second << " field ";
    	}
    	os << "|" <<  "stem:["<< expr << "]\n";
    }
    os << "|===\n";
   }

 }
 os << "== Frames \n\n\n";
 for(auto frm_ : frames){
  auto frm = frm_["frame"];
  auto id = frm["id"].as_str();
  os << "=== " << id << "\n\n";
  std::vector<cldocgen_frame_row> rows;
  build_frame_rows(frm["data"]["payload"]["out"],rows);
  os << "|===\n";
  os << "| Type | Signal \n\n";
  for(auto r : rows){
	  os << "| " << r.width << "|";
	  if (r.signal == "unused") os << r.signal << "\n";
	  else os <<"<<" << r.signal<< ", "<<r.signal <<">>" << "\n";
  }
  os << "|===\n";
 }
}




