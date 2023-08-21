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

                //Arithmetic
                addi32,
                addi64,
                adddbl,
                subi32,
                subi64,
                subdbl,
                //Control Flow

                buc,
                beq,
                bneq,
                blt,
                blteq,
                bgt,
                bgteq,
                bzeroi32,
                bnzeroi32,
                bzeroi64,
                bnzeroi64,
                bzerodbl,
                bnzerodbl,

                //Bitoperators

                andni32,
                andni64,
                andi32,
                andi64,
                ori32,
                ori64,
                noti32,
                noti64,
                xori32,
                xori64,


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
                eqdbl
            };
            static vector<pair<string,string>> mnemonics = {
                {"halt", "Yields the processor."},
                {"noop", "No operation."},
                {"ldi32", "Push 32 bit signed integer."},
                {"ldi64", ""},
                {"lddbl", ""},
                {"sti32", ""},
                {"sri64", ""},
                {"stdbl", ""},
                {"ldptr", ""},
                {"stptr", ""},

                //Arithmetic
                {"addi32", ""},
                {"addi64", ""},
                {"adddbl", ""},
                {"subi32", ""},
                {"subi64", ""},
                {"subdbl", ""},
                //Control Flow

                {"buc", ""},
                {"beq", ""},
                {"bneq", ""},
                {"blt", ""},
                {"blteq", ""},
                {"bgt", ""},
                {"bgteq", ""},
                {"bzeroi32", ""},
                {"bnzeroi32", ""},
                {"bzeroi64", ""},
                {"bnzeroi64", ""},
                {"bzerodbl", ""},
                {"bnzerodbl", ""},

                //Bitoperators

                {"andni32", ""},
                {"andni64", ""},
                {"andi32", ""},
                {"andi64", ""},
                {"ori32", ""},
                {"ori64", ""},
                {"noti32", ""},
                {"noti64", ""},
                {"xori32", ""},
                {"xori64", ""},


                {"muli32", ""},
                {"muli64", ""},
                {"muldbl", ""},
                {"divi32", ""},
                {"divi64", ""},
                {"divdbl", ""},
                {"remi32", ""},
                {"remi64", ""},
                {"lti32", ""},
                {"lti64", ""},
                {"ltdbl", ""},
                {"lteqi32", ""},
                {"lteqi64", ""},
                {"lteqdbl", ""},
                {"gti32", ""},
                {"gti64", ""},
                {"gtdbl", ""},
                {"gteqi32", ""},
                {"gteqi64", ""},
                {"gteqdbl", ""},
                {"eqi32", ""},
                {"eqi64", ""},
                {"eqdbl", ""}
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
  
                    template<typename T> T read_store(size_t ofs){
                        T t = *(T*) &data_seg[ofs];
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
                    void reset();
                private:
                    size_t noop(size_t);
                    size_t ldi32(size_t);
                    size_t ldi64(size_t);
                    size_t lddbl(size_t);
                    size_t sti32(size_t);
                    size_t sti64(size_t);
                    size_t stdbl(size_t);
                    size_t ldptr(size_t);
                    size_t stptr(size_t);
                    size_t addi32(size_t);
                    size_t addi64(size_t);
                    size_t adddbl(size_t);
                    size_t subi32(size_t);
                    size_t subi64(size_t);
                    size_t subdbl(size_t);

                    size_t buc(size_t);
                    size_t beq(size_t);
                    size_t bneq(size_t);
                    size_t blt(size_t);
                    size_t blteq(size_t);
                    size_t bgt(size_t);
                    size_t bgteq(size_t);
                    size_t bzeroi32(size_t);
                    size_t bzeroi64(size_t);
                    size_t bzerodbl(size_t);
                    size_t bnzeroi32(size_t);
                    size_t bnzeroi64(size_t);
                    size_t bnzerodbl(size_t);

                    size_t andni32(size_t);
                    size_t andni64(size_t);
                    size_t andi32(size_t);
                    size_t andi64(size_t);
                    size_t ori32(size_t);
                    size_t ori64(size_t);

                    size_t noti32(size_t);
                    size_t noti64(size_t);
                    size_t xori32(size_t);
                    size_t xori64(size_t);

                    data_t data_seg;
                    stack_t stack;
                    text_t text_seg;
                    size_t stack_top;
                    using fn = size_t (VMEnv::*) (size_t) ;
                    vector<fn> op_dispatch;

                    static constexpr size_t base_opcode_width = 1;
            };


            template<Opcode opcode> size_t emit(VMEnv::text_t& text){
                    auto t {text.size()};
                    text.push_back((VMEnv::text_t::value_type) opcode);
                    return t;
            }
            template<Opcode opcode> size_t emit(VMEnv::text_t& text, int v){
                    auto t {text.size()};
                    text.push_back((VMEnv::text_t::value_type) opcode);
                    text.push_back(v);
                    return t;
            }
           template<typename T> void patch(VMEnv::text_t& text, size_t ofs, T t ){
                    text[ofs] = t;
            }
 
        }
    }
}