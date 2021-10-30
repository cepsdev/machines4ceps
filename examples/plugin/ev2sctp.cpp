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
#include "ceps_ast.hh"
  
#include "core/include/state_machine_simulation_core_reg_fun.hpp"
#include "core/include/state_machine_simulation_core_plugin_interface.hpp"
#include "core/include/state_machine_simulation_core.hpp"


#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <stdlib.h>
#include <map>
#include <algorithm>

static Ism4ceps_plugin_interface* plugin_master = nullptr;

using scope_t = ceps::parser_env::Scope;
using scope_ref_t = ceps::parser_env::Scope&;
using scope_ptr_t = ceps::parser_env::Scope*;

namespace ceps{
 namespace interpreter{
   ceps::interpreter::node_int_t mk_val( int val){
    return ceps::interpreter::mk_int_node(val);
  }

   bool set_val(void*& payload, int val){
    payload = (Nodebase_ptr) ceps::interpreter::mk_int_node(val);
    return payload;
  }

  template <typename Iter> bool set_val(void*& payload, Iter from, Iter to){
    auto container = ceps::interpreter::mk_nodeset();
    payload = (Nodebase_ptr) container;
    for(auto it = from; it != to; ++it)
     container->children().push_back(mk_val(*it));
    return payload;
  }

  template <typename T> bool set_val(std::string name,T* from, T* to, scope_ref_t scope){
    auto sym = scope.insert(name);
    sym->category = ceps::parser_env::Symbol::Category::VAR;
    return set_val(sym->payload, from, to);
  }

  template <typename T> bool set_val(std::string name,T val,scope_ref_t scope){
    auto sym = scope.insert(name);
    sym->category = ceps::parser_env::Symbol::Category::VAR;
    return set_val(sym->payload, val);
  }
 }
}

namespace mme{
  uint16_t constexpr CM_SLAC_PARM_REQ = 1;
  uint16_t constexpr CM_SLAC_PARM_CNF = 2;
  uint16_t constexpr CM_START_ATTEN_CHAR_IND = 3;
  uint16_t constexpr CM_MNBC_SOUND_IND = 4;
  uint16_t constexpr CM_ATTEN_CHAR_IND = 5;
  uint16_t constexpr CM_ATTEN_PROFILE_IND = 6;
  uint16_t constexpr CM_ATTEN_CHAR_RSP = 7;
  uint16_t constexpr CM_VALIDATE_REQ = 8;
  uint16_t constexpr CM_VALIDATE_CNF = 9;
  uint16_t constexpr CM_SLAC_MATCH_REQ = 10;
  uint16_t constexpr CM_SLAC_MATCH_CNF = 11;
  uint16_t constexpr CM_SET_KEY_REQ = 12;
  uint16_t constexpr CM_SET_KEY_CNF = 13;
  uint16_t constexpr CM_AMP_MAP_REQ = 14;
  uint16_t constexpr CM_AMP_MAP_CNF = 15;
}

struct cm_slac_parm_req_t {
      uint8_t application_type;
      uint8_t security_type;
      uint8_t run_id [8];
};

struct cm_slac_parm_cnf_t {
      uint8_t m_sound_target[6];
      uint8_t num_sounds;
      uint8_t time_out;
      uint8_t resp_type;
      uint8_t forwarding_sta [6];
      uint8_t application_type;
      uint8_t security_type;
      uint8_t run_id [8];
};

struct cm_start_atten_char_ind_t{
      uint8_t application_type;
      uint8_t security_type;
      uint8_t num_sounds;
      uint8_t time_out;
      uint8_t resp_type;
      uint8_t forwarding_sta [6];
      uint8_t run_id [8];
};

struct cm_mnbc_sound_ind_t{
      uint8_t application_type;
      uint8_t security_type;
      uint8_t sender_id[17];
      uint8_t cnt;
      uint8_t run_id [8];
      uint8_t reserved[8];
      uint8_t rnd[16];
};

struct cm_atten_char_ind_t{
      uint8_t application_type;
      uint8_t security_type;
      uint8_t source_address[6];
      uint8_t run_id [8];
      uint8_t source_id[17];
      uint8_t resp_id[17];
      uint8_t num_sounds;
      uint8_t atten_profile[59];
};

struct cm_atten_char_rsp_t{
      uint8_t application_type;
      uint8_t security_type;
      uint8_t source_address[6];
      uint8_t run_id [8];
      uint8_t source_id[17];
      uint8_t resp_id[17];
      uint8_t result;
};

struct cm_atten_profile_ind_t{
      uint8_t pev_mac[6];
      uint8_t num_groups;
      uint8_t rsvd;
      uint8_t aag[58];
};

struct cm_validate_req_t{
      uint8_t signal_type;
      uint8_t timer;
      uint8_t result;
};

struct cm_validate_cnf_t{
      uint8_t signal_type;
      uint8_t toggle_num;
      uint8_t result;
};


struct cm_slac_match_req_t{
      uint8_t application_type;
      uint8_t security_type;
      uint16_t mvflength;
      uint8_t pev_id[17];
      uint8_t pev_mac[6];
      uint8_t evse_id[17];
      uint8_t evse_mac[6];
      uint8_t run_id [8];
      uint8_t rsvd [8];
};

struct cm_slac_match_cnf_t{
      uint8_t application_type;
      uint8_t security_type;
      uint16_t mvflength;
      uint8_t pev_id[17];
      uint8_t pev_mac[6];
      uint8_t evse_id[17];
      uint8_t evse_mac[6];
      uint8_t run_id [8];
      uint8_t rsvd [8];
      uint8_t nid[7];
      uint8_t rsvd2;
      uint8_t nmk[16];
};

struct cm_set_key_req_t{
      uint8_t key_type;
      uint32_t my_nonce;
      uint32_t your_nonce;
      uint8_t pid;
      uint16_t prn;
      uint8_t pmn;
      uint8_t cco_capability;
      uint8_t nid[7];
      uint8_t new_eks;
      uint8_t new_key[16];
};

struct cm_amp_map_req_t{
      uint16_t amlen;
      uint8_t amdata[1];
};

struct cm_amp_map_cnf_t{
      uint8_t restype;
};    

struct homeplug_mme_generic{
  char oda[6];
  char osa[6];
  std::uint32_t vlan_tag;
  std::uint16_t mtype;
  std::uint8_t mmv;
  std::uint16_t mmtype;
  std::uint8_t fmi;
  std::uint8_t fmsn;
  union{
    cm_slac_parm_req_t cm_slac_parm_req;
    cm_slac_parm_cnf_t cm_slac_parm_cnf;
    cm_start_atten_char_ind_t cm_start_atten_char_ind;
    cm_mnbc_sound_ind_t cm_mnbc_sound_ind;
    cm_atten_char_ind_t cm_atten_char_ind;
    cm_atten_char_rsp_t cm_atten_char_rsp;
    cm_atten_profile_ind_t cm_atten_profile_ind;
    cm_validate_req_t cm_validate_req;
    cm_validate_cnf_t cm_validate_cnf;
    cm_slac_match_req_t cm_slac_match_req;
    cm_slac_match_cnf_t cm_slac_match_cnf;
    cm_set_key_req_t cm_set_key_req;
    cm_amp_map_req_t cm_amp_map_req;
    cm_amp_map_cnf_t cm_amp_map_cnf;    
    struct{

    } cm_set_key_cnf;
  } mmdata;
};

template<typename Iter>
size_t write_bytes(ceps::ast::Struct_ptr strct, Iter beg, Iter end){
  using namespace ceps::ast;
  auto sit = strct->children().begin();
  size_t written = 0;
  for(auto it = beg; it != end && sit != strct->children().end();++it,++sit){
    if (is<ceps::ast::Ast_node_kind::int_literal>(*sit)){
      *it = value(as_int_ref(*sit));
      ++written;
    }
    else if (is<ceps::ast::Ast_node_kind::long_literal>(*sit)){
      *it = value(as_int64_ref(*sit));
      ++written;
    }
    else if (is<ceps::ast::Ast_node_kind::float_literal>(*sit)){
      *it = value(as_double_ref(*sit));
      ++written;
    }
    else if (is<ceps::ast::Ast_node_kind::string_literal>(*sit)){
      auto const & v = value(as_string_ref(*sit));
      for(size_t i = 0; i < v.length() && it != end; ++i,++it){
        *it = v.at(i);++written;
      }
      if (it == end) break;
    }
  }
  return written;
}

