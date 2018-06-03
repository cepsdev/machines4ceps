/**
 The MIT License (MIT)

Copyright (c) 2018 Tomas Prerovsky <tomas.prerovsky@ceps.technolgy>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
 **/

#include "ceps_all.hh"
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <vector>
#include <mutex>
#include <map>
#include <thread>
#include <sstream>
#include <fstream>

#define VERSION_CEPSJIT_MAJOR 0
#define VERSION_CEPSJIT_MINOR 0

using namespace std;
using namespace ceps::ast;

bool is_addition(Nodebase_ptr p){
    return p->kind() == ceps::ast::Ast_node_kind::binary_operator&& ceps::ast::op(ceps::ast::as_binop_ref(p)) == '+';
}

bool is_variable(Nodebase_ptr expr){
    return expr->kind() == ceps::ast::Ast_node_kind::symbol && kind(as_symbol_ref(expr)) == "Systemstate";
}

std::string get_name_of_var(Nodebase_ptr expr) {return name(as_symbol_ref(expr));}

constexpr auto DEBUG = false;
constexpr auto PRINT_LOC_DEBUG = false;

std::string help_text =
R"(
Usage: cepsjit FILE [FILE...] [--evaluate|-e] [--print-evaluated|-pe] [--print-raw|-pr] [--pretty-print|-pp] [--version|-v] [--verbose|-v]
)";

enum class Machine_architecture {Abstract,x86};
enum class Machine_features {SSE2,SSE4};

enum class MA_Datatype {None,
                        Byte_unsigned_integer,
                        Byte_signed_integer,
                        Word_unsigned_integer,
                        Word_signed_integer,
                        Doubleword_unsigned_integer,
                        Doubleword_signed_integer,
                        Quadword_signed_integer,
                        Quadword_unsigned_integer,
                        Floatingpoint_halfprecision,
                        Floatingpoint_singleprecision,
                        Floatingpoint_doubleprecision,
                        Floatingpoint_doubleextendedprecision
                       };

struct Bucket{
    int idx = -1;
    bool allocated = false;
    MA_Datatype stored_type = MA_Datatype::None;
    bool dont_spill = false;
};

using Buckets = std::vector<Bucket>;

struct Location{
    int reg_idx = -1;
    int addr = 0; //stack allocated data is negative addresses
    Nodebase_ptr ref_data = nullptr;
    bool is_int_literal() const { return ref_data != nullptr && ref_data->kind() == Ast_node_kind::int_literal;}
    bool is_inmem()const {return reg_idx ==-1;}
    bool is_inreg() const {return reg_idx >= 0;}
    bool is_onstack() const {return is_valid() && is_inmem() && addr < 0;}
    bool is_valid() const {return ref_data != nullptr && (is_inreg() || is_inmem()); }
    auto const & ref() const {return ref_data; }
    auto & ref() {return ref_data;}
    void print(std::ostream& os, Machine_architecture ma = Machine_architecture::Abstract) const {
        if (is_int_literal() && !is_inreg()){
            switch(ma){
                case Machine_architecture::Abstract:
                    os << "addr(dw$"<< (std::uint32_t)value(as_int_ref(ref_data)) <<")";
                    break;
                case Machine_architecture::x86:
                    os << "[dw$"<< (std::uint32_t)value(as_int_ref(ref_data)) <<"]";
                    break;
            }
            return;
        }
        switch (ma){
            case Machine_architecture::Abstract :
                if (is_inreg())
                    os << "R"<<reg_idx;
                else if (is_inmem()){
                    if (!is_onstack()) os << "addr("<<addr<<")";
                    else os << "stack("<<-addr<<")";
                }
                break;
            case Machine_architecture::x86 :
                if (is_inreg())
                    os << "xmm"<<reg_idx;
                else if (is_inmem()){
                    if (!is_onstack()) {
                        if (ref_data !=nullptr && ref_data->kind() == ceps::ast::Ast_node_kind::symbol && kind(as_symbol_ref(ref_data)) == "Systemstate")
                        os << "["<<name(as_symbol_ref(ref_data))<<"$Systemstate"<<"]";
                    }
                    else os << "[BP"<<-addr<<"]";
                }
                break;
        }
    }
};

