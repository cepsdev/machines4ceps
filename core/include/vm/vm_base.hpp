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
                halt,                     // Stop execution
                noop,                     // Do nothing 
                ldi32,                    // [ldi32(addr)] :=  *((int32_t*) CS) = *((int32_t*) addr); CS += sizeof(int32_t) 
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

                buc,                      // buc(label) = branch unconditionally to address of label
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
                ldsi8, // ADDR = pop64(CS), push8(*(int8_t*)ADDR)
                ui8toui32, // cast ui8 to ui32
                ui8toui64,
                duptopi8, // duplicate top int8
                swpi64, // swap the two top 64 bit wide elements on the compute stack
                swpi16i64, // swap the top 16 bit wide element with the adjacent 64 bit element on the compute stack
                swpi32i64, // swap the top 32 bit wide element with the adjacent 64 bit element on the compute stack
                swp128i64,// swap the top 128 bit wide element with the adjacent 64 bit element on the compute stack
                swp80i64,
                swpi64b72,
                swp72i64,
                swp128b8,
                lddblimm,
                duptopi128,
                swpi8i64,
                beqi8,
                bneqi8,
                bzeroi8,
                bnzeroi8,
                dbg_printlni32imm,
                swpi64b128,
                swpi160i64,
                swpi192i32,
                asserti32imm,
                assertf64imm,
                swp96i64,
                swp192i64,
                assertsz,
                assert_empty_cs,
                asserti64imm,
                asserti64immsz,
                duptopi192,
                swpi8i128,
                swpi16i128,
                dbg_print_topi64,
                haltimm,
                swpi64i192,
                asserti32immsz,
                assert_deserialized_protobufish_message_equals_str,
                dbg_print_topi32,
                asserteqi32,
                ldi32reg,
                dbg_printsz                
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

                    struct halt_triggered{};

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

                        static constexpr uint32_t R0 = 11;
                        static constexpr uint32_t R1 = R0 + 1;
                        static constexpr uint32_t R2 = R0 + 2;
                        static constexpr uint32_t R3 = R0 + 3;
                        static constexpr uint32_t R4 = R0 + 4;
                        static constexpr uint32_t R5 = R0 + 5;
                        static constexpr uint32_t R6 = R0 + 6;
                        static constexpr uint32_t R7 = R0 + 7;
                        static constexpr uint32_t R8 = R0 + 8;
                        static constexpr uint32_t R9 = R0 + 9;
                        static constexpr uint32_t R10 = R0 + 10;
                        static constexpr uint32_t R11 = R0 + 11;
                        static constexpr uint32_t R12 = R0 + 12;
                        static constexpr uint32_t R13 = R0 + 13;
                        static constexpr uint32_t R14 = R0 + 14;
                        static constexpr uint32_t R15 = R0 + 15;

                        int64_t file[27] = {};
                        map<string, uint32_t> reg_mnemonic2idx { {"SP",SP}, {"FP",FP}, {"CSP",CSP}, {"PC",PC} , 
                                                                 {"ARG0",ARG0 } , {"ARG1",ARG1 } , {"ARG2",ARG2 } , 
                                                                 {"ARG3",ARG3 } , {"ARG4",ARG4 } , {"ARG5",ARG5 }, 
                                                                 {"RES",RES }, {"R0",R0 } , {"R1",R1 }, {"R2",R2 },
                                                                 {"R3",R3 }, {"R4",R4 }, {"R5",R5 }, {"R6",R6 },
                                                                 {"R7",R7 }, {"R8",R8 }, {"R9",R9 }, {"R10",R10 },
                                                                 {"R11",R11 }, {"R12",R12 }, {"R13",R13 }, {"R14",R14 }, {"R15",R15 }
                                                                };
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
                    size_t text_loc{4}; // a valid code location can never be less than 4;
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
                    static constexpr size_t default_text_size {4*4096};

                    text_t resize_text(size_t new_size);

                    size_t store (string  data){
                        if (!data.length()) return mem.heap - mem.base;
                        auto t{reserve(data.length())};
                        if (!t){
                            throw std::runtime_error{"vm::store(: Out of Memory.)"};
                        }
                        memcpy(mem.base + *t, data.c_str(), data.length());
                        return *t;
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
                        if (registers.file[registers_t::CSP] <= 0)
                         throw std::runtime_error{"Invalid operation performed: Pop applied to empty computation stack."};
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
                    size_t swpi32i64(size_t);
                    size_t swp128i64(size_t);
                    size_t swp80i64(size_t);
                    size_t swpi64b72(size_t);
                    size_t swp72i64(size_t);
                    size_t swp128b8(size_t);
                    size_t lddblimm(size_t);
                    size_t duptopi128(size_t);
                    size_t swpi8i64(size_t);
                    size_t beqi8(size_t);
                    size_t bneqi8(size_t);
                    size_t bzeroi8(size_t);
                    size_t bnzeroi8(size_t);
                    size_t dbg_printlni32imm(size_t);
                    size_t swpi64b128(size_t);
                    size_t swpi160i64(size_t);
                    size_t swpi192i32(size_t);
                    size_t asserti32imm(size_t);
                    size_t assertf64imm(size_t);
                    size_t swp96i64(size_t);
                    size_t swp192i64(size_t);
                    size_t assertsz(size_t);
                    size_t assert_empty_cs(size_t);
                    size_t asserti64imm(size_t);
                    size_t asserti64immsz(size_t);
                    size_t duptopi192(size_t);
                    size_t swpi8i128(size_t);
                    size_t swpi16i128(size_t);
                    size_t dbg_print_topi64(size_t);
                    size_t haltimm(size_t);
                    size_t swpi64i192(size_t);
                    size_t asserti32immsz(size_t);
                    size_t assert_deserialized_protobufish_message_equals_str(size_t);
                    size_t dbg_print_topi32(size_t);
                    size_t asserteqi32sz(size_t);
                    size_t ldi32reg(size_t);
                    size_t dbg_printsz(size_t);

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
            template<Opcode opcode> size_t emit(VMEnv& vm,size_t pos, size_t v, size_t vv){
                    *(base_opcode*)(vm.text + pos) = (base_opcode) opcode; 
                    *(size_t*)(vm.text + pos + sizeof(base_opcode)) = v; 
                    *(size_t*)(vm.text + pos + sizeof(base_opcode)+ sizeof(size_t)) = vv; 
                    return pos + sizeof(base_opcode) + sizeof(v) + sizeof(vv);
            }

            template<Opcode opcode> size_t emit(VMEnv& vm,size_t pos, double v){
                *(base_opcode*)(vm.text + pos) = (base_opcode) opcode; 
                *(double*)(vm.text + pos + sizeof(base_opcode)) = v; 
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

            constexpr int MNEM_NOARG       = 2;
            constexpr int MNEM_IMM         = 3;
            constexpr int MNEM_REG_REGOFS  = 4;
            constexpr int MNEM_DBL         = 5;
            constexpr int MNEM_IMM_IMM     = 6;

            static map< string, 
                        tuple<
                         Opcode,
                         string, 
                         size_t (*)(VMEnv& ,size_t), 
                         size_t (*)(VMEnv& ,size_t, addr_t),
                         size_t (*)(VMEnv& ,size_t, VMEnv::reg_t, VMEnv::reg_offs_t),
                         size_t (*)(VMEnv& ,size_t, double),
                         size_t (*)(VMEnv& ,size_t, addr_t, addr_t)
                        > 
                      > mnemonics = {
             
                {"halt", {Opcode::halt,"Yields the processor.",emit<Opcode::halt>,nullptr,nullptr,nullptr,nullptr} } ,
                {"noop", {Opcode::noop,"No operation.",emit<Opcode::noop>,nullptr,nullptr,nullptr,nullptr}},
                {"ldi32",{Opcode::ldi32, "Push 32 bit signed integer.",nullptr,emit<Opcode::ldi32>,nullptr,nullptr,nullptr} },
                {"ldsi32",{Opcode::ldsi32, "Push 32 bit signed integer.",emit<Opcode::ldsi32>,nullptr,nullptr,nullptr,nullptr} },
                {"ldi64",{Opcode::ldi64, "",nullptr,emit<Opcode::ldi64>,nullptr,nullptr,nullptr}},
                {"lddbl",{Opcode::lddbl, "",nullptr,emit<Opcode::lddbl>,nullptr,nullptr,nullptr}},
                {"sti32",{Opcode::sti32, "",nullptr,emit<Opcode::sti32>,nullptr,nullptr,nullptr}},
                {"stsi32",{Opcode::stsi32, "",emit<Opcode::stsi32>,nullptr,nullptr,nullptr,nullptr}},
                {"sri64",{Opcode::sri64, "",nullptr,emit<Opcode::sri64>,nullptr,nullptr,nullptr}},
                {"stdbl",{Opcode::stdbl, "",nullptr,emit<Opcode::stdbl>,nullptr,nullptr,nullptr}},
                {"ldptr",{Opcode::ldptr, "",nullptr,emit<Opcode::ldptr>,nullptr,nullptr,nullptr}},
                {"stptr",{Opcode::stptr, "",nullptr,emit<Opcode::stptr>,nullptr,nullptr,nullptr}},
                {"lea",{Opcode::lea, "",nullptr,emit<Opcode::lea>,nullptr,nullptr,nullptr}},
                {"duptopi32",{Opcode::duptopi32, "",emit<Opcode::duptopi32>,nullptr,nullptr,nullptr,nullptr}},

                //Arithmetic
                
                {"addi32",{Opcode::addi32, "",emit<Opcode::addi32>,nullptr,nullptr,nullptr,nullptr}},
                {"addi64",{Opcode::addi64, "",emit<Opcode::addi64>,nullptr,nullptr,nullptr,nullptr}},
                {"adddbl",{Opcode::adddbl, "",emit<Opcode::adddbl>,nullptr,nullptr,nullptr,nullptr}},
                {"subi32",{Opcode::subi32, "",emit<Opcode::subi32>,nullptr,nullptr,nullptr,nullptr}},
                {"subi64",{Opcode::subi64, "",emit<Opcode::subi64>,nullptr,nullptr,nullptr,nullptr}},
                {"subdbl",{Opcode::subdbl, "",emit<Opcode::subdbl>,nullptr,nullptr,nullptr,nullptr}},

                {"muli32",{Opcode::muli32, "",emit<Opcode::muli32>,nullptr,nullptr,nullptr,nullptr}},
                {"muli64",{Opcode::muli64, "",emit<Opcode::muli64>,nullptr,nullptr,nullptr,nullptr}},
                {"muldbl",{Opcode::muldbl, "",emit<Opcode::muldbl>,nullptr,nullptr,nullptr,nullptr}},
                {"divi32",{Opcode::divi32, "",emit<Opcode::divi32>,nullptr,nullptr,nullptr,nullptr}},
                {"divi64",{Opcode::divi64, "",emit<Opcode::divi64>,nullptr,nullptr,nullptr,nullptr}},
                {"divdbl",{Opcode::divdbl, "",emit<Opcode::divdbl>,nullptr,nullptr,nullptr,nullptr}},
                {"remi32",{Opcode::remi32, "",emit<Opcode::remi32>,nullptr,nullptr,nullptr,nullptr}},
                {"remi64",{Opcode::remi64, "",emit<Opcode::remi64>,nullptr,nullptr,nullptr,nullptr}},
                {"lti32",{Opcode::lti32, "",emit<Opcode::lti32>,nullptr,nullptr,nullptr,nullptr}},
                {"lti64",{Opcode::lti64, "",emit<Opcode::lti64>,nullptr,nullptr,nullptr,nullptr}},
                {"ltdbl",{Opcode::ltdbl, "",emit<Opcode::ltdbl>,nullptr,nullptr,nullptr,nullptr}},
                {"lteqi32",{Opcode::lteqi32, "",emit<Opcode::lteqi32>,nullptr,nullptr,nullptr,nullptr}},
                {"lteqi64",{Opcode::lteqi64, "",emit<Opcode::lteqi64>,nullptr,nullptr,nullptr,nullptr}},
                {"lteqdbl",{Opcode::lteqdbl, "",emit<Opcode::lteqdbl>,nullptr,nullptr,nullptr,nullptr}},
                {"gti32",{Opcode::gti32, "",emit<Opcode::gti32>,nullptr,nullptr,nullptr,nullptr}},
                {"gti64",{Opcode::gti64, "",emit<Opcode::gti64>,nullptr,nullptr,nullptr,nullptr}},
                {"gtdbl",{Opcode::gtdbl, "",emit<Opcode::gtdbl>,nullptr,nullptr,nullptr,nullptr}},
                {"gteqi32",{Opcode::gteqi32, "",emit<Opcode::gteqi32>,nullptr,nullptr,nullptr,nullptr}},
                {"gteqi64",{Opcode::gteqi64, "",emit<Opcode::gteqi64>,nullptr,nullptr,nullptr,nullptr}},
                {"gteqdbl",{Opcode::gteqdbl, "",emit<Opcode::gteqdbl>,nullptr,nullptr,nullptr,nullptr}},
                {"eqi32",{Opcode::eqi32, "",emit<Opcode::eqi32>,nullptr,nullptr,nullptr,nullptr}},
                {"eqi64",{Opcode::eqi64, "",emit<Opcode::eqi64>,nullptr,nullptr,nullptr,nullptr}},
                {"eqdbl",{Opcode::eqdbl, "",emit<Opcode::eqdbl>,nullptr,nullptr,nullptr,nullptr}},

                //Control Flow
                {"buc",{Opcode::buc, "",nullptr,emit<Opcode::buc>,nullptr,nullptr,nullptr}},
                {"beq",{Opcode::beq, "",nullptr,emit<Opcode::beq>,nullptr,nullptr,nullptr}},
                {"bneq",{Opcode::bneq, "",nullptr,emit<Opcode::bneq>,nullptr,nullptr,nullptr}},
                {"blt",{Opcode::blt, "",nullptr,emit<Opcode::blt>,nullptr,nullptr,nullptr}},
                {"blteq",{Opcode::blteq, "",nullptr,emit<Opcode::blteq>,nullptr,nullptr,nullptr}},
                {"bgt",{Opcode::bgt, "",nullptr,emit<Opcode::bgt>,nullptr,nullptr,nullptr}},
                {"bgteq",{Opcode::bgteq, "",nullptr,emit<Opcode::bgteq>,nullptr,nullptr,nullptr}},
                {"bgteqzeroi32",{Opcode::bgteqzeroi32, "",nullptr,emit<Opcode::bgteqzeroi32>,nullptr,nullptr,nullptr}},
                {"blteqzeroi32",{Opcode::blteqzeroi32, "",nullptr,emit<Opcode::blteqzeroi32>,nullptr,nullptr,nullptr}},                
                {"bltzeroi32",{Opcode::bltzeroi32, "",nullptr,emit<Opcode::bltzeroi32>,nullptr,nullptr,nullptr}},
                {"bzeroi32",{Opcode::bzeroi32, "",nullptr,emit<Opcode::bzeroi32>,nullptr,nullptr,nullptr}},
                {"bnzeroi32",{Opcode::bnzeroi32, "",nullptr,emit<Opcode::bnzeroi32>,nullptr,nullptr,nullptr}},
                {"bzeroi64",{Opcode::bzeroi64, "",nullptr,emit<Opcode::bzeroi64>,nullptr,nullptr,nullptr}},
                {"bnzeroi64",{Opcode::bnzeroi64, "",nullptr,emit<Opcode::bnzeroi64>,nullptr,nullptr,nullptr}},
                {"bzerodbl",{Opcode::bzerodbl, "",nullptr,emit<Opcode::bzerodbl>,nullptr,nullptr,nullptr}},
                {"bnzerodbl",{Opcode::bnzerodbl, "",nullptr,emit<Opcode::bnzerodbl>,nullptr,nullptr,nullptr}},
                {"call",{Opcode::call, "",nullptr,emit<Opcode::call>,nullptr,nullptr,nullptr}},
                {"ret",{Opcode::ret, "",emit<Opcode::ret>,nullptr,nullptr,nullptr,nullptr}},
                {"swp",{Opcode::swp, "",emit<Opcode::swp>,nullptr,nullptr,nullptr,nullptr}},

                //Bitoperators
                {"andni32",{Opcode::andni32, "",emit<Opcode::andni32>,nullptr,nullptr,nullptr,nullptr}},
                {"andni64",{Opcode::andni64, "",emit<Opcode::andni64>,nullptr,nullptr,nullptr,nullptr}},
                {"andi32",{Opcode::andi32, "",emit<Opcode::andi32>,nullptr,nullptr,nullptr,nullptr}},
                {"andi64",{Opcode::andi64, "",emit<Opcode::andi64>,nullptr,nullptr,nullptr,nullptr}},
                {"ori32",{Opcode::ori32, "",emit<Opcode::ori32>,nullptr,nullptr,nullptr,nullptr}},
                {"ori64",{Opcode::ori64, "",emit<Opcode::ori64>,nullptr,nullptr,nullptr,nullptr}},
                {"noti32",{Opcode::noti32, "",emit<Opcode::noti32>,nullptr,nullptr,nullptr,nullptr}},
                {"noti64",{Opcode::noti64, "",emit<Opcode::noti64>,nullptr,nullptr,nullptr,nullptr}},
                {"xori32",{Opcode::xori32, "",emit<Opcode::xori32>,nullptr,nullptr,nullptr,nullptr}},
                {"xori64",{Opcode::xori64, "",emit<Opcode::xori64>,nullptr,nullptr,nullptr,nullptr}},
                {"muli32",{Opcode::muli32, "",emit<Opcode::muli32>,nullptr,nullptr,nullptr,nullptr}},
                {"muli32",{Opcode::muldbl, "",emit<Opcode::muldbl>,nullptr,nullptr,nullptr,nullptr}},
                {"setframe",{Opcode::setframe, "",emit<Opcode::setframe>,nullptr,nullptr,nullptr,nullptr}},
                {"cpysi32",{Opcode::cpysi32, "",nullptr,emit<Opcode::cpysi32>,nullptr,nullptr,nullptr}},
                {"wrsi32",{Opcode::wrsi32, "",nullptr,emit<Opcode::wrsi32>,nullptr,nullptr,nullptr}},
                {"popi32",{Opcode::popi32, "",emit<Opcode::popi32>,nullptr,nullptr,nullptr,nullptr}},
                {"pushi32reg",{Opcode::pushi32reg, "",nullptr,emit<Opcode::pushi32reg>,nullptr,nullptr,nullptr}},
                {"popi32reg",{Opcode::popi32reg, "",nullptr,emit<Opcode::popi32reg>,nullptr,nullptr,nullptr}},
                {"pushi32",{Opcode::pushi32, "",emit<Opcode::pushi32>,nullptr,nullptr,nullptr,nullptr}},
                {"sti64",{Opcode::sti64, "",nullptr,emit<Opcode::sti64>,nullptr,nullptr,nullptr}},
                {"ui32toui64",{Opcode::ui32toui64, "",emit<Opcode::ui32toui64>,nullptr,nullptr,nullptr,nullptr}},
                {"ldi64reg",{Opcode::ldi64reg, "",nullptr,nullptr,emit<Opcode::ldi64reg> ,nullptr,nullptr}},
                {"sti64reg",{Opcode::sti64reg, "",nullptr,nullptr,emit<Opcode::sti64reg> ,nullptr,nullptr}},
                {"stsi64",{Opcode::stsi64, "",emit<Opcode::stsi64>, nullptr,nullptr ,nullptr,nullptr}},
                {"ldsi64",{Opcode::ldsi64, "",emit<Opcode::ldsi64>, nullptr,nullptr ,nullptr,nullptr}},

                {"sindbl",{Opcode::sindbl, "",emit<Opcode::sindbl>,nullptr,nullptr,nullptr,nullptr}},
                {"cosdbl",{Opcode::cosdbl, "",emit<Opcode::cosdbl>,nullptr,nullptr,nullptr,nullptr}},
                {"tandbl",{Opcode::tandbl, "",emit<Opcode::tandbl>,nullptr,nullptr,nullptr,nullptr}},
                {"atandbl",{Opcode::atandbl, "",emit<Opcode::atandbl>,nullptr,nullptr,nullptr,nullptr}},
                {"expdbl",{Opcode::expdbl, "",emit<Opcode::expdbl>,nullptr,nullptr,nullptr,nullptr}},
                {"ldsdbl",{Opcode::ldsdbl, "",emit<Opcode::ldsdbl>, nullptr,nullptr ,nullptr,nullptr}},
                {"negdbl",{Opcode::negdbl, "",emit<Opcode::negdbl>,nullptr,nullptr,nullptr,nullptr}},
                {"negi32",{Opcode::negi32, "",emit<Opcode::negi32>,nullptr,nullptr,nullptr,nullptr}},
                {"negi64",{Opcode::negi64, "",emit<Opcode::negi64>,nullptr,nullptr,nullptr,nullptr}},
                {"stsdbl",{Opcode::stsdbl, "",emit<Opcode::stsdbl>, nullptr,nullptr,nullptr,nullptr}},
                {"tanhdbl",{Opcode::tanhdbl, "",emit<Opcode::tanhdbl>,nullptr,nullptr,nullptr,nullptr}},
                {"dbg_printlni32",{Opcode::dbg_printlni32, "",nullptr,emit<Opcode::dbg_printlni32>,nullptr,nullptr,nullptr} },
                {"callx",{Opcode::callx, "",nullptr,emit<Opcode::callx>,nullptr,nullptr,nullptr}},
                {"lea_absolute",{Opcode::lea_absolute, "",nullptr,emit<Opcode::lea_absolute>,nullptr,nullptr,nullptr}},
                {"sti32reg",{Opcode::sti32reg, "",nullptr,nullptr,emit<Opcode::sti32reg>,nullptr,nullptr}},
                {"msg",{Opcode::msg, "",nullptr,emit<Opcode::msg>,nullptr,nullptr,nullptr}},
                {"dbg_print_cs_and_regsimm",{Opcode::dbg_print_cs_and_regsimm, "",nullptr,emit<Opcode::dbg_print_cs_and_regsimm>,nullptr,nullptr,nullptr} },
                {"dbg_print_dataimm",{Opcode::dbg_print_dataimm, "",nullptr,emit<Opcode::dbg_print_dataimm>,nullptr,nullptr,nullptr} },
                {"dbg_deserialize_protobufish_to_json",{Opcode::dbg_deserialize_protobufish_to_json, "",nullptr,emit<Opcode::dbg_deserialize_protobufish_to_json>,nullptr,nullptr,nullptr} },
                {"ldi32imm",{Opcode::ldi32imm, "Push 32 bit signed integer immediate.",nullptr,emit<Opcode::ldi32imm>,nullptr,nullptr,nullptr} },
                
                {"popi64",{Opcode::popi64, "",emit<Opcode::popi64>,nullptr,nullptr,nullptr,nullptr}},
                {"pushi64reg",{Opcode::pushi64reg, "",nullptr,nullptr,emit<Opcode::pushi64reg>,nullptr,nullptr}},
                {"popi64reg",{Opcode::popi64reg, "",nullptr,nullptr,emit<Opcode::popi64reg>,nullptr,nullptr}},
                {"pushi64",{Opcode::pushi64, "",emit<Opcode::pushi64>,nullptr,nullptr,nullptr,nullptr}},
                {"dbg_print_stackimm",{Opcode::dbg_print_stackimm, "",nullptr,emit<Opcode::dbg_print_stackimm>,nullptr,nullptr,nullptr} },
                {"ldi64imm",{Opcode::ldi64imm, "Push 63 bit signed integer immediate.",nullptr,emit<Opcode::ldi64imm>,nullptr,nullptr,nullptr} },
                {"duptopi64",{Opcode::duptopi64, "",emit<Opcode::duptopi64>,nullptr,nullptr,nullptr,nullptr}},
                {"discardtopi32",{Opcode::discardtopi32, "",emit<Opcode::discardtopi32>,nullptr,nullptr,nullptr,nullptr}},
                {"discardtopi64",{Opcode::discardtopi64, "",emit<Opcode::discardtopi64>,nullptr,nullptr,nullptr,nullptr}},
                {"ldi8",{Opcode::ldi8, "Push 8 bit signed integer.",nullptr,emit<Opcode::ldi8>,nullptr,nullptr,nullptr} },
                {"sti8",{Opcode::sti8, "",nullptr,emit<Opcode::sti8>,nullptr,nullptr,nullptr}},
                {"stsi8",{Opcode::stsi8, "",emit<Opcode::stsi8>, nullptr,nullptr,nullptr,nullptr}},
                {"ldsi8",{Opcode::ldsi8, "",emit<Opcode::ldsi8>, nullptr,nullptr,nullptr,nullptr}},
                {"ui8toui32",{Opcode::ui8toui32, "",emit<Opcode::ui8toui32>,nullptr,nullptr,nullptr,nullptr}},
                {"ui8toui64",{Opcode::ui8toui64, "",emit<Opcode::ui8toui64>,nullptr,nullptr,nullptr,nullptr}},
                {"duptopi8",{Opcode::duptopi8, "",emit<Opcode::duptopi8>,nullptr,nullptr,nullptr,nullptr}},
                {"swpi64",{Opcode::swpi64, "",emit<Opcode::swpi64>,nullptr,nullptr,nullptr,nullptr}},
                {"swpi16i64",{Opcode::swpi16i64, "",emit<Opcode::swpi16i64>,nullptr,nullptr,nullptr,nullptr}},
                {"swpi32i64",{Opcode::swpi32i64, "",emit<Opcode::swpi32i64>,nullptr,nullptr,nullptr,nullptr}},
                {"swp128i64",{Opcode::swp128i64, "",emit<Opcode::swp128i64>,nullptr,nullptr,nullptr,nullptr}},
                {"swp80i64",{Opcode::swp80i64, "",emit<Opcode::swp80i64>,nullptr,nullptr,nullptr,nullptr}},
                {"swpi64b72",{Opcode::swpi64b72, "",emit<Opcode::swpi64b72>,nullptr,nullptr,nullptr,nullptr}},
                {"swp72i64",{Opcode::swp72i64, "",emit<Opcode::swp72i64>,nullptr,nullptr,nullptr,nullptr}},
                {"swp128b8",{Opcode::swp128b8, "",emit<Opcode::swp128b8>,nullptr,nullptr,nullptr,nullptr}},
                {"lddblimm",{Opcode::lddblimm, "",nullptr,nullptr,nullptr,emit<Opcode::lddblimm>,nullptr}},
                {"duptopi128",{Opcode::duptopi128, "",emit<Opcode::duptopi128>,nullptr,nullptr,nullptr,nullptr}},
                {"swpi8i64",{Opcode::swpi8i64, "",emit<Opcode::swpi8i64>,nullptr,nullptr,nullptr,nullptr}},
                {"beqi8",{Opcode::beqi8, "",nullptr,emit<Opcode::beqi8>,nullptr,nullptr,nullptr}},
                {"bneqi8",{Opcode::bneqi8, "",nullptr,emit<Opcode::bneqi8>,nullptr,nullptr,nullptr}},
                {"bzeroi8",{Opcode::bzeroi8, "",nullptr,emit<Opcode::bzeroi8>,nullptr,nullptr,nullptr}},
                {"bnzeroi8",{Opcode::bnzeroi8, "",nullptr,emit<Opcode::bnzeroi8>,nullptr,nullptr,nullptr}},
                {"dbg_printlni32imm",{Opcode::dbg_printlni32imm, "",nullptr,emit<Opcode::dbg_printlni32imm>,nullptr,nullptr,nullptr}},
                {"swpi64b128",{Opcode::swpi64b128, "",emit<Opcode::swpi64b128>,nullptr,nullptr,nullptr,nullptr}},
                {"swpi160i64",{Opcode::swpi160i64, "",emit<Opcode::swpi160i64>,nullptr,nullptr,nullptr,nullptr}},
                {"swpi192i32",{Opcode::swpi192i32, "",emit<Opcode::swpi192i32>,nullptr,nullptr,nullptr,nullptr}},
                {"asserti32imm",{Opcode::asserti32imm, "",nullptr,emit<Opcode::asserti32imm>,nullptr,nullptr,nullptr} },
                {"assertf64imm",{Opcode::assertf64imm, "",nullptr,nullptr,nullptr,emit<Opcode::assertf64imm>,nullptr} },
                {"swp96i64",{Opcode::swp96i64, "",emit<Opcode::swp96i64>,nullptr,nullptr,nullptr,nullptr}},
                {"swp192i64",{Opcode::swp192i64, "",emit<Opcode::swp192i64>,nullptr,nullptr,nullptr,nullptr}},
                {"assertszsz",{Opcode::assertsz, "",nullptr,emit<Opcode::assertsz>,nullptr,nullptr,nullptr}},
                {"assert_empty_cs",{Opcode::assert_empty_cs, "",emit<Opcode::assert_empty_cs>,nullptr,nullptr,nullptr,nullptr}},
                {"asserti64imm",{Opcode::asserti64imm, "",nullptr,emit<Opcode::asserti64imm>,nullptr,nullptr,nullptr} },
                {"asserti64immsz",{Opcode::asserti64immsz, "",nullptr,nullptr,nullptr,nullptr,emit<Opcode::asserti64immsz>} },
                {"duptopi192",{Opcode::duptopi192, "",emit<Opcode::duptopi192>,nullptr,nullptr,nullptr,nullptr}},
                {"swpi8i128",{Opcode::swpi8i128, "",emit<Opcode::swpi8i128>,nullptr,nullptr,nullptr,nullptr}},
                {"swpi16i128",{Opcode::swpi16i128, "",emit<Opcode::swpi16i128>,nullptr,nullptr,nullptr,nullptr}},
                {"dbg_print_topi64",{Opcode::dbg_print_topi64, "",emit<Opcode::dbg_print_topi64>,nullptr,nullptr,nullptr,nullptr}},
                {"haltimm",{Opcode::haltimm, "",nullptr,emit<Opcode::haltimm>,nullptr,nullptr,nullptr} },
                {"swpi64i192",{Opcode::swpi64i192, "",emit<Opcode::swpi64i192>,nullptr,nullptr,nullptr,nullptr}},
                {"asserti32immsz",{Opcode::asserti32immsz, "",nullptr,nullptr,nullptr,nullptr,emit<Opcode::asserti32immsz>}},
                {"assert_deserialized_protobufish_message_equals_strsz@OblectamentaDataLabel@",{Opcode::assert_deserialized_protobufish_message_equals_str, "",nullptr,nullptr,nullptr,nullptr,emit<Opcode::assert_deserialized_protobufish_message_equals_str>}},
                {"dbg_print_topi32",{Opcode::dbg_print_topi32, "",emit<Opcode::dbg_print_topi32>,nullptr,nullptr,nullptr,nullptr}},               
                {"asserteqi32sz",{Opcode::asserteqi32, "",nullptr,emit<Opcode::asserteqi32>,nullptr,nullptr,nullptr} },
                {"ldi32reg",{Opcode::ldi32reg, "",nullptr,nullptr,emit<Opcode::ldi32reg> ,nullptr,nullptr}},
                {"dbg_printsz@OblectamentaDataLabel@",{Opcode::dbg_printsz, "",nullptr,emit<Opcode::dbg_printsz>,nullptr,nullptr,nullptr}}                
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
                static constexpr uint32_t F64 = 8;
                static constexpr uint32_t ARRAY = 9;
                static constexpr uint32_t SCOPE = 10;
                uint32_t what;
                size_t size;
            };
            struct msg_node_ex : msg_node{
                static constexpr size_t MAX_NAME = 1024;
                char name[MAX_NAME];
            };
            struct msg_node_int32:msg_node{
                int32_t value;
            };
            struct msg_node_int64:msg_node{
                int64_t value;
            };
            struct msg_node_f64:msg_node{
                double value;
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