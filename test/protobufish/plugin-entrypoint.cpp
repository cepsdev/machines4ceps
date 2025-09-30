
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
#include "core/include/arena.hpp"
#include "core/include/fast-json.hpp"

using namespace std;

namespace cepsplugin{
    static Ism4ceps_plugin_interface* plugin_master = nullptr;
    static const std::string version_info = "protobufish v0.1";
    static constexpr bool print_debug_info{true};
    ceps::ast::node_t plugin_entrypoint(ceps::ast::node_callparameters_t params);
}

Arena<3> protobufish_memory;

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

bool json2protobufish_test_lexer(string json, bool print_info = false){
    using namespace ceps::vm::oblectamenta;
   size_t loc{};
    size_t n{json.length()};
    json_token cur_tok{};
    do {
     auto cur_tok_ = fast_json<decltype(protobufish_memory)>::get_token(json.data(), loc, n);
     if (!cur_tok_) return false;
     cur_tok = *cur_tok_;
     if (cur_tok.type == json_token::number){
        if  (print_info) cout << "Number: "<< setprecision(14) << cur_tok.value_f << '\n';
     } else if (cur_tok.type == json_token::string){
        string str(cur_tok.end - cur_tok.from, ' ');
        strncpy(str.data(),json.data() + cur_tok.from,cur_tok.end - cur_tok.from);
        if  (print_info) cout << "String: '"<< str << "'" << '\n';
        auto seval = fast_json<decltype(protobufish_memory)>::extract_str_value(cur_tok,json);
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
    for(int i = 0; i < 1000000;++i) {
        for(auto e : children(ceps_struct)){
        if (!is<Ast_node_kind::structdef>(e) || name(as_struct_ref(e)) != "serialization_test" ) continue;
        if (print_info) cout << "*** Serialization Tests:\n";
        for(auto ee : children(as_struct_ref(e))){
         if (!is<Ast_node_kind::string_literal>(ee)) continue;
         if (print_info) cout <<">>"<< value(as_string_ref(ee)) << "\n";
         ceps::vm::oblectamenta::fast_json<Arena<3>> jsn;
         auto r{jsn.read(value(as_string_ref(ee)), &protobufish_memory, 0)};
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
				