using Location_index = int;

enum class Abstractopcode {
    None,
    MOV,
    ADD,
    SUB,
    MUL,
    DIV,
    NEG,
    LT,
    GT,
    EQ,
    AND,
    OR,
    NOT,
    BR
};

struct Abstractinstruction {
    Abstractopcode opcode = Abstractopcode::None;
    Location loc1,loc2,loc3;
    Location& dest() {return loc1;}
    Location const & dest() const {return loc1;}
    Location& op() {return loc2;}
    Location const & op() const {return loc2;}
    Location& op1() {return op();}
    Location const & op1() const {return op();}
    Location& op2() {return loc3;}
    Location const & op2() const {return loc3;}

    Location& src() {return loc2;}
    Location const & src() const {return loc2;}


    void print(std::ostream& os, Machine_architecture ma = Machine_architecture::Abstract) const {
        switch (opcode){
            case Abstractopcode::ADD:
                switch (ma){
                    case Machine_architecture::Abstract:
                        os << "ADD ";
                        dest().print(os,ma);os << ", ";
                        op1().print(os,ma);os << ", ";
                        op2().print(os,ma);
                        break;
                    case Machine_architecture::x86:
                        os << "PADDD ";
                        op1().print(os,ma);os << ", ";
                        op2().print(os,ma);
                    break;
                }
                break;
        case Abstractopcode::MOV:
            switch (ma){
                case Machine_architecture::Abstract:
                    os << "MOV ";
                    dest().print(os,ma);os << ", ";
                    src().print(os,ma);
                    break;
                case Machine_architecture::x86:
                    if (is_variable(dest().ref())){
                        os << "MOVSS ";
                        dest().print(os,ma);
                        os<<", ";
                        src().print(os,ma);
                    } else {
                        os << "PINSRB ";
                        dest().print(os,ma); os << ", ";
                        src().print(os,ma); os << ", 0";
                    }
                    break;
            }
            break;

        }
    }
};

using Abstractinstructions = std::vector<Abstractinstruction>;

Abstractinstruction gen_abstract_add(Location const & dest,Location const & op1,Location const & op2){
    return {Abstractopcode::ADD,dest,op1,op2};
}
Abstractinstruction gen_abstract_store (Nodebase_ptr expr,Location const & source){
    return {Abstractopcode::MOV,Location{-1,0,expr},source,{}};
}
Abstractinstruction gen_abstract_mov(Location const & dest,Location const & source){
    return {Abstractopcode::MOV,dest,source,{}};
}

struct Abstract_program_fragment{
    Abstractinstructions instructions;
    std::map<std::string,Nodebase_ptr> globals;
    std::map<std::uint32_t,std::string> dw_constants;

    void insert(Abstractinstruction instruction){instructions.push_back(instruction);}
    void print(std::ostream& os, Machine_architecture ma = Machine_architecture::Abstract) const {
        if (ma == Machine_architecture::x86){
            os << "global main\n";
            os <<"extern puts\n";
           /* os << "section .data\n";
            for(auto e: dw_constants){
                os << "  " << e.second << ": dd "<< e.first <<"\n";
            }
            for(auto e: globals){
                os << "  " << e.first << ": dd 0\n";
            }
            os << "section .bss\n";*/
            os << "section .text\n";
            os << "main:\n";
        }
        for(auto const & e : instructions)
            {e.print(os,ma);os<<"\n";}
        if (ma == Machine_architecture::x86){
            os << "ret\n";os << "section .data\n";
            for(auto e: dw_constants){
                os << "  " << e.second << ": dd "<< e.first <<"\n";
            }
            for(auto e: globals){
                os << "  " << e.first << ": dd 0\n";
            }
        }
    }
    void register_global(Nodebase_ptr expr){
        globals[name(as_symbol_ref(expr))+"$"+kind(as_symbol_ref(expr))]= expr;
    }
};

