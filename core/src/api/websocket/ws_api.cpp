#include "core/include/api/websocket/ws_api.hpp"
#include "core/include/state_machine_simulation_core.hpp"

#include <sys/types.h>
#include <limits>
#include <cstring>
#include <atomic>
#include "../../../cryptopp/sha.h"
#include "../../../cryptopp/filters.h"
#include "../../../cryptopp/hex.h"
#include "core/include/base_defs.hpp"

/*
* State Machine Utilities
*/

template<typename F, typename T> void traverse_sm(std::unordered_set<State_machine*>& m,State_machine* sm, T const & sms, F f){
    f(sm);

    for(auto state: sm->states()){
        if (!state->is_sm() || state->smp() == nullptr) continue;
        if (m.find(state->smp()) != m.end()) continue;
        m.insert(state->smp());
        traverse_sm(m,state->smp(),sms,f);
    }

    for(auto subsm: sm->children()){
        //assert(m.find(subsm) != m.end());
        if (m.find(subsm) != m.end()) continue;

        m.insert(subsm);
        traverse_sm(m,subsm,sms,f);
    }
}

template<typename F, typename T> void traverse_sms(T const & sms, F f){
    std::unordered_set<State_machine*> m;
    for(auto sm: sms){
     if (m.find(sm.second) != m.end()) continue;
     traverse_sm(m,sm.second,sms,f);

     m.insert(sm.second);
    }
}


/*
 * http/websocket routines
*/
static std::string escape_json_string(std::string const & s){
    bool transform_necessary = false;
    for(std::size_t i = 0; i!=s.length();++i){
        auto ch = s[i];
        if (ch == '\n' || ch == '\t'|| ch == '\r' || ch == '"' || ch == '\\'){
            transform_necessary = true; break;
        }
    }
    if (!transform_necessary) return s;

    std::stringstream ss;
    for(std::size_t i = 0; i!=s.length();++i){
        char buffer[2] = {0};
        char ch = buffer[0] = s[i];
        if (ch == '\n') ss << "\\n";
        else if (ch == '\t') ss << "\\t";
        else if (ch == '\r' ) ss << "\\r";
        else if (ch == '"') ss << "\\\"";
        else if (ch == '\\') ss << "\\\\";
        else ss << buffer;
    }
    return ss.str();
}


bool ceps2json(std::stringstream& s,ceps::ast::Nodebase_ptr n){
    using namespace ceps::ast;
    if (n->kind() == Ast_node_kind::int_literal)
     {s << value(as_int_ref(n));return true;}
    else if (n->kind() == Ast_node_kind::float_literal)
     {s << value(as_double_ref(n));return true;}
    else if (n->kind() == Ast_node_kind::string_literal)
     {s << "\"" <<  escape_json_string(value(as_string_ref(n))) << "\"";return true;}
    else if (n->kind() == Ast_node_kind::identifier)
     {s << "\"" <<  escape_json_string(name(as_id_ref(n))) << "\"";return true;}
    else if (n->kind() == Ast_node_kind::structdef){
        auto & st = as_struct_ref(n);
        s << "{ \"type\":\"struct\", \"name\":" <<"\""<< name(st) << "\",\"content\":[";
        for(std::size_t i = 0; i != st.children().size();++i){
            auto nn = st.children()[i];
            if(!nn) continue;
            if (!ceps2json(s,nn)) continue;
            if (1+i != st.children().size()) s << ",";
        }
        s << "]}";
        return true;
    } else if (n->kind() == Ast_node_kind::binary_operator){
        auto& bop = as_binop_ref(n);
        auto what_op = op(bop);
        s << "{\"type\":\"binop\",\"name\":";
        if (ceps::Cepsparser::token::REL_OP_GT_EQ == what_op)
            s << "\">=\",";
        else if (ceps::Cepsparser::token::REL_OP_EQ == what_op)
            s << "\"==\",";
        else if (ceps::Cepsparser::token::REL_OP_GT == what_op)
            s << "\">\",";
        else if (ceps::Cepsparser::token::REL_OP_LT== what_op)
            s << "\"<\",";
        else if (ceps::Cepsparser::token::REL_OP_LT_EQ == what_op)
            s << "\"<=\",";
        else if (ceps::Cepsparser::token::REL_OP_NEQ == what_op)
            s << "\"!=\",";
        else if ('+' == what_op)
            s << "\"+\",";
        else if ('-' == what_op)
            s << "\"-\",";
        else if ('*' == what_op)
            s << "\"*\",";
        else if ('/' == what_op)
            s << "\"/\",";
        else if ('&' == what_op)
            s << "\"&&\",";
        else if ('|' == what_op)
            s << "\"||\",";
        else s << "\"?\",";
        s << "\"left\":";ceps2json(s,bop.left());s << ",";
        s << "\"right\":";ceps2json(s,bop.right());
        s << "}";
        return true;
    } else if (n->kind() == Ast_node_kind::scope){
        auto & scp = as_scope_ref(n);
        s << "{ \"type\":\"scope\", \"content\":[";
        for(std::size_t i = 0; i != scp.children().size();++i){
            auto nn = scp.children()[i];
            if(!nn) continue;
            if(!ceps2json(s,nn)) continue;
            if (1+i != scp.children().size()) s << ",";
        }
        s << "]}";
        return true;
    }
    else if (n->kind() == Ast_node_kind::symbol){
        auto & sy = as_symbol_ref(n);
        s << "{ \"type\":\"symbol\",\"kind\":" << "\""<< kind(sy) << "\", \"name\":" <<"\""<< name(sy) << "\"}";
        return true;
    } else if (n->kind() == Ast_node_kind::func_call){
        s << "{ \"type\":\"func-call\"}";
        return true;
    } else s << "0";
    return true;
}

static bool ceps2json(std::stringstream& s,ceps::ast::Nodeset & n){
    for(auto p : n.nodes())
        if(!ceps2json(s,p))return false;
    return true;
}

static void rtrim(std::string& s){
    if (s.length() == 0) return;auto i = s.length()-1;
    for (;i >= 0 && std::isspace(s[i]);--i);
    if (i == 0) s = {};
    else s.erase(i+1);
}

std::string Websocket_interface::query(State_machine_simulation_core* smc,std::string query){
    std::string r;
    rtrim(query);
    if (query == "root.__proc.sm_exec_ctxt_static"){
        std::stringstream s_info_vec;
        {
            auto const& v = smc->executionloop_context().inf_vec;
            for(std::size_t i = 0; i + 1 < v.size();++i){
                auto e = v[i];
                s_info_vec << e << ",";
            }
            if(!v.empty())s_info_vec << v.back();
        }
        std::stringstream s_transitions_vec;
        {
            auto const& v = smc->executionloop_context().transitions;
            for(std::size_t i = 1; i < v.size();++i){
                auto e = v[i];
                unsigned int props = 0;
                s_transitions_vec << e.root_sms << "," << e.smp << "," << e.from << "," << e.to << "," << e.ev << "," << e.rel_idx;
                if (v.size() > i+1) s_transitions_vec << ",";
            }
        }
        r = "{ \"ok\": true,";
        r += " \"number_toplevel_nodes\":1,";
        r += " \"sresult\":{\"states\": ["+s_info_vec.str()+"],\"transitions\": ["+s_transitions_vec.str()+"]  }";
        r += "}";
    }
    return r;
}