template<typename C, typename Iter>
size_t write_bytes(C const & vec, Iter beg, Iter end){
  using namespace ceps::ast;
  auto sit = vec.begin();
  size_t written = 0;
  for(auto it = beg; it != end && sit != vec.end();++it,++sit){
    if (is<ceps::ast::Ast_node_kind::int_literal>(*sit)){
      *it = value(as_int_ref(*sit));
      ++written;
    }
    else if (is<ceps::ast::Ast_node_kind::long_literal>(*sit)){
      *it = value(as_int64_ref(*sit));
      ++written;
    }
    else if (is<ceps::ast::Ast_node_kind::float_literal>(*sit)){
      *it = value(as_double_ref(*sit));
      ++written;
    }
    else if (is<ceps::ast::Ast_node_kind::string_literal>(*sit)){
      auto const & v = value(as_string_ref(*sit));
      for(size_t i = 0; i < v.length() && it != end; ++i,++it){
        *it = v.at(i);++written;
      }
      if (it == end) break;
    }
  }
  return written;
}

size_t write(std::vector<ceps::ast::Nodebase_ptr> const & v, cm_slac_parm_req_t& msg, size_t size){
  size_t written = 0;
  for(auto e : v){
    if (!ceps::ast::is<ceps::ast::Ast_node_kind::structdef>(e)) continue;
    auto& strct = *ceps::ast::as_struct_ptr(e);
    auto& name = ceps::ast::name(strct);

    if (name == "application_type") 
     written += write_bytes(&strct,((uint8_t*)&msg.application_type),((uint8_t*)&msg.application_type) + sizeof(msg.application_type));
    else if (name == "run_id")
     written += write_bytes(&strct,((uint8_t*)&msg.run_id),((uint8_t*)&msg.run_id) + sizeof(msg.run_id));
    else if (name == "security_type")
     written += write_bytes(&strct,((uint8_t*)&msg.security_type),((uint8_t*)&msg.security_type) + sizeof(msg.security_type));    
  }
  return written;
} 

size_t write(std::vector<ceps::ast::Nodebase_ptr> const & v, cm_slac_parm_cnf_t& msg, size_t size){
  size_t written = 0;
  for(auto e : v){
    if (!ceps::ast::is<ceps::ast::Ast_node_kind::structdef>(e)) continue;
    auto& strct = *ceps::ast::as_struct_ptr(e);
    auto& name = ceps::ast::name(strct);

    if (name == "application_type") 
     written += write_bytes(&strct,((uint8_t*)&msg.application_type),((uint8_t*)&msg.application_type) + sizeof(msg.application_type));
    else if (name == "run_id")
     written += write_bytes(&strct,((uint8_t*)&msg.run_id),((uint8_t*)&msg.run_id) + sizeof(msg.run_id));
    else if (name == "security_type")
     written += write_bytes(&strct,((uint8_t*)&msg.security_type),((uint8_t*)&msg.security_type) + sizeof(msg.security_type));
    else if (name == "num_sounds")
     written += write_bytes(&strct,((uint8_t*)&msg.num_sounds),((uint8_t*)&msg.num_sounds) + sizeof(msg.num_sounds));
    else if (name == "time_out")
     written += write_bytes(&strct,((uint8_t*)&msg.time_out),((uint8_t*)&msg.time_out) + sizeof(msg.time_out));
    else if (name == "m_sound_target")
     written += write_bytes(&strct,((uint8_t*)&msg.m_sound_target),((uint8_t*)&msg.m_sound_target) + sizeof(msg.m_sound_target));
    else if (name == "forwarding_sta")
     written += write_bytes(&strct,((uint8_t*)&msg.forwarding_sta),((uint8_t*)&msg.forwarding_sta) + sizeof(msg.forwarding_sta));
    else if (name == "resp_type")
     written += write_bytes(&strct,((uint8_t*)&msg.resp_type),((uint8_t*)&msg.resp_type) + sizeof(msg.resp_type));
  }
  return written;
} 

size_t write(std::vector<ceps::ast::Nodebase_ptr> const & v, cm_start_atten_char_ind_t& msg, size_t size){
  size_t written = 0;
  for(auto e : v){
    if (!ceps::ast::is<ceps::ast::Ast_node_kind::structdef>(e)) continue;
    auto& strct = *ceps::ast::as_struct_ptr(e);
    auto& name = ceps::ast::name(strct);

    if (name == "application_type") 
     written += write_bytes(&strct,((uint8_t*)&msg.application_type),((uint8_t*)&msg.application_type) + sizeof(msg.application_type));
    else if (name == "run_id")
     written += write_bytes(&strct,((uint8_t*)&msg.run_id),((uint8_t*)&msg.run_id) + sizeof(msg.run_id));
    else if (name == "security_type")
     written += write_bytes(&strct,((uint8_t*)&msg.security_type),((uint8_t*)&msg.security_type) + sizeof(msg.security_type));    
    else if (name == "forwarding_sta")
     written += write_bytes(&strct,((uint8_t*)&msg.forwarding_sta),((uint8_t*)&msg.forwarding_sta) + sizeof(msg.forwarding_sta));    
    else if (name == "resp_type")
     written += write_bytes(&strct,((uint8_t*)&msg.resp_type),((uint8_t*)&msg.resp_type) + sizeof(msg.resp_type));    
    else if (name == "time_out")
     written += write_bytes(&strct,((uint8_t*)&msg.time_out),((uint8_t*)&msg.time_out) + sizeof(msg.time_out));    
    else if (name == "num_sounds")
     written += write_bytes(&strct,((uint8_t*)&msg.num_sounds),((uint8_t*)&msg.num_sounds) + sizeof(msg.num_sounds));   

  }
  return written;
} 


size_t write(std::vector<ceps::ast::Nodebase_ptr> const & v, cm_atten_char_ind_t& msg, size_t size){
  size_t written = 0;
  for(auto e : v){
    if (!ceps::ast::is<ceps::ast::Ast_node_kind::structdef>(e)) continue;
    auto& strct = *ceps::ast::as_struct_ptr(e);
    auto& name = ceps::ast::name(strct);

    if (name == "application_type") 
     written += write_bytes(&strct,((uint8_t*)&msg.application_type),((uint8_t*)&msg.application_type) + sizeof(msg.application_type));
    else if (name == "run_id")
     written += write_bytes(&strct,((uint8_t*)&msg.run_id),((uint8_t*)&msg.run_id) + sizeof(msg.run_id));
    else if (name == "security_type")
     written += write_bytes(&strct,((uint8_t*)&msg.security_type),((uint8_t*)&msg.security_type) + sizeof(msg.security_type));    
    else if (name == "source_id")
     written += write_bytes(&strct,((uint8_t*)&msg.source_id),((uint8_t*)&msg.source_id) + sizeof(msg.source_id));    
    else if (name == "source_address")
     written += write_bytes(&strct,((uint8_t*)&msg.source_address),((uint8_t*)&msg.source_address) + sizeof(msg.source_address));    
    else if (name == "resp_id")
     written += write_bytes(&strct,((uint8_t*)&msg.resp_id),((uint8_t*)&msg.resp_id) + sizeof(msg.resp_id));    
    else if (name == "num_sounds")
     written += write_bytes(&strct,((uint8_t*)&msg.num_sounds),((uint8_t*)&msg.num_sounds) + sizeof(msg.num_sounds));    
    else if (name == "atten_profile")
     written += write_bytes(&strct,((uint8_t*)&msg.atten_profile),((uint8_t*)&msg.atten_profile) + sizeof(msg.atten_profile));    
  }
  return written;
} 