std::ostream& operator << (std::ostream& os, Abstract_program_fragment const & code){
    code.print(os);
    return os;
}

std::ostream& operator << (std::ostream& os, Location const & loc){
   if (PRINT_LOC_DEBUG) os << "loc(";
   if (loc.is_inmem()){
        if (loc.is_onstack()){
            os << "[SP "<< loc.addr<<"]";
        } else {
            os << "["<<loc.addr<<"]";
        }
    } else os << "R-"<<loc.reg_idx;
    if (PRINT_LOC_DEBUG) os << "," << *loc.ref();
    if (PRINT_LOC_DEBUG) os << ")";
    return os;
}

using Locations = std::vector<Location>;
Locations::iterator find_loc_of_var(std::string name,Locations & locs){

    auto r = std::find_if(locs.begin(),
                          locs.end(),
                          [=](Location const & loc){
                               if (loc.ref() == nullptr || !is_variable(loc.ref()) ) return false;
                               return get_name_of_var(loc.ref()) == name;
                          });
    if (DEBUG)
        std::cout << "find_loc_of_var('"<<name<<"'):"<< (locs.end() == r ? -1 : r-locs.begin()) <<"\n";
    return r;
}

Locations::iterator find_loc_of_const(std::uint32_t i,Locations & locs){

    auto r = std::find_if(locs.begin(),
                          locs.end(),
                          [=](Location const & loc){
                               if (loc.ref() == nullptr || loc.ref()->kind() != ceps::ast::Ast_node_kind::int_literal ) return false;
                               return (std::uint32_t)value(as_int_ref(loc.ref())) == i;
                          });
    return r;
}


auto create_registers(Machine_architecture ma){
    Buckets r;
    switch(ma){
    case Machine_architecture::x86:
        {
           for(auto i = 0; i != 8; ++i) r.push_back({i,false});
        }
    }
    return r;
}

std::string str(MA_Datatype type){
    switch(type){
        case MA_Datatype::Doubleword_signed_integer : return "dword signed";
    }
    return "unknown";
}

void print_registers(Buckets const & buckets){
    for (auto bucket : buckets){
        std::cout<< "idx="<<bucket.idx <<" allocated="<<bucket.allocated << "\n";
    }
}



bool invalid_reg(Bucket r) {return r.idx == -1;}


std::string reg_name(Bucket r){
    return "reg_"+std::to_string(r.idx);
}

void gen_load_var_into_reg(Bucket r, Nodebase_ptr v){
    std::cout << "MOV " << reg_name(r)<<","<< name(as_symbol_ref(v)) << "\n";
}

void free_reg(Bucket& r){
    r.allocated = false;
}

int byte_size(Nodebase_ptr n){
    return sizeof(int);
}

std::map<std::string,int> globals;
int next_free_global_addr = 0;

int get_addr_of_global(Nodebase_ptr expr){
    auto it = globals.find(name(as_symbol_ref(expr)));
    if (it == globals.end())
    {
        auto t = next_free_global_addr;
        globals[name(as_symbol_ref(expr))] = t;
        next_free_global_addr += byte_size(expr);
        return t;
    }
    return it->second;
}

MA_Datatype get_type_of_var(Nodebase_ptr expr){
    return MA_Datatype::Doubleword_signed_integer;
}

void loc_name(Location loc){

}
void spill_a_reg(Buckets& regs,
                 Locations& locs);

bool load_loc_into_reg(Location & loc,Buckets& regs,Abstract_program_fragment& imcode){
    if (loc.is_inreg()) return true;
    for(auto& r:regs){
        if(r.allocated) continue;r.allocated=true;
        auto dest_loc = loc;
        dest_loc.reg_idx = r.idx;
        imcode.insert(gen_abstract_mov(dest_loc,loc));
        loc.reg_idx = r.idx;
        loc.addr = 0;
        return true;
    }
    return false;
}
Bucket& get_bucket(Buckets& regs,int idx){
    for(auto & e : regs)
        if (e.idx ==idx) return e;
    throw std::runtime_error{"get_bucket(regs,idx) failed."};
}

