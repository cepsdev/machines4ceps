#include "fibex_import.hpp"

 struct fibex_signal{
	 static std::string normalize_signal_name(std::string);
	 std::string name;
	 double default_value=0;
	 fibex_signal(std::vector<ceps::ast::Nodebase_ptr> const & v){
      for(auto e : v){
    	  if (e->kind() != ceps::ast::Ast_node_kind::structdef) continue;
    	  auto & s = ceps::ast::as_struct_ref(e);
    	  auto s_name = ceps::ast::name(s);
    	  if ("ho:SHORT-NAME" == s_name){
    		name = normalize_signal_name(ceps::ast::Nodeset(e)["ho:SHORT-NAME"].as_str());
    	  } else if ("fx:DEFAULT-VALUE" == s_name){
    		  default_value = std::stod(ceps::ast::Nodeset(e)["fx:DEFAULT-VALUE"].as_str());
    	  }
      }
	 }
 };

 std::string fibex_signal::normalize_signal_name(std::string s){
  std::string::size_type j = 0;
  for(;j!=s.length();++j){
	  if (std::isalpha(s[j])) break;
  }
  if (j == s.length()) return "";
  return s.substr(j);
 }

 ceps::ast::Nodeset sm4ceps::utils::import_fibex(
		        State_machine_simulation_core* smc,
  				ceps::ast::Struct_ptr fibex_struct,
  				ceps::parser_env::Symboltable & sym_tab,
  				ceps::interpreter::Environment& env 
  				){
  ceps::ast::Nodeset r;
  ceps::ast::Nodeset ns{fibex_struct->children()};
  auto signals = ns["fx:ELEMENTS"]["fx:SIGNALS"];
  std::vector<fibex_signal> sig_vec;
  for(auto e : signals.nodes()){
	  if (e->kind() != ceps::ast::Ast_node_kind::structdef || ceps::ast::name(ceps::ast::as_struct_ref(e)) != "fx:SIGNAL") continue;
	  sig_vec.push_back(fibex_signal(ceps::ast::as_struct_ref(e).children()));
  }

  for(auto const & s:sig_vec){
	//r.nodes().push_back(new ceps::ast::Symbol(s.name,"Systemstate",nullptr,nullptr,nullptr) );
  }

  auto glob_sec = new ceps::ast::Struct("Globals",nullptr,nullptr,nullptr);

  for(auto const & s:sig_vec){
	  glob_sec->children().push_back(
	   new ceps::ast::Binary_operator('=',
			                       new ceps::ast::Symbol(s.name,"Systemstate"),
								   new ceps::ast::Double(s.default_value,ceps::ast::all_zero_unit()) )
	  );
  }

  r.nodes().push_back(glob_sec);

  return r;
}
