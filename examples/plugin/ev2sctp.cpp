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
  template <typename T> bool set_val(void*& payload, T val);
  template <typename T> bool set_val(std::string name,T val,scope_ref_t scope){
    auto sym = scope.insert("mme_type");
    sym->category = ceps::parser_env::Symbol::Category::VAR;
    return set_val(sym->payload, val);
  }
  template <> bool set_val(void*& payload, int val){
    payload = ceps::interpreter::mk_int_node(val);
    return payload;
  }
 }
}

namespace mme{
  auto constexpr CM_SLAC_PARM_REQ = 1;
  auto constexpr CM_SLAC_PARM_CNF = 2;
  auto constexpr CM_START_ATTEN_CHAR_IND = 3;
  auto constexpr CM_MNBC_SOUND_IND = 4;
  auto constexpr CM_ATTEN_PROFILE_IND = 5;
  auto constexpr CM_ATTEN_CHAR_RSP = 6;
  auto constexpr CM_VALIDATE_REQ = 7;
  auto constexpr CM_VALIDATE_CNF = 8;
  auto constexpr CM_SLAC_MATCH_REQ = 9;
  auto constexpr CM_SLAC_MATCH_CNF = 10;
  auto constexpr CM_SET_KEY_REQ = 11;
  auto constexpr CM_SET_KEY_CNF = 12;
  auto constexpr CM_AMP_MAP_REQ = 13;
  auto constexpr CM_AMP_MAP_CNF = 14;
}

struct homeplug_mme_generic{
  char oda[6];
  char osa[6];
  std::uint32_t vlan_tag;
  std::uint16_t mtype;
  std::uint8_t mmv;
  std::uint16_t mmtype;
  std::uint8_t fmi;
  std::uint8_t fmsn;
  char mmentry[];
};

struct Ev2sctp_plugin{
    
    ceps::parser_env::Scope scope;    
    ceps::ast::node_t associated_ceps_block = nullptr;
    Ism4ceps_plugin_interface* ceps_engine = nullptr;
    std::map<uint16_t, bool (Ev2sctp_plugin::*)(homeplug_mme_generic&, size_t mme_size) > mme_msg_handler;
    

    /**
     * The plugin is associated with a function name in the ceps context, each time this function gets called the plugin will be reinitialized using
     * the single argument of the call which is called associated ceps block. 
     ***/
    void set_associated_ceps_block(ceps::ast::node_t v){
      auto t = associated_ceps_block;
      associated_ceps_block = v;
      gc(t);
    }

    void handle_homeplug_mme(homeplug_mme_generic&, size_t mme_size);
};

void Ev2sctp_plugin::handle_homeplug_mme(homeplug_mme_generic&, size_t mme_size){

}

static Ev2sctp_plugin plugn;

static ceps::ast::node_t ev2sctp_plugin(ceps::ast::node_callparameters_t params){

    auto t = get_first_child(params); // static_cast<ceps::ast::Nodebase_ptr>(plugin_master->evaluate_fragment_in_global_context(params->children()[0],&plugn.scope));
    if (t) plugn.set_associated_ceps_block(t->clone());



    return nullptr;
}

extern "C" void init_plugin(IUserdefined_function_registry* smc)
{
  (plugin_master = smc->get_plugin_interface())->reg_ceps_plugin("route_events_sctp",ev2sctp_plugin);
  std::cerr << "Registered ev2sctp plugin" << std::endl;
}
