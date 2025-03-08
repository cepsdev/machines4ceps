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
#include <optional>
#include <cstring>

namespace ceps{
    namespace vm{
        namespace oblectamenta{
            using namespace std;
            using addr_t = size_t;
            using base_opcode = uint32_t;
            static constexpr unsigned int max_opcode_width {16}; 
            enum class Opcode:base_opcode{
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
                swp,

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
                eqdbl,
                cpysi32,
                wrsi32,
                setframe,
                popi32,
                pushi32reg,
                popi32reg,
                pushi32,
                sti64,

                ui32toui64, // cast i32 to i64 without sign extension
                ldi64reg,
                sti64reg,
                stsi64,
                ldsi64,
                sindbl,
                cosdbl,
                tandbl,
                atandbl,
                expdbl,
                ldsdbl,
                negdbl,
                negi32,
                negi64,
                stsdbl,
                tanhdbl,
                dbg_printlni32,
                callx,
                lea_absolute,
                sti32reg,
                msg,
                dbg_print_cs_and_regsimm, //print compute stack
                dbg_print_dataimm, //print data section
                dbg_deserialize_protobufish_to_json, //deserialize and print message in protobufish format
                ldi32imm,
                popi64,
                pushi64reg,
                popi64reg,
                pushi64,
                dbg_print_stackimm, //print stack
                ldi64imm,
                duptopi64,
                discardtopi32,
                discardtopi64,
                ldi8,
                sti8,
                stsi8,
                ldsi8,
                ui8toui32, // cast ui8 to ui32
                ui8toui64,
                duptopi8, // duplicate top int8
                swpi64, // swap the two top 64 bit wide elements on the compute stack
                swpi16i64, // swap the top 16 bit wide element with the adjacent 64 bit element on the compute stack
            };

            class EventQueue{
                public:
                virtual void fire_event(int) = 0;
            };

            class VMEnv{
                public:
                    using stack_t = uint8_t*;
                    using data_t = uint8_t*;
                    using text_t = uint8_t*;
                    using compute_stack_t = vector<uint8_t>;

                    struct registers_t{
                        static constexpr uint32_t SP = 0;  // stack pointer
                        static constexpr uint32_t FP = 1;  // frame pointer
                        static constexpr uint32_t CSP = 2; // compute stack
                        static constexpr uint32_t PC = 3; // program counter 
                        static constexpr uint32_t ARG0 = 4; // register for first call argument (%rdi in SysV Amd64 ABI) 
                        static constexpr uint32_t ARG1 = 5; // register for 2nd call argument (%rsi in SysV Amd64 ABI) 
                        static constexpr uint32_t ARG2 = 6; // register for 3rd call argument (%rdx in SysV Amd64 ABI) 
                        static constexpr uint32_t ARG3 = 7; // register for 4th  call argument (%rcx in SysV Amd64 ABI) 
                        static constexpr uint32_t ARG4 = 8; // register for 5th call argument (%r8 in SysV Amd64 ABI) 
                        static constexpr uint32_t ARG5 = 9; // register for 6th call argument (%r9 in SysV Amd64 ABI) 

                        static constexpr uint32_t RES = 10; // register for result (%rax in SysV Amd64 ABI) 

                        int64_t file[11];
                        map<string, uint32_t> reg_mnemonic2idx { {"SP",SP},{"FP",FP}, {"CSP",CSP}, {"PC",PC} , {"ARG0",ARG0 } , {"ARG1",ARG1 } , 
                                                                 {"ARG2",ARG2 } , {"ARG3",ARG3 } , {"ARG4",ARG4 } , {"ARG5",ARG5 }, {"RES",RES } };
                    } registers;

                    using reg_t = uint32_t;
                    using reg_offs_t = int64_t;

                    /*

                    /----- static data ---\/--- dynamic data ----\/--- stack- --\
                    | ---------------------|----------------------|------------- |
                    |
                   base                    heap                   SP            end 

                    */
                    struct mem_t{
                        data_t base{};
                        data_t heap{};
                        stack_t end{};
                    } mem;

                    compute_stack_t compute_stack;
                    text_t text{};
                    size_t text_size{};
                    size_t text_loc{};
                    int lbl_counter{}; // used by assembler to generate unique labels

                    struct exfuncdescr_t{
                        void* addr;
                        string name;
                        unsigned short call_regs;
                        size_t stack_size;
                    };
                    std::vector<exfuncdescr_t> exfuncs;
                    std::vector<exfuncdescr_t>::iterator j;


