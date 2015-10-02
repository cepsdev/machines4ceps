#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "core/include/state_machine_simulation_core.hpp"
extern State_machine_simulation_core* sm_core;


MainWindow* wnd_inst = nullptr;

void event_callback(State_machine_simulation_core::event_t ev){
 if (ev.id_ != "D") return;
 if (ev.payload_.size() == 0 || ev.payload_[0]->kind() != ceps::ast::Ast_node_kind::int_literal) return;
 int v = ceps::ast::value(ceps::ast::as_int_ref(ev.payload_[0]));
 QMetaObject::invokeMethod(wnd_inst, "process_event", Qt::QueuedConnection,Q_ARG(int,v));
}



MainWindow::MainWindow(QWidget *parent) :
  QMainWindow(parent),
  ui(new Ui::MainWindow)
{
  wnd_inst = this;
  sm_core->set_global_event_call_back(event_callback);
  ui->setupUi(this);
}

MainWindow::~MainWindow()
{
  delete ui;
}

void MainWindow::process_event(int data){
 ui->lineEdit->setText(QString::number(data));
}

void MainWindow::on_verticalSlider_valueChanged(int value)
{
  State_machine_simulation_core::event_t ev("E");
  ev.payload_.push_back(new ceps::ast::Int(value,ceps::ast::all_zero_unit()));
  sm_core->main_event_queue().push(ev);
  ui->lineEdit_2->setText(QString::number(value));
}