size_t write(std::vector<ceps::ast::Nodebase_ptr> const & v, cm_mnbc_sound_ind_t& msg, size_t size){
  size_t written = 0;
  for(auto e : v){
    if (!ceps::ast::is<ceps::ast::Ast_node_kind::structdef>(e)) continue;
    auto& strct = *ceps::ast::as_struct_ptr(e);
    auto& name = ceps::ast::name(strct);

    if (name == "application_type") 
     written += write_bytes(&strct,((uint8_t*)&msg.application_type),((uint8_t*)&msg.application_type) + sizeof(msg.application_type));
    else if (name == "run_id")
     written += write_bytes(&strct,((uint8_t*)&msg.run_id),((uint8_t*)&msg.run_id) + sizeof(msg.run_id));
    else if (name == "security_type")
     written += write_bytes(&strct,((uint8_t*)&msg.security_type),((uint8_t*)&msg.security_type) + sizeof(msg.security_type));    
    else if (name == "sender_id")
     written += write_bytes(&strct,((uint8_t*)&msg.sender_id),((uint8_t*)&msg.sender_id) + sizeof(msg.sender_id));    
    else if (name == "cnt")
     written += write_bytes(&strct,((uint8_t*)&msg.cnt),((uint8_t*)&msg.cnt) + sizeof(msg.cnt));    
    else if (name == "rnd")
     written += write_bytes(&strct,((uint8_t*)&msg.rnd),((uint8_t*)&msg.rnd) + sizeof(msg.rnd));    

  }
  return written;
} 

size_t write(std::vector<ceps::ast::Nodebase_ptr> const & v, cm_atten_char_rsp_t& msg, size_t size){
  size_t written = 0;
  for(auto e : v){
    if (!ceps::ast::is<ceps::ast::Ast_node_kind::structdef>(e)) continue;
    auto& strct = *ceps::ast::as_struct_ptr(e);
    auto& name = ceps::ast::name(strct);

    if (name == "application_type") 
     written += write_bytes(&strct,((uint8_t*)&msg.application_type),((uint8_t*)&msg.application_type) + sizeof(msg.application_type));
    else if (name == "run_id")
     written += write_bytes(&strct,((uint8_t*)&msg.run_id),((uint8_t*)&msg.run_id) + sizeof(msg.run_id));
    else if (name == "security_type")
     written += write_bytes(&strct,((uint8_t*)&msg.security_type),((uint8_t*)&msg.security_type) + sizeof(msg.security_type));    
    else if (name == "source_address")
     written += write_bytes(&strct,((uint8_t*)&msg.source_address),((uint8_t*)&msg.source_address) + sizeof(msg.source_address));    
    else if (name == "source_id")
     written += write_bytes(&strct,((uint8_t*)&msg.source_id),((uint8_t*)&msg.source_id) + sizeof(msg.source_id));    
    else if (name == "resp_id")
     written += write_bytes(&strct,((uint8_t*)&msg.resp_id),((uint8_t*)&msg.resp_id) + sizeof(msg.resp_id));    
    else if (name == "result")
     written += write_bytes(&strct,((uint8_t*)&msg.result),((uint8_t*)&msg.result) + sizeof(msg.result));    
  }
  return written;
} 


size_t write(std::vector<ceps::ast::Nodebase_ptr> const & v, cm_atten_profile_ind_t& msg, size_t size){
  size_t written = 0;
  for(auto e : v){
    if (!ceps::ast::is<ceps::ast::Ast_node_kind::structdef>(e)) continue;
    auto& strct = *ceps::ast::as_struct_ptr(e);
    auto& name = ceps::ast::name(strct);

    if (name == "pev_mac") 
     written += write_bytes(&strct,((uint8_t*)&msg.pev_mac),((uint8_t*)&msg.pev_mac) + sizeof(msg.pev_mac));
    else if (name == "num_groups")
     written += write_bytes(&strct,((uint8_t*)&msg.num_groups),((uint8_t*)&msg.num_groups) + sizeof(msg.num_groups));
    else if (name == "aag")
     written += write_bytes(&strct,((uint8_t*)&msg.aag),((uint8_t*)&msg.aag) + sizeof(msg.aag));    
  }
  return written;
} 

size_t write(std::vector<ceps::ast::Nodebase_ptr> const & v, cm_validate_req_t& msg, size_t size){
  size_t written = 0;
  for(auto e : v){
    if (!ceps::ast::is<ceps::ast::Ast_node_kind::structdef>(e)) continue;
    auto& strct = *ceps::ast::as_struct_ptr(e);
    auto& name = ceps::ast::name(strct);

    if (name == "signal_type") 
     written += write_bytes(&strct,((uint8_t*)&msg.signal_type),((uint8_t*)&msg.signal_type) + sizeof(msg.signal_type));
    else if (name == "timer")
     written += write_bytes(&strct,((uint8_t*)&msg.timer),((uint8_t*)&msg.timer) + sizeof(msg.timer));
    else if (name == "result")
     written += write_bytes(&strct,((uint8_t*)&msg.result),((uint8_t*)&msg.result) + sizeof(msg.result));    
  }
  return written;
} 


size_t write(std::vector<ceps::ast::Nodebase_ptr> const & v, cm_validate_cnf_t& msg, size_t size){
  size_t written = 0;
  for(auto e : v){
    if (!ceps::ast::is<ceps::ast::Ast_node_kind::structdef>(e)) continue;
    auto& strct = *ceps::ast::as_struct_ptr(e);
    auto& name = ceps::ast::name(strct);

    if (name == "signal_type") 
     written += write_bytes(&strct,((uint8_t*)&msg.signal_type),((uint8_t*)&msg.signal_type) + sizeof(msg.signal_type));
    else if (name == "toggle_num")
     written += write_bytes(&strct,((uint8_t*)&msg.toggle_num),((uint8_t*)&msg.toggle_num) + sizeof(msg.toggle_num));
    else if (name == "result")
     written += write_bytes(&strct,((uint8_t*)&msg.result),((uint8_t*)&msg.result) + sizeof(msg.result));    
  }
  return written;
} 


size_t write(std::vector<ceps::ast::Nodebase_ptr> const & v, cm_slac_match_req_t& msg, size_t size){
  size_t written = 0;
  for(auto e : v){
    if (!ceps::ast::is<ceps::ast::Ast_node_kind::structdef>(e)) continue;
    auto& strct = *ceps::ast::as_struct_ptr(e);
    auto& name = ceps::ast::name(strct);

    if (name == "application_type") 
     written += write_bytes(&strct,((uint8_t*)&msg.application_type),((uint8_t*)&msg.application_type) + sizeof(msg.application_type));
    else if (name == "security_type")
     written += write_bytes(&strct,((uint8_t*)&msg.security_type),((uint8_t*)&msg.security_type) + sizeof(msg.security_type));
    else if (name == "mvflength")
     written += write_bytes(&strct,((uint8_t*)&msg.mvflength),((uint8_t*)&msg.mvflength) + sizeof(msg.mvflength));    
    else if (name == "pev_id") 
     written += write_bytes(&strct,((uint8_t*)&msg.pev_id),((uint8_t*)&msg.pev_id) + sizeof(msg.pev_id));
    else if (name == "pev_mac")
     written += write_bytes(&strct,((uint8_t*)&msg.pev_mac),((uint8_t*)&msg.pev_mac) + sizeof(msg.pev_mac));
    else if (name == "evse_id")
     written += write_bytes(&strct,((uint8_t*)&msg.evse_id),((uint8_t*)&msg.evse_id) + sizeof(msg.evse_id));    
    else if (name == "evse_mac") 
     written += write_bytes(&strct,((uint8_t*)&msg.evse_mac),((uint8_t*)&msg.evse_mac) + sizeof(msg.evse_mac));
    else if (name == "run_id")
     written += write_bytes(&strct,((uint8_t*)&msg.run_id),((uint8_t*)&msg.run_id) + sizeof(msg.run_id));
  }
  return written;
} 


