#include "mainwindow.h"

#include <signal.h>
#include <sys/types.h>
#include <limits>
#include <cstring>
#include <iostream>
#include "core/include/base_defs.hpp"
#include "core/include/state_machine.hpp"
#include "core/include/state_machine_simulation_core.hpp"
#include <thread>

#include <QApplication>


/*
 * sm4ceps utilitiy functions
*/

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

void smcore_thread(State_machine_simulation_core* sm_core,int argc,char ** argv){
    using namespace std;
    sm_core->set_fatal_error_handler(fatal);
    sm_core->set_non_fatal_error_handler(warn);
    sm_core->set_log_stream(&std::cout);
    signal(SIGPIPE, SIG_IGN);

    try{
        Result_process_cmd_line result_cmd_line;
        result_cmd_line.live_log = true;
        result_cmd_line.quiet = true;
        init_state_machine_simulation(argc,argv,sm_core,result_cmd_line);
        PRINT_DEBUG_INFO = sm_core->print_debug_info_;
        run_state_machine_simulation(sm_core,result_cmd_line);
    }
    catch (ceps::interpreter::semantic_exception & se)
    {
        std::cout << "***Fatal Error: "<< se.what() << std::endl;
    }
    catch (std::runtime_error & re)
    {
        std::cout << "***Fatal Error:" << re.what() << std::endl;
    }
}

int setup_and_run_sm_core(int argc,char ** argv, State_machine_simulation_core** result_sm_core)
{
    using namespace std;
    if (argc <= 1) return EXIT_SUCCESS;

    auto result_cmd_line = process_cmd_line(argc,argv);
    State_machine_simulation_core* sm_core = *result_sm_core = new State_machine_simulation_core;
    new std::thread{smcore_thread,sm_core,argc,argv};
    return EXIT_SUCCESS;
}


extern State_machine_simulation_core* smcore = nullptr;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);


    char * argv_sm4ceps[] = {"", "../hello_world/hello_world.ceps"};

    if (EXIT_SUCCESS != setup_and_run_sm_core(sizeof(argv_sm4ceps) / sizeof(char*),argv_sm4ceps,&smcore) ) return EXIT_FAILURE;

    MainWindow w;
    w.show();

    return a.exec();
}
