#include "streammapping.h"
#include "common.h"
#include <QVBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QRadioButton>
class QMainWindow;

StreamMapping::StreamMapping(std::vector< std::pair< Remote_Interface, Remote_Interface_Type> > ri,MapSelectionEventConsumer* trgt, void (MapSelectionEventConsumer::*cbk)(bool) , QWidget *parent) : QWidget(parent)
{
 using namespace std;
 remote_interfaces = ri;
 auto main_layout = new QHBoxLayout;
 auto left_groupbox = new QGroupBox;
 auto right_groupbox = new QGroupBox;
 auto inner_label = new QLabel(">");
 clbk_target = trgt;
 clbk = cbk;

 {
  auto layout = new QVBoxLayout;
  for(auto const & e : remote_interfaces){
     auto rb = new QRadioButton( (e.first +" ("+e.second+")").c_str() ,this);
     left_selection.push_back(rb);
     layout->addWidget(rb);
     connect( rb, SIGNAL( toggled(bool) ), this, SLOT( mapping_selection_changed(bool) ) );
  }
  left_groupbox->setLayout(layout);
 }
 {
  auto layout = new QVBoxLayout;
  for(auto const & e : sys_info_available_ifs){
     auto rb = new QRadioButton(e.c_str(),this);
     right_selection.push_back(rb);
     layout->addWidget(rb);
     connect( rb, SIGNAL( toggled(bool) ), this, SLOT( mapping_selection_changed(bool) ) );
  }
  right_groupbox->setLayout(layout);
 }

 inner_label->setAlignment(Qt::AlignHCenter | Qt::AlignCenter);
 inner_label->setStyleSheet("font-style: normal;font-size: 24pt;font-weight: bold;");
 {
   main_layout->addWidget(left_groupbox);
   main_layout->addWidget(inner_label);
   main_layout->addWidget(right_groupbox);
 }
 setLayout(main_layout);
}

Stream_Mapping StreamMapping::get_stream_mapping(){
  Stream_Mapping r;
  ssize_t left_select = -1;
  ssize_t right_select = -1;
  for(size_t i = 0; i != left_selection.size();++i) if (left_selection[i]->isChecked()) {left_select = i;break;}
  if (left_select < 0) return r;
  for(size_t i = 0; i != right_selection.size();++i) if (right_selection[i]->isChecked()) {right_select=i;break;}
  if (right_select < 0) return r;
  return std::make_pair(Local_Interface{sys_info_available_ifs[right_select]}, Remote_Interface{remote_interfaces[left_select].first});
}

void StreamMapping::mapping_selection_changed(bool f){
 (clbk_target->*clbk) (f);
}
