
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
#include <string>
#include <memory>
#include <iomanip>

#include "ceps_ast.hh"
#include "core/include/state_machine_simulation_core.hpp"
#include "core/include/vm/vm_base.hpp"

using namespace std;

namespace cepsplugin{
    static Ism4ceps_plugin_interface* plugin_master = nullptr;
    static const std::string version_info = "protobufish v0.1";
    static constexpr bool print_debug_info{true};
    ceps::ast::node_t plugin_entrypoint(ceps::ast::node_callparameters_t params);
}

class Arena{
    struct arena_header{
     arena_header* next{};
     arena_header* prev{};
     char* available{};
     char* limit{};
     size_t counter{};
    };

    arena_header* arenahds{};
    arena_header** arenatails{};
    arena_header* freeblocks{};
    size_t arena_count{};
    float resize_factor {1.1};
    public:
    Arena(size_t arena_count): arena_count{arena_count}{
     arenahds = (arena_header*)memset(malloc(arena_count * sizeof(arena_header)),0,arena_count * sizeof(arena_header));
     arenatails = (arena_header**)memset(malloc(arena_count * sizeof(arena_header*)),0,arena_count * sizeof(arena_header*));
     freeblocks = nullptr;
     for (size_t i{}; i < arena_count; ++i){
        arenatails[i] = &arenahds[i]; 
     }
    }
    char* allocate(size_t n, size_t arena){
        if (arena >= arena_count) return nullptr;
        auto ap = arenatails[arena];
        while(ap->available + n > ap->limit) {
                        
            if(!ap->next){
                if (freeblocks) { 
                    ap->next = freeblocks;
                    freeblocks->prev = ap; 
                    freeblocks = freeblocks->next;
                    if (freeblocks) freeblocks->prev = nullptr;
                    ap = arenatails[arena] = ap->next;
                    ap->next = nullptr;
                    ap->available = (char*)ap + sizeof(arena_header);
                    ap->counter = 0;
                    continue; 
                } else {
                 size_t m = n * resize_factor + sizeof(arena_header);
                 auto ap_prev = ap;
                 ap = ap->next = (arena_header*) malloc(m);
                 if (!ap) return nullptr;
                 ap->prev = ap_prev;
                 ap->next = nullptr;
                 ap->counter = 0;
                 ap->limit = (char*)ap + m;
                 ap->available = (char*)ap + sizeof(arena_header);
                 arenatails[arena] = ap;
                 continue;
               }
            }

        }
        if (ap){
            ap->available += n;
            ++ap->counter;
            return ap->available - n;
        }
        return nullptr;
    }
    arena_header* find_page(char* mem, size_t arena){
        for(auto p = &arenahds[arena]; p; p = p->next){
            if (p->limit > mem && (char*)p < mem) return p;
        }
        return nullptr;
    }

    void free(arena_header& page, size_t arena){
        if (arenatails[arena] == &page) arenatails[arena] = page.prev;
        if (page.next) page.next->prev = page.prev;
        if (page.prev) page.prev->next = page.next; 
        page.next = freeblocks;
        page.prev = nullptr;
        if(freeblocks) freeblocks->prev = &page;
        freeblocks = &page;
        
    }

