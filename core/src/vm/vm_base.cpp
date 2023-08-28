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

#include "core/include/vm/vm_base.hpp"

namespace ceps::vm::oblectamenta{

    size_t VMEnv::run(size_t start){
        for(; Opcode(text_seg[start] & 0xFF) != Opcode::halt;){ 
         start = (this ->*op_dispatch[text_seg[start] & 0xFF])(start);
        }
        return start;
    }
    
    size_t VMEnv::noop(size_t pos) {
        return base_opcode_width + pos;
    }

    size_t VMEnv::addi32(size_t pos){
        push(pop<int>()+pop<int>());
        return base_opcode_width  + pos;    
    }
    size_t VMEnv::addi64(size_t pos){
        push(pop<int64_t>()+pop<int64_t>());
        return base_opcode_width + pos;    
    }
    size_t VMEnv::adddbl(size_t pos){
        push(pop<double>()+pop<double>());
        return base_opcode_width + pos;    
    }

    size_t VMEnv::subi32(size_t pos){
        push(pop<int>()-pop<int>());
        return base_opcode_width + pos;    
    }
    size_t VMEnv::subi64(size_t pos){
        push(pop<int64_t>()-pop<int64_t>());
        return base_opcode_width + pos;    
    }

    size_t VMEnv::subdbl(size_t pos){
        push(pop<double>()-pop<double>());
        return base_opcode_width + pos;    
    }

    size_t VMEnv::duptopi32(size_t pos){
        auto t{pop<int>()};
        push(t);push(t);
        return base_opcode_width +  pos;
    }

    size_t VMEnv::ldi32(size_t pos){
        push(*((int*) &data_seg[text_seg[pos+1]]));
        return base_opcode_width + 1 + pos;
    }

    size_t VMEnv::ldsi32(size_t pos){
        auto t{pop<int32_t>()};
        push(*((int*) &data_seg[t]));
        return base_opcode_width + pos;
    }

    size_t VMEnv::ldi64(size_t){return base_opcode_width;}
    size_t VMEnv::lddbl(size_t pos){
        push(*((double*) &data_seg[text_seg[pos+1]]));
        return base_opcode_width + 1 + pos;
    }
    size_t VMEnv::sti32(size_t pos){
        *(int*)&data_seg[text_seg[pos+1]]  = pop<int>();
        return base_opcode_width + 1 + pos;
    }
    size_t VMEnv::stsi32(size_t pos){
        auto value{pop<int>()};
        auto addr{pop<int>()};
        *(int*)&data_seg[addr]  = value;
        return base_opcode_width + pos;
    }

    size_t VMEnv::sti64(size_t pos){return base_opcode_width;}
    size_t VMEnv::stdbl(size_t pos){
        *(double*)&data_seg[text_seg[pos+1]]  = pop<double>();
        return base_opcode_width + 1 + pos;
    }
    size_t VMEnv::ldptr(size_t pos){return base_opcode_width + pos;}
    size_t VMEnv::stptr(size_t pos){return base_opcode_width + pos;}

    size_t VMEnv::lea(size_t pos){
        push(*((int*) &text_seg[pos+1]));
        return base_opcode_width + 1 + pos;
    }

