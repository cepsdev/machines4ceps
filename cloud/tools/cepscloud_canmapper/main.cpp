#include "mainwindow.h"
#include "common.h"
#include <QApplication>


std::vector<std::string> check_available_ifs(){
    struct ifaddrs *addrs,*tmp;
    std::vector<std::string> r;

    getifaddrs(&addrs);
    tmp = addrs;

    while (tmp)
    {
       if (tmp->ifa_name)
            r.push_back(tmp->ifa_name);

        tmp = tmp->ifa_next;
    }

    freeifaddrs(addrs);
    return r;
}

std::vector<std::string> available_ifs;

std::map<std::string,std::vector<std::string>> mapping_downstream;
std::map<std::string,std::vector<std::string>> mapping_upstream;


int main(int argc, char *argv[])
{
    auto v = check_available_ifs();
    std::sort(v.begin(),v.end());
    auto it = std::unique (v.begin(), v.end());
    v.erase(it,v.end());
    available_ifs = v;

    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
}
