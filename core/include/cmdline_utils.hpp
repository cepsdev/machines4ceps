/*
Copyright 2014,2015,2016,2017,2018,2019,2020,2021 Tomas Prerovsky (cepsdev@hotmail.com).

Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/


#ifndef INC_CMDLINE_UTILS_HPP
#define INC_CMDLINE_UTILS_HPP


#include <string>
#include <vector>

struct Result_process_cmd_line
{
   bool valid = false;
   bool version_flag_set = false;
   bool debug_mode = false;
   bool interactive_mode = false;
   std::string out_path;
   std::vector<std::string> definition_file_rel_paths;
   std::vector<std::string> post_processing_rel_paths;
   bool start_in_server_mode = false;
   std::vector< std::pair<std::string,std::string>> remote_nodes;
   std::string server_port;
   bool quiet = false;
   bool logtrace = false;
   std::string monitor = "";
   bool run_as_monitor = false;
   std::string monitored_node_ip;
   std::string monitored_node_port;
   std::vector<std::string> plugins;
   std::string timeout;
   std::string port;
   bool print_transition_tables = false;
   bool ignore_unresolved_state_id_in_directives = false;
   bool cppgen = false;
   bool cppgen_ignore_print = false;
   bool cppgen_statemachines = false;
   bool enforce_native = false;
   bool ignore_simulations = false;
   bool print_statemachines = false;
   bool dot_gen = false;
   bool dot_gen_one_file_per_top_level_statemachine = false;
   

   // The following two flags control the console dump of the
   // evaluated and raw representation 
   bool print_evaluated_input_tree = false;
   bool print_raw_input_tree = false;
   // The format of the console dump can be altered via diverse format options
   // --format OP1 --format OP2 ... --format OPn
   // OPi can be asciidoc, ansi
   std::vector<std::string> output_format_flags;

   bool print_evaluated_postprocessing_tree = false;
   bool print_signal_generators = false;
   std::string live_log_port = "3000";
   bool live_log = false;
   bool report_format_sexpression = false;
   bool report_verbose = false;
   bool report_format_xml = false;
   bool report_format_json = false;
   bool report_format_ceps = false;
   bool report_includes_cat = false;
   bool dump_asciidoc_can_layer = false;
   bool no_warn = false;
   bool print_event_signatures = false;
   std::string package_file;
   bool ws_api_on = false;
   std::string ws_api_port = "8181";
   bool vcan_api_on = false;
   std::string vcan_api_port = "8182";
   bool dump_stddoc_canlayer = false;
   bool stddoc_canlayer_no_header_no_footer = true;
   bool no_file_output = false;
   bool start_paused = false;
   bool sleep_before_ws_api_on = false;
   bool print_help = false;
   std::string sleep_before_ws_delta_ms;
   std::string push_dir;
   bool create_plugin_project = false;
   Result_process_cmd_line() = default;
   Result_process_cmd_line(bool version_flag_set_,
			bool debug_mode_,
			bool interactive_mode_,
			std::string out_path_,
			std::vector<std::string> definition_file_rel_paths_)
   :version_flag_set(version_flag_set_),
     debug_mode(debug_mode_),
     interactive_mode(interactive_mode_) ,
     out_path(out_path_),
     definition_file_rel_paths(definition_file_rel_paths_)
   {}
};

Result_process_cmd_line process_cmd_line(int argc,char ** argv, Result_process_cmd_line r = Result_process_cmd_line{});

std::string get_rel_out_path(std::string const & def_file);

void mk_directories(std::string const & path);

extern bool print_header_in_sql_files;
extern std::string& get_report_out_dir();
extern bool DEBUG_MODE;

inline bool generate_report() {return get_report_out_dir().length() > 0;}

const auto WARN_XML_PROPERTIES_MISSING_PREFIX_DEFINITION = 100;
const auto WARN_CANNOT_FIND_TEMPLATE = 101;
const auto WARN_NO_INVOCATION_MAPPING_AND_NO_TABLE_DEF_FOUND = 103;
const auto WARN_TESTCASE_ID_ALREADY_DEFINED = 104;
const auto WARN_TESTCASE_EMPTY = 105;
const auto WARN_NO_STATEMACHINES = 106;

extern void warn(int code, std::string const & msg = "");
extern void fatal(int code, std::string const & msg = "");

extern std::string global_out_path;


#endif
