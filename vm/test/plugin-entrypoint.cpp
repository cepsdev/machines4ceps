
#include <stdlib.h>
#include <iostream>
#include <ctype.h>
#include <chrono>
#include <sstream>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <queue>
#include <unordered_map>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <stdlib.h>
#include <map>
#include <algorithm>
#include <future>
#include <memory>
#include <vector>
#include <string>
#include <optional>
#include <stdexcept>

#include "ceps_ast.hh"
#include "core/include/state_machine_simulation_core.hpp"

#include "core/include/vm/vm_base.hpp"
#include "core/include/vm/oblectamenta-assembler.hpp"
#include "core/include/vm/oblectamenta-comp-graph.hpp"
#include "core/include/differentiable_programming/graph_notation_traverser.hpp"


namespace cepsplugin{
    static Ism4ceps_plugin_interface* plugin_master = nullptr;
    static const std::string version_info = "INSERT_NAME_HERE v0.1";
    static constexpr bool print_debug_info{true};
    ceps::ast::node_t operation(ceps::ast::node_callparameters_t params);
    ceps::ast::node_t obj(ceps::ast::node_callparameters_t params);
}

/**
 * @brief ceps entry point for the creation of Oblectamenta VM's objects.
 * 
 * @param params 
 * @return ceps::ast::node_t 
 */
ceps::ast::node_t cepsplugin::obj(ceps::ast::node_callparameters_t params){
    OBLECTAMENTA_AST_PROC_PROLOGUE
    using namespace ceps::vm::oblectamenta::ast;
    
    
    static auto fns{ map<string, node_t (*)(Struct&)>{
        {"vm", [] (Struct& s) -> node_t { 
                if(!children(s).size()) return ast_rep(VMEnv{}); 
                auto vm_opt{read_value<VMEnv>(s)};
                if (vm_opt) return ast_rep(*vm_opt);
                return mk_undef();
               } 
        },
        {   "stack", 
            [] (Struct& s) -> node_t 
                    { 
                        VMEnv vm;
                        auto v{read_value<ser_wrapper_stack>(s)}; 
                        if (!v) return mk_undef(); 
                        return ast_rep(*v,*(*v).vm); 
                    } 
        },
        {"data", [] (Struct& s) -> node_t { VMEnv vm; auto v{read_value<ser_wrapper_data>(s)}; if (!v) return mk_undef(); return ast_rep(*v,*(*v).vm); } },
        {"text", [] (Struct& s) -> node_t { VMEnv vm; auto v{read_value<ser_wrapper_text>(s)}; if (!v) return mk_undef(); return ast_rep(*v,*(*v).vm); } },
        {"compute_stack", [] (Struct& s) -> node_t { VMEnv vm; auto v{read_value<ser_wrapper_cstack>(s)}; if (!v) return mk_undef(); return ast_rep(*v,*(*v).vm); } },
        {"registers", [] (Struct& s) -> node_t { auto v{read_value<VMEnv::registers_t>(s)}; if (!v) return mk_undef(); return ast_rep(*v); } }
    }};
    
    auto data{get_first_child(params)};
    if (!is<Ast_node_kind::structdef>(data))
     return mk_undef();

    auto& ceps_struct = *as_struct_ptr(data);
    auto obj_name{name(ceps_struct)};
    auto factory_it{fns.find(obj_name)};
    if (factory_it == fns.end()) return mk_undef();
    return factory_it->second(ceps_struct);
}

template <typename F, typename E> 
 void traverse(F f, E e){
    if (f(e)){
        if (e.is_leaf()) return;
        for(size_t i{}; i < e.size(); ++i)
         traverse(f, e[i]);
    }
}


