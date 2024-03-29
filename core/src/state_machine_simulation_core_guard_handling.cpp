/*
Copyright 2021 Tomas Prerovsky (cepsdev@hotmail.com).

Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include "core/include/state_machine_simulation_core.hpp"
#include "core/include/base_defs.hpp"
#include <optional>

extern void flatten_args(State_machine_simulation_core* smc,ceps::ast::Nodebase_ptr r, std::vector<ceps::ast::Nodebase_ptr>& v, char op_val = ',');

struct guard_reduce_result_t{
	bool value;
	bool is_symbol;
	std::string name,kind;
};

static std::optional<guard_reduce_result_t> reduce_expr_tree(ceps::ast::Nodebase_ptr root){
	using namespace ceps::ast;

	if(is<Ast_node_kind::int_literal>(root)) 
	 return { {0 != value(as_int_ref(root)), false} };
	else if(is<Ast_node_kind::float_literal>(root)) 
	 return { {std::abs(value(as_double_ref(root))) != 0.0, false } } ;
	else if(is<Ast_node_kind::string_literal>(root)) 
	 return { {value(as_string_ref(root)).length() > 0, false}};
	else if (is<Ast_node_kind::symbol>(root))
	 return {{false,true,name(as_symbol_ref(root)),kind(as_symbol_ref(root))}};
	else if (is_binop(root)) {
		auto lhs = reduce_expr_tree(as_binop_ref(root).left());
		auto rhs = reduce_expr_tree(as_binop_ref(root).right());
		if(!lhs || !rhs) return {};
		auto oper = op_val(as_binop_ref(root));
		if (oper == "==" || oper == "=" || oper == "&&" ){
			if (lhs->is_symbol && rhs->is_symbol){
				return{{ (lhs->name == rhs->name) && (lhs->kind == rhs->kind),false}};
			}
			else if (!lhs->is_symbol && !rhs->is_symbol){
				return {{lhs->value && rhs->value,false}};
			}
			else return {};			
		} else if (oper == "!="  ){
			if (lhs->is_symbol && rhs->is_symbol){
				return{{ !((lhs->name == rhs->name) && (lhs->kind == rhs->kind)),false}};
			}
			else if (!lhs->is_symbol && !rhs->is_symbol){
				return {{!(lhs->value && rhs->value),false}};
			}
			else return {};			
		} else if (oper == "||" ){
			if (!lhs->is_symbol && !rhs->is_symbol){
				return {{lhs->value || rhs->value,false}};
			}
			else return {};			
		}
	} 
	return {};
}

bool State_machine_simulation_core::eval_to_bool(ceps::ast::Nodebase_ptr p)
{
	using namespace ceps::ast;

	if(p == nullptr)
	{
		fatal_(-1,"Expression (null) has no interpretation as a boolean value.");
	}
	if(p->kind() == Ast_node_kind::int_literal) return 0 != ceps::ast::value(ceps::ast::as_int_ref(p));
	else if(p->kind() == Ast_node_kind::float_literal) return std::abs(ceps::ast::value(ceps::ast::as_double_ref(p))) != 0.0;
	else if(p->kind() == Ast_node_kind::string_literal) return ceps::ast::value(ceps::ast::as_string_ref(p)).length() > 0;
	else if (is_binop(p)) {
		auto r = reduce_expr_tree(p);
		if (r){
			auto rr = *r;
			if (!rr.is_symbol) return rr.value;
		} 
	} 

	std::stringstream ss;
	ss << *p;
	warn_(-1,"Expression '"+ss.str()+"' has no interpretation as a boolean value.");
	return false;
}


void print_set_str(std::set<std::string> const & v)
{
	std::cerr << "********\n"; for(auto const & s : v) std::cerr << s << "\n"; std::cerr << "********\n";
}

void State_machine_simulation_core::guards_in_expr(ceps::ast::Nodebase_ptr  expr, std::set<std::string> & v)
{
	if (expr->kind() == ceps::ast::Ast_node_kind::int_literal || expr->kind() == ceps::ast::Ast_node_kind::float_literal || expr->kind() == ceps::ast::Ast_node_kind::string_literal)
		return;

	if(expr->kind() == ceps::ast::Ast_node_kind::symbol && ceps::ast::kind(ceps::ast::as_symbol_ref(expr)) == "Guard")
	{
		if(v.find(ceps::ast::name(ceps::ast::as_symbol_ref(expr))) == v.end() )
		{
			std::string gn = ceps::ast::name(ceps::ast::as_symbol_ref(expr));
			v.insert(gn);
			auto pp = global_guards[gn];
			if(pp == nullptr) fatal_(-1, "Guard '"+gn+"' has no interpretation, i.e. is undefined.");
			guards_in_expr(pp,v);
		}
	}
	auto expr_p = ceps::ast::nlf_ptr(expr);
	for(auto p : expr_p->children())
		guards_in_expr(p,v);
}

bool State_machine_simulation_core::contains_sm_func_calls(ceps::ast::Nodebase_ptr  expr)
{
	if (expr->kind() == ceps::ast::Ast_node_kind::symbol || expr->kind() == ceps::ast::Ast_node_kind::int_literal || expr->kind() == ceps::ast::Ast_node_kind::float_literal || expr->kind() == ceps::ast::Ast_node_kind::string_literal)
		return false;

	if(expr->kind() == ceps::ast::Ast_node_kind::func_call) return true;
	auto expr_p = ceps::ast::nlf_ptr(expr);
	for(auto p : expr_p->children())
		if (contains_sm_func_calls(p)) return true;

	return false;
}

void acc_args(ceps::ast::Nodebase_ptr root,std::vector<ceps::ast::Nodebase_ptr > & args)
{
	if(root->kind() == ceps::ast::Ast_node_kind::binary_operator && op(as_binop_ref(root)) == ',')
	{
		acc_args(as_binop_ref(root).children()[0],args);
		acc_args(as_binop_ref(root).children()[1],args);
	}
	else args.push_back(root);
}

ceps::ast::Nodebase_ptr State_machine_simulation_core::unfold(ceps::ast::Nodebase_ptr expr,
							   std::map<std::string, ceps::ast::Nodebase_ptr>& guard_to_interpretation,
							   std::set<std::string>& path,
							   states_t const & current_states, executionloop_context_t* exec_ctxt)
{

	DEBUG_FUNC_PROLOGUE
	using namespace ceps::ast;

	ceps::ast::Nodebase_ptr nlf_base = nullptr;
	if (expr->kind() == Ast_node_kind::binary_operator)
	{
		nlf_base = ceps::interpreter::mk_bin_op(ceps::ast::op(ceps::ast::as_binop_ref(expr)));
	}
	else if (expr->kind() == Ast_node_kind::unary_operator)
	{
		nlf_base = ceps::interpreter::mk_unary_op(ceps::ast::op(*dynamic_cast<ceps::ast::Unary_operator*>(expr)), nullptr);
	}
	else if (expr->kind() == Ast_node_kind::symbol)
	{
		std::string n = name(as_symbol_ref(expr));


		if (kind(as_symbol_ref(expr)) == "Guard" )
		{

			if (path.find(n) != path.end()) fatal_(-1,"Cyclic dependence of guards detected.");
			auto it = guard_to_interpretation.find(n);
			if (it != guard_to_interpretation.end()) return it->second;

			path.insert(n);
			auto r = guard_to_interpretation[n] = unfold(global_guards[n],guard_to_interpretation,path,current_states,exec_ctxt);
			path.erase(n);
			return r;
		} else return expr;
	}
	else return expr;

	for(auto p : ceps::ast::nlf_ptr(expr)->children())
	{
		ceps::ast::nlf_ptr(nlf_base)->children().push_back( unfold(p,guard_to_interpretation,path,current_states,exec_ctxt));
	}
	return nlf_base;
}

bool contains_compund_states(ceps::ast::Nodebase_ptr expr, bool inside_dot_expr)
{
	if (expr == nullptr) return false;

	if (expr->kind() == ceps::ast::Ast_node_kind::int_literal) return false;
	if (expr->kind() == ceps::ast::Ast_node_kind::float_literal) return false;
	if (expr->kind() == ceps::ast::Ast_node_kind::string_literal) return false;

	auto p = ceps::ast::nlf_ptr(expr);

	if (expr->kind() == ceps::ast::Ast_node_kind::binary_operator && ('.' == ceps::ast::op(ceps::ast::as_binop_ref(expr))))
		inside_dot_expr = true;

	if (expr->kind() == ceps::ast::Ast_node_kind::symbol) return inside_dot_expr;

	for(auto t : p->children())
	{
		bool r = contains_compund_states(t,inside_dot_expr);
		if (r) return r;
	}
	return false;
}

ceps::ast::Nodebase_ptr eval_locked_ceps_expr(State_machine_simulation_core* smc,
										 State_machine* containing_smp,
										 ceps::ast::Nodebase_ptr node,
										 ceps::ast::Nodebase_ptr root_node);


bool State_machine_simulation_core::eval_guard(ceps::Ceps_Environment& ceps_env,
		                                       std::string const & guard_name,
											   states_t const & states, executionloop_context_t* exec_ctxt)
{
	using namespace ceps::ast;

    bool bool_result;
	{
    	auto it =  get_user_supplied_guards().find(guard_name);
    	if (it !=  get_user_supplied_guards().end()){
    		return (*it->second)();
    	}
	}

	Nodebase_ptr guard_expr = global_guards[guard_name];
	if (guard_expr == nullptr)
		fatal_(-1,"Global guard '"+guard_name+"' has no interpretation, i.e. is not defined");

	std::set<std::string> guards_in_rhs;
	guards_in_expr(guard_expr,  guards_in_rhs);
	ceps::ast::Nodebase_ptr  result = nullptr;
	

	if (guards_in_rhs.size() || contains_sm_func_calls(guard_expr) || contains_compund_states(guard_expr,false) ){
		if (guards_in_rhs.find(guard_name) != guards_in_rhs.end())
			fatal_(-1,"Guard '"+guard_name+ "': Cyclic definition.");

		decltype(global_guards) guard_to_interpretation;
		std::set<std::string> eval_path {guard_name};
		std::lock_guard<std::recursive_mutex>g(states_mutex());
		auto guard_unfolded = unfold(guard_expr,guard_to_interpretation,eval_path,states,exec_ctxt);
		result = eval_locked_ceps_expr(this,
									   nullptr,
									   guard_unfolded,
									   nullptr);
	} else	{
		result = eval_locked_ceps_expr(this,
									   nullptr,
									   guard_expr,
									   nullptr);
	}
	if(result != nullptr)
		bool_result = eval_to_bool(result);
	else bool_result = false;

	return bool_result;
}

