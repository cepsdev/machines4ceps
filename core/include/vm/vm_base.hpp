#pragma once

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


#include <vector>
#include <cstdint>
#include <ostream>
#include <iostream> 
#include <map>

namespace ceps{
    namespace vm{
        namespace oblectamenta{
            using namespace std;
            enum class Opcode:uint32_t{
                halt,
                noop,
                ldi32,
                ldsi32,
                ldi64,
                lddbl,
                sti32,
                stsi32,
                sri64,
                stdbl,
                ldptr,
                stptr,
                lea,

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
                bgteqzeroi32,
                blteqzeroi32,
                bltzeroi32,
                bzeroi32,
                bnzeroi32,
                bzeroi64,
                bnzeroi64,
                bzerodbl,
                bnzerodbl,
                call,
                ret,

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
                duptopi32,
                

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



            class VMEnv{
                public:
                    using data_t = vector<uint8_t>;
                    using stack_t = vector<int>;
                    using text_t = vector<int32_t>;

                    size_t store (string  data){
                        size_t t{};
                        for(auto e: data) t = store(e);
                        return t;
                    }
  
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
                        constexpr auto r{sizeof(T) % sizeof(stack_t::value_type)};
                        auto space_needed {sizeof(T) / sizeof(stack_t::value_type) + (r ? 1 : 0) };
                        if (stack_top + space_needed >= stack_seg.size() ) stack_seg.insert(stack_seg.end(), space_needed, {});
                        for (size_t i = 0; i < space_needed; ++i)
                         stack_seg[stack_top+i] = *((stack_t::value_type*) &data + i);
                        stack_top += space_needed;
                        if(sizeof(T) < sizeof(stack_t::value_type)){
                            for(auto i = 1; i < sizeof(T) - sizeof(stack_t::value_type);++i)
                             ((char*)&stack_seg[stack_top-1])[i] = 0;
                        }
                        return t;
                    }
                    template<typename T> T pop(){
                        T r;
                        size_t start = stack_top - sizeof(T) / sizeof(stack_t::value_type);
                        for (size_t i = 0; i < sizeof(T) / sizeof(stack_t::value_type); ++i)
                         *((stack_t::value_type*) &r + i) = stack_seg[start+i];
                        stack_top = start;
                        return r;
                    }

                    text_t& text() {return text_seg;}
                    data_t& data() {return data_seg;}
                    stack_t& stack() {return stack_seg;}

                    size_t run(size_t start = 0);

                    VMEnv();

                    void dump(ostream& os);
                    void reset();
                    size_t stack_top_pos() const { return stack_top;}
                    map<string, size_t>& data_labels(){return label2loc;}
                    size_t data_size() const {return data_seg.size();}
                private:
                    size_t noop(size_t);
                    size_t ldi32(size_t);
                    size_t ldsi32(size_t);
                    size_t ldi64(size_t);
                    size_t lddbl(size_t);
                    size_t sti32(size_t);
                    size_t stsi32(size_t);
                    size_t sti64(size_t);
                    size_t stdbl(size_t);
                    size_t ldptr(size_t);
                    size_t stptr(size_t);
                    size_t lea(size_t);
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
                    size_t bgteqzeroi32(size_t);
                    size_t blteqzeroi32(size_t);
                    size_t bltzeroi32(size_t);
                    size_t bzeroi32(size_t);
                    size_t bzeroi64(size_t);
                    size_t bzerodbl(size_t);
                    size_t bnzeroi32(size_t);
                    size_t bnzeroi64(size_t);
                    size_t bnzerodbl(size_t);
                    size_t call(size_t);
                    size_t ret(size_t);
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
                    size_t duptopi32(size_t);
                    size_t muli32(size_t);


                    text_t text_seg;
                    data_t data_seg;
                    stack_t stack_seg;

                    size_t stack_top;
                    using fn = size_t (VMEnv::*) (size_t) ;
                    vector<fn> op_dispatch;
                    map<string, size_t> label2loc;

                    static constexpr size_t base_opcode_width = 1;
            };