class CepsOblectamentaMnemonicsEmitter{
        std::vector<ceps::ast::node_t> v; 
    public:
        void emitStoreDouble(size_t location){
            v.push_back(
                mk_func_call(
                    ceps::interpreter::mk_symbol("stdbl","OblectamentaOpcode"), 
                    ceps::interpreter::mk_int_node((int)location)
                )            
            );                        
        }
        void emitLoadDouble(size_t location){
            v.push_back(
                mk_func_call(
                    ceps::interpreter::mk_symbol("lddbl","OblectamentaOpcode"), 
                    ceps::interpreter::mk_int_node((int)location)
                )            
            );                        
        }
        void emitMulDouble(){
            v.push_back(ceps::interpreter::mk_symbol("muldbl","OblectamentaOpcode"));
        }
        void emitSubDouble(){
            v.push_back(ceps::interpreter::mk_symbol("subdbl","OblectamentaOpcode"));
        }
        void emitAddDouble(){
            v.push_back(ceps::interpreter::mk_symbol("adddbl","OblectamentaOpcode"));
        }
        void emitDivDouble(){
            v.push_back(ceps::interpreter::mk_symbol("divdbl","OblectamentaOpcode"));
        }
        void emitSinDouble(){
            v.push_back(ceps::interpreter::mk_symbol("sindbl","OblectamentaOpcode"));
        }
        void emitCosDouble(){
            v.push_back(ceps::interpreter::mk_symbol("cosdbl","OblectamentaOpcode"));
        }
        void emitTanDouble(){
            v.push_back(ceps::interpreter::mk_symbol("tandbl","OblectamentaOpcode"));
        }
        void emitAtanDouble(){
            v.push_back(ceps::interpreter::mk_symbol("atandbl","OblectamentaOpcode"));
        }
        void emitExpDouble(){
            v.push_back(ceps::interpreter::mk_symbol("expdbl","OblectamentaOpcode"));
        }
        void emitTanhDouble(){
            v.push_back(ceps::interpreter::mk_symbol("tanhdbl","OblectamentaOpcode"));
        }


        std::vector<ceps::ast::node_t> listing() const { return v;}
};

