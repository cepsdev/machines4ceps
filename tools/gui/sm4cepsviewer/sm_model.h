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
 private:
    std::vector<StandardItemSM*> children;
    State_machine* sm;
    State_machine::State* state;
};

class ModelSM: public QStandardItemModel{
 private:
  void initialize();
  void load();
  void populate_item(StandardItemSM* , State_machine* );
 public:
  explicit ModelSM(State_machine_simulation_core* smc, QObject * parent = 0);
 private:
    State_machine_simulation_core* smcore = nullptr;
};
#endif // SM_MODEL_H
