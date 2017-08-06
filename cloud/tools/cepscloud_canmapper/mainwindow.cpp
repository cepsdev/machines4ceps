#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "streammapping.h"
#include "common.h"



MainWindow::MainWindow(QCoreApplication* app,QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    app_core = app;
    ui->setupUi(this);
    this->statusBar()->showMessage(tr("Ready"));

}

MainWindow::~MainWindow()
{
    delete ui;
}

bool MainWindow::event(QEvent *event)
{
    if (event->type() == (QEvent::Type)(QEvent::User+CTRL_THREAD_ERROR_GETADDR_FAILED)) {
        ui->statusBar->setStyleSheet("color: red");
        this->statusBar()->showMessage(tr("Failed to connect to host: ") + QString( static_cast<QCtrlThreadConnectionFailed*>(event)->reason().c_str()  ));
        return true;
    } else if (event->type() == (QEvent::Type)(QEvent::User+CTRL_THREAD_STATUS_CONNECTION_ESTABLISHED)) {
        ui->statusBar->setStyleSheet("color: green");
        this->statusBar()->showMessage(tr("Connection established."));
        this->ui->connect_btn->setText("Disconnect");
        this->ui->new_downstream_mapping_btn->setEnabled(true);
        this->ui->new_upstream_mapping_btn->setEnabled(true);
        return true;
    }


    return QMainWindow::event(event);
}




void MainWindow::on_pushButton_3_clicked()
{

}

std::vector<Stream_Mapping> MainWindow::get_valid_downstream_mappings(){
 std::vector<Stream_Mapping> r;
 for(int i = 0; i != ui->downstream_tabs->count();++i ){
  auto rr = ((StreamMapping*)(ui->downstream_tabs->widget(i)))->get_stream_mapping();
  if (rr.first.empty() || rr.second.empty()) continue;
  r.push_back(rr);
 }
 return r;
}

void MainWindow::update_status_apply_btn(){
 ui->apply_mappings_btn->setEnabled(get_valid_downstream_mappings().size());
}

void MainWindow::mapping_selection_changed(bool){
  update_status_apply_btn();
}


void MainWindow::on_connect_btn_clicked()
{
    if(ui->connect_btn->text() == "Connect"){
     this->statusBar()->showMessage(tr("Connecting..."));
     current_core.first = this->ui->line_edit_host->text().toStdString();
     current_core.second = this->ui->line_edit_port->text().toStdString();
     auto thrd_info = std::shared_ptr<ctrl_thread_info>(new ctrl_thread_info{});
     thrd_info->ctrl_thread = new std::thread{ctrl_thread_fn,app_core,this,current_core,thrd_info};
    }
}

void MainWindow::on_new_downstream_mapping_btn_clicked()
{
    static int counter = 0;
    if (ui->downstream_tabs->count() == 0) counter = 0;
    std::vector< std::pair<Remote_Interface,Remote_Interface_Type> > remote_interfaces;
    {
        std::lock_guard<std::mutex> lg(global_mutex);
        for(auto e : info_out_channels[current_core])
            remote_interfaces.push_back(e);
    }
    ui->downstream_tabs->insertTab(ui->downstream_tabs->count(),
                                   new StreamMapping(remote_interfaces,this,&MapSelectionEventConsumer::mapping_selection_changed,this),
                                   std::to_string(++counter).c_str());
    ui->delete_downstream_mapping_btn->setEnabled(true);
}

void MainWindow::on_delete_downstream_mapping_btn_clicked()
{
 ui->downstream_tabs->removeTab( ui->downstream_tabs->currentIndex());
 ui->delete_downstream_mapping_btn->setEnabled(ui->downstream_tabs->count());
 update_status_apply_btn();
}



void MainWindow::on_apply_mappings_btn_clicked()
{
 auto r = get_valid_downstream_mappings();
 std::set<Stream_Mapping> done;
 std::vector<int> local_sockets;
 std::vector<int> extended_can_info;
 std::vector<std::string> remote_interface;

 for(size_t i = 0; i!= r.size();++i){
     auto s = r[i];
     if (done.find(s) != done.end()) continue;
     done.insert(s);

     bool extended_can = false;
     {
      std::lock_guard<std::mutex> lg(global_mutex);
      for( auto ss : info_out_channels[current_core])
         if (ss.first == s.second) {
             extended_can = ss.second == "CANX";break;
         }
     }
     extended_can_info.push_back(extended_can);

     //local_sockets.push_back(-1);
     remote_interface.push_back(s.second);
     int sck = -1;// local_sockets[local_sockets.size()-1];

     struct sockaddr_can addr;
     struct ifreq ifr;
     if((sck = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
         auto err = errno;
         for(size_t j = 0; j!=local_sockets.size();++j) ::close(local_sockets[j]);
         ui->statusBar->setStyleSheet("color: red");
         ui->statusBar->showMessage(strerror(err));
         return;
     }
     strcpy(ifr.ifr_name, s.first.c_str());
     ioctl(sck, SIOCGIFINDEX, &ifr);
     addr.can_family  = AF_CAN;
     addr.can_ifindex = ifr.ifr_ifindex;

     if(bind(sck, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
         auto err = errno;
         for(size_t j = 0; j!=local_sockets.size();++j) ::close(local_sockets[j]);
         ui->statusBar->setStyleSheet("color: red");
         ui->statusBar->showMessage(strerror(err));
         return;
     }
     local_sockets.push_back(sck);
 }

 for(size_t i = 0; i != local_sockets.size(); ++i){
     auto t = std::shared_ptr<gateway_thread_info>{ new gateway_thread_info() };
     //std::cout << extended_can_info[i] << std::endl;
     t->gateway_thread = new std::thread{gateway_fn,
                                         this,
                                         current_core,
                                         t,
                                         local_sockets[i],
                                         extended_can_info[i],
                                         remote_interface[i]};
     downstream_threads[current_core].push_back(t);
 }

 ui->statusBar->setStyleSheet("color: green");
 ui->statusBar->showMessage("Local communication structures successfully allocated.Wiring up with remote channels...");


}