size_t write(std::vector<ceps::ast::Nodebase_ptr> const & v, cm_slac_match_cnf_t& msg, size_t size){
  size_t written = 0;
  for(auto e : v){
    if (!ceps::ast::is<ceps::ast::Ast_node_kind::structdef>(e)) continue;
    auto& strct = *ceps::ast::as_struct_ptr(e);
    auto& name = ceps::ast::name(strct);

    if (name == "application_type") 
     written += write_bytes(&strct,((uint8_t*)&msg.application_type),((uint8_t*)&msg.application_type) + sizeof(msg.application_type));
    else if (name == "security_type")
     written += write_bytes(&strct,((uint8_t*)&msg.security_type),((uint8_t*)&msg.security_type) + sizeof(msg.security_type));
    else if (name == "mvflength")
     written += write_bytes(&strct,((uint8_t*)&msg.mvflength),((uint8_t*)&msg.mvflength) + sizeof(msg.mvflength));    
    else if (name == "pev_id") 
     written += write_bytes(&strct,((uint8_t*)&msg.pev_id),((uint8_t*)&msg.pev_id) + sizeof(msg.pev_id));
    else if (name == "pev_mac")
     written += write_bytes(&strct,((uint8_t*)&msg.pev_mac),((uint8_t*)&msg.pev_mac) + sizeof(msg.pev_mac));
    else if (name == "evse_id")
     written += write_bytes(&strct,((uint8_t*)&msg.evse_id),((uint8_t*)&msg.evse_id) + sizeof(msg.evse_id));    
    else if (name == "evse_mac") 
     written += write_bytes(&strct,((uint8_t*)&msg.evse_mac),((uint8_t*)&msg.evse_mac) + sizeof(msg.evse_mac));
    else if (name == "run_id")
     written += write_bytes(&strct,((uint8_t*)&msg.run_id),((uint8_t*)&msg.run_id) + sizeof(msg.run_id));
    else if (name == "nid") 
     written += write_bytes(&strct,((uint8_t*)&msg.nid),((uint8_t*)&msg.nid) + sizeof(msg.nid));
    else if (name == "nmk")
     written += write_bytes(&strct,((uint8_t*)&msg.nmk),((uint8_t*)&msg.nmk) + sizeof(msg.nmk));
  }
  return written;
} 

size_t write(std::vector<ceps::ast::Nodebase_ptr> const & v, cm_set_key_req_t& msg, size_t size){
  size_t written = 0;
  for(auto e : v){
    if (!ceps::ast::is<ceps::ast::Ast_node_kind::structdef>(e)) continue;
    auto& strct = *ceps::ast::as_struct_ptr(e);
    auto& name = ceps::ast::name(strct);

    if (name == "key_type") 
     written += write_bytes(&strct,((uint8_t*)&msg.key_type),((uint8_t*)&msg.key_type) + sizeof(msg.key_type));
    else if (name == "my_nonce")
     written += write_bytes(&strct,((uint8_t*)&msg.my_nonce),((uint8_t*)&msg.my_nonce) + sizeof(msg.my_nonce));
    else if (name == "your_nonce")
     written += write_bytes(&strct,((uint8_t*)&msg.your_nonce),((uint8_t*)&msg.your_nonce) + sizeof(msg.your_nonce));    
    else if (name == "pid") 
     written += write_bytes(&strct,((uint8_t*)&msg.pid),((uint8_t*)&msg.pid) + sizeof(msg.pid));
    else if (name == "prn")
     written += write_bytes(&strct,((uint8_t*)&msg.prn),((uint8_t*)&msg.prn) + sizeof(msg.prn));
    else if (name == "pmn")
     written += write_bytes(&strct,((uint8_t*)&msg.pmn),((uint8_t*)&msg.pmn) + sizeof(msg.pmn));    
    else if (name == "cco_capability") 
     written += write_bytes(&strct,((uint8_t*)&msg.cco_capability),((uint8_t*)&msg.cco_capability) + sizeof(msg.cco_capability));
    else if (name == "new_eks")
     written += write_bytes(&strct,((uint8_t*)&msg.new_eks),((uint8_t*)&msg.new_eks) + sizeof(msg.new_eks));
    else if (name == "nid") 
     written += write_bytes(&strct,((uint8_t*)&msg.nid),((uint8_t*)&msg.nid) + sizeof(msg.nid));
    else if (name == "new_key")
     written += write_bytes(&strct,((uint8_t*)&msg.new_key),((uint8_t*)&msg.new_key) + sizeof(msg.new_key));
  }
  return written;
} 


size_t write(std::vector<ceps::ast::Nodebase_ptr> const & v, cm_amp_map_req_t& msg, size_t size){
  size_t written = 0;
  for(auto e : v){
    if (!ceps::ast::is<ceps::ast::Ast_node_kind::structdef>(e)) continue;
    auto& strct = *ceps::ast::as_struct_ptr(e);
    auto& name = ceps::ast::name(strct);

    if (name == "amlen") 
     written += write_bytes(&strct,((uint8_t*)&msg.amlen),((uint8_t*)&msg.amlen) + sizeof(msg.amlen));
    else if (name == "amdata")
     written += write_bytes(&strct,((uint8_t*)&msg.amdata),((uint8_t*)&msg.amdata) + std::min( size - sizeof(msg.amlen), (size_t) msg.amlen  ));
  }
  return written;
} 

size_t write(std::vector<ceps::ast::Nodebase_ptr> const & v, cm_amp_map_cnf_t& msg, size_t size){
  size_t written = 0;
  for(auto e : v){
    if (!ceps::ast::is<ceps::ast::Ast_node_kind::structdef>(e)) continue;
    auto& strct = *ceps::ast::as_struct_ptr(e);
    auto& name = ceps::ast::name(strct);
    if (name == "restype") 
     written += write_bytes(&strct,((uint8_t*)&msg.restype),((uint8_t*)&msg.restype) + sizeof(msg.restype));
  }
  return written;
} 

class Ev2sctp_plugin{

    void init();

    public:
    
    ceps::parser_env::Scope scope;    
    ceps::ast::node_t associated_ceps_block = nullptr;
    Ism4ceps_plugin_interface* ceps_engine = nullptr;

    std::map<uint16_t, bool (Ev2sctp_plugin::*) (homeplug_mme_generic*, size_t mme_size) > mme_msg_to_symbol_table_setup_routines;

  bool mme_msg_cm_slac_param_req_setup_symtbl(homeplug_mme_generic*, size_t mme_size);
  bool mme_msg_cm_slac_parm_cnf_setup_symtbl(homeplug_mme_generic*, size_t mme_size);
  bool mme_msg_cm_start_atten_char_ind_setup_symtbl(homeplug_mme_generic*, size_t mme_size);
  bool mme_msg_cm_mnbc_sound_ind_setup_symtbl(homeplug_mme_generic*, size_t mme_size);
  bool mme_msg_cm_atten_char_ind_setup_symtbl(homeplug_mme_generic*, size_t mme_size);
  bool mme_msg_cm_atten_profile_ind_setup_symtbl(homeplug_mme_generic*, size_t mme_size);
  bool mme_msg_cm_atten_char_rsp_setup_symtbl(homeplug_mme_generic*, size_t mme_size);
  bool mme_msg_cm_validate_req_setup_symtbl(homeplug_mme_generic*, size_t mme_size);
  bool mme_msg_cm_validate_cnf_setup_symtbl(homeplug_mme_generic*, size_t mme_size);
  bool mme_msg_cm_slac_match_req_setup_symtbl(homeplug_mme_generic*, size_t mme_size);
  bool mme_msg_cm_slac_match_cnf_setup_symtbl(homeplug_mme_generic*, size_t mme_size);
  bool mme_msg_cm_set_key_req_setup_symtbl(homeplug_mme_generic*, size_t mme_size);
  bool mme_msg_cm_set_key_cnf_setup_symtbl(homeplug_mme_generic*, size_t mme_size);
  bool mme_msg_cm_amp_map_req_setup_symtbl(homeplug_mme_generic*, size_t mme_size);
  bool mme_msg_cm_amp_map_cnf_setup_symtbl(homeplug_mme_generic*, size_t mme_size);

    /**
     * The plugin is associated with a function name in the ceps context, each time this function gets called the plugin will be reinitialized using
     * the single argument of the call which is called associated ceps block. 
     ***/
    void set_associated_ceps_block(ceps::ast::node_t v){
      auto t = associated_ceps_block;
      associated_ceps_block = v;
      init();
      gc(t);
    }

