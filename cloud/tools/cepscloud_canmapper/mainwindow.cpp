#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "common.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);


    for(auto e : available_ifs){
        QListWidgetItem* t;
        std::cout << e << std::endl;
        ui->listwidget_local_ifs_downstream->addItem(t = new QListWidgetItem(e.c_str()));
        t->setCheckState(Qt::Unchecked);
        ui->listwidget_local_ifs_upstream->addItem(t = new QListWidgetItem(e.c_str()));
        t->setCheckState(Qt::Unchecked);

    }
}

MainWindow::~MainWindow()
{
    delete ui;
}
