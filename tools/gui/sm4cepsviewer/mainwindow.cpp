#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "svgview.h"

#include <QTimer>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTreeView>
#include <QSplitter>
#include "sm_model.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    m_view(new SvgView),
    m_treeview(new QTreeView)
{
 auto mainSplitter = new QSplitter(Qt::Horizontal);
 mainSplitter->addWidget(m_view);
 mainSplitter->addWidget(m_treeview);
 mainSplitter->setStretchFactor(1, 1);
 setCentralWidget(mainSplitter);

 m_treeview->setAllColumnsShowFocus(true);
 m_treeview->setModel(new ModelSM(smcore,this));

 /*QTimer *timer = new QTimer(this);
 connect(timer, SIGNAL(timeout()), m_view, SLOT(reload()));
 timer->start(0);*/
}

MainWindow::~MainWindow()
{

}