bool ensure_locs_are_in_buckets(Location& l1,
                                Location& l2,
                                Buckets& regs,
                                Locations& locs,
                                Abstract_program_fragment& imcode){

    if(!load_loc_into_reg(l1,regs,imcode)){
      spill_a_reg(regs,locs);
      if(!load_loc_into_reg(l1,regs,imcode)) throw std::runtime_error{"Couldn't allocate a register: Spilling location failed."};
    }
    if(!load_loc_into_reg(l2,regs,imcode)){
      auto t = get_bucket(regs,l1.reg_idx).dont_spill;get_bucket(regs,l1.reg_idx).dont_spill = true;
      spill_a_reg(regs,locs);
      get_bucket(regs,l1.reg_idx).dont_spill = t;
      if(!load_loc_into_reg(l2,regs,imcode)) throw std::runtime_error{"Couldn't allocate a register: Spilling location failed."};
    }

}

Location_index gen_addition(Nodebase_ptr expr,
                      Location_index left_loc_idx,
                      Location_index right_loc_idx,
                      Buckets& regs,
                      Locations& locs,
                      Abstract_program_fragment& imcode){
    Location& left_loc = locs[left_loc_idx];
    Location& right_loc = locs[right_loc_idx];
    ensure_locs_are_in_buckets(left_loc,right_loc,regs,locs,imcode);
    imcode.insert(gen_abstract_add(left_loc,left_loc,right_loc));
    left_loc.ref() = expr;
    return left_loc_idx;
}

void gen_store(Nodebase_ptr expr,
               Location_index rhs_loc_idx,
               Buckets& regs,
               Locations& locs,
               Abstract_program_fragment& imcode){
    Location& rhs_loc = locs[rhs_loc_idx];

    imcode.insert(gen_abstract_store(expr,rhs_loc));
}

int next_free_sp = -4;

int allocate_stackspace(int size){
 auto t = next_free_sp;
 next_free_sp -= size;
 return t;
}

int byte_size(Location& loc){
    return sizeof(int);
}

void save2temporary(Location& loc){
   auto mem_addr = allocate_stackspace(byte_size(loc));
   if (loc.is_inreg()){
       Location temp_loc = {-1,mem_addr,loc.ref()};
       std::cout << "MOV " << temp_loc << ", " << loc << "\n";
       loc = temp_loc;
   }
}

void spill_a_reg(Buckets& regs,
                 Locations& locs){
    for(auto & loc: locs){
        if (!loc.is_inreg()) continue;
        if (get_bucket(regs,loc.reg_idx).dont_spill) continue;
        auto reg_idx = loc.reg_idx;
        save2temporary(loc);
        break;
    }
}