    void handle_homeplug_mme(homeplug_mme_generic*, size_t mme_size);
};

  bool Ev2sctp_plugin::mme_msg_cm_slac_param_req_setup_symtbl(homeplug_mme_generic* msg, size_t mme_size){
    cm_slac_parm_req_t slac_parm_req = msg->mmdata.cm_slac_parm_req;
    ceps::interpreter::set_val("mme_type",mme::CM_SLAC_PARM_REQ,scope);
    ceps::interpreter::set_val("mme_application_type",slac_parm_req.application_type,scope);
    ceps::interpreter::set_val("mme_security_type",slac_parm_req.security_type,scope);
    ceps::interpreter::set_val("mme_run_id",slac_parm_req.run_id,slac_parm_req.run_id+sizeof(slac_parm_req.run_id),scope);
    return true;
  }

  bool Ev2sctp_plugin::mme_msg_cm_slac_parm_cnf_setup_symtbl(homeplug_mme_generic* msg, size_t mme_size){
    cm_slac_parm_cnf_t slac_parm_cnf = msg->mmdata.cm_slac_parm_cnf;
    ceps::interpreter::set_val("mme_type",mme::CM_SLAC_PARM_CNF,scope);
    ceps::interpreter::set_val("mme_application_type",slac_parm_cnf.application_type,scope);
    ceps::interpreter::set_val("mme_security_type",slac_parm_cnf.security_type,scope);
    ceps::interpreter::set_val("mme_run_id",slac_parm_cnf.run_id,slac_parm_cnf.run_id+sizeof(slac_parm_cnf.run_id),scope);  
    ceps::interpreter::set_val("mme_m_sound_target",slac_parm_cnf.m_sound_target,slac_parm_cnf.m_sound_target+sizeof(slac_parm_cnf.m_sound_target),scope);  
    ceps::interpreter::set_val("mme_num_sounds",slac_parm_cnf.num_sounds,scope);
    ceps::interpreter::set_val("mme_time_out",slac_parm_cnf.time_out,scope);
    ceps::interpreter::set_val("mme_resp_type",slac_parm_cnf.resp_type,scope);
    ceps::interpreter::set_val("mme_forwarding_sta",slac_parm_cnf.forwarding_sta,slac_parm_cnf.forwarding_sta+sizeof(slac_parm_cnf.forwarding_sta),scope);  
    return true;
  }

  bool Ev2sctp_plugin::mme_msg_cm_start_atten_char_ind_setup_symtbl(homeplug_mme_generic* msg, size_t mme_size){
    cm_start_atten_char_ind_t cm_start_atten_char_ind = msg->mmdata.cm_start_atten_char_ind;
    ceps::interpreter::set_val("mme_type",mme::CM_START_ATTEN_CHAR_IND,scope);
    ceps::interpreter::set_val("mme_application_type",cm_start_atten_char_ind.application_type,scope);
    ceps::interpreter::set_val("mme_security_type",cm_start_atten_char_ind.security_type,scope);
    ceps::interpreter::set_val("mme_run_id",cm_start_atten_char_ind.run_id,cm_start_atten_char_ind.run_id+sizeof(cm_start_atten_char_ind.run_id),scope);  
    ceps::interpreter::set_val("mme_num_sounds",cm_start_atten_char_ind.num_sounds,scope);  
    ceps::interpreter::set_val("mme_time_out",cm_start_atten_char_ind.time_out,scope);
    ceps::interpreter::set_val("mme_resp_type",cm_start_atten_char_ind.resp_type,scope);
    ceps::interpreter::set_val("mme_forwarding_sta",cm_start_atten_char_ind.forwarding_sta,cm_start_atten_char_ind.forwarding_sta+sizeof(cm_start_atten_char_ind.forwarding_sta),scope);  
    return true;
  }

  bool Ev2sctp_plugin::mme_msg_cm_mnbc_sound_ind_setup_symtbl(homeplug_mme_generic* msg, size_t mme_size){
    cm_mnbc_sound_ind_t cm_mnbc_sound_ind = msg->mmdata.cm_mnbc_sound_ind;
    ceps::interpreter::set_val("mme_type",mme::CM_MNBC_SOUND_IND,scope);
    ceps::interpreter::set_val("mme_application_type",cm_mnbc_sound_ind.application_type,scope);
    ceps::interpreter::set_val("mme_security_type",cm_mnbc_sound_ind.security_type,scope);
    ceps::interpreter::set_val("mme_run_id",cm_mnbc_sound_ind.run_id,cm_mnbc_sound_ind.run_id+sizeof(cm_mnbc_sound_ind.run_id),scope);  
    ceps::interpreter::set_val("mme_sender_id",cm_mnbc_sound_ind.sender_id,cm_mnbc_sound_ind.sender_id+sizeof(cm_mnbc_sound_ind.sender_id),scope);  
    ceps::interpreter::set_val("mme_cnt",cm_mnbc_sound_ind.cnt,scope);  
    ceps::interpreter::set_val("mme_rnd",cm_mnbc_sound_ind.rnd,cm_mnbc_sound_ind.rnd+sizeof(cm_mnbc_sound_ind.rnd),scope);
    return true;
  }

  bool Ev2sctp_plugin::mme_msg_cm_atten_char_ind_setup_symtbl(homeplug_mme_generic* msg, size_t mme_size){
    cm_atten_char_ind_t cm_atten_char_ind = msg->mmdata.cm_atten_char_ind;
    ceps::interpreter::set_val("mme_type",mme::CM_ATTEN_CHAR_IND,scope);
    ceps::interpreter::set_val("mme_application_type",cm_atten_char_ind.application_type,scope);
    ceps::interpreter::set_val("mme_security_type",cm_atten_char_ind.security_type,scope);
    ceps::interpreter::set_val("mme_run_id",cm_atten_char_ind.run_id,cm_atten_char_ind.run_id+sizeof(cm_atten_char_ind.run_id),scope);
    ceps::interpreter::set_val("mme_source_address",cm_atten_char_ind.source_address,cm_atten_char_ind.source_address+sizeof(cm_atten_char_ind.source_address),scope);  
    ceps::interpreter::set_val("mme_source_id",cm_atten_char_ind.source_id,cm_atten_char_ind.source_id+sizeof(cm_atten_char_ind.source_id),scope);  
    ceps::interpreter::set_val("mme_resp_id",cm_atten_char_ind.resp_id,cm_atten_char_ind.resp_id+sizeof(cm_atten_char_ind.resp_id),scope);  
    ceps::interpreter::set_val("mme_atten_profile",cm_atten_char_ind.atten_profile,cm_atten_char_ind.atten_profile+sizeof(cm_atten_char_ind.atten_profile),scope);  
    ceps::interpreter::set_val("mme_num_sounds",cm_atten_char_ind.num_sounds,scope);
    return true;
  }

  bool Ev2sctp_plugin::mme_msg_cm_atten_char_rsp_setup_symtbl(homeplug_mme_generic* msg, size_t mme_size){
    cm_atten_char_rsp_t cm_atten_char_rsp = msg->mmdata.cm_atten_char_rsp;
    ceps::interpreter::set_val("mme_type",mme::CM_ATTEN_CHAR_RSP,scope);
    ceps::interpreter::set_val("mme_application_type",cm_atten_char_rsp.application_type,scope);
    ceps::interpreter::set_val("mme_security_type",cm_atten_char_rsp.security_type,scope);
    ceps::interpreter::set_val("mme_run_id",cm_atten_char_rsp.run_id,cm_atten_char_rsp.run_id+sizeof(cm_atten_char_rsp.run_id),scope);
    ceps::interpreter::set_val("mme_source_address",cm_atten_char_rsp.source_address,cm_atten_char_rsp.source_address+sizeof(cm_atten_char_rsp.source_address),scope);  
    ceps::interpreter::set_val("mme_source_id",cm_atten_char_rsp.source_id,cm_atten_char_rsp.source_id+sizeof(cm_atten_char_rsp.source_id),scope);  
    ceps::interpreter::set_val("mme_resp_id",cm_atten_char_rsp.resp_id,cm_atten_char_rsp.resp_id+sizeof(cm_atten_char_rsp.resp_id),scope);  
    ceps::interpreter::set_val("mme_result",cm_atten_char_rsp.result,scope);
    return true;
  }

  bool Ev2sctp_plugin::mme_msg_cm_atten_profile_ind_setup_symtbl(homeplug_mme_generic* msg, size_t mme_size){
    cm_atten_profile_ind_t cm_atten_profile_ind = msg->mmdata.cm_atten_profile_ind;
    ceps::interpreter::set_val("mme_type",mme::CM_ATTEN_PROFILE_IND,scope);
    ceps::interpreter::set_val("mme_pev_mac",cm_atten_profile_ind.pev_mac,cm_atten_profile_ind.pev_mac+sizeof(cm_atten_profile_ind.pev_mac),scope);  
    ceps::interpreter::set_val("mme_num_groups",cm_atten_profile_ind.num_groups,scope);
    ceps::interpreter::set_val("mme_aag",cm_atten_profile_ind.aag,cm_atten_profile_ind.aag+sizeof(cm_atten_profile_ind.aag),scope);      
    return true;
  }

  bool Ev2sctp_plugin::mme_msg_cm_validate_req_setup_symtbl(homeplug_mme_generic* msg, size_t mme_size){
    cm_validate_req_t cm_validate_req = msg->mmdata.cm_validate_req;
    ceps::interpreter::set_val("mme_type",mme::CM_VALIDATE_REQ,scope);
    ceps::interpreter::set_val("mme_signal_type",cm_validate_req.signal_type,scope);  
    ceps::interpreter::set_val("mme_timer",cm_validate_req.timer,scope);  
    ceps::interpreter::set_val("mme_result",cm_validate_req.result,scope);  
    return true;
  }

  bool Ev2sctp_plugin::mme_msg_cm_validate_cnf_setup_symtbl(homeplug_mme_generic* msg, size_t mme_size){
    cm_validate_cnf_t cm_validate_cnf = msg->mmdata.cm_validate_cnf;
    ceps::interpreter::set_val("mme_type",mme::CM_VALIDATE_CNF,scope);
    ceps::interpreter::set_val("mme_signal_type",cm_validate_cnf.signal_type,scope);  
    ceps::interpreter::set_val("mme_toggle_num",cm_validate_cnf.toggle_num,scope);  
    ceps::interpreter::set_val("mme_result",cm_validate_cnf.result,scope);  
    return true;
  }

  bool Ev2sctp_plugin::mme_msg_cm_slac_match_req_setup_symtbl(homeplug_mme_generic* msg, size_t mme_size){
    cm_slac_match_req_t cm_slac_match_req = msg->mmdata.cm_slac_match_req;
    ceps::interpreter::set_val("mme_type",mme::CM_SLAC_MATCH_REQ,scope);
    ceps::interpreter::set_val("mme_application_type",cm_slac_match_req.application_type,scope);
    ceps::interpreter::set_val("mme_security_type",cm_slac_match_req.security_type,scope);
    ceps::interpreter::set_val("mme_run_id",cm_slac_match_req.run_id,cm_slac_match_req.run_id+sizeof(cm_slac_match_req.run_id),scope);
    ceps::interpreter::set_val("mme_mvflength",cm_slac_match_req.mvflength,scope);
    ceps::interpreter::set_val("mme_pev_id",cm_slac_match_req.pev_id,cm_slac_match_req.pev_id+sizeof(cm_slac_match_req.pev_id),scope);
    ceps::interpreter::set_val("mme_pev_mac",cm_slac_match_req.pev_mac,cm_slac_match_req.pev_mac+sizeof(cm_slac_match_req.pev_mac),scope);
    ceps::interpreter::set_val("mme_evse_id",cm_slac_match_req.evse_id,cm_slac_match_req.evse_id+sizeof(cm_slac_match_req.evse_id),scope);
    ceps::interpreter::set_val("mme_evse_mac",cm_slac_match_req.evse_mac,cm_slac_match_req.evse_mac+sizeof(cm_slac_match_req.evse_mac),scope);
    return true;
  }
  bool Ev2sctp_plugin::mme_msg_cm_slac_match_cnf_setup_symtbl(homeplug_mme_generic* msg, size_t mme_size){
    cm_slac_match_cnf_t cm_slac_match_cnf = msg->mmdata.cm_slac_match_cnf;
    ceps::interpreter::set_val("mme_type",mme::CM_SLAC_MATCH_CNF,scope);
    ceps::interpreter::set_val("mme_application_type",cm_slac_match_cnf.application_type,scope);
    ceps::interpreter::set_val("mme_security_type",cm_slac_match_cnf.security_type,scope);
    ceps::interpreter::set_val("mme_run_id",cm_slac_match_cnf.run_id,cm_slac_match_cnf.run_id+sizeof(cm_slac_match_cnf.run_id),scope);
    ceps::interpreter::set_val("mme_mvflength",cm_slac_match_cnf.mvflength,scope);
    ceps::interpreter::set_val("mme_pev_id",cm_slac_match_cnf.pev_id,cm_slac_match_cnf.pev_id+sizeof(cm_slac_match_cnf.pev_id),scope);
    ceps::interpreter::set_val("mme_pev_mac",cm_slac_match_cnf.pev_mac,cm_slac_match_cnf.pev_mac+sizeof(cm_slac_match_cnf.pev_mac),scope);
    ceps::interpreter::set_val("mme_evse_id",cm_slac_match_cnf.evse_id,cm_slac_match_cnf.evse_id+sizeof(cm_slac_match_cnf.evse_id),scope);
    ceps::interpreter::set_val("mme_evse_mac",cm_slac_match_cnf.evse_mac,cm_slac_match_cnf.evse_mac+sizeof(cm_slac_match_cnf.evse_mac),scope);
    ceps::interpreter::set_val("mme_nid",cm_slac_match_cnf.nid,cm_slac_match_cnf.nid+sizeof(cm_slac_match_cnf.nid),scope);
    ceps::interpreter::set_val("mme_nmk",cm_slac_match_cnf.nmk,cm_slac_match_cnf.nmk+sizeof(cm_slac_match_cnf.nmk),scope);
    return true;
  }
  bool Ev2sctp_plugin::mme_msg_cm_set_key_req_setup_symtbl(homeplug_mme_generic* msg, size_t mme_size){
    cm_set_key_req_t cm_set_key_req = msg->mmdata.cm_set_key_req;
    ceps::interpreter::set_val("mme_type",mme::CM_SET_KEY_REQ,scope);
    ceps::interpreter::set_val("mme_key_type",cm_set_key_req.key_type,scope);
    ceps::interpreter::set_val("mme_my_nonce",cm_set_key_req.my_nonce,scope);
    ceps::interpreter::set_val("mme_your_nonce",cm_set_key_req.your_nonce,scope);
    ceps::interpreter::set_val("mme_pid",cm_set_key_req.pid,scope);
    ceps::interpreter::set_val("mme_prn",cm_set_key_req.prn,scope);
    ceps::interpreter::set_val("mme_pmn",cm_set_key_req.pmn,scope);
    ceps::interpreter::set_val("mme_cco_capability",cm_set_key_req.cco_capability,scope);
    ceps::interpreter::set_val("mme_nid",cm_set_key_req.nid,cm_set_key_req.nid+sizeof(cm_set_key_req.nid),scope);
    ceps::interpreter::set_val("mme_new_eks",cm_set_key_req.new_eks,scope);
    ceps::interpreter::set_val("mme_new_key",cm_set_key_req.new_key,cm_set_key_req.new_key+sizeof(cm_set_key_req.new_key),scope);
    return true;
  }

  bool Ev2sctp_plugin::mme_msg_cm_set_key_cnf_setup_symtbl(homeplug_mme_generic*, size_t mme_size){
    return true;
  }
  bool Ev2sctp_plugin::mme_msg_cm_amp_map_req_setup_symtbl(homeplug_mme_generic* msg, size_t mme_size){
    cm_amp_map_req_t cm_amp_map_req = msg->mmdata.cm_amp_map_req;
    ceps::interpreter::set_val("mme_type",mme::CM_AMP_MAP_REQ,scope);
    ceps::interpreter::set_val("mme_amlen",cm_amp_map_req.amlen,scope);
    ceps::interpreter::set_val("mme_amdata",cm_amp_map_req.amdata,cm_amp_map_req.amdata+std::min((size_t)cm_amp_map_req.amlen, 
               mme_size -  sizeof(cm_amp_map_cnf_t) + 1) ,scope);


    return true;
  }
  bool Ev2sctp_plugin::mme_msg_cm_amp_map_cnf_setup_symtbl(homeplug_mme_generic* msg, size_t mme_size){
    cm_amp_map_cnf_t cm_amp_map_cnf = msg->mmdata.cm_amp_map_cnf;
    ceps::interpreter::set_val("mme_type",mme::CM_AMP_MAP_CNF,scope);
    ceps::interpreter::set_val("mme_restype",cm_amp_map_cnf.restype,scope);

    return true;
  }