    char* reallocate(char* mem, size_t n_old, size_t n_new, size_t arena){
        auto new_mem = allocate(n_new, arena); //(*)
        if (!new_mem) return nullptr;
        memcpy(new_mem,mem, n_old);   
        auto page = find_page(mem,arena);
        if (page && 0 == --page->counter){
            //INVARIANT: page is not referenced && last allocation (*) hit another page
            //=> page is neither head nor tail of page list
            free(*page, arena);            
        }
        return new_mem;
    }
    void free(size_t arena){
        if (!arenahds[arena].next) return; // nothing to free
        //INVARIANT: &arenahds[arena] != arenatails[arena]
        auto t = freeblocks;
        freeblocks = arenahds[arena].next; 
        if(arenahds[arena].next) arenahds[arena].next->prev = nullptr;
        arenatails[arena]->next = t;
        if (t) t->prev = arenatails[arena];
        arenatails[arena] = &arenahds[arena];
        arenahds[arena].next = arenahds[arena].prev = nullptr;
    }
    void free(size_t arena, char* mem){
        auto page = find_page(mem,arena);
        /*if(page){
            cerr << (page->prev == &arenahds[arena]) << "\n";
            cerr << (arenahds[arena].next == page) << "\n";
            cerr << (page->next == nullptr) << "\n"; 
            cerr << (arenahds[arena].prev == nullptr) << "\n";
            cerr << (freeblocks == nullptr) << "\n";
        }*/
        if (page && page != &arenahds[arena] && 0 == --page->counter) free(*page, arena);
        /*if(page){
            cerr << "\n";
            cerr << (arenahds[arena].next == nullptr) << "\n";
            cerr << (arenahds[arena].prev == nullptr) << "\n";
            cerr << (page->next == nullptr) << "\n"; 
            cerr << (page->prev == nullptr) << "\n"; 

        }*/
    
    }
};

Arena protobufish_memory(3);

static string escape_str(string const & s)
{
	string result;
	for(unsigned int i = 0; i != s.length();++i)
	{
		if (s[i] == '\n')
			result+="\\n";
		else if (s[i] == '\\')
			result+="\\\\";
		else if (s[i] == '\t')
			result+="\\t";
		else if (s[i] == '\r')
			result+="\\r";
		else if (s[i] == '"')
			result+="\\\"";
		else if (std::isprint(s[i]))
			result += s[i];
	}
	return result;
}


static size_t find_trailing_zero(char* buffer, size_t size)
{
    size_t r{};
    for (;r < size && buffer[r]; ++r);
    return r;
}

struct json_token{
    enum {undef,id, number, /*3*/string, boolean, null, lbrace, /*7*/rbrace, lsqrbra, rsqrbra, comma,colon, eoi /*end of input*/} type;
    size_t from, end;
    double value_f;
    int64_t value_i;
    bool value_b;    
    bool is_int;
    operator bool() const {return type != undef;}
};

