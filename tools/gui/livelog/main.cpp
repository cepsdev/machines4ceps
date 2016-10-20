#include "mainwindow.h"
#include <QApplication>
#include <signal.h>
#include <sys/types.h>
#include <limits>

int main(int argc, char *argv[])
{
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

 QApplication a(argc, argv);
 MainWindow w;
 w.show();
 return a.exec();
}