template<>
    void ceps::vm::oblectamenta::ComputationGraph<CepsComputeGraphNotationTraverser,CepsOblectamentaMnemonicsEmitter>::compile( CepsComputeGraphNotationTraverser& graph_def,
    CepsOblectamentaMnemonicsEmitter& emitter){
        using namespace std;
        map<string,pair<size_t,int64_t>> addrs;
        map<string,size_t> predef_array_offs;
        

        auto get_addr = [&] (CepsComputeGraphNotationTraverser::expr e) {
            auto array_ref{e.as_array_ref()};
            if (!array_ref) return size_t{};
            auto addr_info{addrs[array_ref->id]}; 
            size_t addr{addr_info.first + array_ref->idx * array_ref->width};
            return addr;
        };
        //First step compute array sizes and addresses

        for(size_t i{0}; i < graph_def.size(); ++i){
            auto stmt{graph_def[i]};
            auto assign_expr{stmt.is_assign_op()};
            if (!assign_expr){
                auto fcall {stmt.as_expr().as_funccall()};
                if( fcall && "array" == fcall->fid.name){
                    //handle declarations
                    auto args{fcall->arguments()};
                    if (args.size() == 3){
                        auto sym{args[0].as_symbol()};
                        auto id{args[1].as_id()};
                        auto mod{args[2].as_funccall()};
                        if (sym && id  && mod && mod->fid.name == "base" && mod->arguments().size() == 1){
                            auto base_addr{mod->arguments()[0].as_int()};
                            if (base_addr) {
                                predef_array_offs[ sym->first ] = *base_addr;
                            }                    
                        }
                    }                    
                }
            }
        }
        for(size_t i{0}; i < graph_def.size(); ++i){
            auto stmt{graph_def[i]};
            auto assign_expr{stmt.is_assign_op()};
            if (!assign_expr) continue;

            auto left_side{assign_expr->lhs()};
            auto right_side{assign_expr->rhs()};
            auto array_ref{left_side.as_array_ref()};
            const auto width = 8;// assume doubles
            if (array_ref){
                auto ar{*array_ref};
                if ( addrs[ar.id].second < width * (ar.idx + 1)) 
                 addrs[ar.id].second = width * (ar.idx + 1);
            }
            traverse(
                [&](decltype(right_side) e){ 
                    auto array_ref{e.as_array_ref()}; 
                    if (array_ref){
                        auto ar{*array_ref};
                        if ( addrs[ar.id].second < width * (ar.idx + 1)) 
                            addrs[ar.id].second = width * (ar.idx + 1);
                        return false;
                    } 
                    return true;
                }, right_side);
        }
        size_t cur_mem_addr{};

        for (auto& e: addrs)
        {
            auto it{predef_array_offs.find(e.first)};
            if (it == predef_array_offs.end()) continue;
            e.second.first = it->second;

            auto t{e.second.first + e.second.second};

            if (t > cur_mem_addr) cur_mem_addr = t;
        }

        for (auto& e: addrs)
        {
            auto it{predef_array_offs.find(e.first)};
            if (it != predef_array_offs.end()) continue;

            e.second.first = cur_mem_addr;
            cur_mem_addr += e.second.second;             
        }

        /*for (auto& e: addrs)
        {

            cout << e.first << " " << e.second.first << " " << e.second.second << '\n';
        }
        exit(0);*/

        //second step: generate code

        for(size_t i{0}; i < graph_def.size(); ++i){
            auto stmt{graph_def[i]};
            auto assign_expr{stmt.is_assign_op()};
            if (!assign_expr) continue;
            auto right_side{assign_expr->rhs()};
            //Emit code for right hand side
            function<bool(CepsComputeGraphNotationTraverser::expr)> parse_fn;
            parse_fn = [&](CepsComputeGraphNotationTraverser::expr e){
                    auto aref{e.as_array_ref()};
                    auto bop{e.as_binary_operation()};
                    if (aref){
                        size_t addr  = get_addr(e);
                        emitter.emitLoadDouble(addr);
                        return false;
                    } else if (bop){
                        traverse(parse_fn, bop->rhs());
                        traverse(parse_fn, bop->lhs());
                        if (bop->op == "*"){
                            emitter.emitMulDouble();
                        } else  if (bop->op == "-"){
                            emitter.emitSubDouble();
                        } else  if (bop->op == "+"){
                            emitter.emitAddDouble();
                        } else if (bop->op == "/"){
                            emitter.emitDivDouble();
                        } 
                        return false;
                    }
                    auto simple_funccall{e.as_noary_or_unary_funccall()};
                    if (simple_funccall && simple_funccall->arg ){
                        traverse(parse_fn,simple_funccall->argument());
                        if (simple_funccall->fid.name == "sin")
                         emitter.emitSinDouble();
                        else if (simple_funccall->fid.name == "cos")
                         emitter.emitCosDouble();
                        else if (simple_funccall->fid.name == "tan")
                         emitter.emitTanDouble();
                        else if (simple_funccall->fid.name == "atan")
                         emitter.emitAtanDouble();
                        else if (simple_funccall->fid.name == "exp")
                         emitter.emitExpDouble();
                        else if (simple_funccall->fid.name == "tanh")
                         emitter.emitTanhDouble();
                        return false;

                    }
                    return true;
                };
            traverse(
                parse_fn,
                right_side
            );

            //Emit store
            auto left_side{assign_expr->lhs()};
            size_t addr  = get_addr(left_side);                        
            emitter.emitStoreDouble(addr);
        }       
    }

// Documentation helper
template <typename T> inline bool Invariant(T e){ return true;}
template <typename T> inline bool Comment(T e){ return true;}
template <typename T1, typename T2> inline bool BookReference(T1 , T2 ){ return true;}

