#ifndef RENDER_STATEMACHINES_H
#define RENDER_STATEMACHINES_H

#include "core/include/state_machine_simulation_core.hpp"
#include "core/include/base_defs.hpp"
#include <map>
#include <iostream>
#include <set>
#include <unordered_set>

#include<QGraphicsScene>

#include "graphviz/gvc.h"

namespace sm4ceps{
 class Render_statemachine_context{
   GVC_t *gvc = nullptr;
   std::map<State_machine*,Agraph_t*> sm2cgraph;
   std::map<Agraph_t*,State_machine*> cgraph2sm;
   Agraph_t* make_cgraph(State_machine*,std::string);
   void make_scene_representation(QGraphicsScene*,Agraph_t*);
 public:
   Render_statemachine_context();
   void add(State_machine*,std::string);
   void layout(QGraphicsScene*,State_machine*);
 };
}


#endif // RENDER_STATEMACHINES_H
