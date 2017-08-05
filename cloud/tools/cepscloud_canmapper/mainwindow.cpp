#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "common.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);


}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_3_clicked()
{

}

void MainWindow::on_connect_btn_clicked()
{
    current_core.first = this->ui->line_edit_host->text().toStdString();
    current_core.second = this->ui->line_edit_port->text().toStdString();
    auto thrd_info = std::shared_ptr<ctrl_thread_info>(new ctrl_thread_info{});
    thrd_info->ctrl_thread = new std::thread{ctrl_thread_fn,thrd_info};
}
