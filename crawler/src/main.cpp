/*
 *
 * @author Tomas Prerovsky <tomas.prerovsky@gmail.com>
 *
 *
 *
 * @section DESCRIPTION
 *
 * Part of State Matrix Gui Generator.
 *
 *
 * Demonstrates usage and semantics of a very simple state matrix.
 *
 *
 */
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <set>
#include <chrono>
#include <memory>

struct Log{
		std::ostream* os_p = nullptr;
		Log() = default;
		template <typename T> Log& operator << (T const & v)
		{
			if (os_p) { *os_p << v;}
			return *this;
		}
	};

struct Debuglogger{
		std::string toprint_;
		Log log_;
		bool p_;

		Debuglogger(std::string const & s,Log log,bool p):toprint_(s),log_(log),p_(p)
		{
			if(p_) log_ << "[DEBUG][ENTER]["<< toprint_<< "]\n";
		}

		~Debuglogger()
		{
			if(p_) log_ << "[DEBUG][LEAVE]["<< toprint_<< "]\n";
		}

		template<typename T> Debuglogger& operator <<(T const & v)
		{
			if (!p_) return *this;
			*(log_.os_p) << v;
			return *this;
		}
	};

#define __funcname__ __PRETTY_FUNCTION__
#define DEBUG_FUNC_PROLOGUE 	Debuglogger debuglog(__funcname__,logger,print_debug_info);
#define DEBUG (debuglog << "[DEBUG]", debuglog)

#include "main.hpp"
#include "cmdline_utils.hpp"

#define VERSION_SMGUIGEN_MAJOR "0"
#define VERSION_SMGUIGEN_MINOR "0.1"



const auto WARN_A = 0;
const auto WARN_B = 1;
const auto WARN_C = 2;
const auto WARN_D = 3;
const auto WARN_E = 3;
const auto WARN_F = 4;
const auto WARN_G = 5;
const auto WARN_H = 6;

const auto ERR_FILE_OPEN_FAILED = 1;
const auto ERR_CEPS_PARSER = 2;
const auto ERR_SM_ID_MISSING_OR_ILLFORMED = 3;

auto print_debug_info = true;
Log logger;

using namespace std;

std::set<std::string> keyword_set = {"id","title","description","checked"};

struct Statematrix;
typedef Statematrix* Statematrix_ptr;


void fatal(int code, std::string const & msg )
{
	stringstream ss;
	if (ERR_FILE_OPEN_FAILED == code)
		ss << "Couldn't open file '" << msg <<"'.";
	else if (ERR_CEPS_PARSER == code)
		ss << "A parser exception occured in file '" << msg << "'.";
	else if (ERR_SM_ID_MISSING_OR_ILLFORMED == code)
		ss << "Declaration of state matrix: id-field is missing/illformed.";
	else ss << msg;
	throw runtime_error{ss.str()};
}

void warn(int code, std::string const & msg)
{
	if (WARN_A  == code)
		cerr << "\n***WARNING. No xml file prefix defined for '"
		<< msg
		<< "', will use default (empty string).\nIf you want different behaviour please add the following to the global namespace:\n\txml{gen_file_prefix{"
		<< msg
		<< "{\"MY_XML_FILENAME_PREFIX_HERE\";};};.\n";
	else if (WARN_B == code)
		cerr << "\n***WARNING. No template found which matches the path "
		<< msg
		<< "." << endl;
	else if (WARN_B == code)
		cerr << "\n***WARNING. There exists neither a xml invocation mapping nor a db2 table definition for  "
		<< msg
		<< ". No file for this particular object will be generated." << endl;
	else if (WARN_C == code)
		cerr << "\n***WARNING. Id already defined in another testcase precondition: "
		<< msg
		<< "." << endl;
	else if (WARN_D == code)
		cerr << "\n***WARNING. Testcase precondition is empty: "
		<< msg
		<< "." << endl;
	else if (WARN_E == code)
		cerr << "\n***WARNING. No statemachines found"
		<< msg
		<< "." << endl;
	else
		cerr << "\n***WARNING. " << msg <<"."<< endl;
}


auto INDENT = 0;

string indent()
{
 string t;
 for(int i = 0; i < INDENT; ++i)
  t.append(" ");
 return t;
}

std::string replace_dot_in_string(const std::string & s,const std::string & r)
{
	std::string t;
	for(size_t i = 0; i < s.length();++i)
	{
		char b[2] = {0};
		b[0] = s[i];
		if (b[0]=='.') t.append(r);
		else t.append(b);
	}
	return t;
}