                    static constexpr size_t default_mem_size {4096};
                    static constexpr size_t default_text_size {4096};

                    text_t resize_text(size_t new_size);

                    size_t store (string  data){
                        size_t t{};
                        for(auto e: data) t = store(e);
                        return t;
                    }
  
                    template<typename T> size_t store(T data){
                        auto t{mem.heap - mem.base};
                        new (mem.heap) T{data};
                        mem.heap += sizeof(data);                        
                        return t;
                    }
                    std::optional<size_t> reserve(size_t n){
                        size_t t = mem.heap - mem.base;
                        if ( (size_t)(mem.end - mem.heap) > n)
                         mem.heap += n;
                        else {
                            size_t new_size = mem.end - mem.base + n;
                            size_t heap_pos = mem.heap-mem.base;
                            auto new_base = new char[new_size];
                            if (!new_base) return {};
                            memcpy(new_base,mem.base, mem.end - mem.base);
                            delete mem.base;
                            mem.base = (data_t)new_base;
                            mem.heap = mem.base + heap_pos;
                            mem.end = mem.base + new_size;
                        }
                        return t;
                    }
  
                    template<typename T> T read_store(size_t ofs){
                        T t = *(T*) (mem.base + ofs);
                        return t;
                    }

                    template<typename T> size_t push_cs(T data){
                        auto t = registers.file[registers_t::CSP];
                        constexpr auto r{sizeof(T) % sizeof(compute_stack_t::value_type)};
                        auto space_needed {sizeof(T) / sizeof(compute_stack_t::value_type) + (r ? 1 : 0) };
                        if (registers.file[registers_t::CSP] + space_needed >= compute_stack.size() ) 
                         compute_stack.insert(compute_stack.end(), space_needed, {});
                        for (size_t i = 0; i < space_needed; ++i)
                         compute_stack[registers.file[registers_t::CSP] + i] = *((compute_stack_t::value_type*) &data + i);
                        registers.file[registers_t::CSP] += space_needed;
                        if(sizeof(T) < sizeof(compute_stack_t::value_type)){
                            for(auto i = 1; i < sizeof(T) - sizeof(compute_stack_t::value_type);++i)
                             ((char*)&compute_stack[registers.file[registers_t::CSP]-1])[i] = 0;
                        }
                        return t;
                    }
                    template<typename T> T pop_cs(){
                        T r;
                        size_t start = registers.file[registers_t::CSP] - sizeof(T) / sizeof(compute_stack_t::value_type);
                        for (size_t i = 0; i < sizeof(T) / sizeof(compute_stack_t::value_type); ++i)
                         *((compute_stack_t::value_type*) &r + i) = compute_stack[start+i];
                        registers.file[registers_t::CSP] = start;
                        return r;
                    }

                    template<typename T> T top(int i){
                        T r;
                        auto st{registers.file[registers_t::CSP] - i*sizeof(compute_stack_t::value_type)};
                        size_t start = st - sizeof(T) / sizeof(compute_stack_t::value_type);
                        for (size_t i = 0; i < sizeof(T) / sizeof(compute_stack_t::value_type); ++i)
                         *((compute_stack_t::value_type*) &r + i) = compute_stack[start+i];
                        return r;
                    }

                    template<typename T> size_t push_data_stack(T data){
                        auto t{registers.file[registers_t::SP]};
                        registers.file[registers_t::SP] -= sizeof(T);
                        for (size_t i = 0; i < sizeof(T); ++i)
                            *(mem.base + registers.file[registers_t::SP] + i ) = *((char*) &data + i);
                        return t;
                    }

