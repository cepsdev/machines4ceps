/*
Copyright 2023 Tomas Prerovsky (cepsdev@hotmail.com).

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
#pragma once

#include <vector>
#include <map>
#include <string>
#include "ceps_ast.hh"
#include "ceps_interpreter.hh"
#include "core/include/vm/vm_base.hpp"

namespace ceps{
    namespace vm{
        namespace oblectamenta{
            namespace assembler {
                static double constexpr text_growth_factor{1.4};
            }
            void oblectamenta_assembler(VMEnv& vm, vector<ceps::ast::node_t> mnemonics, std::map<std::string, int> const & ev_to_id);
        }
    }
} 



#define OBLECTAMENTA_AST_PROC_PROLOGUE \
    using namespace ceps::ast;\
    using namespace ceps::vm::oblectamenta;\
    using namespace ceps::interpreter;\
    using namespace std;


namespace ceps::vm::oblectamenta::ast{
template<typename T> 
    T  fetch(ceps::ast::Struct&);
template<typename T> 
    T  fetch(ceps::ast::node_t);
template<typename T> 
    T  fetch(ceps::ast::Struct&, ceps::vm::oblectamenta::VMEnv&);

template<> ceps::vm::oblectamenta::VMEnv fetch<ceps::vm::oblectamenta::VMEnv>(ceps::ast::Struct& );


template<typename T> 
    std::optional<T> read_value(ceps::ast::Struct& s);
template<typename T> 
    std::optional<T> read_value(ceps::ast::Struct& s, ceps::vm::oblectamenta::VMEnv& vm);
template<typename T> 
    std::optional<T> read_value(size_t idx, ceps::ast::Struct& s);
template<typename T> 
    bool check(ceps::ast::Struct&);
template<typename T> 
    bool check(ceps::ast::node_t);

template<typename T> 
    std::optional<T> read_value(size_t idx, ceps::ast::Struct& s){
        auto & v{children(s)};
        if(v.size() <= idx || !check<T>(v[idx])) return {};
        return fetch<T>(v[idx]);
    }

template<typename T> 
    std::optional<T> read_value(ceps::ast::Struct& s){
        if(!check<T>(s)) return {};
        return fetch<T>(s);
    }

template<typename T> 
    std::optional<T> read_value(ceps::ast::Struct& s, ceps::vm::oblectamenta::VMEnv& vm){
        if(!check<T>(s)) return {};
        return fetch<T>(s,vm);
    }

template<typename T> ceps::ast::node_t ast_rep (T entity);
template<typename T> ceps::ast::node_t ast_rep (T entity, ceps::vm::oblectamenta::VMEnv&);


////////

struct ser_wrapper_stack{
    std::shared_ptr<ceps::vm::oblectamenta::VMEnv> vm;
};

struct ser_wrapper_data{
    std::shared_ptr<ceps::vm::oblectamenta::VMEnv> vm;
};

struct ser_wrapper_text{
    std::shared_ptr<ceps::vm::oblectamenta::VMEnv> vm;    
};

struct ser_wrapper_cstack{
    std::shared_ptr<ceps::vm::oblectamenta::VMEnv> vm;
};


}