#include "mainwindow.h"
#include <QApplication>
#include "core/include/state_machine.hpp"
#include "core/include/state_machine_simulation_core.hpp"
#include <signal.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/types.h>
#include <limits>
#include <cstring>
#include "core/include/cmdline_utils.hpp"
#include "core/include/state_machine_simulation_core.hpp"

State_machine_simulation_core* sm_core;


int main(int argc, char *argv[])
{


  signal(SIGPIPE,SIG_IGN);
  sm_core = new State_machine_simulation_core;
  sm_core->set_log_stream(&std::cerr);

  Result_process_cmd_line result_cmd_line;
  init_state_machine_simulation(argc,argv,sm_core,result_cmd_line);

  new std::thread{run_state_machine_simulation,sm_core,result_cmd_line};



  QApplication a(argc, argv);
  MainWindow w;
  w.show();

  return a.exec();
}
