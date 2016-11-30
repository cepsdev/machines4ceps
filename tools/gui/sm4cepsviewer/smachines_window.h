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
    bool& highlight_currently_selected_states() {return highlight_currently_selected_states_;}
    std::set<int>& highlighted_states() {return highlighted_states_;}
    void highlight_states(std::set<int> const &);
public slots:
    void itemChangedinUnderlyingSMSelection(StandardItemSM*);
private:
    State_machine_simulation_core* assoc_smcore_;
    ModelSM* sm_model_;
    SvgView* svgview_;
    std::set<State_machine*> sm_selection_;
    bool highlight_currently_selected_states_ = true;
    std::set<int> highlighted_states_;
    std::map<std::string,State_machine*> displayed_sms_;
};

#endif