template<>
 template<>
    void ceps::vm::oblectamenta::ComputationGraph<CepsComputeGraphNotationTraverser,
                                                  CepsOblectamentaMnemonicsEmitter>::
                                                  tangent_forward_diff<CepsComputeGraphNotationTraverser>( 
                                                     CepsComputeGraphNotationTraverser& graph_def,
                                                     CepsComputeGraphNotationTraverser& writer,
                                                     string weight_id)
{
    using Traverser = CepsComputeGraphNotationTraverser;

    auto  dot = [&](CepsComputeGraphNotationTraverser::array_ref e) {
        return CepsComputeGraphNotationTraverser::array_ref{e.id+"_dot", e.idx}.as_expr();
    };

    function<CepsComputeGraphNotationTraverser::expr(CepsComputeGraphNotationTraverser::expr)> diff;
    diff = [&](CepsComputeGraphNotationTraverser::expr e) {
                    auto aref{e.as_array_ref()};
                    if (aref)
                        return dot(*aref);
                    else if(auto abinary_op{e.as_binary_operation()};abinary_op){
                        auto binary_op{*abinary_op};
                        if (binary_op.op == "*"){
                            BookReference("Griewank,Walther:EvDev:","p.35, Table 3.3, 3rd row");
                            auto lhs_diff{diff(binary_op.lhs())};
                            auto rhs_diff{diff(binary_op.rhs())};
                            return Traverser::expr::mk_addition(
                                Traverser::expr::mk_multiplication(lhs_diff,binary_op.rhs()),
                                Traverser::expr::mk_multiplication(binary_op.lhs(),rhs_diff)
                            );
                        }                        
                    }
                    return e;
    };

    for(size_t i{}; i < graph_def.size(); ++i){
        auto stmt{graph_def[i]};
        auto assign_expr{stmt.is_assign_op()};
        if (!assign_expr) { 
            writer.push_back(graph_def[i]); continue;}
        Invariant("stmt is an assignment. i.e. it contains a left and a right side.");
        auto right_side{assign_expr->rhs()};
        auto left_side{assign_expr->lhs()};
        
        auto diff_rhs{diff(right_side)};
        auto diff_lhs{diff(left_side)};
        writer.push_back(graph_def[i]);
        writer.push_back(Traverser::expr::mk_assignment(diff_lhs,diff_rhs));
    }
}


ceps::ast::node_t handle_operation_tangent_forward_diff(ceps::ast::Struct& ceps_struct){
    OBLECTAMENTA_AST_PROC_PROLOGUE
    node_struct_t computation_graph{};
    auto result = mk_struct("computation_graph");
    vector<CepsComputeGraphNotationTraverser::array_ref> vars;

    for(auto e : children(ceps_struct))
        if (is<Ast_node_kind::structdef>(e) && name(as_struct_ref(e)) == "differentiable_program" ){
            auto& diff_struct = as_struct_ref(e);
            for(auto e : children(diff_struct))
                if (is<Ast_node_kind::structdef>(e) && name(as_struct_ref(e)) == "computation_graph" )
                    computation_graph = as_struct_ptr(e);
        } else {
            CepsComputeGraphNotationTraverser::expr expr{e};
            auto opt_array_ref{expr.as_array_ref()};
            if (opt_array_ref)
                vars.push_back(*opt_array_ref);
        }

    if (!computation_graph || vars.size() != 1 ) return mk_undef();
    CepsComputeGraphNotationTraverser traverser{children(*computation_graph)};
    CepsComputeGraphNotationTraverser writer;
    //ComputationGraph<CepsComputeGraphNotationTraverser,CepsOblectamentaMnemonicsEmitter> comp_graph;
    //comp_graph.compile(traverser,emitter); 
    ComputationGraph<CepsComputeGraphNotationTraverser,CepsOblectamentaMnemonicsEmitter> comp_graph;
    comp_graph.tangent_forward_diff<CepsComputeGraphNotationTraverser>(
        traverser,writer, {} );
    children(*result) = writer.nodes();
    return result;
}
/**
Backpropagation

Input: Input vector x_n
       Network parameter w
       Error function E_n(w) for input x_n
       Activation function h(a)
Output:
       Error function derivatives \{\partial E_n / \partial w_{ji}\}
//Forward propagation
for j \in all hidden and output units do
 a_j \leftarrow \sum_i w_{ji}z_i
 z_j \leftarrow h(a_j)
end for
//Error evaluation
for k \in all output units do
 \delta_k = \frac{\partial E_n}{\partial a_k}
end for 
*/
template<typename F>
 void for_all_leaves(CepsComputeGraphNotationTraverser::expr e, F f) {
    auto aref{e.as_array_ref()};
    if (aref) {f(*aref); return;}        
    if(auto abinary_op{e.as_binary_operation()};abinary_op){
        for_all_leaves(abinary_op->lhs(),f);
        for_all_leaves(abinary_op->rhs(),f);
    } else if (auto afunc_call{e.as_noary_or_unary_funccall()}; afunc_call) {
        for_all_leaves(afunc_call->argument(),f);          
    }
}

