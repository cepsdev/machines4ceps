/**
 The MIT License (MIT)

Copyright (c) 2015 The authors of ceps

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
 **/


#include "simcoreconnectdlg.h"
#include "ui_simcoreconnectdlg.h"
#include <vector>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/types.h>
#include <limits>
#include <cstring>
#include <map>
#include <algorithm>
#include <set>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <iostream>
#include <QTimer>


extern int controlled_thread_id;
extern int running_thread_id;
extern std::mutex comm_threads_m;
extern std::vector<std::thread*> comm_threads;
extern std::vector<bool> comm_threads_stop_signals;
extern std::vector<int> comm_threads_status;
extern const int COMM_STATUS_CONNECTING ;
extern const int COMM_STATUS_CONNECTED;
extern const int COMM_STATUS_TERMINATED;
extern const int COMM_STATUS_ERR_ALLOC_SOCKET_FAILED;
extern const int COMM_STATUS_ERR_GETADDRINFO_FAILED;
extern void run_as_monitor(int id, std::string ip, std::string port);
extern std::string current_ip,current_port,current_node_name;



SimCoreConnectDlg::SimCoreConnectDlg(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::SimCoreConnectDlg)
{
  QTimer *timer = new QTimer(this);
  connect(timer,SIGNAL(timeout()),this,SLOT(check_comm_thread()));
  timer->start(100);

  ui->setupUi(this);
}

SimCoreConnectDlg::~SimCoreConnectDlg()
{
  delete ui;
}
void SimCoreConnectDlg::check_comm_thread()
{
  //std::cout << "TIMER!" << controlled_thread_id  << std::endl;
  if (controlled_thread_id < 0) return;

  int status = COMM_STATUS_CONNECTING;
  {
   std::lock_guard<std::mutex> g(comm_threads_m);
   status = comm_threads_status[controlled_thread_id];
  }
  if (status == COMM_STATUS_CONNECTING)
   this->ui->comm_status_label->setText("Connecting...");
  else if (status == COMM_STATUS_CONNECTED)
  {
   this->ui->comm_status_label->setText("Connection established.");
   running_thread_id = controlled_thread_id;
   controlled_thread_id = -1;
   current_ip = this->ui->lineEdit->text().toStdString();
   current_port = this->ui->lineEdit_2->text().toStdString();
   accept();
  } else
  {this->ui->comm_status_label->setText("Failed to connect.");controlled_thread_id = -1;this->ui->pushButton->setEnabled(true); }
}

void SimCoreConnectDlg::on_lineEdit_textChanged(const QString &arg1)
{
    if (arg1.length() == 0) this->ui->pushButton->setEnabled(false);
    else this->ui->pushButton->setEnabled(true);
}

void SimCoreConnectDlg::on_pushButton_clicked()
{


  controlled_thread_id = comm_threads.size();
  {
   std::lock_guard<std::mutex> g(comm_threads_m);
   comm_threads_stop_signals.push_back(false);
   comm_threads_status.push_back(COMM_STATUS_CONNECTING);
   if (running_thread_id >= 0) comm_threads_stop_signals[running_thread_id] = true;
   running_thread_id = -1;
  }
  comm_threads.push_back(new std::thread(run_as_monitor,
                                         (int)comm_threads.size(),
                                         this->ui->lineEdit->text().toStdString(),
                                         this->ui->lineEdit_2->text().toStdString() ) );
  this->ui->pushButton->setEnabled(false);
}

void SimCoreConnectDlg::on_lineEdit_2_textChanged(const QString &arg1)
{
    if (arg1.length() == 0) this->ui->pushButton->setEnabled(false);
    else this->ui->pushButton->setEnabled(true);
}

void SimCoreConnectDlg::on_pushButton_2_clicked()
{
 {
   std::lock_guard<std::mutex> g(comm_threads_m);
   if (controlled_thread_id >= 0) comm_threads_stop_signals[controlled_thread_id] = true;
 }
 reject();
}