                    template<typename T> T pop_data_stack(){
                        T r;
                        for (size_t i = 0; i < sizeof(T); ++i)
                            *((uint8_t*) &r + i) = *(mem.base + registers.file[registers_t::SP] + i );
                        registers.file[registers_t::SP] += sizeof(T);
                        return r;
                    }
                    void set_event_queue(EventQueue* new_event_queue) {event_queue = new_event_queue; }
                    size_t run(size_t start = 0);
                    VMEnv();
                    void dump(ostream& os);
                    void reset();
                    map<string, size_t>& data_labels(){return label2loc;}
                private:
                    EventQueue* event_queue = nullptr;
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
                    size_t swp(size_t);
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
                    size_t muli64(size_t);
                    size_t muldbl(size_t);
                    size_t divi32(size_t);
                    size_t divi64(size_t);
                    size_t divdbl(size_t);
                    size_t remi32(size_t);
                    size_t remi64(size_t);
                    size_t lti32(size_t);
                    size_t lti64(size_t);
                    size_t ltdbl(size_t);
                    size_t lteqi32(size_t);
                    size_t lteqi64(size_t);
                    size_t lteqdbl(size_t);
                    size_t gti32(size_t);
                    size_t gti64(size_t);
                    size_t gtdbl(size_t);
                    size_t gteqi32(size_t);
                    size_t gteqi64(size_t);
                    size_t gteqdbl(size_t);
                    size_t eqi32(size_t);
                    size_t eqi64(size_t);
                    size_t eqdbl(size_t);
                    size_t cpysi32(size_t);
                    size_t wrsi32(size_t);
                    size_t setframe(size_t);
                    size_t popi32(size_t);
                    size_t pushi32reg(size_t);
                    size_t popi32reg(size_t);
                    size_t pushi32(size_t);
                    size_t ui32toui64(size_t);
                    size_t ldi64reg(size_t);
                    size_t sti64reg(size_t);
                    size_t stsi64(size_t);
                    size_t ldsi64(size_t);

                    size_t sindbl(size_t);
                    size_t cosdbl(size_t);
                    size_t tandbl(size_t);
                    size_t atandbl(size_t);
                    size_t expdbl(size_t);
                    size_t ldsdbl(size_t);
                    size_t negdbl(size_t);
                    size_t negi32(size_t);
                    size_t negi64(size_t);
                    size_t stsdbl(size_t);
                    size_t tanhdbl(size_t);
                    size_t dbg_printlni32(size_t);
                    size_t callx(size_t);
                    size_t lea_absolute(size_t);
                    size_t sti32reg(size_t);
                    size_t msg(size_t);
                    size_t dbg_print_cs_and_regsimm(size_t);
                    size_t dbg_print_dataimm(size_t);
                    size_t dbg_deserialize_protobufish_to_json(size_t);
                    size_t ldi32imm(size_t);
                    size_t popi64(size_t);
                    size_t pushi64reg(size_t);
                    size_t popi64reg(size_t);
                    size_t pushi64(size_t);
                    size_t dbg_print_stackimm(size_t);
                    size_t ldi64imm(size_t);
                    size_t duptopi64(size_t);
                    size_t discardtopi32(size_t);
                    size_t discardtopi64(size_t);
                    size_t ldi8(size_t);
                    size_t sti8(size_t);
                    size_t stsi8(size_t);
                    size_t ldsi8(size_t);
                    size_t ui8toui32(size_t);
                    size_t ui8toui64(size_t);
                    size_t duptopi8(size_t);
                    size_t swpi64(size_t);
                    size_t swpi16i64(size_t);
    
                    using fn = size_t (VMEnv::*) (size_t) ;
                    vector<fn> op_dispatch;
                    map<string, size_t> label2loc;
                    static constexpr size_t base_opcode_width = sizeof(base_opcode);
                    friend bool copy_stack(VMEnv& from, VMEnv& to);;
            };
            bool copy_stack(VMEnv& from, VMEnv& to);
            bool copy_data(VMEnv& from, VMEnv& to);
            bool copy_compute_stack(VMEnv& from, VMEnv& to);

            template<Opcode opcode> size_t emit(VMEnv& vm,size_t pos){
                    *(base_opcode*)(vm.text + pos) = (base_opcode) opcode; 
                    return pos + sizeof(base_opcode);
            }

            template<Opcode opcode> size_t emit(VMEnv& vm,size_t pos, size_t v){
                    *(base_opcode*)(vm.text + pos) = (base_opcode) opcode; 
                    *(size_t*)(vm.text + pos + sizeof(base_opcode)) = v; 
                    return pos + sizeof(base_opcode) + sizeof(v);
            }

            template<Opcode opcode> size_t emit(VMEnv& vm,size_t pos, int v){
                    *(base_opcode*)(vm.text + pos) = (base_opcode) opcode; 
                    *(int*)(vm.text + pos + sizeof(base_opcode)) = v; 
                    return pos + sizeof(base_opcode) + sizeof(v);
            }