template<>
 template<>
    void ceps::vm::oblectamenta::ComputationGraph<CepsComputeGraphNotationTraverser,
                                                  CepsOblectamentaMnemonicsEmitter>::
                                                  backprop<CepsComputeGraphNotationTraverser>( 
                                                     CepsComputeGraphNotationTraverser& graph_def,
                                                     CepsComputeGraphNotationTraverser& writer,
                                                     string weight_id)
{   
    using Traverser = CepsComputeGraphNotationTraverser;
    auto  dot = [&](CepsComputeGraphNotationTraverser::array_ref e) {
        return CepsComputeGraphNotationTraverser::array_ref{e.id+"_dot", e.idx}.as_expr();
    };



    function<CepsComputeGraphNotationTraverser::expr(CepsComputeGraphNotationTraverser::expr, Traverser::array_ref, set<string> const &)> diff;
    diff = [&](CepsComputeGraphNotationTraverser::expr e, Traverser::array_ref x, set<string> const & constants) {
        auto aref{e.as_array_ref()};
        if (aref){
            if (constants.find(aref->id) != constants.end()) 
             return Traverser::expr::mk_double(0.0);
            else if (x.id == aref->id && x.idx == aref->idx)
             return Traverser::expr::mk_double(1.0);
            else
             return Traverser::expr::mk_double(0.0);
        } else if(auto abinary_op{e.as_binary_operation()};abinary_op){
            auto binary_op{*abinary_op};
            if (binary_op.op == "*"){
                auto lhs_diff{diff(binary_op.lhs(), x, constants)};
                auto rhs_diff{diff(binary_op.rhs(), x, constants)};
                auto l{Traverser::expr::mk_multiplication(lhs_diff,binary_op.rhs())};
                auto r{Traverser::expr::mk_multiplication(binary_op.lhs(),rhs_diff)};

                if (auto a_double{lhs_diff.as_double()}; a_double) 
                    if (*a_double == 0.0) l = Traverser::expr::mk_double(0.0);
                    else if (*a_double == 1.0) l = binary_op.rhs();

                if (auto a_double{binary_op.rhs().as_double()}; a_double) 
                    if (*a_double == 0.0) l = Traverser::expr::mk_double(0.0);

                if (auto a_double{rhs_diff.as_double()}; a_double) 
                    if (*a_double == 0.0) r = Traverser::expr::mk_double(0.0);
                    else if (*a_double == 1.0) r = binary_op.lhs();

                if (auto a_double{binary_op.lhs().as_double()}; a_double) 
                    if (*a_double == 0.0) r = Traverser::expr::mk_double(0.0);

                if (auto l_double{l.as_double()}; l_double)
                 if (auto r_double{r.as_double()}; r_double)
                  return Traverser::expr::mk_double(*l_double + *r_double); 

                if (auto l_double{l.as_double()}; l_double)
                 if (*l_double == 0.0) return r;
                if (auto r_double{r.as_double()}; r_double)
                 if (*r_double == 0.0) return l;


                return Traverser::expr::mk_addition(l,r);
            } else if (binary_op.op == "+"){
                auto lhs_diff{diff(binary_op.lhs(), x, constants)};
                auto rhs_diff{diff(binary_op.rhs(), x, constants)};
                if (auto a_double{lhs_diff.as_double()}; a_double) 
                    if (*a_double == 0.0) return rhs_diff;
                if (auto a_double{rhs_diff.as_double()}; a_double) 
                    if (*a_double == 0.0) return lhs_diff;
                if (auto l_double{lhs_diff.as_double()}; l_double)
                 if (auto r_double{rhs_diff.as_double()}; r_double)
                  return Traverser::expr::mk_double(*l_double + *r_double); 
                return Traverser::expr::mk_addition(
                    lhs_diff,
                    rhs_diff);
            } else if (binary_op.op == "-"){
                auto lhs_diff{diff(binary_op.lhs(), x, constants)};
                auto rhs_diff{diff(binary_op.rhs(), x, constants)};
                if (auto a_double{lhs_diff.as_double()}; a_double) 
                    if (*a_double == 0.0) return rhs_diff;
                if (auto a_double{rhs_diff.as_double()}; a_double) 
                    if (*a_double == 0.0) return lhs_diff;
                if (auto l_double{lhs_diff.as_double()}; l_double)
                 if (auto r_double{rhs_diff.as_double()}; r_double)
                  return Traverser::expr::mk_double(*l_double - *r_double); 
                return Traverser::expr::mk_subtraction(
                    lhs_diff,
                    rhs_diff);
            }
        }
        return e;
    };
    //Determine constants

    set<string> constants;
    set<string> occurs_in_lhs_of_an_equation;
    set<string> occurs_in_rhs_of_an_equation;
    
    for(size_t i{}; i < graph_def.size(); ++i){
        auto stmt{graph_def[i]};
        auto assign_expr{stmt.is_assign_op()};
        if (!assign_expr) continue;
        for_all_leaves(assign_expr->lhs(),[&occurs_in_lhs_of_an_equation](Traverser::array_ref leaf){
            occurs_in_lhs_of_an_equation.insert(leaf.id);
        });
    }
    int nunber_of_ignored_stamts{};
    for(size_t i{}; i < graph_def.size(); ++i){
        auto stmt{graph_def[i]};
        auto assign_expr{stmt.is_assign_op()};
        if (!assign_expr) {++nunber_of_ignored_stamts; continue;}
        for_all_leaves(assign_expr->rhs(),[&](Traverser::array_ref leaf){
            occurs_in_rhs_of_an_equation.insert(leaf.id);
            if (weight_id != leaf.id &&  occurs_in_lhs_of_an_equation.find(leaf.id) ==occurs_in_lhs_of_an_equation.end() )
                constants.insert(leaf.id);
        });
    }

    // Debug output : constant / nonconstant information
    //for(auto t: constants) {cerr << t << "(constant)\n";}
    //for(auto t: occurs_in_lhs_of_an_equation) {cerr << t << '\n';}
    //for(auto t: occurs_in_rhs_of_an_equation) {cerr << t << '\n';}
    
    
    // Determine error function layer, i.e. find the assignments l = r  
    // such that there is no assignment of the form l'= f(l).
    vector<int> error_function_layer;

    for(size_t i{}; i < graph_def.size(); ++i){
        auto stmt{graph_def[i]};
        auto assign_expr{stmt.is_assign_op()};
        if (!assign_expr) continue;
        auto aref{assign_expr->lhs().as_array_ref()};
        if (!aref) continue;
        if (occurs_in_rhs_of_an_equation.find(aref->id) == occurs_in_rhs_of_an_equation.end())
         error_function_layer.push_back(i);
    }
    //for (auto e : error_function_layer) cerr << *graph_def[e].node << '\n'<< '\n'<< '\n';

    vector<int> y_ks; // the line numbers of the output units (y_k)
    // To compute the line numbers of the output units we go essentially have to 
    // find the line numbers of all assignments l = r with l occuring in the error layer
    for(auto ie : error_function_layer){
        for_all_leaves(
            graph_def[ie].is_assign_op()->rhs(), [&](Traverser::array_ref leaf){
               for(size_t _i {graph_def.size()}; _i > 0; --_i){
                    Invariant("_i > 0");
                    auto i{_i-1};
                    Invariant("graph_def.size() > i >= 0");
                    auto stmt{graph_def[i]};
                    auto assign_expr{stmt.is_assign_op()};
                    if (!assign_expr) continue;
                    auto aref{assign_expr->lhs().as_array_ref()};
                    if (!aref) continue;
                    if(leaf.id == aref->id && leaf.idx == aref->idx){
                        y_ks.push_back(i);
                        break;    
                    }
               } 
            }
        );
    }

    sort(y_ks.begin(), y_ks.end());
    auto last = unique(y_ks.begin(), y_ks.end());
    y_ks.erase(last, y_ks.end());
    Invariant("y_ks is the sorted (in ascending order) of exactly the lines i such that at the position i in the computuation graph is l = r with l referenced from the error layer.");

    vector<Traverser::expr> delta_k;
    for(auto it{y_ks.begin()}; it != last;++it){
        for(auto ie : error_function_layer){
            delta_k.push_back(diff(graph_def[ie].is_assign_op()->rhs(),
                                   *graph_def[*it].is_assign_op()->lhs().as_array_ref(),
                                   constants));
        }      
    }
    Invariant("delta_k contains all \delta_k of the output units k.");

    //for(auto e : y_ks)
    //  cerr << e << '\n';

    //for(auto ie : error_function_layer)
    // cerr << ie << '\n';


    vector<Traverser::expr> inner_deltas;
    //for (auto e : delta_k) cerr << *e.root << "\n\n\n\n\n";
    // Let's introduce first a bumch of helper structures
    // for a layer i we need the referred to layers j, i.e. the layers which send information to i
    // First map the id part of array_ref to an index, there are only a handful of them
    vector<string> names2idx;
    
    // fill names2idx
    for(size_t i{}; i < graph_def.size(); ++i){
        auto stmt{graph_def[i]};
        auto assign_expr{stmt.is_assign_op()};
        if (!assign_expr) continue;
        auto aref{assign_expr->lhs().as_array_ref()};
        if(!aref) continue;
        auto& id{aref->id};
        bool found {};
        for(size_t j{}; j< names2idx.size();++j) if (names2idx[j] == id) { found=true;break;}
        if(!found) names2idx.push_back(id);
    }
    
    //for(size_t j{}; j< names2idx.size();++j) cerr << names2idx[j] << " -> " << j << '\n';

    auto name2idx = [&](string n) -> optional<size_t>{
        for(size_t j{}; j< names2idx.size();++j) if (names2idx[j] == n) return j;
        return {};
    };

    // Next structure maps left hand sides to line numbers

    vector<size_t> lhs2line{graph_def.size() * names2idx.size(), 0 };

    for(size_t i{}; i < graph_def.size(); ++i){
        auto stmt{graph_def[i]};
        auto assign_expr{stmt.is_assign_op()};
        if (!assign_expr) continue;
        auto aref{assign_expr->lhs().as_array_ref()};
        if(!aref) continue;
        auto ididx {name2idx(aref->id)};
        lhs2line[*ididx * graph_def.size() + aref->idx] = i;
    }

    auto aref2linenumber = [&](Traverser::array_ref aref) -> size_t {
        return lhs2line[ *name2idx(aref.id) * graph_def.size() + aref.idx];
    };

    
    for(size_t i{}; i < graph_def.size(); ++i){
        auto stmt{graph_def[i]};
        auto assign_expr{stmt.is_assign_op()};
        if (!assign_expr) continue;
    }

    auto delta_next{y_ks};
    while(delta_next.size()){
        decltype(delta_next) delta_current;
        vector <size_t> delta_k;
        vector <size_t> line;
        vector <size_t> weight;
        for(auto k : delta_next)
            for_all_leaves(graph_def[k].is_assign_op()->rhs(),[&](Traverser::array_ref leaf){
                if (weight_id == leaf.id) {weight.push_back(leaf.idx);delta_k.push_back(k);}
                else if (name2idx(leaf.id)) line.push_back(aref2linenumber(leaf));
            });
        /*for(size_t i{}; i < weight.size();++i){
            cerr << delta_k[i] << '\n';
            cerr << weight[i] << '\n';
            cerr << line[i] << '\n';
            cerr << '\n';        
        }*/

        delta_next = delta_current;
    }

    // Forward Propagation
    for(size_t i{}; i < graph_def.size(); ++i){
        auto stmt{graph_def[i]};
        auto assign_expr{stmt.is_assign_op()};
        if (!assign_expr) { 
            writer.push_back(graph_def[i]); continue;}
        Invariant("stmt is an assignment. i.e. it contains a left and a right side.");
        //auto right_side{assign_expr->rhs()};
        //auto left_side{assign_expr->lhs()};
        //cerr << *stmt.node << "\n";
        
        //auto diff_rhs{diff(right_side)};
        //auto diff_lhs{diff(left_side)};
        writer.push_back(graph_def[i]);
        //writer.push_back(Traverser::expr::mk_assignment(diff_lhs,diff_rhs));
    }
    // Error evaluation
    size_t delta_k_i{};
    for(size_t i{}; i < error_function_layer.size(); ++i){
        for (size_t j{}; j < y_ks.size(); ++j){
            writer.push_back(
                Traverser::expr::mk_assignment(Traverser::array_ref{"delta", y_ks[j] /*- nunber_of_ignored_stamts*/ }.as_expr()  , delta_k[delta_k_i]) );
            ++delta_k_i;
        }
    }    
    // Backward propagation
    
    /*for(;;){

    }*/
    //for (auto e : inner_deltas) writer.push_back(e);

}

