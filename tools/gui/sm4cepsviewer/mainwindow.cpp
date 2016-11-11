#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "svgview.h"

#include <QTimer>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTreeView>
#include <QSplitter>
#include "sm_model.h"
#include "graphviz/gvc.h"
#include <stdlib.h>
#include <QPushButton>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    m_view(new SvgView),
    m_treeview(new QTreeView)
{

 m_selections_tab = new QTabWidget();

 m_treeview->setAllColumnsShowFocus(true);
 m_treeview->setModel(m_sm_treeview_model  = new ModelSM(smcore,this));
 connect(m_treeview,SIGNAL(clicked(const QModelIndex &)), this, SLOT(sm_treeview_clicked(const QModelIndex &)) );
 connect(m_sm_treeview_model,SIGNAL(item_checkstate_changed(StandardItemSM*)),this,SLOT(item_checkstate_changed(StandardItemSM*)));

 m_selections_tab->addTab(m_treeview,"" );
 m_selections_tab->addTab(new QWidget,"+" );
 std::cout << m_selections_tab->count() << std::endl;

 connect(m_selections_tab,SIGNAL(currentChanged(int)),this,SLOT(sm_sel_tab_currentChanged(int)),Qt::QueuedConnection);
std::cout << m_selections_tab->count() << std::endl;

 auto mainSplitter = new QSplitter(Qt::Horizontal);
 mainSplitter->addWidget(/*m_treeview*/m_selections_tab);
 mainSplitter->addWidget(m_view);
 mainSplitter->setStretchFactor(1, 1);
 setCentralWidget(mainSplitter);

}


void MainWindow::sm_sel_tab_currentChanged(int index){
 if (index == m_selections_tab->count()-1){
     m_selections_tab->setTabText(index,"");
 }
}

void MainWindow::item_checkstate_changed(StandardItemSM* item){
    std::cout << item->associated_sm()->id() << std::endl;
    if (item->checkState()) sm_selection.insert(item->associated_sm());
    else sm_selection.erase(item->associated_sm());
    {
      std::ofstream of{"out.dot"};
      std::map<std::string,State_machine*> m;
      //m.insert(std::make_pair(item->associated_sm()->id(),item->associated_sm()));
      std::map<State_machine*,bool> dominated;
      for(auto s: sm_selection){
       dominated[s] = false;
       if(s->parent())for(auto p = s->parent();p;p=p->parent()){
        if(sm_selection.find(p) != sm_selection.end()){dominated[s] = true;break;}
       }
      }
      for(auto s : dominated) if (!s.second)m[s.first->id()] = s.first;
      smcore->do_generate_dot_code(m,&sm_selection,of);
      of.flush();
    }
    system("dot -T svg -o out.svg out.dot");
    m_view->reload();
}

void MainWindow::sm_treeview_clicked(const QModelIndex &index){


}

MainWindow::~MainWindow()
{

}
