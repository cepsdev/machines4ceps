#include "smachines_window.h"
#include "sm_model.h"
#include "svgview.h"


Statemachine_window::Statemachine_window(State_machine_simulation_core* assoc_smcore,ModelSM* sm_model,QWidget * parent, Qt::WindowFlags flags):Mdibase(parent,flags){
 assoc_smcore_ = assoc_smcore;
 sm_model_ = sm_model;
 setWidget(svgview_ = new SvgView());
 connect(sm_model_,SIGNAL(item_checkstate_changed(StandardItemSM*)),this,SLOT(itemChangedinUnderlyingSMSelection(StandardItemSM*)));
}

std::string Statemachine_window::get_temporary_filename(){
    return "smwnd_temp"+std::to_string(this_instance_id);
}


void Statemachine_window::itemChangedinUnderlyingSMSelection(StandardItemSM* item){

    if (item->checkState()) sm_selection_.insert(item->associated_sm());
    else sm_selection_.erase(item->associated_sm());

    {
      std::ofstream of{get_temporary_filename()+".dot"};
      std::map<std::string,State_machine*> m;
      std::map<State_machine*,bool> dominated;
      for(auto s: sm_selection_){
       dominated[s] = false;
       if(s->parent())for(auto p = s->parent();p;p=p->parent()){
        if(sm_selection_.find(p) != sm_selection_.end()){dominated[s] = true;break;}
       }
      }
      for(auto s : dominated) if (!s.second)m[s.first->id()] = s.first;
      assoc_smcore_->do_generate_dot_code(m,&sm_selection_,of);
      of.flush();
    }
    system( std::string{"dot -T svg -o "+get_temporary_filename()+"_out.svg"+" "+get_temporary_filename()+".dot"}.c_str());
    svgview_->load_from_file(std::string{get_temporary_filename()+"_out.svg"}.c_str());
}