ceps::ast::node_t handle_operation_backpropagation(ceps::ast::Struct& ceps_struct){
    OBLECTAMENTA_AST_PROC_PROLOGUE
    using namespace ceps::vm::oblectamenta::ast;
    static const string weight_id_default {"w"};

    node_struct_t computation_graph{};
    auto result = mk_struct("computation_graph");
    vector<CepsComputeGraphNotationTraverser::array_ref> vars;
    
    string weight_id {weight_id_default};


    for(auto e : children(ceps_struct))
        if (is<Ast_node_kind::structdef>(e) && name(as_struct_ref(e)) == "differentiable_program" ){
            auto& diff_struct = as_struct_ref(e);
            for(auto e : children(diff_struct))
                if (is<Ast_node_kind::structdef>(e) && name(as_struct_ref(e)) == "computation_graph" )
                    computation_graph = as_struct_ptr(e);
        } else {
            CepsComputeGraphNotationTraverser::expr expr{e};
            auto opt_array_ref{expr.as_array_ref()};
            if (opt_array_ref)
                vars.push_back(*opt_array_ref);
        }

    if (!computation_graph ) return mk_undef();
    CepsComputeGraphNotationTraverser traverser{children(*computation_graph)};
    CepsComputeGraphNotationTraverser writer;

    ComputationGraph<CepsComputeGraphNotationTraverser,CepsOblectamentaMnemonicsEmitter> comp_graph;
    comp_graph.backprop<CepsComputeGraphNotationTraverser>(
        traverser,writer, weight_id );
    children(*result) = writer.nodes();
    return result;
}

