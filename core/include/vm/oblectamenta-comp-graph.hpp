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

namespace ceps{
    namespace vm{
        namespace oblectamenta{

           template<typename T> class NotationTraverser;
             
           struct compgraph_parameter_t{
                string name;
                int idx{};
            };

           template<typename NotationTraverser, typename CodeEmitter> 
           class ComputationGraph{
            public:

             void compile(NotationTraverser&, CodeEmitter& );
             template <typename NotationEmitter> 
              void tangent_forward_diff(NotationTraverser&,NotationEmitter&, compgraph_parameter_t );
             template <typename NotationEmitter> 
              void backprop(NotationTraverser&,NotationEmitter&, compgraph_parameter_t );
           };
        }
    }
} 