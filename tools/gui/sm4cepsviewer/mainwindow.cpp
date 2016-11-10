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
 m_treeview->setModel(m_sm_treeview_model  = new ModelSM(smcore,this));
 connect(m_treeview,SIGNAL(clicked(const QModelIndex &)), this, SLOT(sm_treeview_clicked(const QModelIndex &)) );
 connect(m_sm_treeview_model,SIGNAL(item_checkstate_changed(StandardItemSM*)),this,SLOT(item_checkstate_changed(StandardItemSM*)));
 //m_sm_treeview_model->register_mainwnd(this);

 /*QTimer *timer = new QTimer(this);
 connect(timer, SIGNAL(timeout()), m_view, SLOT(reload()));
 timer->start(0);*/
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

 /*StandardItemSM* item = (StandardItemSM*)m_sm_treeview_model->itemFromIndex(index);
 {
   std::ofstream of{"out.dot"};
   std::map<std::string,State_machine*> m;m.insert(std::make_pair(item->associated_sm()->id(),item->associated_sm()));
   smcore->do_generate_dot_code(m,of);
   of.flush();
 }
 system("dot -T svg -o out.svg out.dot");
 m_view->reload();
*/

 /*Agraph_t *g, *prev = NULL;
 GVC_t *gvc;
 gvc = gvContext();
 int argc = 6;
 char* v[] = {"dot","-T","svg","-o","out.svg","out.dot"};

 gvParseArgs(gvc, argc, v);
 while ((g = gvNextInputGraph(gvc))) {
  if (prev) {
   gvFreeLayout(gvc, prev);
   agclose(prev);
  }
  gvLayoutJobs(gvc, g);
  gvRenderJobs(gvc, g);
  prev = g;
 }
 gvFreeContext(gvc);*/
}

MainWindow::~MainWindow()
{

}