void Ev2sctp_plugin::init(){
  mme_msg_to_symbol_table_setup_routines = 
                  { 
                      {mme::CM_SLAC_PARM_REQ,&Ev2sctp_plugin::mme_msg_cm_slac_param_req_setup_symtbl},
                      {mme::CM_SLAC_PARM_CNF,&Ev2sctp_plugin::mme_msg_cm_slac_parm_cnf_setup_symtbl},
                      {mme::CM_START_ATTEN_CHAR_IND,&Ev2sctp_plugin::mme_msg_cm_start_atten_char_ind_setup_symtbl},
                      {mme::CM_MNBC_SOUND_IND,&Ev2sctp_plugin::mme_msg_cm_mnbc_sound_ind_setup_symtbl},
                      {mme::CM_ATTEN_CHAR_IND,&Ev2sctp_plugin::mme_msg_cm_atten_char_ind_setup_symtbl},
                      {mme::CM_ATTEN_PROFILE_IND,&Ev2sctp_plugin::mme_msg_cm_atten_profile_ind_setup_symtbl},
                      {mme::CM_ATTEN_CHAR_RSP,&Ev2sctp_plugin::mme_msg_cm_atten_char_rsp_setup_symtbl},
                      {mme::CM_VALIDATE_REQ,&Ev2sctp_plugin::mme_msg_cm_validate_req_setup_symtbl},
                      {mme::CM_VALIDATE_CNF,&Ev2sctp_plugin::mme_msg_cm_validate_cnf_setup_symtbl},
                      {mme::CM_SLAC_MATCH_REQ,&Ev2sctp_plugin::mme_msg_cm_slac_match_req_setup_symtbl},
                      {mme::CM_SLAC_MATCH_CNF,&Ev2sctp_plugin::mme_msg_cm_slac_match_cnf_setup_symtbl},
                      {mme::CM_SET_KEY_REQ,&Ev2sctp_plugin::mme_msg_cm_set_key_req_setup_symtbl},
                      {mme::CM_SET_KEY_CNF,&Ev2sctp_plugin::mme_msg_cm_set_key_cnf_setup_symtbl},
                      {mme::CM_AMP_MAP_REQ,&Ev2sctp_plugin::mme_msg_cm_amp_map_req_setup_symtbl},
                      {mme::CM_AMP_MAP_CNF,&Ev2sctp_plugin::mme_msg_cm_amp_map_cnf_setup_symtbl} 
                    };
}