            template<Opcode opcode> size_t emit(VMEnv& vm ,size_t pos, VMEnv::reg_t reg, VMEnv::reg_offs_t reg_ofs){
                    *(base_opcode*)(vm.text + pos) = (base_opcode) opcode; 
                    *(VMEnv::reg_offs_t*)(vm.text + pos + sizeof(base_opcode)) = reg_ofs; 
                    *(VMEnv::reg_t*)(vm.text + pos + sizeof(base_opcode) + sizeof(VMEnv::reg_offs_t) ) = reg; 
                    return pos + sizeof(base_opcode) + sizeof(VMEnv::reg_t) + sizeof(VMEnv::reg_offs_t);
            }

            template<typename T> void patch(VMEnv& vm,size_t ofs, T t ){
                    *(T*)(vm.text + ofs) = t; 
            }

            static map< string, 
                        tuple<
                         Opcode,
                         string, 
                         size_t (*)(VMEnv& ,size_t), 
                         size_t (*)(VMEnv& ,size_t, addr_t),
                         size_t (*)(VMEnv& ,size_t, VMEnv::reg_t, VMEnv::reg_offs_t)
                        > 
                      > mnemonics = {
             
                {"halt", {Opcode::halt,"Yields the processor.",emit<Opcode::halt>,nullptr,nullptr} } ,
                {"noop", {Opcode::noop,"No operation.",emit<Opcode::noop>,nullptr,nullptr}},
                {"ldi32",{Opcode::ldi32, "Push 32 bit signed integer.",nullptr,emit<Opcode::ldi32>,nullptr} },
                {"ldsi32",{Opcode::ldsi32, "Push 32 bit signed integer.",emit<Opcode::ldsi32>,nullptr,nullptr} },
                {"ldi64",{Opcode::ldi64, "",nullptr,emit<Opcode::ldi64>,nullptr}},
                {"lddbl",{Opcode::lddbl, "",nullptr,emit<Opcode::lddbl>,nullptr}},
                {"sti32",{Opcode::sti32, "",nullptr,emit<Opcode::sti32>,nullptr}},
                {"stsi32",{Opcode::stsi32, "",emit<Opcode::stsi32>,nullptr,nullptr}},
                {"sri64",{Opcode::sri64, "",nullptr,emit<Opcode::sri64>,nullptr}},
                {"stdbl",{Opcode::stdbl, "",nullptr,emit<Opcode::stdbl>,nullptr}},
                {"ldptr",{Opcode::ldptr, "",nullptr,emit<Opcode::ldptr>,nullptr}},
                {"stptr",{Opcode::stptr, "",nullptr,emit<Opcode::stptr>,nullptr}},
                {"lea",{Opcode::lea, "",nullptr,emit<Opcode::lea>,nullptr}},
                {"duptopi32",{Opcode::duptopi32, "",emit<Opcode::duptopi32>,nullptr,nullptr}},

                //Arithmetic
                
                {"addi32",{Opcode::addi32, "",emit<Opcode::addi32>,nullptr,nullptr}},
                {"addi64",{Opcode::addi64, "",emit<Opcode::addi64>,nullptr,nullptr}},
                {"adddbl",{Opcode::adddbl, "",emit<Opcode::adddbl>,nullptr,nullptr}},
                {"subi32",{Opcode::subi32, "",emit<Opcode::subi32>,nullptr,nullptr}},
                {"subi64",{Opcode::subi64, "",emit<Opcode::subi64>,nullptr,nullptr}},
                {"subdbl",{Opcode::subdbl, "",emit<Opcode::subdbl>,nullptr,nullptr}},

                {"muli32",{Opcode::muli32, "",emit<Opcode::muli32>,nullptr,nullptr}},
                {"muli64",{Opcode::muli64, "",emit<Opcode::muli64>,nullptr,nullptr}},
                {"muldbl",{Opcode::muldbl, "",emit<Opcode::muldbl>,nullptr,nullptr}},
                {"divi32",{Opcode::divi32, "",emit<Opcode::divi32>,nullptr,nullptr}},
                {"divi64",{Opcode::divi64, "",emit<Opcode::divi64>,nullptr,nullptr}},
                {"divdbl",{Opcode::divdbl, "",emit<Opcode::divdbl>,nullptr,nullptr}},
                {"remi32",{Opcode::remi32, "",emit<Opcode::remi32>,nullptr,nullptr}},
                {"remi64",{Opcode::remi64, "",emit<Opcode::remi64>,nullptr,nullptr}},
                {"lti32",{Opcode::lti32, "",emit<Opcode::lti32>,nullptr,nullptr}},
                {"lti64",{Opcode::lti64, "",emit<Opcode::lti64>,nullptr,nullptr}},
                {"ltdbl",{Opcode::ltdbl, "",emit<Opcode::ltdbl>,nullptr,nullptr}},
                {"lteqi32",{Opcode::lteqi32, "",emit<Opcode::lteqi32>,nullptr,nullptr}},
                {"lteqi64",{Opcode::lteqi64, "",emit<Opcode::lteqi64>,nullptr,nullptr}},
                {"lteqdbl",{Opcode::lteqdbl, "",emit<Opcode::lteqdbl>,nullptr,nullptr}},
                {"gti32",{Opcode::gti32, "",emit<Opcode::gti32>,nullptr,nullptr}},
                {"gti64",{Opcode::gti64, "",emit<Opcode::gti64>,nullptr,nullptr}},
                {"gtdbl",{Opcode::gtdbl, "",emit<Opcode::gtdbl>,nullptr,nullptr}},
                {"gteqi32",{Opcode::gteqi32, "",emit<Opcode::gteqi32>,nullptr,nullptr}},
                {"gteqi64",{Opcode::gteqi64, "",emit<Opcode::gteqi64>,nullptr,nullptr}},
                {"gteqdbl",{Opcode::gteqdbl, "",emit<Opcode::gteqdbl>,nullptr,nullptr}},
                {"eqi32",{Opcode::eqi32, "",emit<Opcode::eqi32>,nullptr,nullptr}},
                {"eqi64",{Opcode::eqi64, "",emit<Opcode::eqi64>,nullptr,nullptr}},
                {"eqdbl",{Opcode::eqdbl, "",emit<Opcode::eqdbl>,nullptr,nullptr}},

                //Control Flow
                {"buc",{Opcode::buc, "",nullptr,emit<Opcode::buc>,nullptr}},
                {"beq",{Opcode::beq, "",nullptr,emit<Opcode::beq>,nullptr}},
                {"bneq",{Opcode::bneq, "",nullptr,emit<Opcode::bneq>,nullptr}},
                {"blt",{Opcode::blt, "",nullptr,emit<Opcode::blt>,nullptr}},
                {"blteq",{Opcode::blteq, "",nullptr,emit<Opcode::blteq>,nullptr}},
                {"bgt",{Opcode::bgt, "",nullptr,emit<Opcode::bgt>,nullptr}},
                {"bgteq",{Opcode::bgteq, "",nullptr,emit<Opcode::bgteq>,nullptr}},
                {"bgteqzeroi32",{Opcode::bgteqzeroi32, "",nullptr,emit<Opcode::bgteqzeroi32>,nullptr}},
                {"blteqzeroi32",{Opcode::blteqzeroi32, "",nullptr,emit<Opcode::blteqzeroi32>,nullptr}},                
                {"bltzeroi32",{Opcode::bltzeroi32, "",nullptr,emit<Opcode::bltzeroi32>,nullptr}},
                {"bzeroi32",{Opcode::bzeroi32, "",nullptr,emit<Opcode::bzeroi32>,nullptr}},
                {"bnzeroi32",{Opcode::bnzeroi32, "",nullptr,emit<Opcode::bnzeroi32>,nullptr}},
                {"bzeroi64",{Opcode::bzeroi64, "",nullptr,emit<Opcode::bzeroi64>,nullptr}},
                {"bnzeroi64",{Opcode::bnzeroi64, "",nullptr,emit<Opcode::bnzeroi64>,nullptr}},
                {"bzerodbl",{Opcode::bzerodbl, "",nullptr,emit<Opcode::bzerodbl>,nullptr}},
                {"bnzerodbl",{Opcode::bnzerodbl, "",nullptr,emit<Opcode::bnzerodbl>,nullptr}},
                {"call",{Opcode::call, "",nullptr,emit<Opcode::call>,nullptr}},
                {"ret",{Opcode::ret, "",emit<Opcode::ret>,nullptr,nullptr}},
                {"swp",{Opcode::swp, "",emit<Opcode::swp>,nullptr,nullptr}},

                //Bitoperators
                {"andni32",{Opcode::andni32, "",emit<Opcode::andni32>,nullptr,nullptr}},
                {"andni64",{Opcode::andni64, "",emit<Opcode::andni64>,nullptr,nullptr}},
                {"andi32",{Opcode::andi32, "",emit<Opcode::andi32>,nullptr,nullptr}},
                {"andi64",{Opcode::andi64, "",emit<Opcode::andi64>,nullptr,nullptr}},
                {"ori32",{Opcode::ori32, "",emit<Opcode::ori32>,nullptr,nullptr}},
                {"ori64",{Opcode::ori64, "",emit<Opcode::ori64>,nullptr,nullptr}},
                {"noti32",{Opcode::noti32, "",emit<Opcode::noti32>,nullptr,nullptr}},
                {"noti64",{Opcode::noti64, "",emit<Opcode::noti64>,nullptr,nullptr}},
                {"xori32",{Opcode::xori32, "",emit<Opcode::xori32>,nullptr,nullptr}},
                {"xori64",{Opcode::xori64, "",emit<Opcode::xori64>,nullptr,nullptr}},
                {"muli32",{Opcode::muli32, "",emit<Opcode::muli32>,nullptr,nullptr}},
                {"muli32",{Opcode::muldbl, "",emit<Opcode::muldbl>,nullptr,nullptr}},
                {"setframe",{Opcode::setframe, "",emit<Opcode::setframe>,nullptr,nullptr}},
                {"cpysi32",{Opcode::cpysi32, "",nullptr,emit<Opcode::cpysi32>,nullptr}},
                {"wrsi32",{Opcode::wrsi32, "",nullptr,emit<Opcode::wrsi32>,nullptr}},
                {"popi32",{Opcode::popi32, "",emit<Opcode::popi32>,nullptr,nullptr}},
                {"pushi32reg",{Opcode::pushi32reg, "",nullptr,emit<Opcode::pushi32reg>,nullptr}},
                {"popi32reg",{Opcode::popi32reg, "",nullptr,emit<Opcode::popi32reg>,nullptr}},
                {"pushi32",{Opcode::pushi32, "",emit<Opcode::pushi32>,nullptr,nullptr}},
                {"sti64",{Opcode::sti64, "",nullptr,emit<Opcode::sti64>,nullptr}},
                {"ui32toui64",{Opcode::ui32toui64, "",emit<Opcode::ui32toui64>,nullptr,nullptr}},
                {"ldi64reg",{Opcode::ldi64reg, "",nullptr,nullptr,emit<Opcode::ldi64reg> }},
                {"sti64reg",{Opcode::sti64reg, "",nullptr,nullptr,emit<Opcode::sti64reg> }},
                {"stsi64",{Opcode::stsi64, "",emit<Opcode::stsi64>, nullptr,nullptr }},
                {"ldsi64",{Opcode::ldsi64, "",emit<Opcode::ldsi64>, nullptr,nullptr }},

                {"sindbl",{Opcode::sindbl, "",emit<Opcode::sindbl>,nullptr,nullptr}},
                {"cosdbl",{Opcode::cosdbl, "",emit<Opcode::cosdbl>,nullptr,nullptr}},
                {"tandbl",{Opcode::tandbl, "",emit<Opcode::tandbl>,nullptr,nullptr}},
                {"atandbl",{Opcode::atandbl, "",emit<Opcode::atandbl>,nullptr,nullptr}},
                {"expdbl",{Opcode::expdbl, "",emit<Opcode::expdbl>,nullptr,nullptr}},
                {"ldsdbl",{Opcode::ldsdbl, "",emit<Opcode::ldsdbl>, nullptr,nullptr }},
                {"negdbl",{Opcode::negdbl, "",emit<Opcode::negdbl>,nullptr,nullptr}},
                {"negi32",{Opcode::negi32, "",emit<Opcode::negi32>,nullptr,nullptr}},
                {"negi64",{Opcode::negi64, "",emit<Opcode::negi64>,nullptr,nullptr}},
                {"stsdbl",{Opcode::stsdbl, "",emit<Opcode::stsdbl>, nullptr,nullptr }},
                {"tanhdbl",{Opcode::tanhdbl, "",emit<Opcode::tanhdbl>,nullptr,nullptr}},
                {"dbg_printlni32",{Opcode::dbg_printlni32, "",nullptr,emit<Opcode::dbg_printlni32>,nullptr} },
                {"callx",{Opcode::callx, "",nullptr,emit<Opcode::callx>,nullptr}},
                {"lea_absolute",{Opcode::lea_absolute, "",nullptr,emit<Opcode::lea_absolute>,nullptr}},
                {"sti32reg",{Opcode::sti32reg, "",nullptr,nullptr,emit<Opcode::sti32reg> }},
                {"msg",{Opcode::msg, "",nullptr,emit<Opcode::msg>,nullptr }},
                {"dbg_print_cs_and_regsimm",{Opcode::dbg_print_cs_and_regsimm, "",nullptr,emit<Opcode::dbg_print_cs_and_regsimm>,nullptr} },
                {"dbg_print_dataimm",{Opcode::dbg_print_dataimm, "",nullptr,emit<Opcode::dbg_print_dataimm>,nullptr} },
                {"dbg_deserialize_protobufish_to_json",{Opcode::dbg_deserialize_protobufish_to_json, "",nullptr,emit<Opcode::dbg_deserialize_protobufish_to_json>,nullptr} },
                {"ldi32imm",{Opcode::ldi32imm, "Push 32 bit signed integer immediate.",nullptr,emit<Opcode::ldi32imm>,nullptr} },
                
                {"popi64",{Opcode::popi64, "",emit<Opcode::popi64>,nullptr,nullptr}},
                {"pushi64reg",{Opcode::pushi64reg, "",nullptr,nullptr,emit<Opcode::pushi64reg>}},
                {"popi64reg",{Opcode::popi64reg, "",nullptr,nullptr,emit<Opcode::popi64reg>}},
                {"pushi64",{Opcode::pushi64, "",emit<Opcode::pushi64>,nullptr,nullptr}},
                {"dbg_print_stackimm",{Opcode::dbg_print_stackimm, "",nullptr,emit<Opcode::dbg_print_stackimm>,nullptr} },
                {"ldi64imm",{Opcode::ldi64imm, "Push 63 bit signed integer immediate.",nullptr,emit<Opcode::ldi64imm>,nullptr} },
                {"duptopi64",{Opcode::duptopi64, "",emit<Opcode::duptopi64>,nullptr,nullptr}},
                {"discardtopi32",{Opcode::discardtopi32, "",emit<Opcode::discardtopi32>,nullptr,nullptr}},
                {"discardtopi64",{Opcode::discardtopi64, "",emit<Opcode::discardtopi64>,nullptr,nullptr}},
                {"ldi8",{Opcode::ldi8, "Push 8 bit signed integer.",nullptr,emit<Opcode::ldi8>,nullptr} },
                {"sti8",{Opcode::sti8, "",nullptr,emit<Opcode::sti8>,nullptr}},
                {"stsi8",{Opcode::stsi8, "",emit<Opcode::stsi8>, nullptr,nullptr }},
                {"ldsi8",{Opcode::ldsi8, "",emit<Opcode::ldsi8>, nullptr,nullptr }},
                {"ui8toui32",{Opcode::ui8toui32, "",emit<Opcode::ui8toui32>,nullptr,nullptr}},
                {"ui8toui64",{Opcode::ui8toui64, "",emit<Opcode::ui8toui64>,nullptr,nullptr}},
                {"duptopi8",{Opcode::duptopi8, "",emit<Opcode::duptopi8>,nullptr,nullptr}},
                {"swpi64",{Opcode::swpi64, "",emit<Opcode::swpi64>,nullptr,nullptr}},
                {"swpi16i64",{Opcode::swpi16i64, "",emit<Opcode::swpi16i64>,nullptr,nullptr}}
            };
            
            #pragma pack(push,1)
            struct msg_node{
                static constexpr uint32_t ROOT = 1;
                static constexpr uint32_t NODE = 2;
                static constexpr uint32_t INT32 = 3;
                static constexpr uint32_t INT64 = 4;
                static constexpr uint32_t UINT32 = 5;
                static constexpr uint32_t UINT64 = 6;
                static constexpr uint32_t SZ = 7;
                uint32_t what;
                size_t size;
            };
            struct msg_node_ex:msg_node{
                static constexpr size_t MAX_NAME = 1024;
                char name[MAX_NAME];
            };
            struct msg_node_int32:msg_node{
                int32_t value;
            };
            struct msg_node_sz:msg_node{
                static constexpr size_t MAX_SZ = 0xFFFFFFFFFF;
                char value[MAX_SZ];
            };

            #pragma pack(pop)
        }//namespace oblectamenta
    }
}

std::ostream& operator << (std::ostream&, ceps::vm::oblectamenta::VMEnv::registers_t);