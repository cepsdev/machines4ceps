#ifndef SMACHINES_WINDOW_H
#define SMACHINES_WINDOW_H

#include <QMdiSubWindow>
#include <QWidget>
#include "mdibase.h"
#include "common.h"
#include <string>

class ModelSM;
class SvgView;
class QStandardItem;
class StandardItemSM;

class Statemachine_window:  public Mdibase {
    Q_OBJECT
    std::string get_temporary_filename();
public:
    Statemachine_window(State_machine_simulation_core* assoc_smcore,ModelSM* sm_model, QWidget * parent = 0, Qt::WindowFlags flags = 0);
public slots:
    void itemChangedinUnderlyingSMSelection(StandardItemSM*);
private:
    State_machine_simulation_core* assoc_smcore_;
    ModelSM* sm_model_;
    SvgView* svgview_;
    std::set<State_machine*> sm_selection_;
};

#endif