void Ev2sctp_plugin::handle_homeplug_mme(homeplug_mme_generic* msg, size_t mme_size){
  auto it = mme_msg_to_symbol_table_setup_routines.find(msg->mmtype);
  if (it == mme_msg_to_symbol_table_setup_routines.end()) return;
  if (associated_ceps_block == nullptr) return;
  if (!(this->*(it->second))(msg, mme_size)) return;
  auto ceps_fragment = static_cast<ceps::ast::Nodebase_ptr>(plugin_master->evaluate_fragment_in_global_context(associated_ceps_block,&scope));
  if (!ceps_fragment) return;
  ceps_fragment = ceps_fragment->clone();

  
  //std::cout << *ceps_fragment << std::endl<< std::endl<< std::endl;

  ceps::ast::Nodeset ns{ceps_fragment};
  auto on_message = ns["setup"]["on_message"];

  for(auto e : on_message.nodes()) {
    ceps::ast::function_target_t func_id; 
    ceps::ast::nodes_t args;
    if (is_a_funccall(e,func_id,args)){
				ceps_engine->queue_internal_event(func_id,args);
    }
  }

  gc(ceps_fragment);
}


static Ev2sctp_plugin plugn;

namespace tests{
 void run_all();
 void cm_slac_param_req();
 void cm_slac_param_cnf();
 void cm_start_atten_char_ind();
 void cm_mnbc_sound_ind();
 void cm_atten_char_ind();
 void cm_atten_profile_ind();
 void cm_atten_char_rsp();
 void cm_validate_req();
 void cm_validate_cnf();
 void cm_slac_match_req();
 void cm_slac_match_cnf();
 void cm_set_key_req();
 void cm_set_key_cnf();
 void cm_amp_map_req();
 void cm_amp_map_cnf();
} 

static ceps::ast::node_t ev2sctp_plugin(ceps::ast::node_callparameters_t params){
    auto t = get_first_child(params); // static_cast<ceps::ast::Nodebase_ptr>(plugin_master->evaluate_fragment_in_global_context(params->children()[0],&plugn.scope));
    if (t) plugn.set_associated_ceps_block(t->clone());
    tests::run_all();
    return nullptr;
}


std::ostream& operator << (std::ostream & os, homeplug_mme_generic const & mme_msg){
  os << "mme header:\n";
  os << "\t";
  os << "osa :";
  for(size_t i = 0; i < sizeof(mme_msg.osa);++i){
    os << " " << (int)mme_msg.osa[i];
  }
  os << "\n";
  os << "\t";
  os << "oda :";
  for(size_t i = 0; i < sizeof(mme_msg.oda);++i){
    os << " " << (int)mme_msg.oda[i];
  }
  os << "\n";
  os << "\t";os << "mmtype: " <<(int) mme_msg.mmtype << "\n";
  os << "\t";os << "mtype: " << (int) mme_msg.mtype << "\n";
  os << "\t";os << "mmv: " << (int) mme_msg.mmv << "\n";
  os << "\t";os << "vlan_tag: " << (int) mme_msg.vlan_tag << "\n";
  os << "\t";os << "fmi: " << (int) mme_msg.fmi << "\n";
  os << "\t";os << "fmsn: " << (int) mme_msg.fmsn << "\n";
  os << "mme data:\n";
  if (mme_msg.mmtype == mme::CM_SLAC_PARM_REQ){
    os << "\t";os << "application_type: " <<(int) mme_msg.mmdata.cm_slac_parm_req.application_type << "\n";
    os << "\t";os << "security_type: " <<(int) mme_msg.mmdata.cm_slac_parm_req.security_type << "\n";
    os << "\t";os << "run_id: " ;
    for(size_t i = 0; i < sizeof(mme_msg.mmdata.cm_slac_parm_req.run_id);++i){
      os << " " << (int)mme_msg.mmdata.cm_slac_parm_req.run_id[i];
    }
    os << "\n";
  }

  return os;
}

