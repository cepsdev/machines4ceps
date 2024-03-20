
#include <stdlib.h>
#include <iostream>
#include <ctype.h>
#include <chrono>
#include <sstream>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <queue>
#include <unordered_map>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <stdlib.h>
#include <map>
#include <algorithm>
#include <future>
#include <memory>
#include <netinet/sctp.h> 
#include <vector>
#include <string>
#include <optional>

#include "ceps_ast.hh"
#include "core/include/state_machine_simulation_core.hpp"

#include "core/include/vm/vm_base.hpp"
#include "core/include/vm/oblectamenta-assembler.hpp"
#include "core/include/vm/oblectamenta-comp-graph.hpp"

#include <endian.h>
#include <bit>

#define ast_proc_prolog  using namespace ceps::ast;\
    using namespace ceps::vm::oblectamenta;\
    using namespace ceps::interpreter;\
    using namespace std;

namespace cepsplugin{
    static Ism4ceps_plugin_interface* plugin_master = nullptr;
    static const std::string version_info = "INSERT_NAME_HERE v0.1";
    static constexpr bool print_debug_info{true};
    ceps::ast::node_t operation(ceps::ast::node_callparameters_t params);
}

struct sha1_t{
    char digest[20];
};

uint32_t rotl5(uint32_t value)
{
    return (value << 5 ) | (value >> 27); 
}

uint32_t rotl1(uint32_t value)
{
    return ( value << 1 ) | (value >> 31); 
}

uint32_t rotl15(uint32_t value)
{
    return ( value << 15 ) | (value >> 17); 
}

uint32_t rotl30(uint32_t value)
{
    return ( value << 30 ) | (value >> 2); 
}
static std::string encode_base64(void * mem, size_t len);
static std::string encode_hex(void * mem, size_t len);

sha1_t sha1(std::string msg){
    using namespace std;


    auto len{msg.size()};
    char* data{new char[len + 64]};
    memset(data, 0, len + 64);
    memcpy(data, msg.c_str(), len);

    sha1_t r;
    memset(r.digest,0,sizeof(r.digest));
    uint64_t bit_length{len * 8};
    //cout << "bit_length: " << bit_length << '\n';
    uint64_t ext_bit_length = bit_length + 1/*append one*/ + 64 /*size of original message as uint64*/;
    uint64_t padded_bit_length{ (512 - ext_bit_length % 512)  + ext_bit_length };
    uint64_t padding_bit_length{ padded_bit_length - bit_length};
    

    *(data + len) = 0x80;

    auto padded_data_len{padded_bit_length >> 3};
    *(uint64_t*)(data + padded_data_len - 8) = htobe64(bit_length);

    uint32_t A = 0x67452301;
    uint32_t B = 0xefcdab89;
    uint32_t C = 0x98badcfe;
    uint32_t D = 0x10325476;
    uint32_t E = 0xc3d2e1f0;

    uint32_t W[80];

    for(size_t i{}; i < padded_data_len ;  i +=64 ){
        auto a = A;
        auto b = B;
        auto c = C;
        auto d = D;        
        auto e = E;

        auto frag = data + i;
        memcpy(W, frag, 64);
        for(size_t j{}; j < 16; ++j) W[j] = be32toh(W[j]);

        for( size_t t = 16; t < 80; ++t){
            W[t] = rotl1(W[t-3] ^ W[t-8] ^ W[t-14] ^ W[t-16]) ;
        }

        for( size_t t = 0; t < 20; ++t){
            auto temp{ rotl5(a) + ( (b & c) | ( (~b) & d) )  + e + W[t] + 0x5a827999 };            
            e = d;
            d = c;
            c = rotl30(b);
            b = a;
            a = temp;
        }
        for( size_t t = 20; t < 40; ++t){
            auto temp{ rotl5(a) + (b ^ c ^ d)  + e + W[t] + 0x6ed9eba1 };
            e = d;
            d = c;
            c = rotl30(b);
            b = a;
            a = temp;
        }
        for( size_t t = 40; t < 60; ++t){
            auto temp{ rotl5(a) + ( (b & c) | (b & d)  | (c & d) )  + e + W[t] + 0x8f1bbcdc };            
            e = d;
            d = c;
            c = rotl30(b);
            b = a;
            a = temp;
        }
        for( size_t t = 60; t < 80; ++t){
            auto temp{ rotl5(a) + ( b ^ c ^ d )  + e + W[t] + 0xca62c1d6 };
            e = d;
            d = c;
            c = rotl30(b);
            b = a;
            a = temp;
        }
        A += a;
        B += b; 
        C += c; 
        D += d; 
        E += e;
    }
    delete []data;

    * (uint32_t*)r.digest = htobe32(A);
    * (uint32_t*)(r.digest  + sizeof(uint32_t)) = htobe32(B);
    * (uint32_t*)(r.digest + 2*sizeof(uint32_t)) =htobe32(C);
    * (uint32_t*)(r.digest + 3*sizeof(uint32_t)) = htobe32(D);
    * (uint32_t*)(r.digest + 4*sizeof(uint32_t)) = htobe32(E);
    return r;
}