    size_t VMEnv::buc(size_t pos){
        return text_seg[pos+1];
    }
    size_t VMEnv::beq(size_t pos){
        if (pop<int>() == pop<int>()) return text_seg[pos+1];
        return base_opcode_width + 1 + pos;
    }
    size_t VMEnv::bneq(size_t pos){
        if (pop<int>() != pop<int>()) return text_seg[pos+1];
        return base_opcode_width + 1 + pos;
    }
    size_t VMEnv::blt(size_t pos){
        if (pop<int>() < pop<int>()) return text_seg[pos+1];
        return base_opcode_width + 1 + pos;
    }
    size_t VMEnv::blteq(size_t pos){
        if (pop<int>() <= pop<int>()) return text_seg[pos+1];
        return base_opcode_width + 1 + pos;
    }
    size_t VMEnv::bgt(size_t pos){
        if (pop<int>() > pop<int>()) return text_seg[pos+1];
        return base_opcode_width + 1 + pos;
    }
    size_t VMEnv::bgteq(size_t pos){
        if (pop<int>() >= pop<int>()) return text_seg[pos+1];
        return base_opcode_width + 1 + pos;
    }
    size_t VMEnv::bgteqzeroi32(size_t pos){
        if (pop<int>() >= 0) return text_seg[pos+1];
        return base_opcode_width + 1 + pos;
    }    
    size_t VMEnv::bzeroi32(size_t pos){
        if (pop<int>() == 0) return text_seg[pos+1];
        return base_opcode_width + 1 + pos;
    }
    size_t VMEnv::bzeroi64(size_t pos){
        if (pop<int64_t>() == 0) return text_seg[pos+1];
        return base_opcode_width + 1 + pos;
    }
    size_t VMEnv::bzerodbl(size_t pos){
        if (pop<double>() == 0.0) return text_seg[pos+1];
        return base_opcode_width + 1 + pos;
    }
    size_t VMEnv::bnzeroi32(size_t pos){
        if (pop<int>() != 0) return text_seg[pos+1];
        return base_opcode_width + 1 + pos;
    }
    size_t VMEnv::bnzeroi64(size_t pos){
        if (pop<int64_t>() != 0) return text_seg[pos+1];
        return base_opcode_width + 1 + pos;
    }
    size_t VMEnv::bnzerodbl(size_t pos){
        if (pop<double>() != 0.0) return text_seg[pos+1];
        return base_opcode_width + 1 + pos;
    }

    size_t VMEnv::andni32(size_t pos){
        push<unsigned int>(pop<unsigned int>() & !pop<unsigned int>());
        return base_opcode_width + pos;
    }
    size_t VMEnv::andni64(size_t pos){
        push<uint64_t>(pop<uint64_t>() & !pop<uint64_t>());
        return base_opcode_width + pos;
    }
    size_t VMEnv::andi32(size_t pos){
        push<unsigned int>(pop<unsigned int>() & pop<unsigned int>());
        return base_opcode_width + pos;
    }
    size_t VMEnv::andi64(size_t pos){
        push<uint64_t>(pop<uint64_t>() & !pop<uint64_t>());
        return base_opcode_width + pos;
    }
    size_t VMEnv::ori32(size_t pos){
        push<unsigned int>(pop<unsigned int>() | pop<unsigned int>());
        return base_opcode_width + pos;
    }
    size_t VMEnv::ori64(size_t pos){
        push<uint64_t>(pop<uint64_t>() | pop<uint64_t>());
        return base_opcode_width + pos;
    }
    size_t VMEnv::noti32(size_t pos){
        push<uint32_t>(~pop<uint32_t>());
        return base_opcode_width + pos;
    }
    size_t VMEnv::noti64(size_t pos){
        push<uint64_t>(~pop<uint64_t>());
        return base_opcode_width + pos;
    }
    size_t VMEnv::xori32(size_t pos){
        push<uint32_t>(pop<uint32_t>() ^ pop<uint32_t>());
        return base_opcode_width + pos;
    }
    size_t VMEnv::xori64(size_t pos){
        push<uint64_t>(pop<uint64_t>() ^ pop<uint64_t>());
        return base_opcode_width + pos;
    }

    size_t VMEnv::muli32(size_t pos){
        push<int32_t>(pop<int32_t>() * pop<int32_t>());
        return base_opcode_width + pos;
    }

    void VMEnv::reset(){
        stack_top = 0;
    }


    VMEnv::VMEnv(){
        stack_top = 0;
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
        op_dispatch.push_back(&VMEnv::bzeroi32);
        op_dispatch.push_back(&VMEnv::bnzeroi32);
        op_dispatch.push_back(&VMEnv::bzeroi64);
        op_dispatch.push_back(&VMEnv::bnzeroi64);
        op_dispatch.push_back(&VMEnv::bzerodbl);
        op_dispatch.push_back(&VMEnv::bnzerodbl);
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
    }
     
    void VMEnv::dump(ostream& os){
        for(ssize_t i = (ssize_t)stack_top - 1; i >= 0; --i )
         os << "| "<< stack_seg[i] << "\t|\n";
    }
    size_t emitX(VMEnv::text_t& text){
        text.push_back(2);
        return 0;
    }
}