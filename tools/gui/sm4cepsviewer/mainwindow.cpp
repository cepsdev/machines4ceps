#include "mainwindow.h"
#include "svgview.h"
#include "smachines_window.h"
#include <QAction>
#include <QTimer>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTreeView>
#include <QSplitter>
#include "sm_model.h"
#include "graphviz/gvc.h"
#include <stdlib.h>
#include <QPushButton>
#include <QToolButton>
#include <QMdiArea>
#include <QDockWidget>
#include <QComboBox>
#include <QStackedLayout>
#include <QGridLayout>
#include <QToolBar>
#include <QLabel>
#include <QProgressBar>
#include <QTextEdit>

MainWindow::MainWindow(State_machine_simulation_core* assoc_smcore,QWidget *parent) :
    QMainWindow(parent),
    m_view(new SvgView),
    m_treeview(new QTreeView)
{
 this->smcore_ = assoc_smcore;
 mdi_area_ = new QMdiArea(this);
 setCentralWidget(mdi_area_);
 connect(mdi_area_, SIGNAL(subWindowActivated(QMdiSubWindow*)), this, SLOT(subWindowActivated(QMdiSubWindow*)));
 setup_dock_widgets();
}

//public slots

void MainWindow::subWindowActivated(QMdiSubWindow* w){

}

void MainWindow::sm_explorer_new_selection(){
    sm_explorer_remove_current_selection_->setDisabled(false);
    QTreeView* t;
    ModelSM* tt;
    Statemachine_window* ttt;
    auto new_treeview_widget_idx = sm_explorer_treeview_stack_->addWidget(t = new QTreeView());
    t->setModel(tt = new ModelSM(smcore_));
    ttt = new Statemachine_window(smcore_,tt);
    mdi_area_->addSubWindow(ttt);
    ttt->show();
    sm_explorer_treeview_stack_->setCurrentWidget(sm_explorer_treeview_stack_->widget(new_treeview_widget_idx));
}

void MainWindow::sm_explorer_remove_selection(){

}

// setup methods
void MainWindow::setup_dock_widgets(){
    setup_dock_widget_cur_sm_sel();
}
void MainWindow::setup_dock_widget_cur_sm_sel(){
    dock_widget_current_sm_selection_ = new QDockWidget(tr("State Chart Explorer"));
    dock_widget_simulation_ctrl_ = new QDockWidget(tr("Simulations"));
    dock_widget_logger_ctrl_ = new QDockWidget;

    dock_widget_current_sm_selection_->setObjectName("state_chart_explorer");
    dock_widget_current_sm_selection_->setAllowedAreas(Qt::LeftDockWidgetArea| Qt::RightDockWidgetArea);
    QVBoxLayout* main_layout = new QVBoxLayout();
    QHBoxLayout* top_layout = new QHBoxLayout();


    //State Machine Explorer
    //Toolbar
    QToolBar* qbar = new QToolBar();

    sm_explorer_new_selection_ = qbar->addAction(QIcon(QPixmap(":/add_sm_selection_icon.png")),"New Selection");
    sm_explorer_remove_current_selection_ = qbar->addAction(QIcon(QPixmap(":/delete_sm_selection_icon.png")),"Remove Selection");
    sm_explorer_remove_current_selection_->setDisabled(true);
    connect(sm_explorer_new_selection_,SIGNAL(triggered()),this,SLOT(sm_explorer_new_selection()));
    connect(sm_explorer_remove_current_selection_,SIGNAL(triggered()),this,SLOT(sm_explorer_remove_selection()));
    qbar->addSeparator();
    top_layout->addWidget(qbar);


    QComboBox* tt;
    top_layout->addWidget(tt = new QComboBox());
    main_layout->addLayout(top_layout);

    QStackedLayout* bottom_layout = sm_explorer_treeview_stack_ = new QStackedLayout();
    main_layout->addLayout(bottom_layout);
    QWidget* t = new QWidget();

    t->setLayout(main_layout);
    dock_widget_current_sm_selection_->setWidget(t);
    addDockWidget(Qt::LeftDockWidgetArea, dock_widget_current_sm_selection_);

    //Simulation Control
    {
     //Toolbar
     QToolBar* qbar = new QToolBar();
     //simulation_ctrl_start_sim_ = qbar->addAction(QIcon(QPixmap(":/add_sm_selection_icon.png")),"Start Selected Simulation");
     simulation_ctrl_start_all_sims_ = qbar->addAction(QIcon(QPixmap(":/simulation_ctrl_play_all.png")),"Start All Simulations");
     QVBoxLayout* main_layout = new QVBoxLayout();
     main_layout->addWidget(qbar);
     QWidget* t = new QWidget();
     QGridLayout* status_layout = new QGridLayout;
     simulation_ctrl_label_runs_ = new QLabel("0");
     simulation_ctrl_errors_ = new QLabel("0");
     simulation_ctrl_failures_ = new QLabel("0");
     QLabel* tl;
     QHBoxLayout* la = new QHBoxLayout;
     la->addWidget(tl = new QLabel("Runs:"));
     la->addWidget(simulation_ctrl_label_runs_);
     QHBoxLayout* lb = new QHBoxLayout;
     lb->addWidget(tl = new QLabel("Errors"));
     lb->addWidget(simulation_ctrl_errors_);
     QHBoxLayout* lc = new QHBoxLayout;
     lc->addWidget(tl = new QLabel("Failures"));
     lc->addWidget(simulation_ctrl_failures_);
     status_layout->addLayout(la,0,0);
     status_layout->addLayout(lb,0,1);
     status_layout->addLayout(lc,0,2);
     main_layout->addLayout(status_layout);
     main_layout->addWidget(new QProgressBar);
     main_layout->addWidget(new QTreeView);
     t->setLayout(main_layout);
     dock_widget_simulation_ctrl_->setWidget(t);
     addDockWidget(Qt::LeftDockWidgetArea, dock_widget_simulation_ctrl_);
    }

    //Logger and others
    {
     QTabWidget* t = new QTabWidget;
     QTreeView* tt;
     t->addTab(tt = new QTreeView, "Log");
     tt->setModel(new LivelogTreeModel(style()));
     //t->addTab(new QTextEdit, "Console");
     dock_widget_logger_ctrl_->setWidget(t);
     addDockWidget(Qt::BottomDockWidgetArea, dock_widget_logger_ctrl_);
    }
}


void MainWindow::sm_sel_tab_currentChanged(int index){

}

void MainWindow::item_checkstate_changed(StandardItemSM* item){
}

void MainWindow::sm_treeview_clicked(const QModelIndex &index){


}

MainWindow::~MainWindow()
{

}
