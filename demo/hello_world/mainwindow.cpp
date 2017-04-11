#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "core/include/base_defs.hpp"
#include "core/include/state_machine.hpp"
#include "core/include/state_machine_simulation_core.hpp"
#include <thread>

extern State_machine_simulation_core* smcore;
sm4ceps::Systemstate<int> Counter{&smcore,"Counter"};

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

void MainWindow::on_pushButton_clicked()
{
 Counter = this->ui->lineEdit->text().toInt();
}

void MainWindow::on_pushButton_2_clicked()
{
 this->ui->lineEdit->setText(std::to_string(Counter.get()).c_str());
}
