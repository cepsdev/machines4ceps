/*
Copyright 2024 Tomas Prerovsky (cepsdev@hotmail.com).

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

#define ast_proc_prolog  \
    using namespace ceps::ast;\
    using namespace ceps::vm::oblectamenta;\
    using namespace ceps::interpreter;\
    using namespace std;


class CepsComputeGraphNotationTraverser{
    
        std::vector<ceps::ast::node_t> v; 
    public:
        struct expr;
        struct array_ref{
            ceps::ast::node_t node{};
            std::string id;
            int64_t idx{};
            uint8_t width{8};
            array_ref() = default;
            array_ref(std::string id, int64_t idx):id{id}, idx{idx} {
                ast_proc_prolog
                node = mk_func_call(ceps::ast::mk_symbol(id,"OblectamentaDataLabel"),mk_int_node(idx) );
            } 
            expr as_expr();
        };

        struct expr;

        struct op_expr{
            ceps::ast::node_t root;
            std::string op;
            expr lhs();
            expr rhs();
        };

        struct func_id{
            std::string name;
            std::optional<std::string> sym_kind;
        };
        
        struct noary_or_unary_funccall_expr{
            ceps::ast::node_t root;
            func_id fid;
            ceps::ast::node_t  arg;
            noary_or_unary_funccall_expr() = default;
            noary_or_unary_funccall_expr(ceps::ast::node_t root, func_id fid, ceps::ast::node_t  arg)
            : root{root}, fid{fid}, arg{arg}{}
            expr argument();
        };

        struct funccall_expr{
            ceps::ast::node_t root;
            func_id fid;
            std::vector<ceps::ast::node_t>  args;
            funccall_expr() = default;
            funccall_expr(ceps::ast::node_t root, func_id fid, std::vector<ceps::ast::node_t>  args)
            : root{root}, fid{fid}, args{args}{}
            std::vector<expr> arguments();
        };

        struct expr{
            ceps::ast::node_t root;

            std::optional<int> as_int(){
                using namespace std;
                using namespace ceps::ast;
                if(!is<ceps::ast::Ast_node_kind::int_literal>(root)) return {};
                return value(as_int_ref(root));
            }

            std::optional<double> as_double(){
                using namespace std;
                using namespace ceps::ast;
                if(!is<ceps::ast::Ast_node_kind::float_literal>(root)) return {};
                return value(as_double_ref(root));
            }

            std::optional<std::string> as_id(){
                using namespace std;
                using namespace ceps::ast;
                if(!is<ceps::ast::Ast_node_kind::identifier>(root)) return {};
                return name(as_id_ref(root));
            }

            std::optional<std::pair<std::string,std::string>> as_symbol(){
                using namespace std;
                using namespace ceps::ast;
                if(!is<ceps::ast::Ast_node_kind::symbol>(root)) return {};
                return { make_pair(name(as_symbol_ref(root)), kind(as_symbol_ref(root))) };
            }

            std::optional<noary_or_unary_funccall_expr> as_noary_or_unary_funccall(){
                using namespace std;
                using namespace ceps::ast;
                if(!is<ceps::ast::Ast_node_kind::func_call>(root)) return {};
				
                string func_id;
				string fkind; 
				string sym_name;
				node_t ftarget; 
				vector<node_t> args;
                if (args.size() > 1) return {};

                is_a_funccall(	root, func_id, fkind, sym_name, ftarget, args);
                return noary_or_unary_funccall_expr( root, 
                                               { (fkind == "" ? func_id : sym_name), fkind == "" ? optional<string>{} : optional<string>{fkind}  },
                                               args.size() == 0 ? nullptr : args[0]);

            }
            
            std::optional<funccall_expr> as_funccall(){
                using namespace std;
                using namespace ceps::ast;
                if(!is<ceps::ast::Ast_node_kind::func_call>(root)) return {};
				
                string func_id;
				string fkind; 
				string sym_name;
				node_t ftarget; 
				vector<node_t> args;

                is_a_funccall(	root, func_id, fkind, sym_name, ftarget, args);
                return funccall_expr( root, 
                                               { (fkind == "" ? func_id : sym_name), fkind == "" ? optional<string>{} : optional<string>{fkind}  },
                                               args);

            }

            std::optional<array_ref> as_array_ref () {
                using namespace std;
                using namespace ceps::ast;

                string sym_name;
	            string sym_kind;
	            vector<node_t> args;
                if (is_a_symbol_with_arguments(root, sym_name, sym_kind, args)){
                    if (sym_kind == "OblectamentaDataLabel" && args.size() == 1 && is<Ast_node_kind::int_literal>(args[0])){
                        return array_ref{sym_name,value(as_int_ref(args[0])) };                        
                    }                 
                }
                return {};
            }
            std::optional<op_expr> as_binary_operation() const{
                using namespace ceps::ast;
                if(is<Ast_node_kind::binary_operator>(root)){
                    return op_expr{root, op_val(as_binop_ref(root)) };
                }
                return {};
            }

            bool is_leaf() const{
                using namespace std;
                using namespace ceps::ast;
                return is<Ast_node_kind::int_literal>(root) || is<Ast_node_kind::float_literal>(root) || is<Ast_node_kind::string_literal>(root)
                       || is<Ast_node_kind::identifier>(root) || is<Ast_node_kind::uint8>(root) || is<Ast_node_kind::long_literal>(root);
            }
            size_t size() const{
                if (is_leaf()) return 0;
                return children(*nlf_ptr(root)).size();
            }
            expr operator[](size_t i){
                return expr{children(*nlf_ptr(root))[i]};
            } 

            static expr mk_assignment(expr lhs, expr rhs);
            static expr mk_addition(expr lhs, expr rhs);
            static expr mk_subtraction(expr lhs, expr rhs);
            static expr mk_multiplication(expr lhs, expr rhs);
            static expr mk_double(double);
        };
        struct stmt{
            ceps::ast::node_t node;
            std::optional<op_expr> is_assign_op() const {
                using namespace ceps::ast; 
                if (!is<Ast_node_kind::binary_operator>(node)) return {};
                if("=" == op_val(as_binop_ref(node))) 
                 return op_expr{node};
                return {};
            }
            expr as_expr(){return expr{node};}

        };
        CepsComputeGraphNotationTraverser(std::vector<ceps::ast::node_t> v):v{v} {}
        CepsComputeGraphNotationTraverser() = default;
        stmt operator[](size_t i){
            return {v[i]};            
        }
        size_t size() const {return v.size();}
        void push_back(stmt s) {v.push_back(s.node);}
        void push_back(expr e) {v.push_back(e.root);}
        std::vector<ceps::ast::node_t> nodes() const {return v;}
};

CepsComputeGraphNotationTraverser::expr CepsComputeGraphNotationTraverser::noary_or_unary_funccall_expr::argument(){
    return expr{arg};
}

std::vector<CepsComputeGraphNotationTraverser::expr> CepsComputeGraphNotationTraverser::funccall_expr::arguments(){
    std::vector<CepsComputeGraphNotationTraverser::expr> r;
    for(auto e: args) r.push_back(expr{e});
    return r;
}

CepsComputeGraphNotationTraverser::expr 
 CepsComputeGraphNotationTraverser::op_expr::lhs() { return { ceps::ast::children(ceps::ast::as_binop_ref(root))[0]} ;}
CepsComputeGraphNotationTraverser::expr 
 CepsComputeGraphNotationTraverser::op_expr::rhs() { return { ceps::ast::children(ceps::ast::as_binop_ref(root))[1]} ;}

CepsComputeGraphNotationTraverser::expr CepsComputeGraphNotationTraverser::array_ref::as_expr(){return expr{node};}

CepsComputeGraphNotationTraverser::expr CepsComputeGraphNotationTraverser::expr::mk_assignment(CepsComputeGraphNotationTraverser::expr lhs, CepsComputeGraphNotationTraverser::expr rhs){
 expr e;
 e.root = mk_binary_op("=",lhs.root, rhs.root);
 return e;
}
CepsComputeGraphNotationTraverser::expr CepsComputeGraphNotationTraverser::expr::mk_addition(CepsComputeGraphNotationTraverser::expr lhs, CepsComputeGraphNotationTraverser::expr rhs){
 expr e;
 e.root = mk_binary_op("+",lhs.root, rhs.root);
 return e;
}
CepsComputeGraphNotationTraverser::expr CepsComputeGraphNotationTraverser::expr::mk_subtraction(CepsComputeGraphNotationTraverser::expr lhs, CepsComputeGraphNotationTraverser::expr rhs){
 expr e;
 e.root = mk_binary_op("-",lhs.root, rhs.root);
 return e;
}
CepsComputeGraphNotationTraverser::expr CepsComputeGraphNotationTraverser::expr::mk_multiplication(CepsComputeGraphNotationTraverser::expr lhs, CepsComputeGraphNotationTraverser::expr rhs){
 expr e;
 e.root = mk_binary_op("*",lhs.root, rhs.root);
 return e;
}
CepsComputeGraphNotationTraverser::expr CepsComputeGraphNotationTraverser::expr::mk_double(double value){
 expr e;
 e.root = ceps::interpreter::mk_double_node(value, ceps::ast::Unit_rep{});
 return e;
}






