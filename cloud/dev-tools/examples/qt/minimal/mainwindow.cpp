#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>

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

void MainWindow::update_frame(int id, int len, char ch0, char ch1, char ch2, char ch3 , char ch4, char ch5, char ch6, char ch7){
 ui->lcdNumber->display(id);
 ui->lcdNumber_2->display(ch0);
 ui->lcdNumber_3->display(ch1);
 ui->lcdNumber_4->display(ch2);
 ui->lcdNumber_5->display(ch3);
 ui->lcdNumber_6->display(ch4);
 ui->lcdNumber_7->display(ch5);
 ui->lcdNumber_8->display(ch6);
 ui->lcdNumber_9->display(ch7);
}
