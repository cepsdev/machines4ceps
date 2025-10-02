#pragma once
/*
Copyright 2025 Tomas Prerovsky (cepsdev@hotmail.com).

Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to iswp128b8n writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include "arena.hpp"
#include <optional>
#include <tuple>
#include <string>
#include <math.h>
#include <utility>
#include "vm/vm_base.hpp"

namespace ceps{
    namespace vm{
        namespace oblectamenta{

struct json_token{
    enum { undef,
           id, 
           number, 
           /*3*/string, 
           boolean, 
           null, 
           lbrace, 
           /*7*/rbrace, 
           lsqrbra, 
           rsqrbra, 
           comma,
           colon, 
           eoi /*end of input*/} type;
    size_t from, end;
    double value_f;
    int64_t value_i;
    bool value_b;    
    bool is_int;
    operator bool() const {return type != undef;}
};

template<typename arena_allocator_t>
 class fast_json{
  public:
  template<typename T>
  using optional = std::optional<T>;
  template<typename T1, typename T2>
  using pair = std::pair<T1,T2>;
  using string = std::string;

  static optional<string> extract_str_value(json_token tok, string msg){
    if (tok.type != json_token::string || tok.from > tok.end || tok.end > msg.length())
     return {};
    string r; r.resize(tok.end - tok.from +10,'\0');
    size_t to{};
    for(size_t i{tok.from}; i < tok.end; ++i){
        if (msg[i] != '\\') {r[to++] = msg[i]; continue;}
        ++i; if (i >= tok.end) return {};
        if (msg[i] == '"') r[to++] = '"';
        else if (msg[i] == '/') r[to++] = '/';
        else if (msg[i] == '\\') r[to++] = '\\';
        else if (msg[i] == 'b') r[to++] = '\b';
        else if (msg[i] == 'f') r[to++] = '\f';
        else if (msg[i] == 'n') r[to++] = '\n';
        else if (msg[i] == 'r') r[to++] = '\r';
        else if (msg[i] == 't') r[to++] = '\t';
        else if (msg[i] == 'u'){
            if (i + 4 >= tok.end) return {};
            auto hexvalue = [&](size_t l)->optional<int> {
                if (isdigit(msg[l])) return msg[l] - '0';
                if(msg[l]=='a' || msg[l]=='b' || msg[l]=='c' || msg[l]=='d' || msg[l]=='e' || msg[l]=='f' )
                return msg[l] - 'a' + 10;
                if(msg[l]=='A' || msg[l]=='B' || msg[l]=='C' || msg[l]=='D' || msg[l]=='E' || msg[l]=='F' )
                return msg[l] - 'A' + 10;
                return {};
            };
            uint16_t v{};
            auto d = hexvalue(i+4); if (d) v = *d; else return{};
            d = hexvalue(i+3); if (d) v += *d << 4; else return{};
            d = hexvalue(i+2); if (d) v += *d << 8; else return{};
            d = hexvalue(i+1); if (d) v += *d << 12; else return{};
            i += 4;
            *(uint16_t*)(r.data()+to) = v;
            to += 2; 
        }
    }
    r.resize(to,'\0');
    return r;
  }
 
  static optional<uint64_t> match_json_integer(char * buffer, size_t& loc, size_t n){
    uint64_t r{};
    auto start_loc{loc};
    for (;loc < n;++loc){
        if (!isdigit(static_cast<unsigned char>(buffer[loc]))) { if (start_loc == loc) return {}; break;  }
        r = 10*r +  (static_cast<decltype(r)>(buffer[loc]) - '0'); 
    }
    if (start_loc + 1 < loc) //more than a single digit was read
     if (buffer[start_loc] == '0') return {};
    return r;
  }

  static optional<double> match_json_fraction(char * buffer, size_t& loc, size_t n){
    double r{};
    double pre {1.0};
    auto start_loc{loc};
    for (;loc < n;++loc){
        if (!isdigit(static_cast<unsigned char>(buffer[loc]))) { if (start_loc == loc) return {}; break;  }
        pre *= 0.1;
        int digit = (static_cast<int>(buffer[loc]) - '0');
        if (digit != 0) r += pre * static_cast<double>(digit); 
    }
    return r;
  }

  static optional<json_token> get_token(char * buffer, size_t& loc, size_t n){
    for(;loc < n;){ 
        if (isspace(static_cast<unsigned char>(buffer[loc]))) {++loc; continue;}
        json_token tok{};
        if (isdigit(buffer[loc]) || buffer[loc] == '.' || buffer[loc] == '-' ){
            tok.type = json_token::number;
            bool sign_neg{buffer[loc] == '-'};
            uint64_t integer_part {};
            double fraction_part {};
            uint64_t exp_part {};
            bool sign_neg_exp_part{};

            if (sign_neg) {++loc; if (loc >= n) return{}; if (!isdigit(buffer[loc])) return {}; }
            
            if (isdigit(buffer[loc])) {
                auto r{match_json_integer(buffer,loc,n)};
                if (!r) return {};
                integer_part = *r;
            }
            if (buffer[loc] == '.'){
                ++loc;
                auto r{match_json_fraction(buffer,loc,n)};
                if (!r) return {};
                fraction_part = *r;
            } else tok.is_int = true;
            if (buffer[loc] == 'e' || buffer[loc] == 'E'){
                loc ++; if (loc >= n) return {};
                if (buffer[loc] == '-'){sign_neg_exp_part = true; ++loc;if (loc >= n) return {};}
                else if (buffer[loc] == '+'){ ++loc;if (loc >= n) return {};}
                auto r{match_json_integer(buffer,loc,n)};
                if (!r) return {};
                exp_part = *r;
                if (exp_part != 0 && sign_neg_exp_part) tok.is_int = true;                               
            }
            tok.value_f = static_cast<double>(integer_part);
            
            tok.value_i = static_cast<int64_t>(integer_part);
            if (sign_neg) tok.value_i *= -1;           
            tok.value_f += fraction_part;
            if (exp_part) {
                if (sign_neg_exp_part) tok.value_f *= pow(0.1,static_cast<double>(exp_part));
                else tok.value_f *= pow(10.0,static_cast<double>(exp_part));
            }                                 
            if (sign_neg) tok.value_f *= -1.0;
            return tok;
        } else if (buffer[loc] == '"'){
            ++loc;
            if (loc >= n) return {};
            tok.from = loc;
            tok.type = json_token::string;
            for(;loc < n && buffer[loc]!='"';++loc){
                if (buffer[loc] == '\\') {
                    ++loc; if (loc >= n) return {};
                    if (buffer[loc] == '\"'|| buffer[loc] == '\\' || buffer[loc] == '/' || buffer[loc] == 'b'|| buffer[loc] == 'f'
                       || buffer[loc] == 'n' || buffer[loc] == 'r' || buffer[loc] == 't' ) continue;
                    if (buffer[loc] != 'u') return {};
                    if (loc + 4 >= n) return {};
                    auto is_hexdigit = [&](size_t l)->bool {
                     if (!isdigit(buffer[l]) && !(buffer[l]=='a' || buffer[l]=='A' || buffer[l]=='b' || buffer[l]=='B'
                                                  || buffer[l]=='c' || buffer[l]=='C' || buffer[l]=='d' || buffer[l]=='D'
                                                  || buffer[l]=='e' || buffer[l]=='E' || buffer[l]=='f' || buffer[l]=='F') ) 
                     return false;
                     return true; 
                    };
                    if (!is_hexdigit(loc+1) || !is_hexdigit(loc+2) || !is_hexdigit(loc+3) || !is_hexdigit(loc+4)) return{};
                } 
            }
            if (buffer[loc]!='"') return {};
            tok.end = loc; ++loc;
            return tok;
        } else if (buffer[loc] == 't' && (loc + 3 < n) && buffer[loc+1] == 'r' && buffer[loc+2] == 'u' && buffer[loc+3] == 'e'){
            loc += 4; if (loc >= n) return json_token{.type=json_token::boolean,.from=loc-4,.end=loc, .value_b = true};
            if (isspace(buffer[loc]) || (!isalpha(buffer[loc]) && !isdigit(buffer[loc])) ) return json_token{.type=json_token::boolean,.from=loc-4,.end=loc, .value_b = true};
            return {};
        } else if (buffer[loc] == 'f' && (loc + 4 < n) && buffer[loc+1] == 'a' && buffer[loc+2] == 'l' && buffer[loc+3] == 's' && buffer[loc+4] == 'e'){
            loc += 5; if (loc >= n) return json_token{.type=json_token::boolean,.from=loc-4,.end=loc, .value_b = false};
            if (isspace(buffer[loc]) || (!isalpha(buffer[loc]) && !isdigit(buffer[loc])) ) return json_token{.type=json_token::boolean,.from=loc-4,.end=loc, .value_b = false};
            return {};
        } else if (buffer[loc] == 'n' && (loc + 3 < n) && buffer[loc+1] == 'u' && buffer[loc+2] == 'l' && buffer[loc+3] == 'l'){
            loc += 4; if (loc >= n) return json_token{.type=json_token::null,.from=loc-4,.end=loc};
            if (isspace(buffer[loc]) || (!isalpha(buffer[loc]) && !isdigit(buffer[loc])) ) 
             return json_token{.type=json_token::null,.from=loc-4,.end=loc};
            return {};
        } else if (buffer[loc] == '{'){++loc;return json_token{.type=json_token::lbrace,.from=loc-1,.end=loc};}
        else if (buffer[loc] == '}'){++loc;return json_token{.type=json_token::rbrace,.from=loc-1,.end=loc};}
        else if (buffer[loc] == ','){++loc;return json_token{.type=json_token::comma,.from=loc-1,.end=loc};}
        else if (buffer[loc] == ':'){++loc;return json_token{.type=json_token::colon,.from=loc-1,.end=loc};}
        else if (buffer[loc] == '['){++loc;return json_token{.type=json_token::lsqrbra,.from=loc-1,.end=loc};}
        else if (buffer[loc] == ']'){++loc;return json_token{.type=json_token::rsqrbra,.from=loc-1,.end=loc};}
        else if (isalpha(buffer[loc])){
            auto start = loc; ++loc;
            for(;(loc < n) && isalnum(buffer[loc]) ;++loc);
            return json_token{.type=json_token::id,.from=start,.end=loc};
        }
        else return {};
    } 
    return {json_token{json_token::eoi}};
   }
   struct ser_ctxt_t{
    json_token cur_tok{.type=json_token::undef};
    json_token lookahead{.type=json_token::undef}; 
    string& input;
    size_t loc;
    size_t n;
    char* buffer;
    size_t total{};
    size_t used{};
    size_t cur_node_ofs;
    arena_allocator_t* arena;
    size_t arena_id;
    ser_ctxt_t(string& input): input{input},loc{}, n{input.length()} {}
    size_t available_space(){
        return total - used;
    }
    bool read_token(){
     if (lookahead.type != json_token::undef){
        cur_tok = lookahead;
        lookahead.type = json_token::undef;
        return true;
     } else { 
       auto cur_tok_ = get_token(input.data(), loc, n);
       if (cur_tok_) cur_tok = *cur_tok_;
       return cur_tok_.has_value();
     }
    }
    json_token peek(){
     if (lookahead.type != json_token::undef) return lookahead;
     auto lookahead_ = get_token(input.data(), loc, n);
     if (!lookahead_) {lookahead.type = json_token::undef; return lookahead;}
     lookahead = *lookahead_;
     return lookahead;
    }
   };
   bool json2protobufish_consume_number(ser_ctxt_t& ctx){
    if (ctx.available_space()<sizeof(msg_node_f64)){
      size_t new_size = (sizeof(msg_node_f64) + ctx.total) * 1.1;
      ctx.buffer = ctx.arena->reallocate(ctx.buffer, ctx.total, new_size , ctx.arena_id);
      if (!ctx.buffer) return false;
      ctx.total = new_size;
    }
    auto cur_node = ((msg_node*)(ctx.buffer + ctx.cur_node_ofs));
    ((msg_node_f64*)(ctx.buffer + ctx.used))->what = msg_node::F64;
    ((msg_node_f64*)(ctx.buffer + ctx.used))->size = sizeof(msg_node_f64);
    ((msg_node_f64*)(ctx.buffer + ctx.used))->value = ctx.cur_tok.value_f;
     ctx.used += sizeof(msg_node_f64); 
     cur_node->size += sizeof(msg_node_f64); 
    return ctx.read_token();
   }
   bool json2protobufish_consume_string(ser_ctxt_t& ctx){
    auto value_ = extract_str_value(ctx.cur_tok, ctx.input);
    if (!value_) return false;
    auto value = *value_;
    if (ctx.available_space()<sizeof(msg_node) + value.length() + 1){
      size_t new_size = (sizeof(msg_node) + value.length() + ctx.total) * 1.1;
      ctx.buffer = ctx.arena->reallocate(ctx.buffer, ctx.total, new_size , ctx.arena_id);
      if (!ctx.buffer) return false;
      ctx.total = new_size;
    }
    auto cur_node = ((msg_node*)(ctx.buffer + ctx.cur_node_ofs));
    auto new_node = (msg_node*)(ctx.buffer + ctx.used); 
    new_node->what = msg_node::SZ;
    new_node->size = sizeof(msg_node) + value.length() + 1;
    ctx.used += sizeof(msg_node) + value.length() + 1; 
    cur_node->size += sizeof(msg_node) + value.length() + 1;
    *(((char*)new_node) + sizeof(msg_node) + value.length()) = '\0';
    strncpy( ((char*)new_node) + sizeof(msg_node), value.data(), value.length());
    return ctx.read_token();
   }
   bool json2protobufish_consume_null(ser_ctxt_t& ctx){
    if (ctx.available_space()<sizeof(msg_node)){
      size_t new_size = (sizeof(msg_node) + ctx.total) * 1.1;
      ctx.buffer = ctx.arena->reallocate(ctx.buffer, ctx.total, new_size , ctx.arena_id);
      if (!ctx.buffer) return false;
      ctx.total = new_size;
    }
    auto cur_node = ((msg_node*)(ctx.buffer + ctx.cur_node_ofs));        
    ((msg_node*)(ctx.buffer + ctx.used))->what = msg_node::NIL;
    ((msg_node*)(ctx.buffer + ctx.used))->size = sizeof(msg_node);
    ctx.used += sizeof(msg_node); 
    cur_node->size += sizeof(msg_node); 
    return ctx.read_token();
   }
   bool json2protobufish_consume_boolean(ser_ctxt_t& ctx){
    if (ctx.available_space()<sizeof(msg_node_bool)){
      size_t new_size = (sizeof(msg_node_bool) + ctx.total) * 1.1;
      ctx.buffer = ctx.arena->reallocate(ctx.buffer, ctx.total, new_size , ctx.arena_id);
      if (!ctx.buffer) return false;
      ctx.total = new_size;
    }
    auto cur_node = ((msg_node*)(ctx.buffer + ctx.cur_node_ofs));        
    ((msg_node_bool*)(ctx.buffer + ctx.used))->what = msg_node::BOOLEAN;
    ((msg_node_bool*)(ctx.buffer + ctx.used))->size = sizeof(msg_node_bool);
    ((msg_node_bool*)(ctx.buffer + ctx.used))->value = ctx.cur_tok.value_b;
    ctx.used += sizeof(msg_node_bool); 
    cur_node->size += sizeof(msg_node_bool);
    return ctx.read_token(); 
   }
   bool json2protobufish_consume_element(ser_ctxt_t& ctx, bool read_token = true){
    if (read_token) if (!ctx.read_token()) return false;
    if (ctx.cur_tok.type == json_token::eoi) return false;
    if (ctx.cur_tok.type == json_token::number){
        if(!json2protobufish_consume_number(ctx)) return false;        
    } else if (ctx.cur_tok.type == json_token::string){
        if(!json2protobufish_consume_string(ctx)) return false;
    } else if (ctx.cur_tok.type == json_token::null){
        if(!json2protobufish_consume_null(ctx)) return false;
    } else if (ctx.cur_tok.type == json_token::boolean){
        if(!json2protobufish_consume_boolean(ctx)) return false;
    } else if (ctx.cur_tok.type == json_token::lbrace){
        if(!json2protobufish_consume_object(ctx)) return false;
    } else if (ctx.cur_tok.type == json_token::lsqrbra){
        if(!json2protobufish_consume_array(ctx)) return false;
    }
    return true;
   }
   bool json2protobufish_consume_member(ser_ctxt_t& ctx){
    auto value_ = extract_str_value(ctx.cur_tok, ctx.input);
    if (!value_) return false;
    auto value = *value_;
    if (ctx.available_space()<sizeof(msg_node) + value.length() + 1){
      size_t new_size = (ctx.total+sizeof(msg_node) + value.length() + 1) * 1.1;
      ctx.buffer = ctx.arena->reallocate(ctx.buffer, ctx.total, new_size , ctx.arena_id);
      if (!ctx.buffer) return false;
      ctx.total = new_size;
    }
    auto cur_node = ((msg_node*)(ctx.buffer + ctx.cur_node_ofs));
       
    if(!ctx.read_token()) return false;
    if (ctx.cur_tok.type != json_token::colon) return false;

    msg_node* new_node = (msg_node*)(ctx.buffer + ctx.used);
    auto new_ofs = ctx.used; 
    new_node->what = msg_node::NODE;
    new_node->size = sizeof(msg_node) + value.length() + 1;
    *( (char*)(ctx.buffer + ctx.used + sizeof(msg_node) + value.length()))= '\0';
    strncpy((char*)(ctx.buffer + ctx.used + sizeof(msg_node)), value.data(), value.length());
    cur_node->size += new_node->size;
   
    auto t = ctx.cur_node_ofs;
    auto old_size = new_node->size;
    ctx.cur_node_ofs =  ctx.used;
    ctx.used += new_node->size;

    if (!json2protobufish_consume_element(ctx)) return false;
    ctx.cur_node_ofs = t;
    cur_node = ((msg_node*)(ctx.buffer + ctx.cur_node_ofs));
    new_node = (msg_node*)(ctx.buffer + new_ofs);
    cur_node->size += new_node->size - old_size;
    return true;
   }
   bool json2protobufish_consume_object(ser_ctxt_t& ctx){
    if (ctx.available_space()<sizeof(msg_node) + 1){
      size_t new_size = (ctx.total + sizeof(msg_node) + 1) * 1.1;
      ctx.buffer = ctx.arena->reallocate(ctx.buffer, ctx.total, new_size , ctx.arena_id);
      if (!ctx.buffer) return false;
      ctx.total = new_size;
    } 
    auto cur_node = ((msg_node*)(ctx.buffer + ctx.cur_node_ofs));   
    msg_node* new_node = (msg_node*)(ctx.buffer + ctx.used);
    auto new_ofs = ctx.used; 
    new_node->what = msg_node::NODE;
    new_node->size = sizeof(msg_node) + 1;
    *((char*)(ctx.buffer + ctx.used + sizeof(msg_node)))= '\0';
    cur_node->size += new_node->size;
    
    auto t = ctx.cur_node_ofs;
    auto old_size = new_node->size;
    ctx.cur_node_ofs = ctx.used;
    ctx.used += new_node->size; 
    if (!ctx.read_token()) return false; //consume '{'
    for(;ctx.cur_tok.type == json_token::string || ctx.cur_tok.type == json_token::comma;){
     if (ctx.cur_tok.type == json_token::string){ // consume members
      if (!json2protobufish_consume_member(ctx)) return false;
     } else if (ctx.cur_tok.type == json_token::comma) if (!ctx.read_token()) return false;
    }//while
    ctx.cur_node_ofs = t;
    cur_node = ((msg_node*)(ctx.buffer + ctx.cur_node_ofs));
    new_node = (msg_node*)(ctx.buffer + new_ofs);
    cur_node->size += new_node->size - old_size;
    if (ctx.cur_tok.type != json_token::rbrace) return false;
    return ctx.read_token(); // consume '}'
   }
   bool json2protobufish_consume_array(ser_ctxt_t& ctx){
    if (ctx.available_space()<sizeof(msg_node)){
      size_t new_size = (ctx.total+sizeof(msg_node)) * 1.1;
      ctx.buffer = ctx.arena->reallocate(ctx.buffer, ctx.total, new_size , ctx.arena_id);
      if (!ctx.buffer) return false;
      ctx.total = new_size;
    }    
    auto cur_node = ((msg_node*)(ctx.buffer + ctx.cur_node_ofs));
    msg_node* new_node = (msg_node*)(ctx.buffer + ctx.used); 
    new_node->what = msg_node::ARRAY;
    new_node->size = sizeof(msg_node);
    cur_node->size += new_node->size;
    
    auto t = ctx.cur_node_ofs;
    auto old_size = new_node->size;
    ctx.cur_node_ofs = ctx.used;
    auto new_cur_ofs = ctx.used;
    ctx.used += new_node->size; 
    if (!ctx.read_token()) return false; //consume '['
    if(ctx.cur_tok.type != json_token::rsqrbra) 
    for(;;){ 
     if(!json2protobufish_consume_element(ctx,false)) return false;
     if (ctx.cur_tok.type == json_token::comma) {if (!ctx.read_token()) return false;}
     else break;
    }//while
    ctx.cur_node_ofs = t;
    cur_node = ((msg_node*)(ctx.buffer + ctx.cur_node_ofs));
    new_node = (msg_node*)(ctx.buffer + new_cur_ofs);
    cur_node->size += new_node->size - old_size;
    if (ctx.cur_tok.type != json_token::rsqrbra) return false;
    return ctx.read_token(); // consume ']'
   }
   bool read_internal(ser_ctxt_t& ctx){
    if (!ctx.read_token()) return false;
    if (ctx.cur_tok.type == json_token::eoi) return false;
    if (ctx.cur_tok.type == json_token::number){
        if(!json2protobufish_consume_number(ctx)) return false;
        return ctx.cur_tok.type == json_token::eoi;
    } else if (ctx.cur_tok.type == json_token::string){
        if(!json2protobufish_consume_string(ctx)) return false;
        return ctx.cur_tok.type == json_token::eoi;
    } else if (ctx.cur_tok.type == json_token::null){
        if(!json2protobufish_consume_null(ctx)) return false;
        return ctx.cur_tok.type == json_token::eoi;
    } else if (ctx.cur_tok.type == json_token::boolean){
        if(!json2protobufish_consume_boolean(ctx)) return false;
        return ctx.cur_tok.type == json_token::eoi;
    } else if (ctx.cur_tok.type == json_token::lbrace){
        if(!json2protobufish_consume_object(ctx)) return false;
        return ctx.cur_tok.type == json_token::eoi;
    } else if (ctx.cur_tok.type == json_token::lsqrbra){
        if(!json2protobufish_consume_array(ctx)) return false;
        return ctx.cur_tok.type == json_token::eoi;
    }
    return false;
   }
   using read_return_t = optional< pair< char* , pair<size_t,size_t> >>;
   
   optional< pair< char* , pair<size_t,size_t> >> read(string json, arena_allocator_t* arena, size_t arena_id){
    ser_ctxt_t ctx{json};
    ctx.total = sizeof(msg_node);
    ctx.used = 0;
    ctx.arena = arena;
    ctx.arena_id = arena_id;
    ctx.buffer = ctx.arena->allocate(ctx.total, ctx.arena_id);
    if (!ctx.buffer) return {};
    
    ctx.cur_node_ofs = 0;
    ((msg_node*)(ctx.buffer + ctx.cur_node_ofs))->what = msg_node::ROOT;
    ((msg_node*)(ctx.buffer + ctx.cur_node_ofs))->size = sizeof(msg_node);
    
    ctx.used += sizeof(msg_node);
    ctx.loc = 0;
    ctx.n = json.length();

    if (!read_internal(ctx)) return {};    
    return {std::make_pair(ctx.buffer, std::make_pair(ctx.total, ctx.used))};
   }
};

