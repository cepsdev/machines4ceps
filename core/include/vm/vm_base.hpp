/*
Copyright 2022 Tomas Prerovsky (cepsdev@hotmail.com).

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
#include <cstdint>
#include <ostream>
#include <iostream>

namespace ceps{
    namespace vm{
        namespace oblectamenta{
            using namespace std;
            enum class Opcode{
                halt,
                noop,
                ldi32,
                ldi64,
                lddbl,
                sti32,
                sri64,
                stdbl,
                ldptr,
                stptr,
                addi32,
                addi64,
                adddbl,
                muli32,
                muli64,
                muldbl,
                divi32,
                divi64,
                divdbl,
                remi32,
                remi64,
                lti32,
                lti64,
                ltdbl,
                lteqi32,
                lteqi64,
                lteqdbl,
                gti32,
                gti64,
                gtdbl,
                gteqi32,
                gteqi64,
                gteqdbl,
                eqi32,
                eqi64,
                eqdbl,
                andi32,
                andi64,
                ori32,
                ori64,
                noti32,
                noti64,
                xori32,
                xori64
            };


            class VMEnv{
                public:
                    using data_t = vector<uint8_t>;
                    using stack_t = vector<int>;
                    using text_t = vector<uint32_t>;
  
                    template<typename T> size_t store(T data){
                        auto t = data_seg.size();
                        data_seg.insert(data_seg.end(), 
                                    (data_t::value_type*) &data, 
                                    (data_t::value_type*) &data + sizeof(data) );
                        return t;
                    }
                    template<typename T> size_t push(T data){
                        auto t = stack_top;
                        for (size_t i = 0; i < sizeof(T) / sizeof(stack_t::value_type); ++i)
                         stack[stack_top+i] = *((stack_t::value_type*) &data + i);
                        stack_top += sizeof(T) / sizeof(stack_t::value_type);
                        return t;
                    }
                    template<typename T> T pop(){
                        T r;
                        size_t start = stack_top - sizeof(T) / sizeof(stack_t::value_type);
                        for (size_t i = 0; i < sizeof(T) / sizeof(stack_t::value_type); ++i)
                         *((stack_t::value_type*) &r + i) = stack[start+i];
                        stack_top = start;
                        return r;
                    }

                    text_t& text() {return text_seg;}

                    size_t run(size_t start = 0);

                    VMEnv();

                    void dump(ostream& os);

                private:
                    void noop(size_t) {}
                    void ldi32(size_t);
                    void ldi64(size_t);
                    void lddbl(size_t);
                    void sti32(size_t);
                    void sti64(size_t);
                    void stdbl(size_t);
                    void ldptr(size_t);
                    void stptr(size_t);
                    void addi32(size_t);
                    void addi64(size_t);
                    void adddbl(size_t);

                    data_t data_seg;
                    stack_t stack;
                    text_t text_seg;
                    size_t stack_top;
                    using fn = void (VMEnv::*) (size_t) ;
                    vector<fn> op_dispatch;
            };


            template<Opcode opcode> size_t emit(VMEnv::text_t& text){
                    auto t {text.size()};
                    text.push_back((VMEnv::text_t::value_type) opcode);
                    return t;
            }
        }
    }
}