            size_t emitX(VMEnv::text_t& text);
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
                    text[ofs + 1] = t;
            }

            static map< string, 
                        tuple<Opcode,string, size_t (*)(VMEnv::text_t&), size_t (*)(VMEnv::text_t&, int) > > 
             mnemonics = {
             
                {"halt", {Opcode::halt,"Yields the processor.",emit<Opcode::halt>,nullptr} },
                {"noop", {Opcode::noop,"No operation.",emit<Opcode::noop>,nullptr}},
                {"ldi32",{Opcode::ldi32, "Push 32 bit signed integer.",nullptr,emit<Opcode::ldi32>} },
                {"ldsi32",{Opcode::ldsi32, "Push 32 bit signed integer.",emit<Opcode::ldsi32>,nullptr} },
                {"ldi64",{Opcode::ldi64, "",nullptr,emit<Opcode::ldi64>}},
                {"lddbl",{Opcode::lddbl, "",nullptr,emit<Opcode::lddbl>}},
                {"sti32",{Opcode::sti32, "",nullptr,emit<Opcode::sti32>}},
                {"stsi32",{Opcode::stsi32, "",emit<Opcode::stsi32>,nullptr}},
                {"sri64",{Opcode::sri64, "",nullptr,emit<Opcode::sri64>}},
                {"stdbl",{Opcode::stdbl, "",nullptr,emit<Opcode::stdbl>}},
                {"ldptr",{Opcode::ldptr, "",nullptr,emit<Opcode::ldptr>}},
                {"stptr",{Opcode::stptr, "",nullptr,emit<Opcode::stptr>}},
                {"lea",{Opcode::lea, "",nullptr,emit<Opcode::lea>}},
                {"duptopi32",{Opcode::duptopi32, "",emit<Opcode::duptopi32>,nullptr}},

                //Arithmetic
                
                {"addi32",{Opcode::addi32, "",emit<Opcode::addi32>,nullptr}},
                {"addi64",{Opcode::addi64, "",emit<Opcode::addi64>,nullptr}},
                {"adddbl",{Opcode::adddbl, "",emit<Opcode::adddbl>,nullptr}},
                {"subi32",{Opcode::subi32, "",emit<Opcode::subi32>,nullptr}},
                {"subi64",{Opcode::subi64, "",emit<Opcode::subi64>,nullptr}},
                {"subdbl",{Opcode::subdbl, "",emit<Opcode::subdbl>,nullptr}},

                {"muli32",{Opcode::muli32, "",emit<Opcode::muli32>,nullptr}},
                {"muli64",{Opcode::muli64, "",emit<Opcode::muli64>,nullptr}},
                {"muldbl",{Opcode::muldbl, "",emit<Opcode::muldbl>,nullptr}},
                {"divi32",{Opcode::divi32, "",emit<Opcode::divi32>,nullptr}},
                {"divi64",{Opcode::divi64, "",emit<Opcode::divi64>,nullptr}},
                {"divdbl",{Opcode::divdbl, "",emit<Opcode::divdbl>,nullptr}},
                {"remi32",{Opcode::remi32, "",emit<Opcode::remi32>,nullptr}},
                {"remi64",{Opcode::remi64, "",emit<Opcode::remi64>,nullptr}},
                {"lti32",{Opcode::lti32, "",emit<Opcode::lti32>,nullptr}},
                {"lti64",{Opcode::lti64, "",emit<Opcode::lti64>,nullptr}},
                {"ltdbl",{Opcode::ltdbl, "",emit<Opcode::ltdbl>,nullptr}},
                {"lteqi32",{Opcode::lteqi32, "",emit<Opcode::lteqi32>,nullptr}},
                {"lteqi64",{Opcode::lteqi64, "",emit<Opcode::lteqi64>,nullptr}},
                {"lteqdbl",{Opcode::lteqdbl, "",emit<Opcode::lteqdbl>,nullptr}},
                {"gti32",{Opcode::gti32, "",emit<Opcode::gti32>,nullptr}},
                {"gti64",{Opcode::gti64, "",emit<Opcode::gti64>,nullptr}},
                {"gtdbl",{Opcode::gtdbl, "",emit<Opcode::gtdbl>,nullptr}},
                {"gteqi32",{Opcode::gteqi32, "",emit<Opcode::gteqi32>,nullptr}},
                {"gteqi64",{Opcode::gteqi64, "",emit<Opcode::gteqi64>,nullptr}},
                {"gteqdbl",{Opcode::gteqdbl, "",emit<Opcode::gteqdbl>,nullptr}},
                {"eqi32",{Opcode::eqi32, "",emit<Opcode::eqi32>,nullptr}},
                {"eqi64",{Opcode::eqi64, "",emit<Opcode::eqi64>,nullptr}},
                {"eqdbl",{Opcode::eqdbl, "",emit<Opcode::eqdbl>,nullptr}},

                //Control Flow
                {"buc",{Opcode::buc, "",nullptr,emit<Opcode::buc>}},
                {"beq",{Opcode::beq, "",nullptr,emit<Opcode::beq>}},
                {"bneq",{Opcode::bneq, "",nullptr,emit<Opcode::bneq>}},
                {"blt",{Opcode::blt, "",nullptr,emit<Opcode::blt>}},
                {"blteq",{Opcode::blteq, "",nullptr,emit<Opcode::blteq>}},
                {"bgt",{Opcode::bgt, "",nullptr,emit<Opcode::bgt>}},
                {"bgteq",{Opcode::bgteq, "",nullptr,emit<Opcode::bgteq>}},
                {"bgteqzeroi32",{Opcode::bgteqzeroi32, "",nullptr,emit<Opcode::bgteqzeroi32>}},
                {"blteqzeroi32",{Opcode::blteqzeroi32, "",nullptr,emit<Opcode::blteqzeroi32>}},                
                {"bltzeroi32",{Opcode::bltzeroi32, "",nullptr,emit<Opcode::bltzeroi32>}},
                {"bzeroi32",{Opcode::bzeroi32, "",nullptr,emit<Opcode::bzeroi32>}},
                {"bnzeroi32",{Opcode::bnzeroi32, "",nullptr,emit<Opcode::bnzeroi32>}},
                {"bzeroi64",{Opcode::bzeroi64, "",nullptr,emit<Opcode::bzeroi64>}},
                {"bnzeroi64",{Opcode::bnzeroi64, "",nullptr,emit<Opcode::bnzeroi64>}},
                {"bzerodbl",{Opcode::bzerodbl, "",nullptr,emit<Opcode::bzerodbl>}},
                {"bnzerodbl",{Opcode::bnzerodbl, "",nullptr,emit<Opcode::bnzerodbl>}},
                {"call",{Opcode::call, "",nullptr,emit<Opcode::call>}},
                {"ret",{Opcode::ret, "",emit<Opcode::ret>,nullptr}},
                //Bitoperators
                {"andni32",{Opcode::andni32, "",emit<Opcode::andni32>,nullptr}},
                {"andni64",{Opcode::andni64, "",emit<Opcode::andni64>,nullptr}},
                {"andi32",{Opcode::andi32, "",emit<Opcode::andi32>,nullptr}},
                {"andi64",{Opcode::andi64, "",emit<Opcode::andi64>,nullptr}},
                {"ori32",{Opcode::ori32, "",emit<Opcode::ori32>,nullptr}},
                {"ori64",{Opcode::ori64, "",emit<Opcode::ori64>,nullptr}},
                {"noti32",{Opcode::noti32, "",emit<Opcode::noti32>,nullptr}},
                {"noti64",{Opcode::noti64, "",emit<Opcode::noti64>,nullptr}},
                {"xori32",{Opcode::xori32, "",emit<Opcode::xori32>,nullptr}},
                {"xori64",{Opcode::xori64, "",emit<Opcode::xori64>,nullptr}},
                {"muli32",{Opcode::muli32, "",emit<Opcode::muli32>,nullptr}},
                {"muli32",{Opcode::muldbl, "",emit<Opcode::muldbl>,nullptr}}

            };

 
        }
    }
}