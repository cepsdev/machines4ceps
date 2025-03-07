/*
Copyright 2022 Tomas Prerovsky (cepsdev@hotmail.com).

Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file excepregt in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include "core/include/vm/vm_base.hpp"
#include <cstring>
#include <cmath>
#include <iomanip>
namespace ceps::vm::oblectamenta{
    size_t deserialize_event_payload(char* buffer, size_t size, std::string& res);
    
   bool copy_stack(VMEnv& from, VMEnv& to){
        auto from_stack_size{from.mem.end - from.mem.base - from.registers.file[VMEnv::registers_t::SP] };
        if (to.mem.end - to.mem.base < from_stack_size ) return false;
        memcpy( to.mem.base + (to.mem.end - to.mem.base - from_stack_size),
                from.mem.base + from.registers.file[VMEnv::registers_t::SP],
                from_stack_size
                );
        to.registers.file[VMEnv::registers_t::SP] = to.mem.end - to.mem.base - from_stack_size;
        return true;
   }

   bool copy_data(VMEnv& from, VMEnv& to){
        auto from_data_size{from.mem.heap - from.mem.base  };
        if (to.mem.end - to.mem.base < from_data_size ) return false;
        memcpy( to.mem.base  ,
                from.mem.base ,
                from_data_size
                );
        to.mem.heap = to.mem.base + from_data_size;
        to.data_labels() = from.data_labels();
        return true;
   }
    
   bool copy_compute_stack(VMEnv& from, VMEnv& to){
        to.registers.file[VMEnv::registers_t::CSP] = from.registers.file[VMEnv::registers_t::CSP];
        to.compute_stack = from.compute_stack;
        return true;
   }

   size_t VMEnv::run(size_t start){
        registers.file[registers_t::PC] = start;
        for(; (Opcode) (*(base_opcode*)(text + registers.file[registers_t::PC] )) != Opcode::halt;){
            registers.file[registers_t::PC]  = 
            (this ->*op_dispatch[ text[registers.file[registers_t::PC]] ])(registers.file[registers_t::PC] );
        }
        return start;
    }
    
    size_t VMEnv::noop(size_t pos) {
        return base_opcode_width + pos;
    }

    size_t VMEnv::addi32(size_t pos){
        push_cs(pop_cs<int>()+pop_cs<int>());
        return base_opcode_width  + pos;    
    }
    size_t VMEnv::addi64(size_t pos){
        push_cs(pop_cs<int64_t>()+pop_cs<int64_t>());
        return base_opcode_width + pos;    
    }
    size_t VMEnv::adddbl(size_t pos){
        push_cs(pop_cs<double>()+pop_cs<double>());
        return base_opcode_width + pos;    
    }

    size_t VMEnv::subi32(size_t pos){
        push_cs(pop_cs<int>()-pop_cs<int>());
        return base_opcode_width + pos;    
    }
    size_t VMEnv::subi64(size_t pos){
        push_cs(pop_cs<int64_t>()-pop_cs<int64_t>());
        return base_opcode_width + pos;    
    }

    size_t VMEnv::subdbl(size_t pos){
        push_cs(pop_cs<double>()-pop_cs<double>());
        return base_opcode_width + pos;    
    }

    size_t VMEnv::duptopi32(size_t pos){
        auto t{pop_cs<int>()};
        push_cs(t);push_cs(t);
        return base_opcode_width +  pos;
    }

    size_t VMEnv::duptopi64(size_t pos){
        auto t{pop_cs<int64_t>()};
        push_cs(t);push_cs(t);
        return base_opcode_width +  pos;
    }

    size_t VMEnv::duptopi8(size_t pos){
        auto t{pop_cs<int8_t>()};
        push_cs(t);push_cs(t);
        return base_opcode_width +  pos;
    }

    size_t VMEnv::ldi32(size_t pos){
        push_cs(*((int*) (mem.base +  *((addr_t*)(text+pos+base_opcode_width)) )   ));
        return base_opcode_width + sizeof(addr_t) + pos;
    }

    size_t VMEnv::ldi32imm(size_t pos){
        push_cs(*((int*)(text+pos+base_opcode_width))) ;
        return base_opcode_width + sizeof(addr_t) + pos;
    }

    size_t VMEnv::ldi64imm(size_t pos){
        push_cs(*((int64_t*)(text+pos+base_opcode_width))) ;
        return base_opcode_width + sizeof(addr_t) + pos;
    }

    size_t VMEnv::dbg_printlni32(size_t pos){
        cout << (*((int*) (mem.base +  *((addr_t*)(text+pos+base_opcode_width)) )   )) << '\n';
        return base_opcode_width + sizeof(addr_t) + pos;
    }

    size_t VMEnv::dbg_print_cs_and_regsimm(size_t pos){
        //cout << (*((int*) (mem.base +  *((addr_t*)(text+pos+base_opcode_width)) )   )) << '\n';
        cout << "\nRegisters: " << registers << "\n";
        if (registers.file[registers_t::CSP]){ 
            cout << "\nCompute Stack:\n\n";
            if (registers.file[registers_t::CSP] >=4) 
             cout << " Top Element (int32, uint32): " << *(int32_t*)&compute_stack[registers.file[registers_t::CSP] - 4]
                                            << "   " << *(uint32_t*)&compute_stack[registers.file[registers_t::CSP] - 4]  << "\n";
            if (registers.file[registers_t::CSP] >=8) 
             cout << " Top Element (int64, uint64): " << *(int64_t*)&compute_stack[registers.file[registers_t::CSP] - 8]
                                            << "   " << *(uint64_t*)&compute_stack[registers.file[registers_t::CSP] - 8]  << "\n";
            cout << "\n Bytes:\n";        
            size_t w {8};
            for(size_t i{}; i <  registers.file[registers_t::CSP]; ++i){
                auto v {compute_stack[i]};
                cout.width(4);
                cout << (uint32_t) v << " ";
                if ( (i + 1)% w == 0) cout << "\n"; 
            }
            cout << "\n";
        }

        return base_opcode_width + sizeof(addr_t) + pos;
    }

    size_t VMEnv::dbg_print_dataimm(size_t pos){
        //cout << (*((int*) (mem.base +  *((addr_t*)(text+pos+base_opcode_width)) )   )) << '\n';
        cout << "\nStatic data ("; cout << mem.heap - mem.base << " bytes) :\n";
        auto w{8};
        auto i{0};
        //hexdump
        for ( auto p = mem.base; p != mem.heap; ++p){
            if (i % w == 0) cout << setw(5) << i << ": ";
            cout << setw(3) << (int) *p << " ";
            ++i;
            if (i % w == 0) {       
                if (i){
                    cout << "  ";
                    for (auto pp = p - w; pp != p;++pp)
                        if (isprint(*pp)) cout << (char) *pp << " ";
                        else cout << ". ";
                }
                cout << "\n";
            }
        }
        cout << "\n";
        if (data_labels().size()){
            size_t sym_col_w{0};
            for (auto e : data_labels() ) if (sym_col_w < e.first.length()) sym_col_w = e.first.length();
            cout << "\nSymboltable:\n\n";
            for (auto e : data_labels() ){
                cout.width(sym_col_w); cout << std::left << e.first << "  " ;
                cout.width(6); cout << e.second << "\n";
            }
        }
        return base_opcode_width + sizeof(addr_t) + pos;
    }

    size_t VMEnv::dbg_print_stackimm(size_t pos){
        cout << "\nStack("; cout << (uint64_t)( (mem.end - mem.base) - registers.file[registers_t::SP] )<< " bytes) :\n";
        auto w{8};
        auto i{0};
        //hexdump
        for ( auto p = mem.base + registers.file[registers_t::SP] ; p != mem.end; ++p){
            if (i % w == 0) cout << setw(5) << i << ": ";
            cout << setw(3) << (int) *p << " ";
            ++i;
            if (i % w == 0) {       
                if (i){
                    cout << "  ";
                    for (auto pp = p - w; pp != p;++pp)
                        if (isprint(*pp)) cout << (char) *pp << " ";
                        else cout << ". ";
                }
                cout << "\n";
            }
        }
        cout << "\n";
        return base_opcode_width + sizeof(addr_t) + pos;
    }

    size_t VMEnv::dbg_deserialize_protobufish_to_json (size_t pos){
        char* buffer = (char*)mem.base +  *((addr_t*)(text+pos+base_opcode_width));
        string res;
        deserialize_event_payload(buffer,(mem.heap  - mem.base)- *((addr_t*)(text+pos+base_opcode_width)) , res);
        cout << res << "\n";
        return base_opcode_width + sizeof(addr_t) + pos;
    }

// Replace address on stack with referenced value
    size_t VMEnv::ldsi32(size_t pos){
        auto addr{pop_cs<addr_t>()};
        push_cs(*((int*) &mem.base[  addr ]));
        return base_opcode_width + pos;
    }

    size_t VMEnv::ldsi64(size_t pos){
        auto addr{pop_cs<addr_t>()};
        push_cs(*((int64_t*) &mem.base[  addr ]));
        return base_opcode_width + pos;
    }
    
    size_t VMEnv::ldsdbl(size_t pos){
        auto addr{pop_cs<addr_t>()};
        push_cs(*((double*) &mem.base[  addr ]));
        return base_opcode_width + pos;
    }      

    size_t VMEnv::ldi64(size_t pos){
        push_cs(*((int64_t*) &mem.base[  *((addr_t*)(text+pos+base_opcode_width)) ]));
        return base_opcode_width + sizeof(addr_t) + pos;
    }

    size_t VMEnv::ldi64reg(size_t pos){
        reg_offs_t reg_offs{ *(reg_offs_t*)(text + pos + base_opcode_width) };
        reg_t reg{ *(reg_t*)(text + pos + base_opcode_width +  sizeof(reg_offs_t) ) };
        push_cs<int64_t>(registers.file[reg] + reg_offs);
        return base_opcode_width + sizeof(reg_offs_t) + sizeof(reg_t) + pos;
    }

    size_t VMEnv::sti64reg(size_t pos){
        reg_offs_t reg_offs{ *(reg_offs_t*)(text + pos + base_opcode_width) };
        reg_t reg{ *(reg_t*)(text + pos + base_opcode_width +  sizeof(reg_offs_t) ) };
        registers.file[reg] =  pop_cs<int64_t>() + reg_offs;
        return base_opcode_width + sizeof(reg_offs_t) + sizeof(reg_t) + pos;
    }

    size_t VMEnv::sti32reg(size_t pos){
        reg_offs_t reg_offs{ *(reg_offs_t*)(text + pos + base_opcode_width) };
        reg_t reg{ *(reg_t*)(text + pos + base_opcode_width +  sizeof(reg_offs_t) ) };
        auto t {(int64_t)pop_cs<int32_t>()};
        registers.file[reg] =  t + reg_offs;
        return base_opcode_width + sizeof(reg_offs_t) + sizeof(reg_t) + pos;
    }

    size_t VMEnv::lddbl(size_t pos){
        auto addr{*((addr_t*)(text+pos+base_opcode_width))};
        push_cs(*((double*) &mem.base[  *((addr_t*)(text+pos+base_opcode_width)) ]));
        return base_opcode_width + sizeof(addr_t) + pos;
    }
    size_t VMEnv::sti32(size_t pos){
        auto t{pop_cs<int32_t>()};
        *((int*) &mem.base[  *((addr_t*)(text+pos+base_opcode_width)) ]) = t;
        return base_opcode_width + sizeof(addr_t) + pos;
    }
    size_t VMEnv::stsi32(size_t pos){
        auto addr{pop_cs<addr_t>()};
        auto value{pop_cs<int>()};
        *(int*)&mem.base[addr]  = value;
        return base_opcode_width + pos;
    }
    size_t VMEnv::stsi64(size_t pos){
        auto addr{pop_cs<addr_t>()};
        auto value{pop_cs<int64_t>()};
        *(decltype(value)*)&mem.base[addr]  = value;
        return base_opcode_width + pos;
    }
     size_t VMEnv::stsdbl(size_t pos){
        auto addr{pop_cs<addr_t>()};
        auto value{pop_cs<double>()};
        *(decltype(value)*)&mem.base[addr]  = value;
        return base_opcode_width + pos;
    }   
    size_t VMEnv::sti64(size_t pos){
        auto t{pop_cs<int64_t>()};
        *((int64_t*) &mem.base[  *((addr_t*)(text+pos+base_opcode_width)) ]) = t;
        return base_opcode_width + sizeof(addr_t) + pos;
    }
    size_t VMEnv::stdbl(size_t pos){
        auto t{pop_cs<double>()};
        *((double*) &mem.base[  *((addr_t*)(text+pos+base_opcode_width)) ]) = t;
        return base_opcode_width + sizeof(addr_t) + pos;
    }
    size_t VMEnv::ldptr(size_t pos){return base_opcode_width + pos;}
    size_t VMEnv::stptr(size_t pos){return base_opcode_width + pos;}

    size_t VMEnv::lea(size_t pos){
        push_cs( *((addr_t*)(text+pos+base_opcode_width)) );
        return base_opcode_width + sizeof(addr_t) + pos;
    }

    size_t VMEnv::lea_absolute(size_t pos){
        push_cs( mem.base + *((addr_t*)(text+pos+base_opcode_width)) );
        return base_opcode_width + sizeof(addr_t) + pos;
    }

    size_t VMEnv::buc(size_t pos){
        return *(size_t*)(text + pos  + base_opcode_width) ;
    }
    size_t VMEnv::beq(size_t pos){
        if (pop_cs<int>() == pop_cs<int>()) return *(size_t*)(text + pos  + base_opcode_width);
        return base_opcode_width + sizeof(addr_t) + pos;
    }
    size_t VMEnv::bneq(size_t pos){
        if (pop_cs<int>() != pop_cs<int>()) return *(size_t*)(text + pos  + base_opcode_width);
        return base_opcode_width + sizeof(addr_t) + pos;
    }
    size_t VMEnv::blt(size_t pos){
        if (pop_cs<int>() < pop_cs<int>()) return *(size_t*)(text + pos  + base_opcode_width);
        return base_opcode_width + sizeof(addr_t) + pos;
    }
    size_t VMEnv::blteq(size_t pos){
        if (pop_cs<int>() <= pop_cs<int>()) return *(size_t*)(text + pos  + base_opcode_width);
        return base_opcode_width + sizeof(addr_t) + pos;
    }
    size_t VMEnv::bgt(size_t pos){
        if (pop_cs<int>() > pop_cs<int>()) return *(size_t*)(text + pos  + base_opcode_width);
        return base_opcode_width + sizeof(addr_t) + pos;
    }
    size_t VMEnv::bgteq(size_t pos){
        if (pop_cs<int>() >= pop_cs<int>()) return *(size_t*)(text + pos  + base_opcode_width);
        return base_opcode_width + sizeof(addr_t) + pos;
    }
    size_t VMEnv::bgteqzeroi32(size_t pos){
        if (pop_cs<int>() >= 0) return *(size_t*)(text + pos  + base_opcode_width);
        return base_opcode_width + sizeof(addr_t) + pos;
    }
    size_t VMEnv::blteqzeroi32(size_t pos){
        auto v = pop_cs<int>(); 
        if (v <= 0) return *(size_t*)(text + pos  + base_opcode_width);
        return base_opcode_width + sizeof(addr_t) + pos;
    }
    size_t VMEnv::bltzeroi32(size_t pos){
        if (pop_cs<int>() < 0) return *(size_t*)(text + pos  + base_opcode_width);
        return base_opcode_width + sizeof(addr_t) + pos;
    }
    size_t VMEnv::bzeroi32(size_t pos){
        if (pop_cs<int>() == 0) return *(size_t*)(text + pos  + base_opcode_width);
        return base_opcode_width + sizeof(addr_t) + pos;
    }
    size_t VMEnv::bzeroi64(size_t pos){
        if (pop_cs<int64_t>() == 0) return *(size_t*)(text + pos  + base_opcode_width);
        return base_opcode_width + sizeof(addr_t) + pos;
    }
    size_t VMEnv::bzerodbl(size_t pos){
        if (pop_cs<double>() == 0.0) return *(size_t*)(text + pos  + base_opcode_width);
        return base_opcode_width + sizeof(addr_t) + pos;
    }
    size_t VMEnv::bnzeroi32(size_t pos){
        if (pop_cs<int>() != 0) return *(size_t*)(text + pos  + base_opcode_width);
        return base_opcode_width + sizeof(addr_t) + pos;
    }
    size_t VMEnv::bnzeroi64(size_t pos){
        if (pop_cs<int64_t>() != 0) return *(size_t*)(text + pos  + base_opcode_width);
        return base_opcode_width + sizeof(addr_t) + pos;
    }
    size_t VMEnv::bnzerodbl(size_t pos){
        if (pop_cs<double>() != 0.0) return *(size_t*)(text + pos  + base_opcode_width);
        return base_opcode_width + sizeof(addr_t) + pos;
    }

    size_t VMEnv::call(size_t pos){
        push_data_stack<size_t>(base_opcode_width + sizeof(addr_t) + pos);
        return *(size_t*)(text + pos  + base_opcode_width);
    }
    
    size_t VMEnv::callx(size_t pos){
        auto addr =  *(size_t*)(text + pos  + base_opcode_width);
        //auto fp = (void (*) (int,int,const char *)) addr;
        auto fp = (void (*) ()) addr;
        
    
        #ifdef __x86_64__
        auto arg0{registers.file[registers_t::ARG0]};
        auto arg1{registers.file[registers_t::ARG1]};
        auto arg2{registers.file[registers_t::ARG2]};
        //fp(arg0,arg1,(char const*)arg2);

        asm __volatile__ (
            "mov %0 , %%rdi;"
            "mov %1 , %%rsi;"
            "mov %2 , %%rdx;"
            "movq %3, %%rax;"
            //"call *%%rax;" !! Crashes
            :: "r"(arg0),"r"(arg1),"r"(arg2), "r"(addr): "%rdi","%rsi","%rdx", "%rax"
        );
        fp();//Hack see comment above
        #endif
        return base_opcode_width + sizeof(addr_t) + pos;
    }

    size_t VMEnv::ret(size_t pos){
        auto target_addr{pop_data_stack<size_t>()};
        return target_addr;
    }
    size_t VMEnv::swp(size_t pos){
        auto t1{pop_cs<int>()};
        auto t2{pop_cs<int>()};
        push_cs(t1);push_cs(t2);
        return base_opcode_width + pos;
    }

    size_t VMEnv::swpi64(size_t pos){
        auto t1{pop_cs<int64_t>()};
        auto t2{pop_cs<int64_t>()};
        push_cs(t1);push_cs(t2);
        return base_opcode_width + pos;
    }

    size_t VMEnv::swpi16i64(size_t pos){
        auto t1{pop_cs<int16_t>()};
        auto t2{pop_cs<int64_t>()};
        push_cs(t1);push_cs(t2);
        return base_opcode_width + pos;
    }

    size_t VMEnv::andni32(size_t pos){
        push_cs<unsigned int>(pop_cs<unsigned int>() & !pop_cs<unsigned int>());
        return base_opcode_width + pos;
    }
    size_t VMEnv::andni64(size_t pos){
        push_cs<uint64_t>(pop_cs<uint64_t>() & !pop_cs<uint64_t>());
        return base_opcode_width + pos;
    }
    size_t VMEnv::andi32(size_t pos){
        push_cs<unsigned int>(pop_cs<unsigned int>() & pop_cs<unsigned int>());
        return base_opcode_width + pos;
    }
    size_t VMEnv::andi64(size_t pos){
        push_cs<uint64_t>(pop_cs<uint64_t>() & !pop_cs<uint64_t>());
        return base_opcode_width + pos;
    }
    size_t VMEnv::ori32(size_t pos){
        push_cs<unsigned int>(pop_cs<unsigned int>() | pop_cs<unsigned int>());
        return base_opcode_width + pos;
    }
    size_t VMEnv::ori64(size_t pos){
        push_cs<uint64_t>(pop_cs<uint64_t>() | pop_cs<uint64_t>());
        return base_opcode_width + pos;
    }
    size_t VMEnv::noti32(size_t pos){
        push_cs<uint32_t>(pop_cs<uint32_t>()? 0 : 1);
        return base_opcode_width + pos;
    }
    size_t VMEnv::noti64(size_t pos){
        push_cs<uint64_t>(pop_cs<uint64_t>() ? 0 : 1);
        return base_opcode_width + pos;
    }
    size_t VMEnv::xori32(size_t pos){
        push_cs<uint32_t>(pop_cs<uint32_t>() ^ pop_cs<uint32_t>());
        return base_opcode_width + pos;
    }
    size_t VMEnv::xori64(size_t pos){
        push_cs<uint64_t>(pop_cs<uint64_t>() ^ pop_cs<uint64_t>());
        return base_opcode_width + pos;
    }

    size_t VMEnv::muli32(size_t pos){
        push_cs<int32_t>(pop_cs<int32_t>() * pop_cs<int32_t>());
        return base_opcode_width + pos;
    }
    size_t VMEnv::muli64(size_t pos){
        push_cs<int64_t>(pop_cs<int64_t>() * pop_cs<int64_t>());
        return base_opcode_width + pos;
    }
    size_t VMEnv::muldbl(size_t pos){
        push_cs<double>(pop_cs<double>() * pop_cs<double>());
        return base_opcode_width + pos;
    }
    size_t VMEnv::divi32(size_t pos){
        push_cs<int32_t>(pop_cs<int32_t>() / pop_cs<int32_t>());
        return base_opcode_width + pos;
    }
    size_t VMEnv::divi64(size_t pos){
        push_cs<int64_t>(pop_cs<int64_t>() / pop_cs<int64_t>());
        return base_opcode_width + pos;
    }
    size_t VMEnv::divdbl(size_t pos){
        push_cs<double>(pop_cs<double>() / pop_cs<double>());
        return base_opcode_width + pos;
    }
    size_t VMEnv::remi32(size_t pos){
        push_cs<int32_t>(pop_cs<int32_t>() % pop_cs<int32_t>());
        return base_opcode_width + pos;
    }
    size_t VMEnv::remi64(size_t pos){
        push_cs<int64_t>(pop_cs<int64_t>() % pop_cs<int64_t>());
        return base_opcode_width + pos;
    }
    // relational operators
    size_t VMEnv::lti32(size_t pos){
        auto a{pop_cs<int32_t>()};
        auto b{pop_cs<int32_t>()};
        push_cs( a < b ? 1 : 0  );
        return base_opcode_width  + pos;
    }
    size_t VMEnv::lti64(size_t pos){
        auto a{pop_cs<int64_t>()};
        auto b{pop_cs<int64_t>()};
        push_cs<int32_t>( a < b ? 1 : 0  );
        return base_opcode_width  + pos;
    }
    size_t VMEnv::ltdbl(size_t pos){
        auto a{pop_cs<double>()};
        auto b{pop_cs<double>()};
        push_cs<int32_t>( a < b ? 1 : 0  );
        return base_opcode_width  + pos;
    }
    size_t VMEnv::lteqi32(size_t pos){
        auto a{pop_cs<int32_t>()};
        auto b{pop_cs<int32_t>()};
        push_cs( a <= b ? 1 : 0  );
        return base_opcode_width  + pos;
    }
    size_t VMEnv::lteqi64(size_t pos){
        auto a{pop_cs<int64_t>()};
        auto b{pop_cs<int64_t>()};
        push_cs<int32_t>( a <= b ? 1 : 0  );
        return base_opcode_width  + pos;
    }
    size_t VMEnv::lteqdbl(size_t pos){
        auto a{pop_cs<double>()};
        auto b{pop_cs<double>()};
        push_cs<int32_t>( a <= b ? 1 : 0  );
        return base_opcode_width  + pos;
    }
    size_t VMEnv::gti32(size_t pos){
        auto a{pop_cs<int32_t>()};
        auto b{pop_cs<int32_t>()};
        push_cs( a > b ? 1 : 0  );
        return base_opcode_width  + pos;
    }
    size_t VMEnv::gti64(size_t pos){
        auto a{pop_cs<int64_t>()};
        auto b{pop_cs<int64_t>()};
        push_cs<int32_t>( a > b ? 1 : 0  );
        return base_opcode_width  + pos;
    }
    size_t VMEnv::gtdbl(size_t pos){
        auto a{pop_cs<double>()};
        auto b{pop_cs<double>()};
        push_cs<int32_t>( a > b ? 1 : 0  );
        return base_opcode_width  + pos;
    }
    size_t VMEnv::gteqi32(size_t pos){
        auto a{pop_cs<int32_t>()};
        auto b{pop_cs<int32_t>()};
        push_cs( a >= b ? 1 : 0  );
        return base_opcode_width  + pos;
    }
    size_t VMEnv::gteqi64(size_t pos){
        auto a{pop_cs<int64_t>()};
        auto b{pop_cs<int64_t>()};
        push_cs<int32_t>( a >= b ? 1 : 0  );
        return base_opcode_width  + pos;
    }
    size_t VMEnv::gteqdbl(size_t pos){
        auto a{pop_cs<double>()};
        auto b{pop_cs<double>()};
        push_cs<int32_t>( a > b ? 1 : 0  );
        return base_opcode_width  + pos;
    }
    size_t VMEnv::eqi32(size_t pos){
        auto a{pop_cs<int32_t>()};
        auto b{pop_cs<int32_t>()};
        push_cs( a == b ? 1 : 0  );
        return base_opcode_width  + pos;
    }
    size_t VMEnv::eqi64(size_t pos){
        auto a{pop_cs<int64_t>()};
        auto b{pop_cs<int64_t>()};
        push_cs<int32_t>( a == b ? 1 : 0  );
        return base_opcode_width  + pos;
    }
    size_t VMEnv::eqdbl(size_t pos){
        auto a{pop_cs<double>()};
        auto b{pop_cs<double>()};
        push_cs<int32_t>( a == b ? 1 : 0  );
        return base_opcode_width  + pos;
    }
    size_t VMEnv::cpysi32(size_t pos){
        throw std::string{"Deprecated"};
        return base_opcode_width + pos + 1;
    }
    size_t VMEnv::wrsi32(size_t pos){
        throw std::string{"Deprecated"};
        return base_opcode_width + pos + 1;
    }
    size_t VMEnv::setframe(size_t pos){
        throw std::string{"Deprecated"};
        return base_opcode_width + pos;
    }

    size_t VMEnv::popi32(size_t pos){
        registers.file[registers_t::SP] += sizeof(int32_t);
        return base_opcode_width + pos;
    }

    size_t VMEnv::popi32reg(size_t pos){
        auto v{*(int32_t*) &mem.base[registers.file[registers_t::SP]]};
        registers.file[registers_t::SP] += sizeof(int32_t);
        auto reg{*((int*) &text[pos+1])};
        registers.file[reg] = v;
        return base_opcode_width + pos + 1;
    }

    size_t VMEnv::pushi32reg(size_t pos){
        auto reg{*((int*) &text[pos+1])};
        registers.file[registers_t::SP] -= sizeof(int32_t);
        *(int32_t*) &mem.base[registers.file[registers_t::SP]] = registers.file[reg]; 
        return base_opcode_width + pos + 1;
    }

    size_t VMEnv::pushi32(size_t pos){
        auto v{*((int*) &mem.base[text[pos+1]])};
        registers.file[registers_t::SP] -= sizeof(int32_t);
        *(int32_t*) &mem.base[registers.file[registers_t::SP]] = v; 
        return base_opcode_width + pos +1;
    }

    size_t VMEnv::popi64reg(size_t pos){
        auto v{*(int64_t*) &mem.base[registers.file[registers_t::SP]]};
        registers.file[registers_t::SP] += sizeof(int64_t);
        auto reg{*((int*) &text[pos+1])};
        registers.file[reg] = v;
        return base_opcode_width + pos + 1;
    }

    size_t VMEnv::pushi64reg(size_t pos){
        reg_offs_t reg_offs{ *(reg_offs_t*)(text + pos + base_opcode_width) };
        reg_t reg{ *(reg_t*)(text + pos + base_opcode_width +  sizeof(reg_offs_t) ) };
        registers.file[registers_t::SP] -= sizeof(int64_t);
        *(int64_t*) &mem.base[registers.file[registers_t::SP]] = registers.file[reg] + reg_offs; 
        return base_opcode_width + sizeof(reg_offs_t) + sizeof(reg_t) + pos;
    }

    
    size_t VMEnv::pushi64(size_t pos){
        auto v{*((int64_t*) &mem.base[text[pos+1]])};
        registers.file[registers_t::SP] -= sizeof(int64_t);
        *(int64_t*) &mem.base[registers.file[registers_t::SP]] = v; 
        return base_opcode_width + pos +1;
    }

    size_t VMEnv::popi64(size_t pos){
        registers.file[registers_t::SP] += sizeof(int64_t);
        return base_opcode_width + pos;
    }

    size_t VMEnv::ui32toui64(size_t pos){
        auto v{pop_cs<uint32_t>()};
        uint64_t to{v};
        push_cs<uint64_t> (to);
        return base_opcode_width  + pos;
    }

    size_t VMEnv::ui8toui32(size_t pos){
        auto v{pop_cs<uint8_t>()};
        uint32_t to{v};
        push_cs<uint32_t> (to);
        return base_opcode_width  + pos;
    }
    size_t VMEnv::ui8toui64(size_t pos){
        auto v{pop_cs<uint8_t>()};
        uint64_t to{v};
        push_cs<uint64_t> (to);
        return base_opcode_width  + pos;
    }

    size_t VMEnv::sindbl(size_t pos){
        push_cs<double>(sin(pop_cs<double>()));
        return base_opcode_width + pos;
    }

    size_t VMEnv::cosdbl(size_t pos){
        push_cs<double>(cos(pop_cs<double>()));
        return base_opcode_width + pos;
    }

    size_t VMEnv::tandbl(size_t pos){
        push_cs<double>(tan(pop_cs<double>()));
        return base_opcode_width + pos;
    }
    
    size_t VMEnv::atandbl(size_t pos){
        push_cs<double>(atan(pop_cs<double>()));
        return base_opcode_width + pos;
    }

    size_t VMEnv::expdbl(size_t pos){
        push_cs<double>(exp(pop_cs<double>()));
        return base_opcode_width + pos;
    }
    size_t VMEnv::tanhdbl(size_t pos){
        push_cs<double>(tanh(pop_cs<double>()));
        return base_opcode_width + pos;
    }

    size_t VMEnv::negdbl(size_t pos){
        push_cs<double>(-1.0 * (pop_cs<double>()));
        return base_opcode_width + pos;
    }
    size_t VMEnv::negi32(size_t pos){
        push_cs<int>(-1 * (pop_cs<int>()));
        return base_opcode_width + pos;
    }
    size_t VMEnv::negi64(size_t pos){
        push_cs<int64_t>(-1 * (pop_cs<int64_t>()));
        return base_opcode_width + pos;
    }
    size_t VMEnv::msg(size_t pos){
        auto event_id { *((addr_t*)(text+pos+base_opcode_width))};
        if (event_queue) event_queue->fire_event(event_id);
        return base_opcode_width + sizeof(addr_t) + pos;
    }

    size_t VMEnv::discardtopi32(size_t pos){
        pop_cs<int32_t>();
        return base_opcode_width + pos;
    }
    size_t VMEnv::discardtopi64(size_t pos){
        pop_cs<int64_t>();
        return base_opcode_width + pos;
    }
    size_t VMEnv::ldi8(size_t pos){
        push_cs(*((int8_t*) &mem.base[  *((addr_t*)(text+pos+base_opcode_width)) ]));
        return base_opcode_width + sizeof(addr_t) + pos;
    }
    size_t VMEnv::sti8(size_t pos){
        auto t{pop_cs<int8_t>()};
        *((int8_t*) &mem.base[  *((addr_t*)(text+pos+base_opcode_width)) ]) = t;
        return base_opcode_width + sizeof(addr_t) + pos;
    }
    size_t VMEnv::stsi8(size_t pos){
        auto addr{pop_cs<addr_t>()};
        auto value{pop_cs<int8_t>()};
        *(int8_t*)&mem.base[addr]  = value;
        return base_opcode_width + pos;
    }
    size_t VMEnv::ldsi8(size_t pos){
        auto addr{pop_cs<addr_t>()};
        push_cs(*((int8_t*) &mem.base[  addr ]));
        return base_opcode_width + pos;
    }

    void VMEnv::reset(){
        registers.file[registers_t::SP] = 0;
    }

    VMEnv::text_t VMEnv::resize_text(size_t new_size){
        auto t{this->text};
        auto old_size{text_size};
        text_size = new_size;
        text = (uint8_t*) new char[text_size];
        memset(text,(int)Opcode::halt, text_size );
        if(!text) { throw std::runtime_error{"oblectamenta_assembler: out of text space"}; }
        memcpy(text,t,std::min(old_size,new_size));
        delete[] t;
        return text;
    }

    VMEnv::VMEnv(){
        mem.heap = mem.base = new remove_pointer_t<data_t>[default_mem_size];
        for(auto p{mem.heap}; p - mem.heap < default_mem_size;) *p++ = 0;
        mem.end = mem.base + default_mem_size;
        text = new remove_pointer_t<text_t>[text_size = default_text_size];
        emit<Opcode::halt>(*this,0);

        registers.file[registers_t::SP] = mem.end - mem.base;
        registers.file[registers_t::CSP] = 0;
        registers.file[registers_t::FP] = 0;
        registers.file[registers_t::PC] = 0;

        op_dispatch.push_back(&VMEnv::noop);
        op_dispatch.push_back(&VMEnv::noop); 
        op_dispatch.push_back(&VMEnv::ldi32);
        op_dispatch.push_back(&VMEnv::ldsi32);
        op_dispatch.push_back(&VMEnv::ldi64);
        op_dispatch.push_back(&VMEnv::lddbl);
        op_dispatch.push_back(&VMEnv::sti32);
        op_dispatch.push_back(&VMEnv::stsi32);
        op_dispatch.push_back(&VMEnv::sti64);
        op_dispatch.push_back(&VMEnv::stdbl);
        op_dispatch.push_back(&VMEnv::ldptr);
        op_dispatch.push_back(&VMEnv::stptr);
        op_dispatch.push_back(&VMEnv::lea);
        op_dispatch.push_back(&VMEnv::addi32);
        op_dispatch.push_back(&VMEnv::addi64);
        op_dispatch.push_back(&VMEnv::adddbl);
        op_dispatch.push_back(&VMEnv::subi32);
        op_dispatch.push_back(&VMEnv::subi64);
        op_dispatch.push_back(&VMEnv::subdbl);
        op_dispatch.push_back(&VMEnv::buc);
        op_dispatch.push_back(&VMEnv::beq);
        op_dispatch.push_back(&VMEnv::bneq);
        op_dispatch.push_back(&VMEnv::blt);
        op_dispatch.push_back(&VMEnv::blteq);
        op_dispatch.push_back(&VMEnv::bgt);
        op_dispatch.push_back(&VMEnv::bgteq);
        op_dispatch.push_back(&VMEnv::bgteqzeroi32);
        op_dispatch.push_back(&VMEnv::blteqzeroi32);
        op_dispatch.push_back(&VMEnv::bltzeroi32);
        op_dispatch.push_back(&VMEnv::bzeroi32);
        op_dispatch.push_back(&VMEnv::bnzeroi32);
        op_dispatch.push_back(&VMEnv::bzeroi64);
        op_dispatch.push_back(&VMEnv::bnzeroi64);
        op_dispatch.push_back(&VMEnv::bzerodbl);
        op_dispatch.push_back(&VMEnv::bnzerodbl);
        op_dispatch.push_back(&VMEnv::call);
        op_dispatch.push_back(&VMEnv::ret);
        op_dispatch.push_back(&VMEnv::swp);
        op_dispatch.push_back(&VMEnv::andni32);
        op_dispatch.push_back(&VMEnv::andni64);
        op_dispatch.push_back(&VMEnv::andi32);
        op_dispatch.push_back(&VMEnv::andi64);
        op_dispatch.push_back(&VMEnv::ori32);
        op_dispatch.push_back(&VMEnv::ori64);
        op_dispatch.push_back(&VMEnv::noti32);
        op_dispatch.push_back(&VMEnv::noti64);
        op_dispatch.push_back(&VMEnv::xori32);
        op_dispatch.push_back(&VMEnv::xori64);
        op_dispatch.push_back(&VMEnv::duptopi32);
        op_dispatch.push_back(&VMEnv::muli32);
        op_dispatch.push_back(&VMEnv::muli64);
        op_dispatch.push_back(&VMEnv::muldbl);
        op_dispatch.push_back(&VMEnv::divi32);
        op_dispatch.push_back(&VMEnv::divi64);
        op_dispatch.push_back(&VMEnv::divdbl);
        op_dispatch.push_back(&VMEnv::remi32);
        op_dispatch.push_back(&VMEnv::remi64);
        op_dispatch.push_back(&VMEnv::lti32);
        op_dispatch.push_back(&VMEnv::lti64);
        op_dispatch.push_back(&VMEnv::ltdbl);
        op_dispatch.push_back(&VMEnv::lteqi32);
        op_dispatch.push_back(&VMEnv::lteqi64);
        op_dispatch.push_back(&VMEnv::lteqdbl);
        op_dispatch.push_back(&VMEnv::gti32);
        op_dispatch.push_back(&VMEnv::gti64);
        op_dispatch.push_back(&VMEnv::gtdbl);
        op_dispatch.push_back(&VMEnv::gteqi32);
        op_dispatch.push_back(&VMEnv::gteqi64);
        op_dispatch.push_back(&VMEnv::gteqdbl);
        op_dispatch.push_back(&VMEnv::eqi32);
        op_dispatch.push_back(&VMEnv::eqi64);
        op_dispatch.push_back(&VMEnv::eqdbl);
        op_dispatch.push_back(&VMEnv::cpysi32);
        op_dispatch.push_back(&VMEnv::wrsi32);
        op_dispatch.push_back(&VMEnv::setframe);
        op_dispatch.push_back(&VMEnv::popi32);
        op_dispatch.push_back(&VMEnv::pushi32reg);
        op_dispatch.push_back(&VMEnv::popi32reg);
        op_dispatch.push_back(&VMEnv::pushi32);
        op_dispatch.push_back(&VMEnv::sti64);       
        op_dispatch.push_back(&VMEnv::ui32toui64);       
        op_dispatch.push_back(&VMEnv::ldi64reg);
        op_dispatch.push_back(&VMEnv::sti64reg);
        op_dispatch.push_back(&VMEnv::stsi64);  
        op_dispatch.push_back(&VMEnv::ldsi64);

        op_dispatch.push_back(&VMEnv::sindbl);
        op_dispatch.push_back(&VMEnv::cosdbl);
        op_dispatch.push_back(&VMEnv::tandbl);
        op_dispatch.push_back(&VMEnv::atandbl);
        op_dispatch.push_back(&VMEnv::expdbl);
        op_dispatch.push_back(&VMEnv::ldsdbl);

        op_dispatch.push_back(&VMEnv::negdbl);
        op_dispatch.push_back(&VMEnv::negi32);
        op_dispatch.push_back(&VMEnv::negi64);
        op_dispatch.push_back(&VMEnv::stsdbl);
        op_dispatch.push_back(&VMEnv::tanhdbl);
        op_dispatch.push_back(&VMEnv::dbg_printlni32);
        op_dispatch.push_back(&VMEnv::callx);
        op_dispatch.push_back(&VMEnv::lea_absolute);
        op_dispatch.push_back(&VMEnv::sti32reg);
        op_dispatch.push_back(&VMEnv::msg);
        op_dispatch.push_back(&VMEnv::dbg_print_cs_and_regsimm);
        op_dispatch.push_back(&VMEnv::dbg_print_dataimm);
        op_dispatch.push_back(&VMEnv::dbg_deserialize_protobufish_to_json);
        op_dispatch.push_back(&VMEnv::ldi32imm);

        op_dispatch.push_back(&VMEnv::popi64);
        op_dispatch.push_back(&VMEnv::pushi64reg);
        op_dispatch.push_back(&VMEnv::popi64reg);
        op_dispatch.push_back(&VMEnv::pushi64);
        op_dispatch.push_back(&VMEnv::dbg_print_stackimm);
        op_dispatch.push_back(&VMEnv::ldi64imm);
        op_dispatch.push_back(&VMEnv::duptopi64);


        op_dispatch.push_back(&VMEnv::discardtopi32);
        op_dispatch.push_back(&VMEnv::discardtopi64);
        op_dispatch.push_back(&VMEnv::ldi8);
        op_dispatch.push_back(&VMEnv::sti8);
        op_dispatch.push_back(&VMEnv::stsi8);
        op_dispatch.push_back(&VMEnv::ldsi8);
        op_dispatch.push_back(&VMEnv::ui8toui32);
        op_dispatch.push_back(&VMEnv::ui8toui64);
        op_dispatch.push_back(&VMEnv::duptopi8);
        op_dispatch.push_back(&VMEnv::swpi64);
        op_dispatch.push_back(&VMEnv::swpi16i64);
    }     
    void VMEnv::dump(ostream& os){
       // for(ssize_t i = registers.file[registers_t::SP] - 1; i >= 0; --i )
    //  os << "| "<< stack_seg[i] << "\t|\n";
    }
}

std::ostream& operator << (std::ostream& os, ceps::vm::oblectamenta::VMEnv::registers_t regs){
    
    size_t w {};
    for(auto r : regs.reg_mnemonic2idx) if (r.first.length() > w) w = r.first.length();
    size_t lc {};
    auto nl {regs.reg_mnemonic2idx.size()};
    for(auto r : regs.reg_mnemonic2idx){
        os.width(w);os << r.first << ": " ;
        os << " ";
        os << regs.file[r.second];
        if (++lc < nl) os << " | ";
    }
    return os;
}