template<typename F>
size_t traverse_binary_representation(char* buffer, size_t size, F f){
    if (size == 0) return {};
    if (size < sizeof(msg_node)) return {};

    msg_node& root{ *(msg_node*)buffer };
    size_t len_extra_info{};
    if (root.what == msg_node::NODE){
        char * buf = buffer + sizeof(msg_node);
        size_t t{};for (;t < size - sizeof(msg_node) && buf[t]; ++t);
        len_extra_info = t + 1;
    }

    auto hd_size = sizeof(msg_node) + len_extra_info;
    
    if (root.size <  hd_size) return 0;
    auto content_size = root.size - hd_size;
    size_t consumed_content_bytes{};
    
    if (root.what == msg_node::ROOT || root.what == msg_node::NODE || root.what == msg_node::ARRAY){
        f(&root,true);
        bool contains_nodes {};        
        if (content_size){
            for (;consumed_content_bytes < content_size;){ 
                msg_node& n{ *(msg_node*)(buffer + hd_size + consumed_content_bytes)};
                contains_nodes |= (n.what == msg_node::NODE); 
                consumed_content_bytes += 
                 traverse_binary_representation(buffer+hd_size+consumed_content_bytes, content_size - consumed_content_bytes, f);
            }
        }
        f(&root,false);
    } else if (root.what == msg_node::INT32){
        msg_node_int32& m{ *(msg_node_int32*)&root};
        f(&m, false);
        return sizeof(msg_node_int32);
    } else if (root.what == msg_node::INT64){
        msg_node_int64& m{ *(msg_node_int64*)&root};
        f(&m, false);
        return sizeof(msg_node_int64);
    } else if (root.what == msg_node::BOOLEAN){
        msg_node_bool& m{ *(msg_node_bool*)&root};
        f(&m, false);
        return sizeof(msg_node_bool);
    } else if (root.what == msg_node::F64){
        msg_node_f64& m{ *(msg_node_f64*)&root};
        f(&m, false);
        return sizeof(msg_node_f64);
    } else if (root.what == msg_node::SZ){
        msg_node_sz& m{ *(msg_node_sz*)&root};
        f(&m, false);
        return sizeof(msg_node) + strlen(m.value) + 1;
    } else if (root.what == msg_node::NIL){
        f(&root, false);
        return sizeof(msg_node);
    }
    return hd_size + content_size;
}

}}}