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

void traverse_directories(std::string const & current_path,std::vector<std::string>& definition_files,std::string const & rel_out_path)
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

Result_process_cmd_line process_cmd_line(int argc,char ** argv)
{
	using namespace std;
	global_out_path = "";


	bool version_flag_set = false;
	bool interactive_mode = false;
	
	string out_path = "";
	std::vector<std::string> definition_file_rel_paths;

	Result_process_cmd_line r;

	for(int i = 1; i < argc;++i)
	{
			string arg{argv[i]};
			int ofs = 0;
			bool is_output_path = false;
			bool is_report_out = false;

			if (arg.substr(0,2) == "-o") {is_output_path =  true;ofs = 2;}
			if (arg.substr(0,2) == "-i") {interactive_mode =  true;ofs = 2;continue;}
			else if (arg.substr(0,18) == "--output_directory") {is_output_path = true;ofs = 18;}
			else if (arg.substr(0,2) == "-v") {version_flag_set = true;continue;}
			else if (arg.substr(0,9) == "--version") {version_flag_set = true;continue;}
			else if (arg == "-d" || arg == "--debug"){DEBUG_MODE = true; continue; }
			else if (arg == "--print-no-header-in-sql"){print_header_in_sql_files = false;continue;}
			else if (arg == "--print-header-in-sql"){print_header_in_sql_files = true;continue;}
			else if (arg == "--dont_increment_sequencer"){DONT_INCREMENT_SEQUENCER = true;continue;}
			else if (arg.substr(0,18) == "--report_directory") {is_report_out = true; ofs = 18;}
			else if (arg == "--server") {r.start_in_server_mode = true;continue;}
			//else if (arg.substr(0,6) == "--port") {r.server_port = arg.substr(6); continue;}
			else if (arg.substr(0,5) == "--rip") {r.remote_nodes.push_back(std::make_pair(arg.substr(5), std::string{}));continue;}
			else if (arg.substr(0,7) == "--rport") { if(r.remote_nodes.size()) r.remote_nodes.back().second = arg.substr(7); continue;}
			else if (arg == "--quiet") {r.quiet = true; continue;}
			else if (arg == "--run_as_monitor") {r.run_as_monitor = true; continue;}
			else if (arg.substr(0,6) == "--rmip") {r.monitored_node_ip = arg.substr(6); continue;}
			else if (arg.substr(0,8) == "--rmport") {r.monitored_node_port = arg.substr(8); continue;}
			else if (arg.substr(0,9) == "--monitor") {r.monitor = arg.substr(9); continue;}
			else if (arg.substr(0,8) == "--plugin") {r.plugins.push_back(arg.substr(8)); continue;}
			else if (arg.substr(0,9) == "--timeout") {r.timeout = arg.substr(9); continue;}
			else if (arg == "--ignore_unresolved_state_id_in_directives" || arg == "--iursd") {r.ignore_unresolved_state_id_in_directives = true; continue;}

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







			if (is_output_path)
				global_out_path = out_path = normalize_path(arg.substr(ofs));
			else if (is_report_out)
				get_report_out_dir() = normalize_path(arg.substr(ofs));
			else
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
					traverse_directories(arg,definition_file_rel_paths,"");

				}
				else definition_file_rel_paths.push_back(arg);

			}
	}//for

	r.version_flag_set = version_flag_set;
	r.debug_mode = DEBUG_MODE;
	r.interactive_mode = interactive_mode;
	r.out_path = out_path;
	r.definition_file_rel_paths = definition_file_rel_paths;
	return r;
}//process_cmd_line