optional<string> extract_str_value(json_token tok, string msg){
    //cerr << ">>" << msg << "<<" << tok.from << " - " << tok.from<< "\n";
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

optional<uint64_t> match_json_integer(char * buffer, size_t& loc, size_t n){
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

optional<double> match_json_fraction(char * buffer, size_t& loc, size_t n){
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

optional<json_token> get_token(char * buffer, size_t& loc, size_t n){
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
            if (isspace(buffer[loc]) || !isalpha(buffer[loc]) && !isdigit(buffer[loc]) ) return json_token{.type=json_token::boolean,.from=loc-4,.end=loc, .value_b = true};
            return {};
        } else if (buffer[loc] == 'f' && (loc + 4 < n) && buffer[loc+1] == 'a' && buffer[loc+2] == 'l' && buffer[loc+3] == 's' && buffer[loc+4] == 'e'){
            loc += 5; if (loc >= n) return json_token{.type=json_token::boolean,.from=loc-4,.end=loc, .value_b = false};
            if (isspace(buffer[loc]) || !isalpha(buffer[loc]) && !isdigit(buffer[loc]) ) return json_token{.type=json_token::boolean,.from=loc-4,.end=loc, .value_b = false};
            return {};
        } else if (buffer[loc] == 'n' && (loc + 3 < n) && buffer[loc+1] == 'u' && buffer[loc+2] == 'l' && buffer[loc+3] == 'l'){
            loc += 4; if (loc >= n) return json_token{.type=json_token::null,.from=loc-4,.end=loc};
            if (isspace(buffer[loc]) || !isalpha(buffer[loc]) && !isdigit(buffer[loc]) ) 
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

bool json2protobufish_test_lexer(string json, bool print_info = false){
   size_t loc{};
    size_t n{json.length()};
    json_token cur_tok{};
    do {
     auto cur_tok_ = get_token(json.data(), loc, n);
     if (!cur_tok_) return false;
     cur_tok = *cur_tok_;
     if (cur_tok.type == json_token::number){
        if  (print_info) cout << "Number: "<< setprecision(14) << cur_tok.value_f << '\n';
     } else if (cur_tok.type == json_token::string){
        string str(cur_tok.end - cur_tok.from, ' ');
        strncpy(str.data(),json.data() + cur_tok.from,cur_tok.end - cur_tok.from);
        if  (print_info) cout << "String: '"<< str << "'" << '\n';
        auto seval = extract_str_value(cur_tok,json);
        if  (print_info) if (!seval) cout << "String is not well formed\n";
        else  cout << "String (evaluated): '"<< *seval << "'" << '\n';
     } else if (cur_tok.type == json_token::boolean) {
        if  (print_info) cout << cur_tok.value_b << '\n';
     } else if (cur_tok.type == json_token::colon){
        if  (print_info) cout << ":\n";
     } else if (cur_tok.type == json_token::comma){
        if  (print_info) cout << ",\n";
     } else if (cur_tok.type == json_token::rsqrbra){
        if  (print_info) cout << "]\n";
     } else if (cur_tok.type == json_token::lsqrbra){
        if  (print_info) cout << "[\n";
     } else if (cur_tok.type == json_token::rbrace){
        if  (print_info) cout << "}\n";
     } else if (cur_tok.type == json_token::lbrace){
        if  (print_info) cout << "{\n";
     } else if (cur_tok.type == json_token::null) {
        if  (print_info) cout << "null" << '\n';
     } else if (cur_tok.type == json_token::eoi){
        if  (print_info) cout << "EOI\n";
     }
    } while (cur_tok.type != json_token::eoi);
    return true;
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
 Arena* arena;
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
    using namespace ceps::vm::oblectamenta;
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
    using namespace ceps::vm::oblectamenta;
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
    using namespace ceps::vm::oblectamenta;
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
    using namespace ceps::vm::oblectamenta;
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
bool json2protobufish_consume_object(ser_ctxt_t& ctx);
bool json2protobufish_consume_array(ser_ctxt_t& ctx);

bool json2protobufish_consume_element(ser_ctxt_t& ctx, bool read_token = true){
    using namespace ceps::vm::oblectamenta;
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
   using namespace ceps::vm::oblectamenta;
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
    using namespace ceps::vm::oblectamenta;
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
    using namespace ceps::vm::oblectamenta;
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

bool json2protobufish_internal(ser_ctxt_t& ctx){
    using namespace ceps::vm::oblectamenta;
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

optional< pair< char* , pair<size_t,size_t> >> json2protobufish(string json, Arena* arena, size_t arena_id){
    using namespace ceps::vm::oblectamenta;
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

    if (!json2protobufish_internal(ctx)) return {};    
    return {make_pair(ctx.buffer, make_pair(ctx.total, ctx.used))};
}

static size_t protobufish2stdrep(char* buffer, size_t size, std::string& res){
    using namespace ceps::vm::oblectamenta;
    string r;
    if (size == 0) return {};
    
    if (size < sizeof(msg_node)) return {};

    msg_node& root{ *(msg_node*)buffer };
    size_t len_extra_info{};
    if (root.what == msg_node::NODE){
        auto t = find_trailing_zero(buffer + sizeof(msg_node), size - sizeof(msg_node));
        len_extra_info = t + 1;
    }

    auto hd_size = sizeof(msg_node) + len_extra_info;
    
    if (root.size <  hd_size) return 0;
    auto content_size = root.size - hd_size;
    size_t consumed_content_bytes{};
    
    if (root.what == msg_node::ROOT || root.what == msg_node::NODE || root.what == msg_node::ARRAY){
        string prefix,suffix;
        if (root.what == msg_node::NODE)
        {
            if (!strlen((char*)((msg_node_ex*)buffer)->name)) prefix = "";
            else prefix = string{"\""}+ (char*)((msg_node_ex*)buffer)->name +"\":";
        } else if (root.what == msg_node::ROOT) {
            prefix = "";
            suffix = "";
        } else {
            prefix = "[";
            suffix = "]";
        }
        string inner;
        bool contains_nodes {};
        
        if (content_size){
            for (;consumed_content_bytes < content_size;){ 
                msg_node& n{ *(msg_node*)(buffer + hd_size + consumed_content_bytes)};
                string t;
                contains_nodes |= (n.what == msg_node::NODE); 
                consumed_content_bytes += 
                 protobufish2stdrep(buffer+hd_size+consumed_content_bytes, content_size - consumed_content_bytes, t);
                inner += t;
                if (consumed_content_bytes < content_size )inner += ",";
            }
        }
        if (root.what == msg_node::NODE ) 
         if (inner.size() == 0) inner = "{}";
         else if (contains_nodes) inner = "{" + inner + "}"; 
        res = prefix  + inner + suffix;   
    } else if (root.what == msg_node::INT32){
        msg_node_int32& m{ *(msg_node_int32*)&root};
        stringstream ss;
        ss << m.value;
        res = ss.str();
        return sizeof(msg_node_int32);
    } else if (root.what == msg_node::INT64){
        msg_node_int64& m{ *(msg_node_int64*)&root};
        stringstream ss;
        ss << m.value;
        res = ss.str();
        return sizeof(msg_node_int64);
    } else if (root.what == msg_node::BOOLEAN){
        msg_node_bool& m{ *(msg_node_bool*)&root};
        stringstream ss;
        ss << m.value;
        res = ss.str();
        return sizeof(msg_node_bool);
    } else if (root.what == msg_node::F64){
        msg_node_f64& m{ *(msg_node_f64*)&root};
        stringstream ss;
        ss << m.value;
        res = ss.str();
        return sizeof(msg_node_f64);
    } else if (root.what == msg_node::SZ){
        msg_node_sz& m{ *(msg_node_sz*)&root};
        res = "\"" + escape_str(m.value)+ "\"";
        return sizeof(msg_node) + strlen(m.value) + 1;
    } else if (root.what == msg_node::NIL){
        res = "null";
        return sizeof(msg_node);
    }
    return hd_size + content_size;
}


static size_t protobufish2json(char* buffer, size_t size, std::string& res){
    using namespace ceps::vm::oblectamenta;
    string r;
    if (size == 0) return {};
    
    if (size < sizeof(msg_node)) return {};

    msg_node& root{ *(msg_node*)buffer };
    size_t len_extra_info{};
    if (root.what == msg_node::NODE){
        auto t = find_trailing_zero(buffer + sizeof(msg_node), size - sizeof(msg_node));
        len_extra_info = t + 1;
    }

    auto hd_size = sizeof(msg_node) + len_extra_info;
    
    if (root.size <  hd_size) return 0;
    auto content_size = root.size - hd_size;
    size_t consumed_content_bytes{};
    
    if (root.what == msg_node::ROOT || root.what == msg_node::NODE || root.what == msg_node::ARRAY){
        string prefix,suffix;
        if (root.what == msg_node::NODE)
        {
            if (!strlen((char*)((msg_node_ex*)buffer)->name)) {prefix = "{"; suffix = "}";}
            else prefix = string{"\""}+ (char*)((msg_node_ex*)buffer)->name +"\":";
        } else if (root.what == msg_node::ROOT) {
            prefix = "";
            suffix = "";
        } else {
            prefix = "[";
            suffix = "]";
        }
        string inner;
        bool contains_nodes {};
        
        if (content_size){
            for (;consumed_content_bytes < content_size;){ 
                msg_node& n{ *(msg_node*)(buffer + hd_size + consumed_content_bytes)};
                string t;
                contains_nodes |= (n.what == msg_node::NODE); 
                consumed_content_bytes += 
                 protobufish2json(buffer+hd_size+consumed_content_bytes, content_size - consumed_content_bytes, t);
                inner += t;
                if (consumed_content_bytes < content_size )inner += ",";
            }
        }
        
        /*if (root.what == msg_node::NODE ) 
         if (inner.size() == 0) inner = "{}";
         else if (contains_nodes) inner = "{" + inner + "}";*/ 

        res = prefix  + inner + suffix;   
    } else if (root.what == msg_node::INT32){
        msg_node_int32& m{ *(msg_node_int32*)&root};
        stringstream ss;
        ss << m.value;
        res = ss.str();
        return sizeof(msg_node_int32);
    } else if (root.what == msg_node::INT64){
        msg_node_int64& m{ *(msg_node_int64*)&root};
        stringstream ss;
        ss << m.value;
        res = ss.str();
        return sizeof(msg_node_int64);
    } else if (root.what == msg_node::BOOLEAN){
        msg_node_bool& m{ *(msg_node_bool*)&root};
        res = m.value?"true":"false" ;
        return sizeof(msg_node_bool);
    } else if (root.what == msg_node::F64){
        msg_node_f64& m{ *(msg_node_f64*)&root};
        stringstream ss;
        ss << m.value;
        res = ss.str();
        return sizeof(msg_node_f64);
    } else if (root.what == msg_node::SZ){
        msg_node_sz& m{ *(msg_node_sz*)&root};
        res = "\"" + escape_str(m.value)+ "\"";
        return sizeof(msg_node) + strlen(m.value) + 1;
    } else if (root.what == msg_node::NIL){
        res = "null";
        return sizeof(msg_node);
    }
    return hd_size + content_size;
}

ceps::ast::node_t cepsplugin::plugin_entrypoint(ceps::ast::node_callparameters_t params){
    using namespace std;
    using namespace ceps::ast;
    using namespace ceps::interpreter;
    bool print_info{true};
    bool print_hexdump{};

    auto data = get_first_child(params);    
    if (!is<Ast_node_kind::structdef>(data)) return nullptr;
    auto& ceps_struct = *as_struct_ptr(data);
    //Lexer Tests
    for(auto e : children(ceps_struct)){
        if (!is<Ast_node_kind::structdef>(e) || name(as_struct_ref(e)) != "lexer_test" ) continue;
        if (print_info) cout << "*** Lexer Tests:\n";
        for(auto ee : children(as_struct_ref(e))){
         if (!is<Ast_node_kind::string_literal>(ee)) continue;
         if (print_info) cout <<"Input: "<< value(as_string_ref(ee)) << "\n";
         auto r{json2protobufish_test_lexer(value(as_string_ref(ee)), print_info)};
         if (print_info) if (!r) cout << "Failed\n";
        }
    }
    // Serialization
    for(int i = 0; i < 10000;++i) {
        for(auto e : children(ceps_struct)){
        if (!is<Ast_node_kind::structdef>(e) || name(as_struct_ref(e)) != "serialization_test" ) continue;
        if (print_info) cout << "*** Serialization Tests:\n";
        for(auto ee : children(as_struct_ref(e))){
         if (!is<Ast_node_kind::string_literal>(ee)) continue;
         if (print_info) cout <<">>"<< value(as_string_ref(ee)) << "\n";
         auto r{json2protobufish(value(as_string_ref(ee)), &protobufish_memory, 0)};
         if (print_info)if (!r) cout << "Failed\n";
         string deser;
         if (print_info && print_hexdump){
          int w = 10;
          int i = 0;
          if (r) for ( auto p = r->first; p != r->first + r->second.second; ++p){
            if (i % w == 0) cout << setw(5) << i << ": ";
            cout << setw(3) << (int)(*(unsigned char*)p) << " ";
            ++i;
            if (i % w == 0) {       
                if (i){
                    cout << "  ";
                    for (auto pp = p - (w-1); ;++pp){
                        if (isprint(*pp)) cout << (char) *pp << " ";
                        else cout << ". ";
                        if (p == pp) break;
                    }

                }
                cout << "\n";
            }
          }
          cout << "\n";
         }
         protobufish2json(r->first, r->second.second,deser);
         if (print_info)if (r)cout << "<<" << deser << "\n";
         protobufish_memory.free(0,r->first);
        }
    }
    //protobufish_memory.free(0);

    }

    if (print_info) cout <<"\n\n";
    auto result = mk_struct("result");
    children(*result).push_back(mk_int_node(42));
    return result;
}

extern "C" void init_plugin(IUserdefined_function_registry* smc)
{
  cepsplugin::plugin_master = smc->get_plugin_interface();
  cepsplugin::plugin_master->reg_ceps_phase0plugin("protobufish", cepsplugin::plugin_entrypoint);
}					
				