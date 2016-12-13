#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "render_statemachines.h"
#include <QBrush>
#include <QColor>

MainWindow::MainWindow(State_machine_simulation_core* smcore, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    sm_render_ctxt = new sm4ceps::Render_statemachine_context{};
    QGraphicsScene* scene = new QGraphicsScene();
    for(auto & sm :State_machine::statemachines)
        if (sm.second->parent() == nullptr ){
            sm_render_ctxt->add(sm.second,sm.first);
            sm_render_ctxt->layout(scene,sm.second);
        }
    //scene->setForegroundBrush(QBrush(Qt::lightGray, Qt::Dense4Pattern));
    //scene->setForegroundBrush(QColor(255, 255, 255, 127));
    ui->graphicsView->setRenderHints(QPainter::Antialiasing
    | QPainter::TextAntialiasing);
    ui->graphicsView->setScene(scene);
}

MainWindow::~MainWindow()
{
    delete ui;
}