ceps::ast::node_t cepsplugin::operation(ceps::ast::node_callparameters_t params){
    OBLECTAMENTA_AST_PROC_PROLOGUE
    using namespace ceps::vm::oblectamenta::ast;
        
    auto data = get_first_child(params);    
    if (!is<Ast_node_kind::structdef>(data)) {
        return mk_undef();
    }
    auto& ceps_struct = *as_struct_ptr(data);
    if (name(ceps_struct) == "run" && children(ceps_struct).size()){
        for(auto e : children(ceps_struct))
         if (is<Ast_node_kind::structdef>(e) && name(as_struct_ref(e)) == "vm" ){
            auto maybe_vm {read_value<VMEnv>(as_struct_ref(e))};
            if (maybe_vm){
                auto& vm{*maybe_vm};
                vm.run(0);
                return ast_rep<ceps::vm::oblectamenta::VMEnv&> (vm);
            } else {
                
                return mk_undef();
            }
         }
    } else if (name(ceps_struct) == "compile_diffprog" && children(ceps_struct).size()){
        for(auto e : children(ceps_struct))
            if (is<Ast_node_kind::structdef>(e) && name(as_struct_ref(e)) == "differentiable_program" ){
                auto& diff_struct = as_struct_ref(e);
            for(auto e : children(diff_struct))
                if (is<Ast_node_kind::structdef>(e) && name(as_struct_ref(e)) == "computation_graph" ){
                    auto& computation_graph = as_struct_ref(e);
                    CepsComputeGraphNotationTraverser traverser{children(computation_graph)};
                    CepsOblectamentaMnemonicsEmitter emitter;
                    ComputationGraph<CepsComputeGraphNotationTraverser,CepsOblectamentaMnemonicsEmitter> comp_graph;
                    comp_graph.compile(traverser,emitter); 
                    auto result = mk_struct("asm");
                    ceps::ast::children(*result) = emitter.listing();
                    return result;
                }
            }
    } else if (name(ceps_struct) == "tangent_forward_diff" )
        return handle_operation_tangent_forward_diff(ceps_struct);
    else if (name(ceps_struct) == "backpropagation" )
        return handle_operation_backpropagation(ceps_struct);
    return mk_undef();
}

extern "C" void init_plugin(IUserdefined_function_registry* smc)
{
  cepsplugin::plugin_master = smc->get_plugin_interface();
  cepsplugin::plugin_master->reg_ceps_phase0plugin("operation", cepsplugin::operation);
  cepsplugin::plugin_master->reg_ceps_phase0plugin("obj", cepsplugin::obj);
}					
				