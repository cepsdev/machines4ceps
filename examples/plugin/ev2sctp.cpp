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
  uint16_t constexpr CM_ATTEN_CHAR_IND = 6;
  uint16_t constexpr CM_ATTEN_PROFILE_IND = 7;
  uint16_t constexpr CM_ATTEN_CHAR_RSP = 8;
  uint16_t constexpr CM_VALIDATE_REQ = 9;
  uint16_t constexpr CM_VALIDATE_CNF = 10;
  uint16_t constexpr CM_SLAC_MATCH_REQ = 11;
  uint16_t constexpr CM_SLAC_MATCH_CNF = 12;
  uint16_t constexpr CM_SET_KEY_REQ = 13;
  uint16_t constexpr CM_SET_KEY_CNF = 14;
  uint16_t constexpr CM_AMP_MAP_REQ = 15;
  uint16_t constexpr CM_AMP_MAP_CNF = 16;
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
      uint16_t nmk;
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
    struct{} cm_set_key_cnf;
  } mmdata;
};

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
    ceps::interpreter::set_val("mme_pev_mac",cm_atten_profile_ind.aag,cm_atten_profile_ind.aag+sizeof(cm_atten_profile_ind.aag),scope);  
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

  bool Ev2sctp_plugin::mme_msg_cm_slac_match_req_setup_symtbl(homeplug_mme_generic*, size_t mme_size){
    return true;
  }
  bool Ev2sctp_plugin::mme_msg_cm_slac_match_cnf_setup_symtbl(homeplug_mme_generic*, size_t mme_size){
    return true;
  }
  bool Ev2sctp_plugin::mme_msg_cm_set_key_req_setup_symtbl(homeplug_mme_generic*, size_t mme_size){
    return true;
  }
  bool Ev2sctp_plugin::mme_msg_cm_set_key_cnf_setup_symtbl(homeplug_mme_generic*, size_t mme_size){
    return true;
  }
  bool Ev2sctp_plugin::mme_msg_cm_amp_map_req_setup_symtbl(homeplug_mme_generic*, size_t mme_size){
    return true;
  }
  bool Ev2sctp_plugin::mme_msg_cm_amp_map_cnf_setup_symtbl(homeplug_mme_generic*, size_t mme_size){
    return true;
  }


void Ev2sctp_plugin::init(){
  mme_msg_to_symbol_table_setup_routines = { {mme::CM_SLAC_PARM_REQ,&Ev2sctp_plugin::mme_msg_cm_slac_param_req_setup_symtbl},
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
                      {mme::CM_AMP_MAP_CNF,&Ev2sctp_plugin::mme_msg_cm_amp_map_cnf_setup_symtbl} };
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
} 

static ceps::ast::node_t ev2sctp_plugin(ceps::ast::node_callparameters_t params){
    auto t = get_first_child(params); // static_cast<ceps::ast::Nodebase_ptr>(plugin_master->evaluate_fragment_in_global_context(params->children()[0],&plugn.scope));
    if (t) plugn.set_associated_ceps_block(t->clone());
    tests::run_all();
    return nullptr;
}

extern "C" void init_plugin(IUserdefined_function_registry* smc)
{
  plugn.ceps_engine = plugin_master = smc->get_plugin_interface(); 
  (plugin_master = smc->get_plugin_interface())->reg_ceps_plugin("route_events_sctp",ev2sctp_plugin);
  std::cerr << "Registered ev2sctp plugin" << std::endl;
}


/////// Tests

 void tests::run_all(){
   cm_slac_param_req();
   cm_slac_param_cnf();
   cm_start_atten_char_ind();
   cm_mnbc_sound_ind();

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

 

