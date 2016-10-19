#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "sm4ceps_livelog_treemodel.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->treeView->setModel(new LivelogTreeModel);
}

MainWindow::~MainWindow()
{
    delete ui;
}