static char base64set[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static std::string encode_base64(void * mem, size_t len){
 unsigned char * memory = (unsigned char*)mem;
 if (len == 0) return {};
 int rest = len % 3;
 size_t main_part = len - rest;
 int out_len = (len / 3) * 4;
 short unsigned int padding = 0;
 if (rest == 1) {out_len += 4;padding=2;} else if (rest == 2){ out_len +=4;padding=1;}
 std::string r;
 r.resize(out_len);
 size_t j = 0;
 size_t jo = 0;

 for(; j < main_part; j+=3,jo+=4){
  r[jo] = base64set[ *(memory + j) >> 2];
  r[jo+1] = base64set[  ( (*(memory + j) & 3) << 4)  | ( *(memory + j + 1) >> 4) ];
  r[jo+2] = base64set[ ( (*(memory + j + 1) & 0xF) << 2 )  | (*(memory + j + 2) >> 6) ];
  r[jo+3] = base64set[*(memory + j + 2) & 0x3F];
 }
 if (rest == 1){
  r[jo] = base64set[ *(memory + j) >> 2];
  r[jo+1] = base64set[ (*(memory + j) & 3) << 4];
  j+=2;jo+=2;
 } else if (rest == 2) {
  r[jo] = base64set[ *(memory + j) >> 2];
  r[jo+1] = base64set[  ( (*(memory + j) & 3) << 4)  | ( *(memory + j + 1) >> 4) ];
  r[jo+2] = base64set[ (*(memory + j + 1) & 0xF) << 2 ];
  j+=3;jo+=3;
 }
 if (padding == 1) r[jo]='='; else if (padding == 2) {r[jo] = '='; r[jo+1] = '=';}
 return r;
}


static std::string encode_base64(std::string s){
 return encode_base64((void*)s.c_str(),s.length());
}

static std::string encode_hex(void * mem, size_t len){
 if (len == 0) return {};
 unsigned char * memory = (unsigned char*)mem;
 std::string r;
 char m[] = {'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};
 for(size_t i{}; i < len;++i)
  {
    r.append(1,m[memory[i] >> 4 ]);
    r.append(1,m[memory[i] & 0xf]);
  }
 return r;
}

ceps::ast::node_t cepsplugin::operation(ceps::ast::node_callparameters_t params){
    ast_proc_prolog
    if (!children(params).size())    
     return mk_undef();

    auto data = get_first_child(params);    

    if (!is<Ast_node_kind::structdef>(data)) {
        return mk_undef();
    }
    auto& ceps_struct = *as_struct_ptr(data);
    if (name(ceps_struct) == "sha1" && children(ceps_struct).size()){
        for(auto e : children(ceps_struct)){
            if(is<Ast_node_kind::string_literal>(e)){
                auto r = sha1( value(as_string_ref(e)) );
                auto a = mk_struct("hex");
                children(a).push_back(mk_string(encode_hex(r.digest, sizeof(sha1_t::digest))));
                auto b = mk_struct("base64");
                children(b).push_back(mk_string(encode_base64(r.digest, sizeof(sha1_t::digest))));
                auto c = mk_struct("sha1_result");
                children(c).push_back(a);
                children(c).push_back(b);
                return  c;
            }
        }
    } 
    return mk_undef();
}

extern "C" void init_plugin(IUserdefined_function_registry* smc)
{
  cepsplugin::plugin_master = smc->get_plugin_interface();
  cepsplugin::plugin_master->reg_ceps_phase0plugin("operation", cepsplugin::operation);
}					
				