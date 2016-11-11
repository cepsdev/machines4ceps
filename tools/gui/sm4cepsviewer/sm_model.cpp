#include "sm_model.h"


StandardItemSM::StandardItemSM(State_machine* sm_, State_machine::State* state_)
    :QStandardItem(sm_ != nullptr ? sm_->id().c_str():state_->id().c_str() ),sm(sm_),state(state_)
{
 setCheckable(true);setEditable(false);
}


void StandardItemSM::setData(const QVariant &value, int role){
 QStandardItem::setData(value,role);
 if (role == Qt::CheckStateRole){
   ModelSM* m = (ModelSM*)model();
   if (m == nullptr) return;
   m->item_checked_state_changed(this);
 }
}

ModelSM::ModelSM(State_machine_simulation_core* smc, QObject * parent):
    QStandardItemModel(parent),smcore(smc)
{
 initialize();load();
}

void ModelSM::item_checked_state_changed(StandardItemSM* item)
{
 emit item_checkstate_changed(item);
}

void ModelSM::initialize()
{
 setHorizontalHeaderLabels(QStringList() << tr("Compound State"));
 for (auto column = 1; column < columnCount(); ++column)
         horizontalHeaderItem(column)->setTextAlignment(Qt::AlignVCenter|Qt::AlignRight);
}

void ModelSM::populate_item(StandardItemSM* r, State_machine* s){
    for(auto state: s->states()){
        if (!state->is_sm() || state->smp() == nullptr) continue;
        auto subsm = state->smp();
        auto current_parent_item  = new StandardItemSM(subsm,nullptr);
        r->appendRow(current_parent_item);
        populate_item(current_parent_item,subsm);
    }

    for(auto subsm: s->children()){
        auto current_parent_item  = new StandardItemSM(subsm,nullptr);
        r->appendRow(current_parent_item);
        populate_item(current_parent_item,subsm);
    }
}

void ModelSM::load()
{
    for(std::pair<std::string,State_machine*> sm: State_machine::statemachines){
        if (sm.second->parent() != nullptr) continue;
        auto current_parent_item  = new StandardItemSM(sm.second,nullptr);
        invisibleRootItem()->appendRow(current_parent_item);
        populate_item(current_parent_item,sm.second);
    }
}