static ceps::ast::node_t ev2sctp_send_mme(ceps::ast::node_callparameters_t params){
    //std::cout << "******** SEND MME 1" << std::endl;
    auto msg = get_first_child(params); // static_cast<ceps::ast::Nodebase_ptr>(plugin_master->evaluate_fragment_in_global_context(params->children()[0],&plugn.scope));
    //std::cout << *t << std::endl;
    auto ns = ceps::ast::Nodeset{msg}["mme"];
    auto header = ns["header"];
    auto mmtype = header["mmtype"];    
    auto fmi = header["fmi"].as_int_noexcept();
    auto fmsn = header["fmsn"].as_int_noexcept();
    auto mmv = header["mmv"].as_int_noexcept();
    auto mtype = header["mtype"].as_int_noexcept();
    auto oda = header["oda"];
    auto osa = header["osa"];
    auto vlan_tag = header["vlan_tag"].as_int_noexcept();

    auto payload = ns["payload"];
    
    auto mme_type = mmtype.as_int_noexcept();
    
    if (!mme_type.has_value()) return nullptr;
    char mme_msg_buffer[sizeof(homeplug_mme_generic)*2] = {0};
    homeplug_mme_generic& mme_msg = *((homeplug_mme_generic*)mme_msg_buffer);
    homeplug_mme_generic mme_msg2;

    mme_msg.mmtype = mme_type.value();
    if(fmi.has_value()) mme_msg.fmi = fmi.value();
    if(fmsn.has_value()) mme_msg.fmsn = fmsn.value();
    if(mmv.has_value()) mme_msg.mmv = mmv.value();
    if(mtype.has_value()) mme_msg.mtype = mtype.value();

    if(!osa.empty()) write_bytes(osa.nodes(), ((uint8_t*) &mme_msg.osa), ((uint8_t*) &mme_msg.osa) + sizeof(mme_msg.osa));
    if(!oda.empty()) write_bytes(oda.nodes(), ((uint8_t*) &mme_msg.oda), ((uint8_t*) &mme_msg.oda) + sizeof(mme_msg.oda));
    if(vlan_tag.has_value()) mme_msg.vlan_tag = vlan_tag.value();





    if (mme_msg.mmtype == mme::CM_SLAC_PARM_REQ)
     write(payload.nodes(), mme_msg.mmdata.cm_slac_parm_req, sizeof(mme_msg.mmdata.cm_slac_parm_req));
    else if (mme_msg.mmtype == mme::CM_SLAC_PARM_CNF)
     write(payload.nodes(), mme_msg.mmdata.cm_slac_parm_cnf, sizeof(mme_msg.mmdata.cm_slac_parm_cnf));
    else if (mme_msg.mmtype == mme::CM_START_ATTEN_CHAR_IND)
     write(payload.nodes(), mme_msg.mmdata.cm_start_atten_char_ind, sizeof(mme_msg.mmdata.cm_start_atten_char_ind));
    else if (mme_msg.mmtype == mme::CM_MNBC_SOUND_IND)
     write(payload.nodes(), mme_msg.mmdata.cm_mnbc_sound_ind, sizeof(mme_msg.mmdata.cm_mnbc_sound_ind));
    else if (mme_msg.mmtype == mme::CM_START_ATTEN_CHAR_IND)
     write(payload.nodes(), mme_msg.mmdata.cm_atten_char_ind, sizeof(mme_msg.mmdata.cm_atten_char_ind));
    else if (mme_msg.mmtype == mme::CM_ATTEN_CHAR_RSP)
     write(payload.nodes(), mme_msg.mmdata.cm_atten_char_rsp, sizeof(mme_msg.mmdata.cm_atten_char_rsp));
    else if (mme_msg.mmtype == mme::CM_ATTEN_PROFILE_IND)
     write(payload.nodes(), mme_msg.mmdata.cm_atten_profile_ind, sizeof(mme_msg.mmdata.cm_atten_profile_ind));
    else if (mme_msg.mmtype == mme::CM_VALIDATE_REQ)
     write(payload.nodes(), mme_msg.mmdata.cm_validate_req, sizeof(mme_msg.mmdata.cm_validate_req));
    else if (mme_msg.mmtype == mme::CM_VALIDATE_CNF)
     write(payload.nodes(), mme_msg.mmdata.cm_validate_cnf, sizeof(mme_msg.mmdata.cm_validate_cnf));
    else if (mme_msg.mmtype == mme::CM_SLAC_MATCH_REQ)
     write(payload.nodes(), mme_msg.mmdata.cm_slac_match_req, sizeof(mme_msg.mmdata.cm_slac_match_req));
    else if (mme_msg.mmtype == mme::CM_SLAC_MATCH_CNF)
     write(payload.nodes(), mme_msg.mmdata.cm_slac_match_cnf, sizeof(mme_msg.mmdata.cm_slac_match_cnf));
    else if (mme_msg.mmtype == mme::CM_AMP_MAP_REQ)
     write(payload.nodes(), mme_msg.mmdata.cm_amp_map_req, sizeof(mme_msg.mmdata.cm_amp_map_req));
    else if (mme_msg.mmtype == mme::CM_AMP_MAP_CNF)
     write(payload.nodes(), mme_msg.mmdata.cm_amp_map_cnf, sizeof(mme_msg.mmdata.cm_amp_map_cnf));

    std::cout << "!!!!!!!" << std::endl;
    
    std::cout << mme_msg << std::endl;

    std::cout << "!!!!!!!" << std::endl;

    //size_t write(std::vector<ceps::ast::Nodebase_ptr> const & v, cm_slac_parm_req_t& msg, size_t size);

    //std::cout << "******** SEND MME 2" << std::endl;
    return nullptr;
}

extern "C" void init_plugin(IUserdefined_function_registry* smc)
{
  plugn.ceps_engine = plugin_master = smc->get_plugin_interface(); 
  plugin_master->reg_ceps_plugin("route_events_sctp",ev2sctp_plugin);
  plugin_master->reg_ceps_plugin("send_mme",ev2sctp_send_mme);

  std::cerr << "Registered ev2sctp plugin" << std::endl;
}


/////// Tests

 void tests::run_all(){
    cm_slac_param_req();
    cm_slac_param_cnf();
    cm_start_atten_char_ind();
    cm_mnbc_sound_ind();
    cm_atten_char_ind();
    cm_atten_profile_ind();
    cm_atten_char_rsp();
    cm_validate_req();
    cm_validate_cnf();
    cm_slac_match_req();
    cm_slac_match_cnf();
    cm_set_key_req();
    cm_set_key_cnf();
    cm_amp_map_req();
    cm_amp_map_cnf();
 }

 void tests::cm_slac_param_req(){
   homeplug_mme_generic msg {.mmtype = mme::CM_SLAC_PARM_REQ};
   plugn.handle_homeplug_mme(&msg, sizeof(msg));
 }

 void tests::cm_slac_param_cnf(){
   homeplug_mme_generic msg {.mmtype = mme::CM_SLAC_PARM_CNF};
   plugn.handle_homeplug_mme(&msg, sizeof(msg));
 }

void tests::cm_start_atten_char_ind(){
   homeplug_mme_generic msg {.mmtype = mme::CM_START_ATTEN_CHAR_IND};
   plugn.handle_homeplug_mme(&msg, sizeof(msg));
}

void tests::cm_mnbc_sound_ind(){
   homeplug_mme_generic msg {.mmtype = mme::CM_MNBC_SOUND_IND};
   plugn.handle_homeplug_mme(&msg, sizeof(msg));
}

void tests::cm_atten_char_ind(){
  homeplug_mme_generic msg {.mmtype = mme::CM_ATTEN_CHAR_IND};
   plugn.handle_homeplug_mme(&msg, sizeof(msg));
}

void tests::cm_atten_profile_ind(){
  homeplug_mme_generic msg {.mmtype = mme::CM_ATTEN_PROFILE_IND};
   plugn.handle_homeplug_mme(&msg, sizeof(msg));
}

void tests::cm_atten_char_rsp(){
  homeplug_mme_generic msg {.mmtype = mme::CM_ATTEN_CHAR_RSP};
   plugn.handle_homeplug_mme(&msg, sizeof(msg));
}

void tests::cm_validate_req(){
  homeplug_mme_generic msg {.mmtype = mme::CM_VALIDATE_REQ};
   plugn.handle_homeplug_mme(&msg, sizeof(msg));
}

void tests::cm_validate_cnf(){
  homeplug_mme_generic msg {.mmtype = mme::CM_VALIDATE_CNF};
   plugn.handle_homeplug_mme(&msg, sizeof(msg));
}

void tests::cm_slac_match_req(){
  homeplug_mme_generic msg {.mmtype = mme::CM_SLAC_MATCH_REQ};
   plugn.handle_homeplug_mme(&msg, sizeof(msg));
}

void tests::cm_slac_match_cnf(){
  homeplug_mme_generic msg {.mmtype = mme::CM_SLAC_MATCH_CNF};
   plugn.handle_homeplug_mme(&msg, sizeof(msg));
}

void tests::cm_set_key_req(){
  homeplug_mme_generic msg {.mmtype = mme::CM_SET_KEY_REQ};
   plugn.handle_homeplug_mme(&msg, sizeof(msg));
}

void tests::cm_set_key_cnf(){
  homeplug_mme_generic msg {.mmtype = mme::CM_SET_KEY_CNF};
   plugn.handle_homeplug_mme(&msg, sizeof(msg));
}

void tests::cm_amp_map_req(){
  homeplug_mme_generic msg {.mmtype = mme::CM_AMP_MAP_REQ};
   plugn.handle_homeplug_mme(&msg, sizeof(msg));
}

void tests::cm_amp_map_cnf(){
  homeplug_mme_generic msg {.mmtype = mme::CM_AMP_MAP_CNF};
   plugn.handle_homeplug_mme(&msg, sizeof(msg));
}
 

