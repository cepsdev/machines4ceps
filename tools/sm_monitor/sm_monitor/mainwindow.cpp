#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "ceps_all.hh"
#include "simcoreconnectdlg.h"
#include "ui_simcoreconnectdlg.h"
#include <QTimer>
#include <QStringList>
#include <QTableWidgetItem>
#include <QString>
#include <sstream>
#include <regex>

extern int controlled_thread_id;
extern int running_thread_id;
extern std::mutex comm_threads_m;
extern std::vector<std::thread*> comm_threads;
extern std::vector<bool> comm_threads_stop_signals;
extern std::vector<int> comm_threads_status;
extern const int COMM_STATUS_CONNECTING;
extern const int COMM_STATUS_CONNECTED;
extern const int COMM_STATUS_TERMINATED;
extern const int COMM_STATUS_ERR_ALLOC_SOCKET_FAILED;
extern const int COMM_STATUS_ERR_GETADDRINFO_FAILED;
extern void run_as_monitor(int id, std::string ip, std::string port);

extern std::mutex sysstates_table_m;
extern std::vector<std::pair<std::string,ceps::ast::Nodebase_ptr>> sysstates_table;

std::vector<std::pair<QTableWidgetItem*,QTableWidgetItem*>> table_entries;

std::string current_ip,current_port,current_node_name;

bool update_data = true;
bool cmd_changed = false;


QStringList table_header{"Systemstate","(Type Value [SI Unit])"};


std::string current_filter;
std::string send_command;
std::mutex send_command_mtx;

MainWindow::MainWindow(QWidget *parent) :
  QMainWindow(parent),
  ui(new Ui::MainWindow)
{
  QTimer *timer = new QTimer(this);
  connect(timer,SIGNAL(timeout()),this,SLOT(check_comm_thread()));

  timer->start(100);
  ui->setupUi(this);
  this->ui->tableWidget->setRowCount(1);
  this->ui->tableWidget->setColumnCount(2);
  this->ui->tableWidget->setHorizontalHeaderLabels(table_header);
  this->ui->tableWidget->setColumnWidth(0,this->width()*0.92*1.0/4.0);
  this->ui->tableWidget->setColumnWidth(1,this->width()*0.92*3.0/4.0);
  connect(this->ui->lineEdit, static_cast<void(QComboBox::*)(int)>(&QComboBox::activated),
    [=](int index){ cmd_changed=true;});
  connect(this->ui->lineEdit, static_cast<void(QComboBox::*)(const QString &)>(&QComboBox::activated),
    [=](const QString &text){ current_filter = text.toStdString(); });
}

MainWindow::~MainWindow()
{

  delete ui;
}

void MainWindow::check_comm_thread()
{

 int status = COMM_STATUS_CONNECTED;
 if (running_thread_id<0) status = COMM_STATUS_TERMINATED;

 if (running_thread_id>=0){
   std::lock_guard<std::mutex> g(comm_threads_m);
   status = comm_threads_status[running_thread_id];
  }

 if (running_thread_id < 0 || status != COMM_STATUS_CONNECTED)
 {
  running_thread_id = -1;
  this->ui->actionConnect->setIcon(QIcon(":/disconnect_btn.jpg"));
  this->setWindowTitle("sm4ceps Monitoring Tool (disconnected)");
  this->ui->actionStop_update->setEnabled(false);
  this->ui->actionStart_update->setEnabled(false);
  return;
 }
 this->ui->actionStop_update->setEnabled(update_data);
 this->ui->actionStart_update->setEnabled(!update_data);
 std::string t = "sm4ceps Monitoring Tool ("+current_ip+"@"+current_port+")";
 this->setWindowTitle(t.c_str());

 this->ui->actionConnect->setIcon(QIcon(":/connect_btn.jpg"));


 if (!update_data) return;

 std::lock_guard<std::mutex> g(sysstates_table_m);

 this->ui->tableWidget->setRowCount(sysstates_table.size());
 for(auto & e : table_entries) {delete e.first; delete e.second;}
 table_entries.clear();
 std::string filter =  current_filter;
 std::regex pattern;
 bool is_assignment;

 if (filter.length())
 {
  auto n = filter.find_first_of("=");
  is_assignment = (n != std::string::npos && cmd_changed);
  auto t = filter.substr(0,n);
  n = t.find_first_not_of(' ');
  t= t.substr(n);
  n = t.find_first_of(' ');
  t = t.substr(0,n);

  pattern = std::regex(t);
 }

 if (is_assignment) {
  std::lock_guard<std::mutex> g(send_command_mtx);
  cmd_changed = false;
  send_command=filter;
}



 for(auto & e : sysstates_table){
    std::stringstream ss;
    ss << *e.second;

    if (filter.length()){
       std::smatch m;
       if (!std::regex_search (e.first,m,pattern) && !std::regex_search (ss.str(),m,pattern) ) continue;
    }


    table_entries.push_back(std::make_pair(new QTableWidgetItem(QString(e.first.c_str())),new QTableWidgetItem( QString( ss.str().c_str()) )));
 }

 for(size_t i = 0;i < table_entries.size();++i){
  this->ui->tableWidget->setItem(i,0,table_entries[i].first);

  this->ui->tableWidget->setItem(i,1,table_entries[i].second);
  }
}
void MainWindow::on_actionConnect_triggered(bool checked)
{
    auto dlg = new SimCoreConnectDlg(this);
    dlg->exec();
    delete dlg;
    if (running_thread_id>=0)
    {
     this->ui->actionConnect->setIcon(QIcon(":/connect_btn.jpg"));
    }
}

void MainWindow::on_actionStop_update_triggered()
{
    update_data = false;
}

void MainWindow::on_actionStart_update_triggered()
{
    update_data = true;
}