void process_files(	std::vector<std::string> const & file_names,std::string& last_file_processed,ceps::ast::Nodeset* current_universe,ceps::Ceps_Environment * ceps_env_current)
{
	DEBUG_FUNC_PROLOGUE

	for(auto const & file_name : file_names)
	{
		DEBUG << "[PROCESSING INPUT FILES]"<<file_name<<"\n";
		std::fstream def_file{ last_file_processed = file_name};
		if (!def_file) fatal(ERR_FILE_OPEN_FAILED,file_name);
		Ceps_parser_driver driver{ceps_env_current->get_global_symboltable(),def_file};
		ceps::Cepsparser parser{driver};

		if (parser.parse() != 0 || driver.errors_occured())
			fatal(ERR_CEPS_PARSER, file_name);

		std::vector<ceps::ast::Nodebase_ptr> generated_nodes;
		ceps::interpreter::evaluate(*current_universe,
									driver.parsetree().get_root(),
									ceps_env_current->get_global_symboltable(),
									ceps_env_current->interpreter_env(),
									&generated_nodes
									);
	}//for
}//process_def_files


bool get_name_of_id(ceps::ast::Nodeset const & ns,std::string& result)
{
	if (ns.nodes().size() != 1) return false;
	if (ns.nodes().front()->kind() != ceps::ast::Ast_node_kind::identifier) return false;
	result = ceps::ast::name(ceps::ast::as_id_ref(ns.nodes().front()));
	return true;
}

template <typename F>
 size_t foreach_struct(ceps::ast::Nodeset const & ns, F f)
 {
	size_t structs_read{};
	for(auto p : ns.nodes())
	{
		if (p->kind() != ceps::ast::Ast_node_kind::structdef) continue;
		++structs_read;
		f(ceps::ast::as_struct_ref(p));
	}
	return structs_read;
 }

void build_sm(ceps::ast::Struct const & st,Statematrix_ptr parent)
{

}

void process_content(ceps::ast::Nodeset* up,ceps::Ceps_Environment * ceps_env_current)
{
	using namespace ceps::ast;
	DEBUG_FUNC_PROLOGUE
	auto u = *up;
	auto state_matrices = u[all{"state_matrix"}];
	DEBUG <<"[state_matrices]"<< state_matrices << "\n";
	for (auto sm_:state_matrices)
	{
		auto sm = sm_["state_matrix"];
		auto id_ = sm["id"];
		if (id_.size() != 1) fatal(ERR_SM_ID_MISSING_OR_ILLFORMED,"");
		std::string id;
		if(!get_name_of_id(id_,id)) fatal(ERR_SM_ID_MISSING_OR_ILLFORMED,"");
		DEBUG << "[SM_ID]" << id << "\n";

		foreach_struct(sm,[](ceps::ast::Struct const & st){});

	}
}



ceps::Ceps_Environment* ceps_env_current = nullptr;
ceps::ast::Nodeset* current_universe = nullptr;


int main(int argc,char ** argv)
{
	logger.os_p = &std::cout;


	if (argc <= 1)
	{
		cout <<  "\nA State Matrix GUI generator based on ceps and qt.\n";
		cout << "Usage: " << argv[0] << " FILE [FILE...] [-i] [-oPATH]\n";
		cout << "\n";
		cout << "Example:\n " << argv[0] <<" a.ceps b.ceps -o."<< " \n loads all state matrix descriptions in a.ceps and b.ceps, processes them and writes result to the working directory.\n\n";
		return EXIT_FAILURE;
	}


	auto result_cmd_line = process_cmd_line(argc,argv);
	print_debug_info = result_cmd_line.debug_mode;
	DEBUG_FUNC_PROLOGUE

	if (result_cmd_line.version_flag_set)
	{
			#ifdef __GNUC__
			 		std::cout << "\n"
							  << "VERSION " VERSION_SMGUIGEN_MAJOR << "."<<  VERSION_SMGUIGEN_MINOR   <<" (" __DATE__ << ") BUILT WITH GCC "<< "" __VERSION__ ""<< " on GNU/LINUX "
			 #ifdef __LP64__
							  << "64BIT"
			 #else
							  << "32BIT"
			 #endif
							  << "\n" << std::endl;
			#else
				#ifdef _MSC_FULL_VER
					std::cout << "\n"
						<< "VERSION " VERSION_SMGUIGEN_MAJOR << "." << VERSION_SMGUIGEN_MINOR << " (" __DATE__ << ") BUILT WITH MS VISUAL C++ " <<  _MSC_FULL_VER  << " on Windows "
					#ifdef _WIN64
						<< "64BIT"
					#else
						<< "32BIT"
					#endif
						<< "\n" << std::endl;
				#endif
			#endif
	}

	// Do sanity checks

	for(std::string const & f : result_cmd_line.definition_file_rel_paths)
	 if (!std::ifstream{f})
	 {
		 std::cerr << "\n***Error: Couldn't open file '" << f << "' " << std::endl << std::endl;
		 return EXIT_FAILURE;
	 }



	string last_file_processed;
	ceps::Ceps_Environment ceps_env{PRELUDE};
	ceps_env_current = &ceps_env;
	ceps::ast::Nodeset universe;
	current_universe = &universe;

	try{

		 process_files(	result_cmd_line.definition_file_rel_paths,
				 last_file_processed,
				 current_universe,
				 ceps_env_current);
		 process_content(current_universe,ceps_env_current);
	}

	catch (std::runtime_error & re)
	{
		std::cerr << "***Error while evaluating file '" + last_file_processed + "':" << re.what() << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}//main



