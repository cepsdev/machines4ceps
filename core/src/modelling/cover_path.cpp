#include "core/include/state_machine_simulation_core.hpp"
#include "core/include/modelling/cover_path.hpp"
#include "core/include/modelling/gensm.hpp"


static size_t cover_path_counter = 0;

static std::string dot_expr_to_string(ceps::ast::Nodebase_ptr e, std::string sep = "_"){
	if (e->kind() == ceps::ast::Ast_node_kind::identifier)
		return ceps::ast::name(ceps::ast::as_id_ref(e));
    if (e->kind() == ceps::ast::Ast_node_kind::binary_operator)
    	return dot_expr_to_string(ceps::ast::as_binop_ref(e).left(),sep)+sep+dot_expr_to_string(ceps::ast::as_binop_ref(e).right(),sep);
    return "";
}

ceps::ast::Nodeset sm4ceps::modelling::cover_path(ceps::ast::Struct_ptr what,
		                                          ceps::ast::Nodebase_ptr root,
 				                                  ceps::parser_env::Symbol* sym,
												  ceps::parser_env::Symboltable & symtab,
												  ceps::interpreter::Environment& env,
  				                                  ceps::ast::Nodebase_ptr ,ceps::ast::Nodebase_ptr )
{
 using namespace sm4ceps::modelling::gensm;
 using namespace ceps::ast;
 std::string part_id;
 std::vector<std::pair<ceps::ast::Nodebase_ptr,std::string>> steps;

 for(auto e: what->children()){
	 if ( e->kind() == Ast_node_kind::binary_operator && op(as_binop_ref(e)) == '='  &&
		  (as_binop_ref(e).left()->kind() == Ast_node_kind::identifier && name(as_id_ref(as_binop_ref(e).left()))=="id" ) ) {
		 part_id = name(as_id_ref(as_binop_ref(e).right()));
		 break;
	 } else if ( e->kind() == Ast_node_kind::binary_operator && op(as_binop_ref(e)) == '.' ){
		 //step new ceps::ast::Call_parameters($1,nullptr,nullptr);
		 steps.push_back(
				 std::make_pair(new ceps::ast::Func_call(new ceps::ast::Identifier("in_state"),new ceps::ast::Call_parameters(e,nullptr,nullptr) ),
				                dot_expr_to_string(e,"_"))
		 );
	 }
 }

 if (part_id.length() == 0) part_id = std::to_string(++cover_path_counter);

 auto gen_sm = sm("__coverpath___"+part_id);

 for(auto e : steps) std::cout << e.second << std::endl;

 /*int guard_ctr = 0;
 gen_sm.add_state("Initial");
 for(auto& e : range2guard){
	 gen_sm.add_state(e.first);
 }

 for(auto& p : range2guard)
	 gen_sm.add_transition("Initial",p.first,p.second);

 for(auto& from : range2guard){
	 for(auto& to : range2guard){
		 if (from.first == to.first) continue;
		 gen_sm.add_transition(from.first,to.first,to.second);
	 }
 }*/

 gen_sm.header = new ceps::ast::strct{"cover",ceps::ast::ident{"cover_edges_upto_1"}};

 Nodeset result;
 result.nodes().push_back(what);
 result.nodes().push_back(gen_sm.ns().nodes()[0]);
 //result.nodes().insert(result.nodes().begin(),gen_sm.ns().nodes().begin(),gen_sm.ns().nodes().end());
 return result;
}
