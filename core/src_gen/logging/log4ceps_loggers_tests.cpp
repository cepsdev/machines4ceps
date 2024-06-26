#include <iostream>
#include <type_traits>
#include <tuple>
#include <cstdint>
#include <memory>
#include <iterator>
#include <limits>
#include <cstdlib>
#include <time.h>
#include <sstream>
#include <vector>
#include <thread>
#include <chrono>
#include <random>
#include "log4ceps_states.hpp"
#include "log4ceps_records.hpp"
#include "log4ceps_loggers.hpp"

#ifdef _WIN32
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#else
#endif

#include "ceps_all.hh"

 

void process_ceps_file(	std::string const & file_name,
						ceps::Ceps_Environment& ceps_env,
						ceps::ast::Nodeset& universe )
{
 std::fstream ceps_file{file_name};
 if (!ceps_file) throw std::runtime_error("Couldn't open file:"+file_name);
 Ceps_parser_driver driver{ceps_env.get_global_symboltable(),ceps_file};
 ceps::Cepsparser parser{driver};
 if (parser.parse() != 0 || driver.errors_occured())
  throw std::runtime_error("Syntax error.");
 std::vector<ceps::ast::Nodebase_ptr> generated_nodes;
 ceps::interpreter::evaluate(universe,
							driver.parsetree().get_root(),
							ceps_env.get_global_symboltable(),
							ceps_env.interpreter_env(),
							&generated_nodes
							);
}//process_ceps_file




/*Start section "Log Printers"*/
namespace log4ceps_test{ namespace meta_info{
 enum base_type{Float,Double,Int,String,Dynamicbitset,Timestamp};
 using state_name=std::string;using value_types_with_units=std::vector<base_type,int/*ceps::ast::Unit_rep*/>;
 std::vector< std::pair<std::string,std::vector<std::pair<base_type,ceps::ast::Unit_rep>>>>Trace()
 {
  return   {
    std::make_pair(std::string("Timestamp"),std::vector<std::pair<base_type,ceps::ast::Unit_rep>>    {
     std::make_pair(base_type::Timestamp,ceps::ast::Unit_rep(0,0,0,0,0,0,0))    }
    ),
    std::make_pair(std::string("Current_states"),std::vector<std::pair<base_type,ceps::ast::Unit_rep>>    {
     std::make_pair(base_type::Dynamicbitset,ceps::ast::Unit_rep(0,0,0,0,0,0,0))    }
    )
  };
 }
 void log_print( log4ceps_loggers::logger_Trace_t& logger) {
  for (auto it = logger.logger().cbegin(); it != logger.logger().cend();++it)
  {
   std::cout << (*it).states() << std::endl;
  }
 }
 void log_print_ceps( log4ceps_loggers::logger_Trace_t& logger) {
  using namespace ceps::ast;
  ceps::Ceps_Environment ceps_env1;  ceps::ast::Nodeset universe1;
  try{
  process_ceps_file("mfqsmid.ceps" ,ceps_env1 ,universe1);
  } catch (std::runtime_error & re){std::cerr << "*** Error in void log_print_ceps( log4ceps_loggers::logger_Trace_t& logger): " << re.what() << std::endl;return;}
  std::map<int,ceps::ast::Nodebase_ptr> m1;
  auto mm1 = universe1[all{"map"}];
  if(mm1.size() == 0)  {std::cerr << "*** Error in void log_print_ceps( log4ceps_loggers::logger_Trace_t& logger): No map definition found.\n";return;}
  mm1 = universe1["map"];
  {auto& cont = mm1.nodes();auto& mp = m1;
  for(int i = 0; i != cont.size();++i){
   if (i + 1 == cont.size()) break;
   if (cont[i]->kind() != ceps::ast::Ast_node_kind::int_literal) continue;
   mp[ceps::ast::value(ceps::ast::as_int_ref(cont[i]))] = cont[i+1];
  }}
  for (auto it = logger.logger().cbegin(); it != logger.logger().cend();++it)
  {
   std::cout << "Trace{\n";
   // Write Timestamp
   std::cout << " Timestamp{ ";
   auto state0 = log4ceps::get_value<0>((*it).states());
   std::cout << state0 << ";" ;
   std::cout << " };\n";   // Write Current_states
   std::cout << " Current_states{ ";
   auto state1 = log4ceps::get_value<1>((*it).states());
   for(int j = 0;j != log4ceps::get_value<0>(state1).size();++j){
    auto v = log4ceps::get_value<0>(state1).get(j);
    if(!v) continue;
    if (m1.find(j) == m1.end())continue;
   std::cout << *m1[j] << ";";
   }
   std::cout << " };\n";   std::cout << "};\n";  }
 }
}}/*End section "Log Printers"*/
void test_1(){
 log4ceps_loggers::logger_Trace.logger().init(log4ceps::persistence::memory_mapped_file("test.bin", 1024, true));
 {
  using namespace log4ceps_test::meta_info;  using namespace log4ceps_loggers;
  using namespace log4ceps_states;
  log4ceps::get_value<0>(Current_states).set(1);
  log4ceps_loggers::log_event(log4ceps_events::Step(), log4ceps_loggers::logger_Trace);
  log4ceps::get_value<0>(Current_states).set(2);
  log4ceps_loggers::log_event(log4ceps_events::Step(), log4ceps_loggers::logger_Trace);
  log4ceps::get_value<0>(Current_states).set(3);
  log4ceps_loggers::log_event(log4ceps_events::Step(), log4ceps_loggers::logger_Trace);
  log4ceps::get_value<0>(Current_states).reset(3);
  log4ceps_loggers::log_event(log4ceps_events::Step(), log4ceps_loggers::logger_Trace);
 }
}
void run_all_tests(){
 test_1();
}