Location_index jit_gen_code_expression(Nodebase_ptr expr,
                                 ceps::Ceps_Environment & env,
                                 Buckets& buckets,
                                 Locations& locs,
                                 Abstract_program_fragment& imcode){
 if (DEBUG) {
     std::cout << "jit_gen_code_expression: "<< Nodeset{expr} << std::endl;
     print_registers(buckets);
 }
 if (is_addition(expr)){
     auto left_loc_idx = jit_gen_code_expression(as_binop_ref(expr).left(),env,buckets,locs,imcode);
     auto right_loc_idx = jit_gen_code_expression(as_binop_ref(expr).right(),env,buckets,locs,imcode);
     if (left_loc_idx == Location_index{-1} || right_loc_idx == Location_index{-1}) return Location_index{-1};
     auto l =  gen_addition(expr,left_loc_idx,right_loc_idx,buckets,locs,imcode);
     return l;
 } else if (expr->kind() == ceps::ast::Ast_node_kind::binary_operator && op(as_binop_ref(expr)) == '=' ) {
     auto right_loc_idx = jit_gen_code_expression(as_binop_ref(expr).right(),env,buckets,locs,imcode);
     Location& l1 = locs[right_loc_idx];
     if(!load_loc_into_reg(l1,buckets,imcode)){
       spill_a_reg(buckets,locs);
       if(!load_loc_into_reg(l1,buckets,imcode)) throw std::runtime_error{"Couldn't allocate a register: Spilling location failed."};
     }
     if(is_variable(as_binop_ref(expr).left())){
         imcode.register_global(as_binop_ref(expr).left());
         gen_store(as_binop_ref(expr).left(),right_loc_idx,buckets,locs,imcode);
     }
     return right_loc_idx;
 } else if (is_variable(expr)) {
     auto loc_it = find_loc_of_var(ceps::ast::name(ceps::ast::as_symbol_ref(expr)),locs);
     if (loc_it == locs.end()){
         if (kind(as_symbol_ref(expr)) == "Systemstate"){
             imcode.register_global(expr);
             auto new_loc = Location{-1,0,expr};
             locs.push_back(new_loc);
             return locs.size()-1;
         }
     } else {
         return loc_it-locs.begin();
     }
 } else if (expr->kind() == ceps::ast::Ast_node_kind::int_literal){
     std::uint32_t i = value(as_int_ref(expr));
     auto it = imcode.dw_constants.find(i);
     imcode.dw_constants[i] = "dw$"+std::to_string(i);
     auto loc_it = find_loc_of_const(i,locs);
     if (loc_it == locs.end()){
         auto new_loc = Location{-1,0,expr};
         locs.push_back(new_loc);
     }
     return locs.size()-1;
 }
 return Location_index{-1};
}

void debug_print_locs_x86 (Locations locs) {
    std::cout << "Locs:\n";
    for(auto const & l : locs)
        {l.print(std::cout,Machine_architecture::x86);std::cout << "\n";}
}


