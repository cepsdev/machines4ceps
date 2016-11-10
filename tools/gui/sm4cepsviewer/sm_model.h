#ifndef SM_MODEL_H
#define SM_MODEL_H

#include "common.h"
#include <QStandardItem>
#include <vector>
#include <QStandardItemModel>

class StandardItemSM : public QStandardItem
{
 public:
  explicit StandardItemSM(State_machine* , State_machine::State*);
    State_machine* associated_sm(){return sm;}
    State_machine::State* associated_state(){return state;}
    void setData(const QVariant &value, int role) override;
 private:
    std::vector<StandardItemSM*> children;
    State_machine* sm;
    State_machine::State* state;
};

class ModelSM: public QStandardItemModel{
  Q_OBJECT
 private:
  void initialize();
  void load();
  void populate_item(StandardItemSM* , State_machine* );
 public:
  explicit ModelSM(State_machine_simulation_core* smc, QObject * parent = 0);
  void item_checked_state_changed(StandardItemSM* );
signals:
  void item_checkstate_changed(StandardItemSM*);
 private:
    State_machine_simulation_core* smcore = nullptr;
};
#endif // SM_MODEL_H
