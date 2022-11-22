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

namespace ceps{
    namespace vm{
        namespace oblectamenta{
            using namespace std;
            enum class Opcode{
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
                    using text_t = vector<uint8_t>;
                    template<typename T> size_t store(T data){
                        auto t = text.size();
                        text.insert(text.end(), 
                                    (text_t::value_type*) &data, 
                                    (text_t::value_type*) &data + sizeof(data) );
                        return t;
                    }
                private:
                    text_t text;
            };
        }
    }
}