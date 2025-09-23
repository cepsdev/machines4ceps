
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

namespace cepsplugin{
    static Ism4ceps_plugin_interface* plugin_master = nullptr;
    static const std::string version_info = "protobufish v0.1";
    static constexpr bool print_debug_info{true};
    ceps::ast::node_t plugin_entrypoint(ceps::ast::node_callparameters_t params);
}

using namespace std;

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

using namespace std;
struct json_token{
    enum {number, string, boolean, null, lbrace, rbrace, lsqrbra, rsqrbra, comma,colon, eoi /*end of input*/} type;
    size_t from, end;
    double value_f;
    int64_t value_i;
    bool value_b;    
    bool is_int;
};

optional<string> extract_str_value(json_token tok, string msg){
    if (tok.type != json_token::string || tok.from >= tok.end || tok.end > msg.length())
     return {};
    string r; r.resize(tok.end - tok.from + 1,'\0');
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
        else return {};
    } 
    return {json_token{json_token::eoi}};
}

optional< pair< char* , size_t >> json2protobufish(string json){
    size_t available{32};
    size_t used{};
    auto buffer = new char[available];
    size_t loc{};
    size_t n{json.length()};
    json_token cur_tok{};
    do {
     auto cur_tok_ = get_token(json.data(), loc, n);
     if (!cur_tok_) return {};
     cur_tok = *cur_tok_;
     if (cur_tok.type == json_token::number){
        cerr << "Number: "<< setprecision(14) << cur_tok.value_f << '\n';
     } else if (cur_tok.type == json_token::string){
        string str(cur_tok.end - cur_tok.from, ' ');
        strncpy(str.data(),json.data() + cur_tok.from,cur_tok.end - cur_tok.from);
        cerr << "String: '"<< str << "'" << '\n';
        auto seval = extract_str_value(cur_tok,json);
        if (!seval) cerr << "String is not well formed\n";
        else cerr << "String (evaluated): '"<< *seval << "'" << '\n';
     } else if (cur_tok.type == json_token::boolean) {
        cerr << cur_tok.value_b << '\n';
     } else if (cur_tok.type == json_token::colon){
        cerr << ":\n";
     } else if (cur_tok.type == json_token::comma){
        cerr << ",\n";
     } else if (cur_tok.type == json_token::rsqrbra){
        cerr << "]\n";
     } else if (cur_tok.type == json_token::lsqrbra){
        cerr << "[\n";
     } else if (cur_tok.type == json_token::rbrace){
        cerr << "}\n";
     } else if (cur_tok.type == json_token::lbrace){
        cerr << "{\n";
     } else if (cur_tok.type == json_token::eoi){
        cerr << "EOI\n";
     }
    } while (cur_tok.type != json_token::eoi);

    return {make_pair(buffer, available+used)};
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
    
    if (root.what == msg_node::ROOT || root.what == msg_node::NODE){
        string prefix,suffix;
        if (root.what == msg_node::NODE)
        {
            prefix = string{"\""}+ (char*)((msg_node_ex*)buffer)->name +"\":";
        } else {
            prefix = "{";
            suffix = "}";
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
    } else if (root.what == msg_node::F64){
        msg_node_f64& m{ *(msg_node_f64*)&root};
        stringstream ss;
        ss << m.value;
        res = ss.str();
        return sizeof(msg_node_f64);
    } else if (root.what == msg_node::SZ){
        msg_node_sz& m{ *(msg_node_sz*)&root};
        res = "\"" + escape_str(m.value)+ "\"";
        return sizeof(msg_node) + res.size() + 1;
    }
    return hd_size + content_size;
}

ceps::ast::node_t cepsplugin::plugin_entrypoint(ceps::ast::node_callparameters_t params){
    using namespace std;
    using namespace ceps::ast;
    using namespace ceps::interpreter;

    auto data = get_first_child(params);    
    if (!is<Ast_node_kind::structdef>(data)) return nullptr;
    auto& ceps_struct = *as_struct_ptr(data);
    
    for(auto e : children(ceps_struct)){
        if (!is<Ast_node_kind::string_literal>(e)) continue;

        cout <<"Input: "<< value(as_string_ref(e)) << "\n";
        auto r{json2protobufish(value(as_string_ref(e)))};
        if (!r) cout << "Failed\n";
    }
    cout <<"\n\n";
    auto result = mk_struct("result");
    children(*result).push_back(mk_int_node(42));
    return result;
}

extern "C" void init_plugin(IUserdefined_function_registry* smc)
{
  cepsplugin::plugin_master = smc->get_plugin_interface();
  cepsplugin::plugin_master->reg_ceps_phase0plugin("protobufish", cepsplugin::plugin_entrypoint);
}					
				