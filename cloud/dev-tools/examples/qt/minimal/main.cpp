#include "mainwindow.h"
#include <QApplication>
#include "../../dev-tools/include/create_ceps_cloud_streaming_endpoint.h"
MainWindow* w;

static void myhook(net::can::can_frame frame){
    QMetaObject::invokeMethod(w, "update_frame", Qt::QueuedConnection,
                              Q_ARG(int, frame.can_id),
                              Q_ARG(int, frame.can_dlc),
                              Q_ARG(char, frame.data[0]),
                              Q_ARG(char, frame.data[1]),
                              Q_ARG(char, frame.data[2]),
                              Q_ARG(char, frame.data[3]),
                              Q_ARG(char, frame.data[4]),
                              Q_ARG(char, frame.data[5]),
                              Q_ARG(char, frame.data[6]),
                              Q_ARG(char, frame.data[7])
            );
}


int main(int argc, char *argv[])
{

    QApplication a(argc, argv);
    w = new MainWindow{};
    w->show();

    global_ctrlregistry.reg_down_stream_hook("CAN","PCAN-USB-1",myhook);
    setup_and_run_cepscloud_streaming_endpoint(SIMBOX_HOST,SIMBOX_HOST_PORT,argc,argv);

    return a.exec();
}
