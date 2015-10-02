#include "mainwindow.h"
#include <QApplication>


void fatal(int,std::string){}
void warn(int,std::string){}

int main(int argc, char *argv[])
{
  QApplication a(argc, argv);
  MainWindow w;
  w.show();

  return a.exec();
}
