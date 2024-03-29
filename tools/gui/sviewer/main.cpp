#include "mainwindow.h"
#include <QApplication>
#include <signal.h>
#include <sys/types.h>
#include <limits>
#include <cstring>
#include <iostream>
#include "core/include/base_defs.hpp"


#include "core/include/state_machine.hpp"
#include "core/include/state_machine_simulation_core.hpp"

#define VERSION_STATECHARTVIEWER_MAJOR "0"
#define VERSION_STATECHARTVIEWER_MINOR "0.1"


void fatal(int code, std::string const & msg )
{
    using namespace std;
    stringstream ss;
    ss << msg;
    throw runtime_error{ss.str()};
}

void warn(int code, std::string const & msg)
{
    using namespace std;
    cerr << "\n***WARNING. " << msg <<"."<< endl;
}


int setup_and_run_sm_core(int argc,char ** argv, State_machine_simulation_core** result_sm_core)
{
    using namespace std;
    if (argc <= 1)
    {
        cout <<  "\nsviewer renders state machines and is part of sm4ceps.\n";
        cout << "Usage: " << argv[0] << " FILE [FILE...] [-i] [-oPATH] [--debug]\n";
        cout << "\n";
        cout << "Example:\n " << argv[0] <<" a.ceps b.ceps .\n";
        return EXIT_SUCCESS;
    }

    auto result_cmd_line = process_cmd_line(argc,argv);

    if (result_cmd_line.version_flag_set)
    {
            #ifdef __GNUC__
                    std::cout << "\n"
                              << "VERSION " VERSION_STATECHARTVIEWER_MAJOR << "."<<  VERSION_STATECHARTVIEWER_MAJOR   <<" (" __DATE__ << ") BUILT WITH GCC "<< "" __VERSION__ ""<< " on GNU/LINUX "
             #ifdef __LP64__
                              << "64BIT"
             #else
                              << "32BIT"
             #endif
                              << "\n(C) BY THE AUTHORS OF sm4ceps \n" << std::endl;
            #else
                #ifdef _MSC_FULL_VER
                    std::cout << "\n"
                        << "VERSION " VERSION_SM4CEPS_MAJOR << "." << VERSION_SM4CEPS_MINOR << " (" __DATE__ << ") BUILT WITH MS VISUAL C++ " <<  _MSC_FULL_VER  << " on Windows "
                    #ifdef _WIN64
                        << "64BIT"
                    #else
                        << "32BIT"
                    #endif
                        << "\n(C) BY THE AUTHORS OF sm4ceps\n" << std::endl;
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

#ifdef _WIN32
    WORD wVersionRequested;
    WSADATA wsaData;
    int err;
    wVersionRequested = MAKEWORD(2, 2);
    err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0) {
        std::cerr << "***Error: WSAStartup failed(" << err << ")\n";
        return 1;
    }

    if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
        std::cerr << "***Error:Could not find a usable version of Winsock.dll\n";
        WSACleanup();
        return 1;
    }
#else
    signal(SIGPIPE, SIG_IGN);
#endif

    State_machine_simulation_core* sm_core = *result_sm_core = new State_machine_simulation_core;
    sm_core->set_fatal_error_handler(fatal);
    sm_core->set_non_fatal_error_handler(warn);
    sm_core->set_log_stream(&std::cout);
    string last_file_processed;
    try{
        Result_process_cmd_line result_cmd_line;
        init_state_machine_simulation(argc,argv,sm_core,result_cmd_line);
        PRINT_DEBUG_INFO = sm_core->print_debug_info_;
        run_state_machine_simulation(sm_core,result_cmd_line);
    }
    catch (ceps::interpreter::semantic_exception & se)
    {
        std::cout << "***Fatal Error: "<< se.what() << std::endl;
        return EXIT_FAILURE;
    }
    catch (std::runtime_error & re)
    {
        std::cout << "***Fatal Error:" << re.what() << std::endl;
        return EXIT_FAILURE;
    }

#ifdef _WIN32
    WSACleanup();
#endif

    return EXIT_SUCCESS;
}//main

#include "graphviz/gvc.h"

int test(int argc, char** argv){
    GVC_t *gvc;
    Agraph_t *g;
    FILE *fp;
    gvc = gvContext();
    if (argc > 1)
    fp = fopen(argv[1], "r");
    else
    fp = stdin;
    g = agread(fp, 0);
    gvLayout(gvc, g, "dot");
    gvRender(gvc, g, "plain", stdout);
    gvFreeLayout(gvc, g);
    agclose(g);
    return (gvFreeContext(gvc));
}

int main(int argc, char *argv[])
{
    State_machine_simulation_core* smcore;

    //test(argc,argv);

    if (0!=setup_and_run_sm_core(argc,argv,&smcore)) return EXIT_FAILURE;

    QApplication a(argc, argv);
    MainWindow w{smcore};

    w.show();

    return a.exec();
}