static std::pair<bool,std::string> get_http_attribute_content(std::string attr, std::vector<std::pair<std::string,std::string>> const & http_header){
 for(auto const & v : http_header){
	 if (v.first == attr)
		 return {true,v.second};
 }
 return {false,{}};
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

static bool field_with_content(std::string attr, std::string value,std::vector<std::pair<std::string,std::string>> const & http_header){
 auto r = get_http_attribute_content(attr,http_header);
 if (!r.first) return false;
 return r.second == value;
}

static std::tuple<bool,std::string,std::vector<std::pair<std::string,std::string>>> read_http_request(int sck,std::string& unconsumed_data){
 using header_t = std::vector<std::pair<std::string,std::string>>;
 std::tuple<bool,std::string,header_t> r;

 constexpr auto buf_size = 4096;
 char buf[buf_size];
 std::string buffer = unconsumed_data;
 std::string eom = "\r\n\r\n";
 std::size_t eom_pos = 0;

 unconsumed_data.clear();
 bool http_req_complete = false;
 ssize_t readbytes = 0;
 ssize_t buf_pos = 0;

 for(; (readbytes=recv(sck,buf,buf_size-1,0)) > 0;){
  buf[readbytes] = 0;
  for(buf_pos = 0; buf_pos < readbytes; ++buf_pos){
   if (buf[buf_pos] == eom[eom_pos])++eom_pos;else eom_pos = 0;
   if (eom_pos == eom.length()){
	http_req_complete = true;
	if (buf_pos+1 < readbytes) unconsumed_data = buf+buf_pos+1;
	buf[buf_pos+1] = 0;
	break;
   }
  }
  buffer.append(buf);
  if(http_req_complete) break;
 }

 if (http_req_complete) {
  header_t header;
  std::string first_line;
  size_t line_start = 0;
  for(size_t i = 0; i < buffer.length();++i){
	if (i+1 < buffer.length() && buffer[i] == '\r' && buffer[i+1] == '\n' ){
		if (line_start == 0) first_line = buffer.substr(line_start,i);
		else if (line_start != i){
		 std::string attribute;
		 std::string content;
		 std::size_t j = line_start;
		 for(;j < i && buffer[j]==' ';++j);
		 auto attr_start = j;
		 for(;j < i && buffer[j]!=':';++j);
		 attribute = buffer.substr(attr_start,j-attr_start);
		 ++j;
		 for(;j < i && buffer[j]==' ' ;++j);
		 auto cont_start = j;
		 auto cont_end = i - 1;
		 for(;buffer[cont_end] == ' ';--cont_end);
		 content = buffer.substr(cont_start, cont_end - cont_start + 1);
         header.push_back(std::make_pair(attribute,content));
		}
		line_start = i + 2;++i;
	}
  }
  return std::make_tuple(true,first_line,header);
 }

 return std::make_tuple(false,std::string{},header_t{});
}

static std::string sha1(std::string s){
 CryptoPP::SHA1 sha1;
 std::string hash;
 auto a = new CryptoPP::StringSink(hash);
 auto b = new CryptoPP::HexEncoder(a);
 auto c = new CryptoPP::HashFilter(sha1, b);
 CryptoPP::StringSource(s, true, c);
 return hash;
}

struct websocket_frame{
 std::vector<unsigned char> payload;
 bool fin = false;
 bool rsv1 = false;
 bool rsv2 = false;
 bool rsv3 = false;
 std::uint8_t opcode = 0;
};

static std::pair<bool,websocket_frame> read_websocket_frame(int sck){
	websocket_frame r;
	std::uint16_t header;

	auto bytesread = recv(sck,&header,sizeof header,0);
    if (bytesread != sizeof header) return {false,{}};

    r.opcode = header & 0xF;
    r.fin  = header & 0x80;
    r.rsv1 = header & 0x40;
    r.rsv2 = header & 0x20;
    r.rsv3 = header & 0x10;
    bool mask = header >> 15;
    std::uint8_t payload_len_1 = (header >> 8) & 0x7F;

    size_t payload_len = payload_len_1;

    if (payload_len_1 == 126){
     std::uint16_t v;
     bytesread = recv(sck,&v,sizeof v,0);
     if (bytesread != sizeof v) return {false,{}};
     payload_len = ntohs(v);
    } else if (payload_len_1 == 127){
     std::uint64_t v;
     bytesread = recv(sck,&v,sizeof v,0);
     if (bytesread != sizeof v) return {false,{}};
     payload_len = be64toh(v);
    }

    std::uint32_t mask_key = 0;
    if (mask){
     bytesread = recv(sck,&mask_key,sizeof mask_key,0);
     if (bytesread != sizeof mask_key) return {false,{}};
    }

    constexpr size_t bufsize = 4;unsigned char buf[bufsize];
    size_t payload_bytes_read = 0;
    r.payload.resize(payload_len);

    for(;payload_bytes_read < payload_len;){
    	ssize_t toread = std::min(payload_len - payload_bytes_read,bufsize);
    	bytesread = recv(sck,(char*)buf,toread,0);
    	if (bytesread != toread) return {false,{}};
    	for(size_t i = 0; (ssize_t)i < bytesread; ++i) r.payload[payload_bytes_read+i] = buf[i] ^ ((unsigned char *)&mask_key)[ (payload_bytes_read+i) % 4];
    	payload_bytes_read += bytesread;
    }

	return {true,r};
}


extern ceps::ast::Nodebase_ptr eval_locked_ceps_expr(State_machine_simulation_core* smc,
		 State_machine* containing_smp,
		 ceps::ast::Nodebase_ptr node,
		 ceps::ast::Nodebase_ptr root_node);



template<typename F> struct cleanup{
	F f_;
	cleanup(F f):f_(f){}
	~cleanup(){f_();}
};

static void comm_act_as_websocket_server(State_machine_simulation_core::dispatcher_thread_ctxt_t * ctx,
		State_machine_simulation_core* smc,sockaddr_storage claddr, int sck,std::string ev_id,std::string sock_name,bool reg_sock,bool reuse_sock){
 auto cleanup_f = [smc,reg_sock,sck,sock_name](){
  if (reg_sock){
	 	std::lock_guard<std::recursive_mutex> g(smc->get_reg_sock_mtx());
	 	smc->get_reg_socks()[sock_name] = std::tuple<int,bool>{-1,false};
  }
  close(sck);
 };
 cleanup<decltype(cleanup_f)> cl{cleanup_f};

 std::string unconsumed_data;
 auto rhr = read_http_request(sck,unconsumed_data);
 if (!std::get<0>(rhr)) return;

 auto const & attrs = std::get<2>(rhr);
 if (!field_with_content("Upgrade","websocket",attrs) || !field_with_content("Connection","Upgrade",attrs)) return;
 auto r = get_http_attribute_content("Sec-WebSocket-Key",attrs);
 if (!r.first)return;

 auto phrase = r.second+"258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
 unsigned char digest[CryptoPP::SHA::DIGESTSIZE];
 CryptoPP::SHA().CalculateDigest(digest, (unsigned char *)phrase.c_str(),phrase.length());
 auto hash = encode_base64(digest,CryptoPP::SHA::DIGESTSIZE);
 std::stringstream response;
 response
  << "HTTP/1.1 101 Switching Protocols\r\n"
  << "Upgrade: websocket\r\n"
  << "Connection: Upgrade\r\n"
  << "Sec-WebSocket-Accept: "
  << hash <<"\r\n\r\n";
 auto byteswritten = write(sck, response.str().c_str(),response.str().length());
 if (byteswritten == (ssize_t)response.str().length()){
  if (reg_sock){
	std::lock_guard<std::recursive_mutex> g(smc->get_reg_sock_mtx());
	smc->get_reg_socks()[sock_name] = std::tuple<int,bool>{sck,true};
  }
  for(;;){
   auto frm = read_websocket_frame(sck);
   if (!frm.first) break;
   std::vector<unsigned char> payload = std::move(frm.second.payload);
   while (!frm.second.fin){
    frm = read_websocket_frame(sck);
    if (!frm.first) break;
    payload.reserve(payload.size()+frm.second.payload.size());
    payload.insert(payload.end(),frm.second.payload.begin(),frm.second.payload.end());
   }
   if (!frm.first) break;
   if(frm.second.opcode == 1) {
    std::string s; s.resize(payload.size());for(size_t j = 0; j < payload.size();++j)s[j] = payload[j];//std::cout << s << std::endl;
    State_machine_simulation_core::event_t ev(ev_id);
    ev.already_sent_to_out_queues_ = true;
    ev.payload_.push_back(new ceps::ast::String(s));
 	smc->main_event_queue().push(ev);
   }
  }//for
 }
}


bool send_ws_text_msg(int sck, std::string const & msg){
    auto len = msg.length();
    bool fin = true;
    bool ext1_len = len >= 126 && len <= 65535;
    bool ext2_len = len > 65535;

    std::uint16_t header = 0;
    if (fin) header |= 0x80;
    if(!ext1_len && !ext2_len) header |= len << 8;
    else if (ext1_len) header |= 126 << 8;
    else header |= 127 << 8;
    header |= 1;
    auto wr = write(sck, &header,sizeof header );
    if(wr != sizeof header) return false;
    if (ext1_len)
    {
      std::uint16_t v = len;v = htons(v);
      if( (wr = write(sck, &v,sizeof v )) != sizeof v) return false;
    }
    if (ext2_len)
    {
      std::uint64_t v = len;v = htobe64(v);
      if( (wr = write(sck, &v,sizeof v )) != sizeof v) return false;
    }
    if ( (wr = write(sck, msg.c_str(),len )) != (int)len) return false;
    return true;
}



class Execute_query : public sm4ceps_plugin_int::Executioncontext {
    std::string query;
    int sck;
 public:
    Execute_query() = default;
    Execute_query(std::string q, int s):query{q}, sck{s}{}
    void  run(State_machine_simulation_core* ctxt){
      std::string r;

      try {
          rtrim(query);
          if (query == "root.__proc.current_states"){
               std::stringstream s;
               auto const& v = ctxt->executionloop_context().current_states;
               std::size_t c = 0;for(std::size_t i = 0; i != v.size(); ++i)
                   if (v[i] &&
                           !ctxt->executionloop_context().get_inf(i,executionloop_context_t::HIDDEN)) ++c;

               for(std::size_t i = 0; i != v.size(); ++i){
                 if (!v[i]) continue;
                 if (ctxt->executionloop_context().get_inf(i,executionloop_context_t::HIDDEN)) continue;
                 s << i;--c;if(c!=0) s <<",";
               }
               //std::cerr << s.str() << std::endl;
               r = "{ \"ok\": true,";
               r += " \"number_toplevel_nodes\":1,";
               r += " \"sresult\":["+s.str()+"]";
               r += "}";
           } else if (query == "root.__proc.current_states_with_coverage"){
              std::stringstream s;
              auto const& v = ctxt->executionloop_context().current_states;
              std::size_t c = 0;for(std::size_t i = 0; i != v.size(); ++i)
                  if (v[i] && !ctxt->executionloop_context().get_inf(i,executionloop_context_t::HIDDEN)) ++c;

              for(std::size_t i = 0; i != v.size(); ++i){
                if (!v[i]) continue;
                if (ctxt->executionloop_context().get_inf(i,executionloop_context_t::HIDDEN))continue;
                s << i;--c;if(c!=0) s <<",";
              }
              std::stringstream s_cov_report;
              auto report = ctxt->make_report();
              ceps2json(s_cov_report,report);
              for(auto n:report.nodes()) if(n!=nullptr)delete n;
              //std::cerr << s.str() << std::endl;
              r = "{ \"ok\": true,";
              r += " \"number_toplevel_nodes\":2,";
              r += " \"sresult\":{\"current_states\": ["+s.str()+"], \"coverage\": "+s_cov_report.str()+" }";
              r += "}";
          } else if (query == "root.__proc.coverage"){
              std::stringstream s;
              std::stringstream s_cov_report;
              auto report = ctxt->make_report();
              ceps2json(s_cov_report,report);
              for(auto n:report.nodes()) if(n!=nullptr)delete n;
              r = "{ \"ok\": true,";
              r += " \"number_toplevel_nodes\":1,";
              r += " \"sresult\":{\"coverage\": "+s_cov_report.str()+" }";
              r += "}";
          } else {
               std::stringstream s;
               s << query;
               Ceps_parser_driver driver{ctxt->ceps_env_current().get_global_symboltable(),s};
               ceps::Cepsparser parser{driver};
               if (parser.parse() != 0 || driver.errors_occured()){
                 r = "{ \"ok\": false, \"reason\": \"\" }";
               } else {
                 std::vector<ceps::ast::Nodebase_ptr> generated_nodes;
                 ceps::interpreter::evaluate_without_modifying_universe(ctxt->current_universe(),
                                          driver.parsetree().get_root(),
                                          ctxt->ceps_env_current().get_global_symboltable(),
                                          ctxt->ceps_env_current().interpreter_env(),
                                          &generated_nodes
                                          );
                 std::stringstream out;
                 for(std::size_t i = 0; i != generated_nodes.size();++i){
                     auto e = generated_nodes[i];
                     if (e) ceps2json(out,e);
                     if (1 + i != generated_nodes.size()) out << ",";
                 }
                 r = "{ \"ok\": true,";
                 r += " \"number_toplevel_nodes\":"+std::to_string(generated_nodes.size())+",";
                 r += " \"sresult\":["+out.str()+"]";
                 r += "}";
                 //std::cout << out.str() << std::endl;
                 //for(auto e : generated_nodes) delete e;
            }
           }
      }
      catch (ceps::interpreter::semantic_exception & se)
      {
          r = "{ \"ok\": false,\"exception\":\"ceps::interpreter::semantic_exception\",  \"reason\": \""+std::string{se.what()}+"\" }";
      }
      catch (std::runtime_error & re)
      {
          r = "{ \"ok\": false,\"exception\":\"ceps::interpreter::semantic_exception\", \"reason\": \""+std::string{re.what()}+"\" }";
      }
      if(!send_ws_text_msg(sck,r)) closesocket(sck);
    }
};

class Execute_report : public sm4ceps_plugin_int::Executioncontext {
    int sck;
 public:
    Execute_report() = default;
    Execute_report(int s): sck{s}{}
    void  run(State_machine_simulation_core* ctxt){
      std::string r;

      std::stringstream out;
      auto ns = ctxt->make_report();

      for(std::size_t i = 0; i != ns.nodes().size();++i){
          auto e = ns.nodes()[i];
          if (e) ceps2json(out,e);
          if (1 + i != ns.nodes().size()) out << ",";
      }
      r = "{ \"ok\": true,";
      r += " \"number_toplevel_nodes\":"+std::to_string(ns.nodes().size())+",";
      r += " \"sresult\":["+out.str()+"]";
      r += "}";

      if(!send_ws_text_msg(sck,r)) closesocket(sck);
    }
};

class Exported_events_query : public sm4ceps_plugin_int::Executioncontext {
    int sck;
 public:
    Exported_events_query() = default;
    Exported_events_query(int s): sck{s}{}
    void  run(State_machine_simulation_core* ctxt){
      std::string r;
      std::stringstream out;
      auto n = ctxt->exported_events().size();
      std::size_t i = 0;
      for(auto const & e : ctxt->exported_events()){
          out << "\"" << e << "\"";
          if (i + 1 != ctxt->exported_events().size()) out << ",";
          ++i;
      }
      r = "{ \"ok\": true,";
      r += " \"number_exported_events\":"+std::to_string(ctxt->exported_events().size())+",";
      r += " \"sresult\":["+out.str()+"]";
      r += "}";

      if(!send_ws_text_msg(sck,r)) closesocket(sck);
    }
};



class Execute_push : public sm4ceps_plugin_int::Executioncontext {
    std::string query;
    int sck;
 public:
    Execute_push() = default;
    Execute_push(std::string q, int s):query{q}, sck{s}{}
    void run(State_machine_simulation_core* ctxt){
      std::string r;

      std::stringstream s;
      s << query;
      try {
       Ceps_parser_driver driver{ctxt->ceps_env_current().get_global_symboltable(),s};
       ceps::Cepsparser parser{driver};
       if (parser.parse() != 0 || driver.errors_occured()){
         r = "{ \"ok\": false, \"reason\": \"\" }";
       } else {
         ceps::interpreter::evaluate(ctxt->current_universe(),
                                  driver.parsetree().get_root(),
                                  ctxt->ceps_env_current().get_global_symboltable(),
                                  ctxt->ceps_env_current().interpreter_env(),
                                  nullptr
                                  );
         r = "{ \"ok\": true }";
         int ctr = ctxt->push_modules.size();
         std::string t;
         for(;;++ctr){
             t = std::to_string(ctr)+".ceps";
             bool found = false;
             for(auto&e:ctxt->push_modules) if (e == t){break;found=true;}
             if (found)continue;
             std::ofstream of{ctxt->push_dir+t};
             if (!of) return;
             of << query;
             break;
         }
         std::ofstream of{ctxt->push_dir + "package.ceps"};
         if(!of) return;
         of << "package{\n";
         of << " modules{\n";
         for (auto&e : ctxt->push_modules)
            of << "  \""<<e<<"\";\n";
         of << "  \""<<t<<"\";\n";
         of << " };";
         of << "\n};";
       }
      }
      catch (ceps::interpreter::semantic_exception & se)
      {
          r = "{ \"ok\": false,\"exception\":\"ceps::interpreter::semantic_exception\",  \"reason\": \""+std::string{se.what()}+"\" }";
      }
      catch (std::runtime_error & re)
      {
          r = "{ \"ok\": false,\"exception\":\"ceps::interpreter::semantic_exception\", \"reason\": \""+std::string{re.what()}+"\" }";
      }
      if(!send_ws_text_msg(sck,r)) closesocket(sck);
    }
};
void Websocket_interface::handler(int sck){
 using wstable_t = std::vector<State_machine_simulation_core::global_states_t::iterator>;
 auto period = std::shared_ptr<std::chrono::microseconds>(new std::chrono::microseconds{1000});
 auto watched_signals = std::shared_ptr<wstable_t>(new wstable_t{});
 auto watched_signals_m = std::shared_ptr<std::mutex>(new std::mutex{});
 auto shutdown_update_thread = std::shared_ptr<std::atomic_bool>(new std::atomic_bool{false});



 auto cleanup_f = [this,&sck](){
     using namespace std;
     if (sck != -1) close(sck);
     lock_guard<std::mutex> lg(handler_threads_status_mutex_);
     for(auto& s : handler_threads_status_)
        if (get<1>(s) && get<0>(s))
          {get<1>(s)=false;get<2>(s)=-1;}

 };
 cleanup<decltype(cleanup_f)> cl{cleanup_f};


 std::string unconsumed_data;
 auto rhr = read_http_request(sck,unconsumed_data);
 if (!std::get<0>(rhr)) return;

 auto const & attrs = std::get<2>(rhr);
 if (!field_with_content("Upgrade","websocket",attrs) || !field_with_content("Connection","Upgrade",attrs)) return;
 auto r = get_http_attribute_content("Sec-WebSocket-Key",attrs);
 if (!r.first)return;

 auto phrase = r.second+"258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
 unsigned char digest[CryptoPP::SHA::DIGESTSIZE];
 CryptoPP::SHA().CalculateDigest(digest, (unsigned char *)phrase.c_str(),phrase.length());
 auto hash = encode_base64(digest,CryptoPP::SHA::DIGESTSIZE);
 std::stringstream response;
 response << "HTTP/1.1 101 Switching Protocols\r\n"<< "Upgrade: websocket\r\n"<< "Connection: Upgrade\r\n"
  << "Sec-WebSocket-Accept: "<< hash <<"\r\n\r\n";

 auto byteswritten = send(sck, response.str().c_str(),response.str().length(),0);
 if (byteswritten != (ssize_t)response.str().length()) return;


 auto gen_update_reply = [watched_signals](std::string& reply){
     bool comma = false;
     for(auto e : *watched_signals){
         //if (e->second->kind() == ceps::ast::Ast_node_kind::float_literal)
         std::string v;
         if (e->second->kind() == ceps::ast::Ast_node_kind::float_literal)
             v = std::to_string(ceps::ast::value(ceps::ast::as_double_ref(e->second)));
         else if (e->second->kind() == ceps::ast::Ast_node_kind::int_literal)
             v = std::to_string(ceps::ast::value(ceps::ast::as_int_ref(e->second)));
         if (comma) reply += ",";
         reply += "{ \"name\": \"" + e->first +"\"," + "\"value\":" + v + "}";
         comma = true;
     }
 };

 auto send_reply = [sck](std::string const & reply) -> bool   {
     auto len = reply.length();
     bool fin = true;
     bool ext1_len = len >= 126 && len <= 65535;
     bool ext2_len = len > 65535;

     std::uint16_t header = 0;
     if (fin) header |= 0x80;
     if(!ext1_len && !ext2_len) header |= len << 8;
     else if (ext1_len) header |= 126 << 8;
     else header |= 127 << 8;
     header |= 1;
     auto wr = write(sck, &header,sizeof header );
     if(wr != sizeof header) return false;
     if (ext1_len)
     {
       std::uint16_t v = len;v = htons(v);
       if( (wr = write(sck, &v,sizeof v )) != sizeof v) return false;
     }
     if (ext2_len)
     {
       std::uint64_t v = len;v = htobe64(v);
       if( (wr = write(sck, &v,sizeof v )) != sizeof v) return false;
     }
     if ( (wr = write(sck, reply.c_str(),len )) != (int)len) return false;
     return true;
 };

 auto update_thread = new std::unique_ptr<std::thread>{
   new std::thread{ [watched_signals,watched_signals_m,period,gen_update_reply,shutdown_update_thread,send_reply,this] () {
    for(;!*shutdown_update_thread;){
     std::string reply = "{\"ok\":true, \"signals\" : [";
     std::this_thread::sleep_for(*period);
     {
         std::lock_guard<std::mutex> g2(*watched_signals_m);
         if (watched_signals->size() == 0) continue;
         std::lock_guard<std::recursive_mutex>g(smc_->states_mutex());
         gen_update_reply(reply);
         reply += "]}";
         if(!send_reply(reply)) return;
     }
    }
   }}};

 std::string host_name;
 {
  char buffer[1025] = {0};
  if (0 == gethostname(buffer,1024)) host_name = buffer;
 }

 for(;;){
   auto frm = read_websocket_frame(sck);
   if (!frm.first) break;
   std::vector<unsigned char> payload = std::move(frm.second.payload);
   while (!frm.second.fin){
    frm = read_websocket_frame(sck);
    if (!frm.first) break;
    payload.reserve(payload.size()+frm.second.payload.size());
    payload.insert(payload.end(),frm.second.payload.begin(),frm.second.payload.end());
   }
   if (!frm.first) break;
   if(frm.second.opcode == 1) {
    std::string s;
    s.resize(payload.size());
    for(size_t j = 0; j < payload.size();++j)s[j] = payload[j];

    std::stringstream hs{s};
    std::vector<std::string> cmd;
    bool line_wise = false;
    for(;hs;){
        std::string l;
        if(!line_wise) hs >> l; else std::getline(hs,l);
        if (l.size() == 0 || l == "\n" || l == "\r\n" || l == "\n\r") continue;
        cmd.push_back(l);
        if (cmd.size() == 1 && cmd[0] == "PUSH"){
            cmd.push_back(s.substr(5));
            break;
        } else if (cmd.size() == 1 && (cmd[0] == "RESTART_STATEMACHINES" || cmd[0] == "EVENTL") ){
            line_wise = true;
        }
    }
    std::string reply = "{\"ok\": false}";

    if (cmd.size() != 0){
     decltype(cmd) args;
     for (std::size_t i = 1; i != cmd.size(); ++i ) if (cmd[i].length()) {args.push_back(cmd[i]);}

     if (cmd[0] == "WATCH"){
         std::lock_guard<std::recursive_mutex>g(smc_->states_mutex());
         for(std::size_t i = 1; i != cmd.size();++i){
          const auto & sig{cmd[i]};
          auto it = smc_->get_global_states().find(sig);
          if (it == smc_->get_global_states().end()) continue;

          bool already_watched = false;
          for(auto const & e : *watched_signals) if(e->first == sig){already_watched = true;break;}
          if (!already_watched) watched_signals->push_back(it);
         }
         reply = "{\"ok\": true, \"num_of_signals_watched\" : "+std::to_string(watched_signals->size())+ "}";
     } else if (cmd[0] == "GLOBAL_SYSSTATES") {
         std::stringstream ss;
         ss << "{\"ok\":true, \"global_states\" : [";
         std::lock_guard<std::recursive_mutex>g(smc_->states_mutex());
         auto const & states = smc_->get_global_states();
         std::vector< std::pair<std::string,ceps::ast::Nodebase_ptr> > v;
         for(auto const & vv : states) v.push_back(vv);
         for(std::size_t i = 0; i != v.size(); ++i){
             ss << "{ \"name\": ";
             auto p = v[i];
             ss << "\"" << p.first << "\",";
             ss << "\"type\":\"";
             if (p.second == nullptr) { ss << "(null)";}
             else if (p.second->kind() == ceps::ast::Ast_node_kind::int_literal) {ss << "int";}
             else if (p.second->kind() == ceps::ast::Ast_node_kind::float_literal) {ss << "float";}
             else if (p.second->kind() == ceps::ast::Ast_node_kind::string_literal) {ss << "string";}
             else {ss << "object";}
             ss << "\"}";
             if (i + 1 != states.size()) ss << ",";
         }
         ss << "]}";
         reply = ss.str();
     } else if (cmd[0] == "KNOWN_SIMCORES" && args.size() == 0){
         reply = "{\"ok\":true, \"simcores\" : [";
         if (nullptr != smc_->vcan_api()){
             auto known_sim_cores = smc_->vcan_api()->fetch_known_simcores_thread_safe();
             for(std::size_t i = 0; i != known_sim_cores.entries.size(); ++i){
                 auto const & e = known_sim_cores.entries[i];
                 if (e.role == "this_core") continue;
                 reply+="{";
                  if (e.host_name.length()) { reply+="\"host_name\":"; reply+= "\"" + e.host_name + "\",";}
                  else {reply+="\"host_name\":"; reply+= "\"" + host_name + "\",";}

                  reply+="\"name\":"; reply+= "\"" + e.name + "\",";
                  reply+="\"short_name\":"; reply+= "\"" + e.short_name + "\",";
                  reply+="\"vcan_api_port\":"; reply+= "\"" + e.port + "\",";
                  reply+="\"ws_api_port\":"; reply+= "\"" + e.ws_api_port + "\",";
                  reply+="\"role\":"; reply+= "\"" + e.role + "\"";
                 reply+="}";
                 if (i + 1 != known_sim_cores.entries.size()) reply += ",";
             }
         }
         reply += "]}";
     } else if (cmd[0] == "GET_UPDATE" && args.size() == 0){
         reply = "{\"ok\":true, \"signals\" : [";
         std::lock_guard<std::mutex> g2(*watched_signals_m);
         std::lock_guard<std::recursive_mutex>g(smc_->states_mutex());

         gen_update_reply(reply);
         reply += "]}";
     } else if (cmd[0] == "GET_UPDATE" && args.size() == 1) {
        int v;
        v =  std::stoi(args[0]);

        if (v >= 10) {
            reply = "{\"ok\": true}";
            std::lock_guard<std::mutex> g2(*watched_signals_m);
            *period = std::chrono::microseconds{1000*v};
        }
     } else if ( (cmd[0] == "SET_VALUE" || cmd[0] == "SET_VALUE_NO_REPLY") && args.size() == 3) {
         auto const & name = args[0];
         auto const & type = args[1];
         auto const & value = args[2];
         //std::cout << name << "/" <<type << "/"<< value << std::endl;
         std::lock_guard<std::recursive_mutex>g(smc_->states_mutex());
         auto it = smc_->get_global_states().find(name);
         if (it == smc_->get_global_states().end()) std::cout << "not found" << std::endl;
         if (it != smc_->get_global_states().end()){
          reply = "{\"ok\": true}";
          try{
              if (type == "number" || type == "double"){
               if (it->second->kind() == ceps::ast::Ast_node_kind::float_literal){
                 auto v = std::stod(value);
                 ceps::ast::value(ceps::ast::as_double_ref(it->second)) = v;
               } else if (it->second->kind() == ceps::ast::Ast_node_kind::int_literal){
                 auto v = (int)std::stod(value);
                 ceps::ast::value(ceps::ast::as_int_ref(it->second)) = v;
               }
              }else if (type == "int"){
               if (it->second->kind() == ceps::ast::Ast_node_kind::int_literal){
                 auto v = std::stoi(value);
                 ceps::ast::value(ceps::ast::as_int_ref(it->second)) = v;
               }
              }else if (type == "string"){
               if (it->second->kind() == ceps::ast::Ast_node_kind::string_literal){
                 ceps::ast::value(ceps::ast::as_string_ref(it->second)) = value;
               }
              }
             } catch (std::invalid_argument e1) {
               reply = "{\"ok\": false,\"reason\":\"invalid argument\"}";
             } catch (std::out_of_range e2){
               reply = "{\"ok\": false,\"reason\":\"out of range\"}";
             }
         }
         if (cmd[0] == "SET_VALUE_NO_REPLY") continue;
      } else if (cmd[0] == "PING") {
         reply = "{\"ok\": true , \"reply\":\"PONG\" }";
      } else if (cmd[0] == "EVENTL" && args.size()) {
         auto const & name = args[0];
         if (args.size() == 1){
           State_machine_simulation_core::event_t ev;
           ev.id_ = name;
           smc_->enqueue_event(ev);
           reply = "{\"ok\": true }";
         }
      }else if (cmd[0] == "EVENT" && args.size()) {
        auto const & name = args[0];
        if (args.size() == 1){
          State_machine_simulation_core::event_t ev;
          ev.id_ = name;
          smc_->enqueue_event(ev);
          reply = "{\"ok\": true }";
        }
     } else if (cmd[0] == "GET_KNOWN_STREAMING_ENDPOINTS") {
         reply = "{\"ok\": true,\n\"endpoints\":[";
         std::lock_guard<std::mutex> lk(smc_->vcan_wsapi_mutex());
         size_t i = 0;
         for( auto e : smc_->streaming_endpoints_registered_via_vcan_api()){
             bool last = i + 1 == smc_->streaming_endpoints_registered_via_vcan_api().size();
             reply+="{\"host\":\""+e.first+"\",\"port\":\""+e.second+"\"}";
             if (!last) reply += ",";
             ++i;
         }
         reply += "]}";
      } else if (cmd[0] == "QUERY") {
         std::string query;
         for(auto const & s : args)
             query += s +" ";

         State_machine_simulation_core::event_t ev;
         auto exec = new Execute_query{query,sck};
         ev.exec = exec;
         smc_->enqueue_event(ev);
         continue;
      } else if (cmd[0] == "RESTART_STATEMACHINES") {
         smc_->restart_state_machines(args);
         reply = "{\"ok\": true }";
      } else if (cmd[0] == "EXPORTED_EVENTS") {
         State_machine_simulation_core::event_t ev;
         auto exec = new Exported_events_query{sck};
         ev.exec = exec;
         smc_->enqueue_event(ev);
         continue;
      } else if (cmd[0] == "PUSH") {

         State_machine_simulation_core::event_t ev;
         auto exec = new Execute_push{cmd[1],sck};
         ev.exec = exec;
         smc_->enqueue_event(ev);
         continue;
      } else if (cmd[0] == "REPORT") {
       State_machine_simulation_core::event_t ev;
       auto exec_report = new Execute_report{sck};
       ev.exec = exec_report;
       smc_->enqueue_event(ev);
       continue;
     } else if (cmd[0] == "SUBSCRIBE") {
        if (args.size() > 0 && args[0] == "COVERAGE") {
            handle_subscribe_coverage(sck);sck=-1;return;
        }
      }
    }//cmd.size()!=0
    //std::cout << reply << std::endl;
    if(!send_reply(reply)) break;
   }
  }//for
}


static bool subscribe_coverage_handler(std::vector<executionloop_context_t::state_present_rep_t> const & new_states,void* ctxt_,int& status){
    auto ctxt = static_cast<Websocket_interface::subscribe_coverage_handler_ctxt_t*>(ctxt_);
    if (status == 1){ // Very first call
        auto& exec = ctxt->smc->executionloop_context();
        if (exec.start_of_covering_states_valid()){
            Websocket_interface::coverage_update_msg_t msg;
            msg.what = Websocket_interface::coverage_update_msg_t::what_t::INIT;
            msg.coverage_state_table = exec.coverage_state_table;
            msg.coverage_transitions_table = exec.coverage_transitions_table;
            msg.enter_times = exec.enter_times;
            msg.exit_times = exec.exit_times;
            msg.time_stamp = exec.start_execution_time_stamp_hres;
            msg.current_states = new_states;
            ctxt->q->push(msg);
        } else return false;
        return true;
    }

    auto t = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> d = t - ctxt->last;
    auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(d).count();
    if (delta < ctxt->delta_ms) {
        status = 4;
        return true;
    }
    auto timestamp_barrier = ctxt->last;
    auto& exec = ctxt->smc->executionloop_context();
    if (exec.log_timing && exec.time_stamp < timestamp_barrier) timestamp_barrier = exec.time_stamp;
    ctxt->last = t;

    if (exec.start_of_covering_states_valid()){
        //Handling of Entering & Exiting times
        //>>
        //std::cerr << exec.log_timing << std::endl;
        //<<
        Websocket_interface::coverage_update_msg_t msg;
        msg.what = Websocket_interface::coverage_update_msg_t::what_t::UPDATE;
        msg.coverage_state_table = exec.coverage_state_table;
        msg.coverage_transitions_table = exec.coverage_transitions_table;
        msg.enter_times = exec.enter_times;
        msg.exit_times = exec.exit_times;
        msg.time_stamp = timestamp_barrier;
        msg.current_states = new_states;
        ctxt->q->push(msg);
    } else return false;
    return true;
}


class Subscribe_coverage : public sm4ceps_plugin_int::Executioncontext {
    threadsafe_queue<Websocket_interface::coverage_update_msg_t,std::queue<Websocket_interface::coverage_update_msg_t>>* q;
    int subscribe_channel_id;
 public:
    Subscribe_coverage() = default;
    Subscribe_coverage(int subscribe_channel_id_,
                       threadsafe_queue<Websocket_interface::coverage_update_msg_t,std::queue<Websocket_interface::coverage_update_msg_t>>*q_)
                       :subscribe_channel_id{subscribe_channel_id_},q{q_}{}
    void run(State_machine_simulation_core* ctxt){
        auto ts = std::chrono::high_resolution_clock::now();
        if (ctxt->executionloop_context().log_timing) ts = ctxt->executionloop_context().time_stamp;
        ctxt->register_execution_context_loop_handler_cover_state_changed(
                    subscribe_channel_id,
                    subscribe_coverage_handler,
                    new Websocket_interface::subscribe_coverage_handler_ctxt_t{ctxt,
                                                          q,
                                                          subscribe_channel_id,
                                                          ts,
                                                          ctxt->min_time_delta_between_coverage_status_updates_for_coverage_handlers_in_ms()}
            );
    }
};

void Websocket_interface::handle_subscribe_coverage_thread(threadsafe_queue<coverage_update_msg_t,std::queue<coverage_update_msg_t>>* q){
    std::map<int,std::vector<std::string>> stateindex2categories;
    std::map<std::string,int> categories;
    std::vector<executionloop_context_t::state_present_rep_t> current_states;
    std::unordered_map<int, unsigned long long> root2active_categories;
    auto& ctx = this->smc_->executionloop_context();

    auto get_root_sm = [&](executionloop_context_t const & ctx, int state){
        for(;ctx.get_parent(state) != 0;) state = ctx.get_parent(state);
        return state;
    };

    auto compute_categories = [&](){
        for(auto& e : root2active_categories){
            e.second = 0;
        }
        for(auto i = ctx.start_of_covering_states;  i < current_states.size();++i){
            if (!current_states[i]) continue;
            if (!stateindex2categories[i].size()) continue;
            auto const & v = stateindex2categories[i];
            auto & r = root2active_categories[get_root_sm(ctx,i)];
            for(auto const & c : v) r|=(1 << categories[c]);
        }
    };

    auto compute_categories_diff = [&](){
        auto t = root2active_categories;

        std::vector<int> r;
        for(auto& e : root2active_categories){
            e.second = 0;
        }
        for(auto i = ctx.start_of_covering_states;  i < current_states.size();++i){
            if (!current_states[i]) continue;
            if (!stateindex2categories[i].size()) continue;
            auto const & v = stateindex2categories[i];
            auto & r = root2active_categories[get_root_sm(ctx,i)];
            for(auto const & c : v) r|=(1 << categories[c]);
        }
        for(auto e: root2active_categories){
            //std::cerr  << e.first << " " << e.second << std::endl;
            if (e.second != t[e.first]) r.push_back(e.first);
        }
        return r;
    };

    auto send_reply = [&](int sck,std::string const & reply,std::string const & prefix="",std::string const & postfix="") -> bool   {
        auto len = prefix.length()+reply.length()+postfix.length();
        bool fin = true;
        bool ext1_len = len >= 126 && len <= 65535;
        bool ext2_len = len > 65535;

        std::uint16_t header = 0;
        if (fin) header |= 0x80;
        if(!ext1_len && !ext2_len) header |= len << 8;
        else if (ext1_len) header |= 126 << 8;
        else header |= 127 << 8;
        header |= 1;
        auto wr = write(sck, &header,sizeof header );
        if(wr != sizeof header) return false;
        if (ext1_len)
        {
          std::uint16_t v = len;v = htons(v);
          if( (wr = write(sck, &v,sizeof v )) != sizeof v) return false;
        }
        if (ext2_len)
        {
          std::uint64_t v = len;v = htobe64(v);
          if( (wr = write(sck, &v,sizeof v )) != sizeof v) return false;
        }
        if (prefix.length()) if ( (wr = write(sck, prefix.c_str(),prefix.length() )) != (int)prefix.length()) return false;
        if (reply.length()) if ( (wr = write(sck, reply.c_str(),reply.length() )) != (int)reply.length()) return false;
        if (postfix.length()) if ( (wr = write(sck, postfix.c_str(),postfix.length() )) != (int)postfix.length()) return false;
        return true;
    };
    auto cleanup_f = [this](){
        using namespace std;

    };

    auto compute_root_sms = [&](executionloop_context_t const & ctx){
        std::vector<int> r;
        for(std::size_t i = 0;i != ctx.coverage_state_table.size();++i){
            auto state = i+ctx.start_of_covering_states;
            if (ctx.get_inf(state,executionloop_context_t::SM) && ctx.get_parent(state) == 0 ){
                r.push_back(state);
            }
        }
        std::sort(r.begin(),r.end());
        auto last = std::unique(r.begin(), r.end());
        r.erase(last,r.end());
        return r;
    };

    auto compute_total_of_transitions = [&]( executionloop_context_t const & ctx,
                                             std::unordered_map<int,int> & sm2_total_transitions ){
        if( ctx.start_of_covering_transitions_valid() ){
           for(std::size_t i = 0;i != ctx.coverage_transitions_table.size();++i){
               if (ctx.transitions[ctx.start_of_covering_transitions + i].start()) continue;
               auto from_state = ctx.transitions[ctx.start_of_covering_transitions + i].from;
               auto to_state = ctx.transitions[ctx.start_of_covering_transitions + i].to;

               if (
                   ctx.get_inf(from_state,executionloop_context_t::DONT_COVER) ||
                   ctx.get_inf(from_state,executionloop_context_t::INIT) ||
                   ctx.get_inf(from_state,executionloop_context_t::FINAL) ||
                   ctx.get_inf(from_state,executionloop_context_t::SM) ) continue;

               if (
                   ctx.get_inf(to_state,executionloop_context_t::DONT_COVER) ||
                   ctx.get_inf(to_state,executionloop_context_t::INIT) ||
                   ctx.get_inf(to_state,executionloop_context_t::FINAL) ||
                   ctx.get_inf(to_state,executionloop_context_t::SM) ) continue;
               if (to_state == from_state && ctx.get_inf(ctx.get_parent(from_state),executionloop_context_t::DONT_COVER_LOOPS))
                   continue;
               ++sm2_total_transitions[get_root_sm(ctx,from_state)];
           }
        }
    };

    auto compute_transition_coverage = [&](executionloop_context_t const & ctx,
                                           std::vector<int> const & coverage_transitions_table,
                                           std::unordered_map<int,int>& sm2_covered_transitions){
        if( ctx.start_of_covering_transitions_valid() ){
           for(std::size_t i = 0;i != coverage_transitions_table.size();++i){
               if (ctx.transitions[ctx.start_of_covering_transitions + i].start()) continue;
               auto from_state = ctx.transitions[ctx.start_of_covering_transitions + i].from;
               auto to_state = ctx.transitions[ctx.start_of_covering_transitions + i].to;

               if (
                   ctx.get_inf(from_state,executionloop_context_t::DONT_COVER) ||
                   ctx.get_inf(from_state,executionloop_context_t::INIT) ||
                   ctx.get_inf(from_state,executionloop_context_t::FINAL) ||
                   ctx.get_inf(from_state,executionloop_context_t::SM) ) continue;

               if (
                   ctx.get_inf(to_state,executionloop_context_t::DONT_COVER) ||
                   ctx.get_inf(to_state,executionloop_context_t::INIT) ||
                   ctx.get_inf(to_state,executionloop_context_t::FINAL) ||
                   ctx.get_inf(to_state,executionloop_context_t::SM) ) continue;
               if (to_state == from_state && ctx.get_inf(ctx.get_parent(from_state),executionloop_context_t::DONT_COVER_LOOPS))
                   continue;
               //auto sms = ctx.get_assoc_sm(ctx.transitions[ctx.start_of_covering_transitions + i].root_sms);
               //
               if (coverage_transitions_table[i]) ++sm2_covered_transitions[get_root_sm(ctx,from_state)];
           }
        }
    };
    cleanup<decltype(cleanup_f)> cl{cleanup_f};
    std::unordered_map<int,int> channel2socket;
    std::unordered_map<int,std::thread*> channel2writer_thread;

    std::vector<int> top_level_sms;
    std::unordered_map<int,double> sm2cov;
    std::unordered_map<int,int> sm2_total_transitions;
    std::unordered_map<int,int> sm2_covered_transitions;
    std::vector<int> coverage_state_table_last;
    std::vector<int> coverage_transitions_table_last;
    std::map<std::string,std::vector<int>> label2states;
    std::map<int,std::vector<int>> root2states;

    long long next_msg_id = 0;
    struct channel_msg_t{
        long long id;
        bool init_msg = false;
        std::stringstream msg;
    };
    std::vector<channel_msg_t> channel_out;
    constexpr auto channel_size = 32;
    channel_out.resize(channel_size);
    int ch_out_start = 0;
    int ch_out_end = 0;
    std::mutex channel_mutex;
    std::mutex writer_thread_map_mutex;
    std::condition_variable cv_channel;
    bool coverage_tables_are_current = false;
    top_level_sms = compute_root_sms(ctx);

    auto compute_init_msg_main_part = [&]( std::vector<int>& coverage_transitions_table,
                                           std::vector<int>& coverage_table,
                                           executionloop_context_t::enter_times_t& enter_times,
                                           executionloop_context_t::exit_times_t& exit_times)->std::stringstream{
        for(auto e : top_level_sms) {
            sm2cov[e] = 0.0;sm2_total_transitions[e] = 0;sm2_covered_transitions[e] = 0;
        }
        compute_total_of_transitions(ctx,sm2_total_transitions);
        compute_transition_coverage(ctx,coverage_transitions_table,sm2_covered_transitions);
        for(auto e : sm2_total_transitions){
            if ( e.second ) {
                if (sm2_covered_transitions[e.first] == e.second) sm2cov[e.first] = 1.0;
                else  sm2cov[e.first] = (double)sm2_covered_transitions[e.first] / (double)e.second;
            } else sm2cov[e.first] = 1.0;
        }
        if (label2states.size() == 0){
            for(std::size_t i = 0;i != ctx.coverage_state_table.size();++i){
                auto state = i+ctx.start_of_covering_states;
                if (!ctx.get_inf(state,executionloop_context_t::SM) &&
                    !ctx.get_inf(state,executionloop_context_t::DONT_COVER) &&
                    !ctx.get_inf(state,executionloop_context_t::INIT) &&
                    !ctx.get_inf(state,executionloop_context_t::FINAL) &&
                    !ctx.get_inf(state,executionloop_context_t::HIDDEN) &&
                    ctx.get_inf(state,executionloop_context_t::HAS_LABEL) ){
                   label2states[ctx.state_labels[state]].push_back(state);
                }
            }
        }
        if (root2states.size() == 0){
            for(std::size_t i = 0;i != ctx.coverage_state_table.size();++i){
                auto state = i+ctx.start_of_covering_states;
                if (!ctx.get_inf(state,executionloop_context_t::SM) &&
                    !ctx.get_inf(state,executionloop_context_t::DONT_COVER) &&
                    !ctx.get_inf(state,executionloop_context_t::INIT) &&
                    !ctx.get_inf(state,executionloop_context_t::FINAL) &&
                    !ctx.get_inf(state,executionloop_context_t::HIDDEN)  ){
                   root2states[get_root_sm(ctx,state)].push_back(state);
                }
            }
        }

        std::stringstream ss;
        ss << "\"ok\":" << "true" << "," << "\n";
        ss << "\"what\":" << "\"" << "init" <<"\""<<  "," << "\n";
        ss << "\"toplevel_sms\":" << "[";
        bool first_in_list = true;
        for(std::size_t i = 0;i != top_level_sms.size();++i){
            if (!first_in_list) ss << ",";
            ss << top_level_sms[i];
            first_in_list = false;
        }
        ss << "]," << "\n";
        ss << "\"toplevel_sms_labels\":" << "[";
        first_in_list = true;
        for(std::size_t i = 0;i != top_level_sms.size();++i){
            auto state = top_level_sms[i];
            auto assoc_sm = ctx.get_assoc_sm(state);
            if (!assoc_sm) continue;
            if (!first_in_list) ss << ",";
            auto label = assoc_sm->label();
            if(label.length()==0) label = assoc_sm->id();
            ss << "\""<<escape_json_string(label)<<"\"";
            first_in_list = false;
        }
        ss << "]," << "\n";
        ss << "\"toplevel_sms_transition_coverage\":" << "[";
        first_in_list = true;
        for(std::size_t i = 0;i != top_level_sms.size();++i){
            if (!first_in_list) ss << ",";
            ss << sm2cov[top_level_sms[i]];
            first_in_list = false;
        }
        ss << "]," << "\n";
        ss << "\"labeled_states\":" << "[";
        first_in_list = true;
        for(auto label: label2states){
            if (!label.second.size()) continue;
            if (!first_in_list) ss << ",";
            ss << "\""<<escape_json_string(label.first)<<"\"";
            for(auto state: label.second) ss << "," << state;
            first_in_list = false;
        }
        ss << "]," << "\n";
        ss << "\"root2childstates\":" << "[";
        first_in_list = true;
        for(auto root: root2states){
            if (!root.second.size()) continue;
            if (!first_in_list) ss << ",";
            ss << root.first;
            for(auto state: root.second) ss << "," << state;
            ss << ",0";
            first_in_list = false;
        }
        ss << "]," << "\n";
        ss << "\"covered_states\":" << "[";
        first_in_list = true;
        for(auto s=0;s !=coverage_table.size();++s){
            if (!coverage_table[s]) continue;
            if (!first_in_list) ss << ",";
            auto state = s+ctx.start_of_covering_states;
            ss << state;

            first_in_list = false;
        }
        ss << "]," << "\n";

        ss << "\"category_mapping\":" << "[";
        first_in_list = true;
        for(auto & e : categories){
            if (!first_in_list) ss << ",";
            ss << "\""<<escape_json_string(e.first)<< "\","<<e.second;
            first_in_list = false;
        }
        ss << "]," << "\n";

        ss << "\"entering_times\":" << "[";
        first_in_list = true;
        for(auto e:enter_times){
            if (!first_in_list) ss << ",";
            auto state = e.first;
            ss << state;
            auto d = e.second - ctx.start_execution_time_stamp_hres;
            auto secs = std::chrono::duration_cast<std::chrono::seconds>(d).count();
            auto millisecs = std::chrono::duration_cast<std::chrono::milliseconds>(d).count() % 1000;
            auto microsecs = std::chrono::duration_cast<std::chrono::microseconds>(d).count() % 1000;
            ss << "," << secs << "," << millisecs << "," << microsecs;
            first_in_list = false;
        }
        ss << "]," << "\n";

        ss << "\"exiting_times\":" << "[";
        first_in_list = true;
        for(auto e:exit_times){
            if (!first_in_list) ss << ",";
            auto state = e.first;
            ss << state;
            auto d = e.second - ctx.start_execution_time_stamp_hres;
            auto secs = std::chrono::duration_cast<std::chrono::seconds>(d).count();
            auto millisecs = std::chrono::duration_cast<std::chrono::milliseconds>(d).count() % 1000;
            auto microsecs = std::chrono::duration_cast<std::chrono::microseconds>(d).count() % 1000;
            ss << "," << secs << "," << millisecs << "," << microsecs;
            first_in_list = false;
        }
        ss << "]," << "\n";
        ss << "\"exec_context_start_utc\":" << std::chrono::system_clock::to_time_t(ctx.start_execution_time_stamp_system) << ",\n";
        ss << "\"total_of_states\":" << coverage_table.size() << ",\n";
        ss << "\"total_of_transitions\":" << coverage_transitions_table.size() << ",\n";

        {
            compute_categories();
            ss << "\"category_changes\":" << "[";
            first_in_list = true;
            for(auto e:root2active_categories){
                auto cats = e.second;
                if(cats == 0) continue;
                if (!first_in_list) ss << ",";
                ss << e.first << "," << cats;
                first_in_list = false;
            }
            ss << "]" << "\n";
        }
        return ss;
    };

    auto compute_update_msg_main_part = [&](std::vector<int>& coverage_table_new,
                                            std::vector<int>& coverage_table_old,
                                            std::vector<int>& coverage_transitions_table_new,
                                            std::vector<int>& coverage_transitions_table_old,
                                            executionloop_context_t::enter_times_t& enter_times,
                                            executionloop_context_t::exit_times_t& exit_times,
                                            std::chrono::time_point<std::chrono::high_resolution_clock>& time_stamp
                                            )->std::stringstream{
        std::vector<int> changed_sms;
        for(std::size_t i = 0; i != coverage_table_new.size(); ++i){
            auto state = i+ctx.start_of_covering_states;
            if (coverage_table_new[i] != coverage_table_old[i]){
                changed_sms.push_back(get_root_sm(ctx,state));
            }
        }
        std::sort(changed_sms.begin(),changed_sms.end());
        auto last = std::unique(changed_sms.begin(), changed_sms.end());
        changed_sms.erase(last,changed_sms.end());

        for(auto e : top_level_sms) {
            sm2_covered_transitions[e] = 0;
        }
        compute_transition_coverage(ctx,coverage_transitions_table_new,sm2_covered_transitions);
        for(auto e : sm2_total_transitions){
            if ( e.second ) {
                if (sm2_covered_transitions[e.first] == e.second) sm2cov[e.first] = 1.0;
                else  sm2cov[e.first] = (double)sm2_covered_transitions[e.first] / (double)e.second;
            } else sm2cov[e.first] = 1.0;
        }
        std::stringstream ss;
        ss << "\"ok\":" << "true" << "," << "\n";
        ss << "\"what\":" << "\"" << "update" <<"\""<<  "," << "\n";
        ss << "\"toplevel_sms\":" << "[";
        bool first_in_list = true;
        for(std::size_t i = 0;i != changed_sms.size();++i){
            if (!first_in_list) ss << ",";
            ss << changed_sms[i];
            first_in_list = false;
        }
        ss << "]," << "\n";

        ss << "\"toplevel_sms_transition_coverage\":" << "[";
        first_in_list = true;
        for(std::size_t i = 0;i != changed_sms.size();++i){
            if (!first_in_list) ss << ",";
            ss << sm2cov[changed_sms[i]];
            first_in_list = false;
        }
        ss << "]," << "\n";
        ss << "\"covered_states\":" << "[";
        first_in_list = true;
        for(auto s=0;s !=coverage_table_new.size();++s){
            if (!coverage_table_new[s]) continue;
            if (coverage_table_new[s] == coverage_table_old[s]) continue;
            if (!first_in_list) ss << ",";
            auto state = s+ctx.start_of_covering_states;
            ss << state;
            first_in_list = false;
        }
        ss << "]," << "\n";

        ss << "\"covered_states\":" << "[";
        first_in_list = true;
        for(auto s=0;s !=coverage_table_new.size();++s){
            if (!coverage_table_new[s]) continue;
            if (coverage_table_new[s] == coverage_table_old[s]) continue;
            if (!first_in_list) ss << ",";
            auto state = s+ctx.start_of_covering_states;
            ss << state;
            first_in_list = false;
        }
        ss << "]," << "\n";

        ss << "\"entering_times\":" << "[";
        first_in_list = true;
        for(auto e:enter_times){
            if (time_stamp > e.second) continue;
            if (!first_in_list) ss << ",";
            auto state = e.first;
            ss << state;
            auto d = e.second - ctx.start_execution_time_stamp_hres;
            auto secs = std::chrono::duration_cast<std::chrono::seconds>(d).count();
            auto millisecs = std::chrono::duration_cast<std::chrono::milliseconds>(d).count() % 1000;
            auto microsecs = std::chrono::duration_cast<std::chrono::microseconds>(d).count() % 1000;
            ss << "," << secs << "," << millisecs << "," << microsecs;
            first_in_list = false;
        }
        ss << "]," << "\n";

        ss << "\"exiting_times\":" << "[";
        first_in_list = true;
        for(auto e:exit_times){
            if (time_stamp > e.second) continue;
            if (!first_in_list) ss << ",";
            auto state = e.first;
            ss << state;
            auto d = e.second - ctx.start_execution_time_stamp_hres;
            auto secs = std::chrono::duration_cast<std::chrono::seconds>(d).count();
            auto millisecs = std::chrono::duration_cast<std::chrono::milliseconds>(d).count() % 1000;
            auto microsecs = std::chrono::duration_cast<std::chrono::microseconds>(d).count() % 1000;
            ss << "," << secs << "," << millisecs << "," << microsecs;
            first_in_list = false;
        }
        ss << "]," << "\n";

        {
            auto v = compute_categories_diff();
            ss << "\"category_changes\":" << "[";
            first_in_list = true;
            for(auto e:v){
                if (!first_in_list) ss << ",";
                auto cats = root2active_categories[e];
                ss << e << "," << cats;
                first_in_list = false;
            }
            ss << "]" << "\n";
        }
        return ss;
    };

    auto create_init_msg = [&](coverage_update_msg_t& msg,
                               executionloop_context_t::enter_times_t& enter_times,
                               executionloop_context_t::exit_times_t& exit_times){
        auto m = compute_init_msg_main_part(coverage_transitions_table_last,coverage_state_table_last,enter_times,exit_times);
        {
          std::lock_guard<std::mutex> lk(channel_mutex);
          channel_out.clear();
          channel_out.resize(channel_size);
          channel_out[0]=channel_msg_t{next_msg_id++,true,std::move(m)};
          ch_out_start=0;ch_out_end=1;
        }
        cv_channel.notify_all();
    };

    executionloop_context_t::enter_times_t current_enter_times;
    executionloop_context_t::exit_times_t current_exit_times;

    //Compute Category Mapping

    int counter = 0;
    auto f = [this,&stateindex2categories,&counter,&categories](State_machine* cur_sm){
        for(auto it = cur_sm->states().begin(); it != cur_sm->states().end(); ++it) {
         auto state = *it;
         std::vector<std::string> v;
         for(auto e: state->categories()) {
             v.push_back(e);
             auto it = categories.find(e);
             if (it==categories.end()) categories[e] = counter++;
         }
         if (v.size()) stateindex2categories[state->idx_] = v;
        }
     };

    traverse_sms(State_machine::statemachines,f);
    //Computation of Categories

    for(auto i : top_level_sms){
        root2active_categories[i] = 0;
    }

    for(;;){
        coverage_update_msg_t msg;

        if (!q->wait_for_data_with_timeout(std::chrono::milliseconds(5000))){
            cv_channel.notify_all();
            continue;
        } else q->wait_and_pop(msg);

        if (msg.what != coverage_update_msg_t::NEW_CHANNEL){
           current_enter_times = msg.enter_times;
           current_exit_times = msg.exit_times;
           current_states = msg.current_states;
        }

        if (msg.what == coverage_update_msg_t::what_t::NEW_CHANNEL){
            if (msg.payload.channel.socket == -1) continue;
            if (coverage_tables_are_current) create_init_msg(msg,current_enter_times,current_exit_times);

            channel2socket[msg.payload.channel.id] = msg.payload.channel.socket;
            {
             std::stringstream ss;
             ss << "{";
                ss << "\"ok\":" << "true" << "," << "\n";
                ss << "\"channel\":" << msg.payload.channel.id  << ",\n";
                ss << "\"topic\":\"COVERAGE\"\n";
             ss << "}";
             auto writer_thread = std::thread{[&channel_out, channel_size, &next_msg_id,
                                                   &cv_channel, &channel_mutex, &ch_out_start,
                                                   &ch_out_end, &send_reply,&next_msg_id,
                                                   &writer_thread_map_mutex,&channel2writer_thread]
              (std::string intro,int sck,int channel_id)->void{
               auto cleanup_f = [&](){
                   if (sck != -1) close(sck);
               };
               cleanup<decltype(cleanup_f)> cl{cleanup_f};
               if(!send_reply(sck,intro)) return;
               long long unseen_msg_id = 0;
               bool init_read = false;
               for(;;){
                   std::vector<std::string> msgs;
                   {
                     std::unique_lock<std::mutex> lk(channel_mutex);
                     cv_channel.wait(lk, [&unseen_msg_id,&next_msg_id]{return unseen_msg_id != next_msg_id;});
                     auto new_unseen_msg_id = unseen_msg_id;
                     for(int i = ch_out_start; i != ch_out_end; i = (i + 1) % channel_size ){
                         if (channel_out[i].id < unseen_msg_id) continue;
                         new_unseen_msg_id = std::max(channel_out[i].id+1,new_unseen_msg_id);
                         if (init_read && channel_out[i].init_msg) continue;
                         msgs.push_back(channel_out[i].msg.str());
                         init_read = init_read || channel_out[i].init_msg;
                     }
                     unseen_msg_id = new_unseen_msg_id;
                   }
                   for(auto& m : msgs) if(!send_reply(sck,m,"{\"channel\":" +std::to_string(channel_id) + ",\n","}")) return;
                   if (msgs.size() == 0){
                       if(!send_reply(sck,"{\"channel\":" +std::to_string(channel_id) + ",\n \"topic\":\"COVERAGE\",\n \"what\":\"NOOP\"\n}")) return;
                   }
               }
             },std::move(ss.str()),msg.payload.channel.socket,msg.payload.channel.id};             
             writer_thread.detach();
            }
        } else if (msg.what == coverage_update_msg_t::what_t::INIT) {
            coverage_state_table_last = msg.coverage_state_table;
            coverage_transitions_table_last = msg.coverage_transitions_table;
            coverage_tables_are_current = true;
            create_init_msg(msg,current_enter_times,current_exit_times);
        } else if (msg.what == coverage_update_msg_t::what_t::UPDATE) {
            auto m = compute_update_msg_main_part (msg.coverage_state_table,
                                                   coverage_state_table_last,
                                                   msg.coverage_transitions_table,
                                                   coverage_transitions_table_last,
                                                   msg.enter_times,
                                                   msg.exit_times,
                                                   msg.time_stamp);
            coverage_state_table_last = msg.coverage_state_table;
            coverage_transitions_table_last = msg.coverage_transitions_table;
            {
              std::lock_guard<std::mutex> lk(channel_mutex);
              bool full = ((ch_out_end+1) % channel_size) == ch_out_start;
              {channel_out[ch_out_end]=channel_msg_t{next_msg_id++,false,std::move(m)};ch_out_end = (ch_out_end+1)%channel_size;}
              if(full) ch_out_start = (ch_out_start+1)%channel_size;
            }
            cv_channel.notify_all();
        }
    }
}

void Websocket_interface::handle_subscribe_coverage(int sck){
    auto ch_id = this->next_subscribe_channel_id++;
    if (subscribe_channels2thread.size() == 0){
        State_machine_simulation_core::event_t ev;
        coverage_channel_queue = new threadsafe_queue<coverage_update_msg_t,std::queue<coverage_update_msg_t>>;
        auto exec = new Subscribe_coverage{ch_id,coverage_channel_queue};
        ev.exec = exec;
        smc_->enqueue_event(ev);
        subscribe_channels2thread[ch_id] = new std::thread{&Websocket_interface::handle_subscribe_coverage_thread,
                                                           this,
                                                           coverage_channel_queue
                                                          };
    } else subscribe_channels2thread[ch_id] = subscribe_channels2thread[ch_id-1];
    coverage_update_msg_t msg;
    msg.what = coverage_update_msg_t::what_t::NEW_CHANNEL;
    msg.payload.channel.id = ch_id;
    msg.payload.channel.socket = sck;
    coverage_channel_queue->push(msg);
}


void Websocket_interface::dispatcher(){
 socklen_t addrlen = sizeof(struct sockaddr_storage);
 struct sockaddr_storage claddr = {0};
 int cfd = -1;
 struct addrinfo hints = {0};
 struct addrinfo* result, * rp;
 int lfd = -1;

 hints.ai_canonname = nullptr;
 hints.ai_addr = nullptr;
 hints.ai_next = nullptr;
 hints.ai_socktype = SOCK_STREAM;
 hints.ai_family = AF_INET;
 hints.ai_flags = AI_PASSIVE;// | AI_NUMERICSERV;

 if (getaddrinfo(nullptr,port_.c_str(),&hints,&result) != 0)
   smc_->fatal_(-1,"getaddrinfo failed");
 int optval=1;
 for(rp=result;rp;rp=rp->ai_next)
 {
  lfd = socket(rp->ai_family,rp->ai_socktype,rp->ai_protocol);
  if(lfd == -1) continue;
  if (setsockopt(lfd,SOL_SOCKET,SO_REUSEADDR,(char*)&optval,sizeof(optval))) smc_->fatal_(-1,"setsockopt");
  if (bind(lfd,rp->ai_addr,rp->ai_addrlen) == 0) break;
  close(lfd);
 }
 if (!rp) smc_->fatal_(-1,"Websocket_interface::dispatcher(): Could not bind socket to any address.port="+port_);
 if (listen(lfd,128)==-1)smc_->fatal_(-1,"listen");
 freeaddrinfo(result);

 for(;!smc_->shutdown();)
 {
  cfd = accept(lfd, (struct sockaddr*) &claddr, &addrlen);

  if (cfd == -1)  continue;
  {
    using namespace std;
    {
     lock_guard<std::mutex> lg(handler_threads_status_mutex_);
     thread_status_type* pp = nullptr;
     for(auto& s : handler_threads_status_) if (!get<1>(s) && get<0>(s)) {
        if(!pp) pp = &s;
     }//for
     if (pp) {*pp = thread_status_type{nullptr,true,cfd};} else handler_threads_status_.push_back(thread_status_type{nullptr,true,cfd});
    }
    std::thread tp {&Websocket_interface::handler,this,cfd};
    tp.detach();

  }
 }
}

void Websocket_interface::start(){
    dispatcher_thread_ = new std::thread(&Websocket_interface::dispatcher,this);
}






