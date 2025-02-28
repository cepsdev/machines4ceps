﻿/*
Copyright 2014-2024 Tomas Prerovsky (cepsdev@hotmail.com).

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

#include "core/include/cmdline_utils.hpp"
#include <iostream>
#include <sys/stat.h>
#ifndef _WIN32

#include <dirent.h>
#include <unistd.h>

#else

#include "windows.h"

#endif

#include <map>
#include <stdlib.h>

bool DEBUG_MODE = false;
bool DONT_INCREMENT_SEQUENCER = false;
std::string global_out_path = "./";

std::map<std::string,std::string> output_directory;

bool print_header_in_sql_files = true;

std::string get_rel_out_path(std::string const & def_file)
{
	return output_directory[def_file];
}

std::string normalize_path(std::string const & path)
{
#ifndef _WIN32
	if (path.length() && path[path.length()-1] != '/')
		return path + "/";
#else
	if (path.length() && path[path.length() - 1] != '\\' && path[path.length() - 1] != '/')
		return path + "\\";
#endif
	return path;
}//normalize_path

void mk_directories(std::string const & path)
{

	
#ifndef _WIN32
	for(size_t i = 0; i < path.length(); ++i)
	{
		if (path[i] != '/')
			continue;
		auto s = path.substr(0,i);
		if (s == ".")
			continue;
		struct stat st {0};

		if (stat(s.c_str(), &st) != -1)
			continue;

		if (-1 == mkdir(s.c_str(),0700))
		{
			std::cerr << "***Error: Cannot create directory '"<< s <<"' '"<< path << "'\n";
			exit(1);
		}
	}
#else
	for (size_t i = 0; i < path.length(); ++i)
	{
		if (path[i] != '/' && path[i] != '\\')
			continue;
		auto s = path.substr(0, i);
		if (s == ".")
			continue;
		struct stat st { 0 };

		if (stat(s.c_str(), &st) != -1)
			continue;

        auto r = CreateDirectoryA(s.c_str(), NULL);
	}
#endif
}

void traverse_directories(std::string const & current_path,
						  std::vector<std::string>& definition_files,
						  std::string const & rel_out_path)
{
#ifndef _WIN32
	DIR* dir_stream = opendir(current_path.c_str());
	if (dir_stream == NULL)
		{
			std::cerr << "***Error: Couldn't open directory '"<< current_path << "'\n";
			exit(1);
		}
	for(;;)
	{
		auto dir_descr = readdir(dir_stream);
		if (dir_descr == NULL)
			break;
		std::string dir_entry{dir_descr->d_name};
		std::string dir_entry_full{current_path + dir_entry};
		if (dir_entry == "." || dir_entry == "..")
			continue;
		bool is_dir = false;
		struct stat stat_info{0};
		stat(dir_entry_full.c_str(),&stat_info);
		is_dir = S_ISDIR(stat_info.st_mode);

		if (is_dir)
		{

			traverse_directories(dir_entry_full+"/",definition_files,rel_out_path+ ( rel_out_path.length() == 0 ? "" : "/" ) +dir_entry);
		
		} else if (dir_entry.length() > 4 && dir_entry.substr(dir_entry.length()-4,4 ) == ".def")
		{

			output_directory[dir_entry_full] = rel_out_path;
			definition_files.push_back(dir_entry_full);
		}
	}
#else
    WIN32_FIND_DATAA ffd = { 0 };
	HANDLE hf = INVALID_HANDLE_VALUE;
	std::string cc = current_path +  "*" ;
	hf = FindFirstFileA(cc.c_str(), &ffd);
	if (hf == INVALID_HANDLE_VALUE)
	{
		printf("FindFirstFile failed (%d)\n", GetLastError());
		exit(1);
	}

	do
	{
		//std::cout << (char*)ffd.cFileName << std::endl;

		std::string dir_entry{ ffd.cFileName };
		std::string dir_entry_full{ current_path + dir_entry };
		if (dir_entry == "." || dir_entry == "..")
			continue;
		bool is_dir = (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY;

		if (is_dir)
		{

			traverse_directories(dir_entry_full + "/", definition_files, rel_out_path + (rel_out_path.length() == 0 ? "" : "/") + dir_entry);
		}
		else if (dir_entry.length() > 4 && dir_entry.substr(dir_entry.length() - 4, 4) == ".def")
		{

			output_directory[dir_entry_full] = rel_out_path;
			definition_files.push_back(dir_entry_full);
		}
	} while (FindNextFileA(hf, &ffd) != 0);

	FindClose(hf);

#endif
}

static std::string report_dir;

std::string& get_report_out_dir()
{

	return report_dir;
}

Result_process_cmd_line process_cmd_line(int argc,char ** argv, Result_process_cmd_line r_init)
{
	using namespace std;
	global_out_path = "";


	bool version_flag_set = false;
	bool interactive_mode = false;
	bool post_processing = false;

	std::vector<std::string> definition_file_rel_paths;
	std::vector<std::string> post_processing_rel_paths;

	Result_process_cmd_line r = r_init;
	r.valid = true;

	string current_root_struct_name{};

	

	for(int i = 1; i < argc;++i)
	{
			string arg{argv[i]};

            if (arg == "-o") {r.out_path = argv[++i];continue;}
            if (arg.substr(0,2) == "-i") {interactive_mode =  true;continue;}
			else if (arg.substr(0,2) == "-v") {version_flag_set = true;continue;}
			else if (arg.substr(0,9) == "--version") {version_flag_set = true;continue;}
			else if (arg == "-d" || arg == "--debug"){DEBUG_MODE = true; continue; }
			else if (arg == "--print-no-header-in-sql"){print_header_in_sql_files = false;continue;}
			else if (arg == "--print-header-in-sql"){print_header_in_sql_files = true;continue;}
			else if (arg == "--dont_increment_sequencer"){DONT_INCREMENT_SEQUENCER = true;continue;}
			else if (arg == "--server") {r.start_in_server_mode = true;continue;}
			//else if (arg.substr(0,6) == "--port") {r.server_port = arg.substr(6); continue;}
			else if (arg.substr(0,5) == "--rip") {r.remote_nodes.push_back(std::make_pair(arg.substr(5), std::string{}));continue;}
			else if (arg.substr(0,7) == "--rport") { if(r.remote_nodes.size()) r.remote_nodes.back().second = arg.substr(7); continue;}
			else if (arg == "--quiet") {r.quiet = true; continue;}
			else if (arg == "--run_as_monitor") {r.run_as_monitor = true; continue;}
			else if (arg == "--logtrace") {r.logtrace = true; continue;}
			else if (arg.substr(0,6) == "--rmip") {r.monitored_node_ip = arg.substr(6); continue;}
			else if (arg.substr(0,8) == "--rmport") {r.monitored_node_port = arg.substr(8); continue;}
			else if (arg.substr(0,9) == "--monitor") {r.monitor = arg.substr(9); continue;}
			else if (arg.substr(0,8) == "--plugin") {r.plugins.push_back(arg.substr(8)); continue;}
			else if (arg.substr(0,9) == "--timeout") {if (i+1 == argc) break; r.timeout = argv[i+1]; ++i; continue;}
			else if (arg == "--print_transition_tables" || arg == "--ptt") {r.print_transition_tables=true; continue;}
			else if (arg == "--cppgen") {r.cppgen = true; continue;}
            else if (arg == "--enforce_native"){r.enforce_native = true; continue;}
			else if (arg == "--ignore_unresolved_state_id_in_directives" || arg == "--iursd") {r.ignore_unresolved_state_id_in_directives = true; continue;}
			else if (arg == "--ignore_simulations") {r.ignore_simulations=true; continue;}
			else if (arg == "--cppgen_ignore_print") {r.cppgen_ignore_print=true;continue;}
			else if (arg == "--cppgen_statemachines") {r.cppgen_statemachines=true;continue;}
			else if (arg == "--live_log") {r.live_log=true;continue;}
			else if (arg == "--print_statemachines") {r.print_statemachines=true;continue;}
			else if (arg == "--dot_gen") {r.dot_gen=true;continue;}
			else if (arg == "--dot_gen_one_file_per_top_level_statemachine") {r.dot_gen_one_file_per_top_level_statemachine=true;continue;}
			else if (arg == "--print_evaluated_input_tree") {r.print_evaluated_input_tree=true;continue;}
			else if (arg == "--print_signal_generators") {r.print_signal_generators=true;continue;}
		    else if (arg == "--report_format_sexpression") {r.report_format_sexpression=true;continue;}
		    else if (arg == "--report_verbose") {r.report_verbose=true;continue;}
		    else if (arg == "--report_format_xml") {r.report_format_xml=true;continue;}
		    else if (arg == "--report_format_json") {r.report_format_json=true;continue;}
		    else if (arg == "--report_format_ceps") {r.report_format_ceps=true;continue;}
		    else if (arg == "--print_evaluated_postprocessing_tree") {r.print_evaluated_postprocessing_tree=true;continue;}
		    else if (arg == "--dump_asciidoc_can_layer") {r.dump_asciidoc_can_layer=true;continue;}
            else if (arg == "--dump_stddoc_canlayer") {r.dump_stddoc_canlayer=true;continue;}
            else if (arg == "--stddoc_canlayer_complete_html_page") {r.stddoc_canlayer_no_header_no_footer=false;continue;}
		    else if (arg == "--no_warn") {r.no_warn=true;continue;}
		    else if (arg == "--print_event_signatures") {r.print_event_signatures=true;continue;}
		    else if (arg == "--post_processing") {post_processing=true;continue;}
		    else if (arg == "--package_file") {++i;if (i >= argc)break;r.package_file = argv[i] ; continue;}
            else if (arg == "--ws_api") {++i;if (i >= argc)break;r.ws_api_on = true;r.ws_api_port = argv[i] ; continue;}
            else if (arg == "--sleep_before_ws_api") {++i;if (i >= argc)break;r.sleep_before_ws_api_on = true;r.sleep_before_ws_delta_ms = argv[i] ; continue;}
            else if (arg == "--vcan_api") {++i;if (i >= argc)break;r.vcan_api_on = true;r.vcan_api_port = argv[i] ; continue;}
            else if (arg == "--push_dir") {++i;if (i >= argc)break;r.push_dir = argv[i] ; continue;}
            else if (arg == "--no_file_output") {r.no_file_output=true; continue;}
            else if (arg == "--report_includes_cat") {r.report_includes_cat=true; continue;}
            else if (arg == "--start_paused") {r.start_paused=true; continue;}			
            else if (arg == "--print_raw_input_tree") {r.print_raw_input_tree=true; continue;}
            else if (arg == "--pr") {r.print_raw_input_tree=true; continue;}
            else if (arg == "--pre") {r.print_raw_input_tree=r.print_evaluated_input_tree=true; continue;}
            else if (arg == "--pe") {r.print_evaluated_input_tree=true; continue;}
			else if (arg == "--ppe") {r.print_evaluated_postprocessing_tree=true;continue;}
            else if (arg == "--format") { if (i+1 == argc) break; r.output_format_flags.push_back(argv[i+1]); ++i;continue;}
            else if (arg == "--link" || arg == "-l") { if (i+1 == argc) break; r.shard_objects.push_back(argv[i+1]); ++i;continue;}

            else if (arg == "--report_state_machines_only") { 
				r.attributes.push_back(
					Result_process_cmd_line::attribute_t{"report_state_machines_only",{}}
				); 
				continue;
			}
            else if (arg == "--report_state_machines") { 
				Result_process_cmd_line::attribute_t attr{"report_state_machines",{}};
				auto j = i + 1;
				for( ;j < argc;++j){
					string cur_arg{argv[j]};
					if (cur_arg.length() && cur_arg[0] =='-') break;
					attr.second.push_back(cur_arg);
				}
				r.attributes.push_back(attr);
				i = j - 1;
				continue;
			}
			else if (arg == "--root_struct"){
				if (i+1 == argc) break;
				current_root_struct_name = argv[i+1];
				++i;
				continue;
			}
			else if (arg == "--doc-option") { 
				if (i+1 == argc) break; 
				r.output_format_flags.push_back(std::string{"doc-option-"}+argv[i+1]); ++i;continue;}
			else if (arg == "--help") {r.print_help = true;continue;}
			else if (arg == "--create_plugin_project") {
				r.create_plugin_project = true;
				if (i+1 < argc){
					string t{argv[i+1]};
					if (t.length() && t[0] != '-'){
					 	r.create_plugin_project_name = t;
						++i;
					}
				}
				continue;
			} else if (arg == "--create_plugin_ceps_root"){
				if (i+1 < argc) {
					r.create_plugin_ceps_root = argv[i + 1];
					++i;
				}
				continue;
			}
			else if (arg.substr(0, 6) == "--port") { 
				if (arg.length() > 6) {
					r.server_port = r.port = arg.substr(6);
				}
				else {
					++i;
					if (i >= argc) break;
					r.server_port = r.port = argv[i];
				}
				continue; 
			}

			{
				bool is_dir = false;
#ifndef _WIN32
				struct stat stat_info{0};
				stat(argv[i],&stat_info);
				is_dir = S_ISDIR(stat_info.st_mode);
#else
				DWORD api_call_result = GetFileAttributesA(argv[i]);
				if (INVALID_FILE_ATTRIBUTES == api_call_result)
				{
					std::cerr << "***Error: Unexpected error occured when requesting file attributes for '" << argv[i] << "'\n";
					exit(1);
				}
				is_dir = (FILE_ATTRIBUTE_DIRECTORY & api_call_result) == FILE_ATTRIBUTE_DIRECTORY;
#endif
				if(is_dir)
				{
#ifndef _WIN32
					if (arg.size() > 0 && arg[arg.size() -1] != '/')
						arg+="/";
#else
					if (arg.size() > 0 && arg[arg.size() - 1] != '/' && arg[arg.size() - 1] != '\\')
						arg += "/";
#endif
					auto size_before = definition_file_rel_paths.size();
					traverse_directories(arg,definition_file_rel_paths,"");
					for (auto i = size_before; i < definition_file_rel_paths.size(); ++i)
					 r.root_struct.push_back(current_root_struct_name);
					current_root_struct_name = {};
				}
				else {
					if (post_processing) {
						post_processing_rel_paths.push_back(arg);
						r.root_struct_post.push_back(current_root_struct_name);
					}
					else {
						definition_file_rel_paths.push_back(arg);
						r.root_struct.push_back(current_root_struct_name);
					}
					current_root_struct_name = {};
				}

			}
	}//for

	r.version_flag_set = version_flag_set;
	r.debug_mode = DEBUG_MODE;
	r.interactive_mode = interactive_mode;
	r.definition_file_rel_paths = definition_file_rel_paths;
	r.post_processing_rel_paths = post_processing_rel_paths;
	return r;
}//process_cmd_line

