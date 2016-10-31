#include "render_statemachines.h"

#include <QGraphicsTextItem>
#include <QGraphicsRectItem>



Agraph_t* sm4ceps::Render_statemachine_context::make_cgraph(State_machine* sm,std::string fqn){
 auto g = agopen((char*)fqn.c_str(),Agdirected,nullptr);
 std::map<std::string,Agnode_t *> name2node;
 auto get_node = [&] (std::string name)
  { if (name2node.find(name) == name2node.end()){name2node[name]=agnode(g,(char*)name.c_str(),1);} return name2node[name]; };

 for(State_machine::Transition & t : sm->transitions()){
     if (t.from().is_sm() || t.to().is_sm()) continue;
     Agnode_t * from = get_node(t.from().id());
     Agnode_t * to = get_node(t.to().id());
     agedge(g,from,to,nullptr,1);
 }
 return g;
}

void sm4ceps::Render_statemachine_context::make_scene_representation(QGraphicsScene*,Agraph_t*){

}

sm4ceps::Render_statemachine_context::Render_statemachine_context():gvc(gvContext()){

}

void sm4ceps::Render_statemachine_context::add(State_machine*sm,std::string fqn){
    if (sm2cgraph.find(sm) == sm2cgraph.end()) sm2cgraph[sm] = make_cgraph(sm,fqn);
    auto cgraph = sm2cgraph[sm];
}

void sm4ceps::Render_statemachine_context::layout(QGraphicsScene*sc,State_machine*sm){
    auto cgraph = sm2cgraph[sm];
    if (cgraph == nullptr) return;
    auto r = gvLayout(gvc, cgraph, "dot");
    gvRender (gvc, cgraph, "dot", nullptr);
    //drawGraph(cgraph);
    for (Agnode_t * n = agfstnode(cgraph); n; n = agnxtnode(cgraph,n)) {
        std::cout << ND_label(n)->text << std::endl;
        std::cout << ND_bb(n).LL.x << ",";
        std::cout << ND_bb(n).LL.y << ",";
        std::cout << ND_bb(n).UR.x << ",";
        std::cout << ND_bb(n).UR.y << std::endl;
        QGraphicsRectItem* inserted_rect = sc->addRect(ND_bb(n).LL.x,ND_bb(n).UR.y,ND_bb(n).UR.x-ND_bb(n).LL.x,ND_bb(n).LL.y - ND_bb(n).UR.y );
        auto t = new QGraphicsTextItem();
        t->setPlainText(ND_label(n)->text);
        t->setX(ND_bb(n).LL.x);
        t->setY(ND_bb(n).LL.y);
        sc->addItem(t);
        for (Agedge_t * e = agfstout(cgraph,n); e; e = agnxtout(cgraph,e)) {
         std::cout << "edge:" << std::endl;
         std::cout << ED_spl(e)->size << std::endl;
         if (ED_spl(e)->size == 0) continue;
         std::cout <<"\t"<< ED_spl(e)->list[0].ep.x << std::endl;
         std::cout <<"\t"<< ED_spl(e)->list[0].ep.y << std::endl;
         std::cout <<"\t"<< ED_spl(e)->list[0].sp.x << std::endl;
         std::cout <<"\t"<< ED_spl(e)->list[0].sp.y << std::endl;
         std::cout << "\t"<< ED_spl(e)->list[0].size << std::endl;
         std::cout << "\t"<< ED_spl(e)->list[0].list[0].x << std::endl;
         std::cout << "\t"<< ED_spl(e)->list[0].list[0].y << std::endl;
         std::cout << "\t"<< ED_spl(e)->list[0].list[1].x << std::endl;
         std::cout << "\t"<< ED_spl(e)->list[0].list[1].y << std::endl;
         std::cout << "\t"<< ED_spl(e)->list[0].list[2].x << std::endl;
         std::cout << "\t"<< ED_spl(e)->list[0].list[2].y << std::endl;
         std::cout << "\t"<< ED_spl(e)->list[0].list[3].x << std::endl;
         std::cout << "\t"<< ED_spl(e)->list[0].list[3].y << std::endl;

         QPainterPath path;
         path.moveTo(ED_spl(e)->list[0].list[0].x, ED_spl(e)->list[0].list[0].y);
         path.cubicTo(ED_spl(e)->list[0].list[1].x, ED_spl(e)->list[0].list[1].y,
                      ED_spl(e)->list[0].list[2].x, ED_spl(e)->list[0].list[2].y,
                      ED_spl(e)->list[0].list[3].x, ED_spl(e)->list[0].list[3].y);


         QGraphicsPathItem* pp = new QGraphicsPathItem;
         pp->setPath(path);

         sc->addItem(pp);

        }
    }
}
