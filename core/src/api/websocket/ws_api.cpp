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
 * http/websocket routines
*/

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

static void ceps2json(std::stringstream& s,ceps::ast::Nodebase_ptr n){
    using namespace ceps::ast;
    if (n->kind() == Ast_node_kind::int_literal)
        s << value(as_int_ref(n));
    else if (n->kind() == Ast_node_kind::float_literal)
        s << value(as_double_ref(n));
    else if (n->kind() == Ast_node_kind::string_literal)
        s << "\"" <<  escape_json_string(value(as_string_ref(n))) << "\"";
    else if (n->kind() == Ast_node_kind::identifier)
        s << "\"" <<  escape_json_string(name(as_id_ref(n))) << "\"";
    else if (n->kind() == Ast_node_kind::structdef){
        auto & st = as_struct_ref(n);
        s << "{ \"type\":\"struct\", \"name\":" <<"\""<< name(st) << "\",\"content\":[";
        for(std::size_t i = 0; i != st.children().size();++i){
            auto nn = st.children()[i];
            if(!nn) continue;
            ceps2json(s,nn);if (1+i != st.children().size()) s << ",";
        }
        s << "]}";
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
    } else if (n->kind() == Ast_node_kind::scope){
        auto & scp = as_scope_ref(n);
        s << "{ \"type\":\"scope\", \"content\":[";
        for(std::size_t i = 0; i != scp.children().size();++i){
            auto nn = scp.children()[i];
            if(!nn) continue;
            ceps2json(s,nn);if (1+i != scp.children().size()) s << ",";
        }
        s << "]}";
    }
    else if (n->kind() == Ast_node_kind::symbol){
        auto & sy = as_symbol_ref(n);
        s << "{ \"type\":\"symbol\",\"kind\":" << "\""<< kind(sy) << "\", \"name\":" <<"\""<< name(sy) << "\"}";
    }
}

class Execute_query : public sm4ceps_plugin_int::Executioncontext {
    std::string query;
    int sck;
 public:
    Execute_query() = default;
    Execute_query(std::string q, int s):query{q}, sck{s}{}
    void  run(State_machine_simulation_core* ctxt){
      std::string r;

      std::stringstream s;
      s << query;
      try {
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



 auto cleanup_f = [this,sck](){
     using namespace std;
     lock_guard<std::mutex> lg(handler_threads_status_mutex_);
     for(auto& s : handler_threads_status_)
        if (get<1>(s) && get<0>(s) && get<2>(s)==sck)
          {get<1>(s)=false;get<2>(s)=-1;close(sck);}

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
    //handle command
    //std::cout << s << std::endl;

    std::stringstream hs{s};
    std::vector<std::string> cmd;
    for(;hs;){
        std::string l;
        hs >> l;
        cmd.push_back(l);
        if (cmd.size() == 1 && cmd[0] == "PUSH"){
            cmd.push_back(s.substr(5));
            break;
        }
    }
    std::string reply = "{\"ok\": false}";

    if (cmd.size() != 0){
     decltype(cmd) args;
     for (std::size_t i = 1; i != cmd.size(); ++i ) if (cmd[i].length()) {args.push_back(cmd[i]);}
     //std::cout << args.size() << std::endl;

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
      } else if (cmd[0] == "EVENT" && args.size()) {
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
    }
    }//cmd.size()!=0
    //std::cout << reply << std::endl;
    if(!send_reply(reply)) break;
   }
  }//for
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
    lock_guard<std::mutex> lg(handler_threads_status_mutex_);
    thread_status_type* pp = nullptr;
    for(auto& s : handler_threads_status_) if (!get<1>(s) && get<0>(s)) {
        get<0>(s)->join();delete get<0>(s);get<0>(s)=nullptr;if(!pp) pp = &s;
    }//for
    auto tp = new thread(&Websocket_interface::handler,this,cfd);
    if (pp) {*pp = thread_status_type{tp,true,cfd};} else handler_threads_status_.push_back(thread_status_type{tp,true,cfd});
  }
 }
}

void Websocket_interface::start(){
    dispatcher_thread_ = new std::thread(&Websocket_interface::dispatcher,this);
}