int main(int argc, char*argv[])
{
    bool evaluate = false;
    bool print_evaluated = false;
    bool print_raw = false;
    bool print_pretty = false;
    bool print_version = false;
    bool print_verbose = false;
    std::vector<std::string> files;
    if (argc == 1)
    {
        std::cout << help_text << std::endl;
        return EXIT_SUCCESS;
    }

    for (int i = 1; i < argc;++i)
    {
        string arg{argv[i]};
        if (arg == "-e" || arg == "--evaluate")
        {
            evaluate = true;
        }
        else if (arg == "-pp" || arg == "--pretty-print")
        {
            print_pretty  = true;
        }
        else if (arg == "-pe" || arg == "--print-evaluated")
        {
            print_evaluated = true;
        }
        else if (arg == "-pr" || arg == "--print-raw")
        {
            print_raw = true;
        }
        else if (arg == "--version")
        {
            print_version = true;
        }
        else if (arg == "--verbose" || arg == "-v")
        {
            print_verbose = true;
        }
        else {

            if (!ifstream{arg})
            {
                std::cerr << "\n***Error: Couldn't open file '" << arg << "' " << std::endl;
                return EXIT_FAILURE;
            }
            files.push_back(arg);
        }
    }

    if (print_version)
    {

#ifdef __GNUC__
        std::cout << "\n"
            << "VERSION " <<  VERSION_CEPSJIT_MAJOR << "." << VERSION_CEPSJIT_MINOR << " (" __DATE__ << ") BUILT WITH GCC " << "" __VERSION__ "" << " on GNU/LINUX "
#ifdef __LP64__
            << "64BIT"
#else
            << "32BIT"
#endif
            << "\n(C) BY THE AUTHORS OF ceps\n" << std::endl;
#else
#ifdef _MSC_FULL_VER
        std::cout << "\n"
            << "VERSION " << VERSION_CEPSJIT_MAJOR << "." << VERSION_CEPSJIT_MINOR << " (" __DATE__ << ") BUILT WITH MS VISUAL C++ " << _MSC_FULL_VER << " FOR WINDOWS "
#ifdef _WIN64
            << "64BIT"
#else
            << "32BIT"
#endif
            << "\n(C) BY THE AUTHORS OF ceps\n" << std::endl;
#endif
#endif


    }

    ceps::Ceps_Environment ceps_env{""};
    Nodeset universe;
    for(std::string const & filename : files)
    {
        ifstream in{filename};
        if (!in)
        {
            std::cerr << "\n***Error: Couldn't open file '" << filename << "' " << std::endl;
            return EXIT_FAILURE;
        }


        try{
            Ceps_parser_driver driver(ceps_env.get_global_symboltable(),in);
            ceps::Cepsparser parser(driver);

            if (parser.parse() != 0)
                continue;

            if (driver.errors_occured())
                continue;

            auto root = ceps::ast::nlf_ptr(driver.parsetree().get_root());

            char buffer[PATH_MAX] = {};
            if ( buffer != realpath(filename.c_str(),buffer) ){
                std::cerr << "\n***Error: realpath() failed for '" << filename << "' " << std::endl;
                return EXIT_FAILURE;
            }


            root->children().insert(root->children().begin(),new ceps::ast::Struct("@@file",new ceps::ast::String(std::string{buffer}),nullptr,nullptr));

            if (print_raw)
            {
                if (print_pretty )
                    std::cout << ceps::ast::Nodebase::pretty_print <<  *driver.parsetree().get_root() << std::endl << std::endl;
                else
                    std::cout << *driver.parsetree().get_root() << std::endl << std::endl;
            }
            //ceps_env.eval_and_merge( driver.parsetree() , false);
            if (evaluate)
            {
                /*ceps::ast::Nodebase_ptr p = ceps::interpreter::evaluate(	driver.parsetree().get_root(),
                                                                            ceps_env.get_global_symboltable(),
                                                                            ceps_env.interpreter_env());*/

                std::vector<ceps::ast::Nodebase_ptr> generated_nodes;

                ceps::interpreter::evaluate(universe,
                                            driver.parsetree().get_root(),
                                            ceps_env.get_global_symboltable(),
                                            ceps_env.interpreter_env(),&generated_nodes);

                auto p = new Root();
                //p->children().insert(p->children().end(), universe.nodes().begin(), universe.nodes().end());
                p->children().insert(p->children().end(), generated_nodes.begin(), generated_nodes.end());

                Nodeset jit_expressions = Nodeset{p}[all{"jit_eval"}];
                auto regs = create_registers(Machine_architecture::x86);
                auto locations = Locations{};
                Abstract_program_fragment imcode;

                for(auto jit_expr_ : jit_expressions){
                    if (DEBUG) std::cout << "main().jit_eval: "<<jit_expr_["jit_eval"]<<"\n";
                    auto jit_expr = jit_expr_["jit_eval"];
                    try{
                        for(auto p : jit_expr.nodes()){
                            //std::cout << *p << "\n";
                            auto r = jit_gen_code_expression(p,
                                                    ceps_env,
                                                    regs,
                                                    locations,
                                                    imcode);
                            if (r == Location_index{-1}){
                                std::cerr<< "***Error:jit:Failed to compile " << *p << std::endl;
                                exit(1);
                            }
                        }
                    } catch (std::runtime_error const & e){
                        std::cerr << e.what() << "\n";
                        exit(2);
                    }
                }
                imcode.print(std::cout, Machine_architecture::x86);


                if (print_evaluated)
                {
                    if (print_verbose){
                     if (print_pretty )
                        std::cout << ceps::ast::Nodebase::pretty_print <<  *p << std::endl;
                     else
                        std::cout << *p << std::endl;
                    } else {
                        for(auto pp: p->children()){
                            if (pp->kind() == ceps::ast::Ast_node_kind::structdef && ceps::ast::name(ceps::ast::as_struct_ref(pp)).substr(0,2) == "@@" ) continue;
                             if (print_pretty )
                                std::cout << ceps::ast::Nodebase::pretty_print <<  *pp << std::endl;
                             else
                                std::cout << *pp;
                        }
                    }
                }
            }
        } catch (ceps::interpreter::semantic_exception & se)
        {
            std::cerr << "[ERROR][Interpreter]:"<< se.what() << std::endl;
        }
        catch (std::runtime_error & re)
        {
            std::cerr << "[ERROR][System]:"<< re.what() << std::endl;
        }
    }
}
