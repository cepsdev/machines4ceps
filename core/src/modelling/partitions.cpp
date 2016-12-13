#include "core/include/state_machine_simulation_core.hpp"
#include "core/include/modelling/partitions.hpp"
#include "core/include/modelling/gensm.hpp"

ceps::ast::Nodeset sm4ceps::modelling::standard_value_partition_sm(ceps::ast::Struct_ptr what,ceps::ast::Nodebase_ptr root,
 				ceps::parser_env::Symbol* sym,ceps::parser_env::Symboltable & symtab,ceps::interpreter::Environment& env,
  				ceps::ast::Nodebase_ptr ,ceps::ast::Nodebase_ptr )
{
 using namespace sm4ceps::modelling::gensm;
 using namespace ceps::ast;
 std::string part_id;
 for(auto e: what->children()){
	 if ( e->kind() == Ast_node_kind::binary_operator && op(as_binop_ref(e)) == '='  &&
		  (as_binop_ref(e).left()->kind() == Ast_node_kind::identifier && name(as_id_ref(as_binop_ref(e).left()))=="id" ) ) {
		 part_id = name(as_id_ref(as_binop_ref(e).right()));
		 break;
	 }
 }

 auto gen_sm = sm("mod_gen_stdvpart_"+part_id);
 int range_counter = 0;
 std::map<std::string,ceps::ast::Nodebase_ptr> range2guard;
 std::map<ceps::ast::Struct_ptr,ceps::ast::Nodebase_ptr> range_sm2guard;

 for(auto& e : what->children()){
  if (e->kind() == Ast_node_kind::scope){
   ++range_counter;
   auto& range = as_scope_ref(e).children();
   if(range.size() == 1){
	   range2guard["set_"+std::to_string(range_counter)] = range[0];
   } else if (range.size() > 1){
	 auto first_entry = range[0];
	 auto second_entry = range[1];
     if (second_entry->kind() == Ast_node_kind::identifier){
    	 range2guard[name(as_id_ref(second_entry))] = first_entry;
     } else if (second_entry->kind() == ceps::ast::Ast_node_kind::structdef && "Statemachine" == name(as_struct_ref(second_entry))){
    	 range_sm2guard[as_struct_ptr(second_entry)] = first_entry;
     }
   }
  }
 }

 int guard_ctr = 0;
 for(auto& e : range2guard){
	 gen_sm.add_state(e.first);
 }

 for(auto& from : range2guard){
	 for(auto& to : range2guard){
		 if (from.first == to.first) continue;
		 gen_sm.add_transition(from.first,to.first,to.second);
	 }
 }

 Nodeset result;
 result.nodes().push_back(gen_sm.ns().nodes()[0]);
 //result.nodes().insert(result.nodes().begin(),gen_sm.ns().nodes().begin(),gen_sm.ns().nodes().end());
 return result;
}
