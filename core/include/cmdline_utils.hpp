/*
Copyright 2014, cpsdev (cepsdev@hotmail.com).
All rights reserved.
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.
    * Neither the name of Google Inc. nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#ifndef INC_CMDLINE_UTILS_HPP
#define INC_CMDLINE_UTILS_HPP


#include <string>
#include <vector>

struct Result_process_cmd_line
{
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

    bool print_evaluated_input_tree = false;
    bool print_evaluated_postprocessing_tree = false;
    bool print_signal_generators = false;

    std::string live_log_port = "3000";
    bool live_log = false;

    bool report_format_sexpression = false;
    bool report_verbose = false;
    bool report_format_xml = false;
    bool report_format_json = false;
    bool report_format_ceps = false;

    bool dump_asciidoc_can_layer = false;
    bool no_warn = false;
    bool print_event_signatures = false;


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
