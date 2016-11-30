#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QModelIndex>
#include <QItemSelection>
#include <set>
#include "common.h"
#include "sm4ceps_livelog_treemodel.h"

class ModelSM ;
class StandardItemSM;
class QTabWidget;
class QMdiArea;
class QLabel;


namespace Ui {
class MainWindow;
}

class SvgView;
class QTreeView;
class QMdiSubWindow;
class QAction;
class QStackedLayout;
class Statemachine_window;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(State_machine_simulation_core* assoc_smcore,QWidget *parent = 0);
    ~MainWindow();

public slots:
    void subWindowActivated(QMdiSubWindow*);
    void sm_explorer_new_selection();
    void sm_explorer_remove_selection();
    void logview_elem_activated(const QModelIndex&);
    void logview_elem_clicked(const QModelIndex&);
    void logview_selection_changed(const QItemSelection &, const QItemSelection &);
    void logview_first_elem();
    void logview_play_resume_elem();
    void logview_last_elem();
    void logview_filter_elem();

    void sm_treeview_clicked(const QModelIndex &index);
    void item_checkstate_changed(StandardItemSM*);
    void sm_sel_tab_currentChanged(int index);
private:
    void setup_dock_widgets();
    void setup_dock_widget_cur_sm_sel();
private:
    State_machine_simulation_core* smcore_;
    QMdiArea* mdi_area_;
    QDockWidget* dock_widget_current_sm_selection_;
    QDockWidget* dock_widget_simulation_ctrl_;
    QDockWidget* dock_widget_logger_ctrl_;
    LivelogTreeModel* logview_model_;
    std::set<Statemachine_window*> active_sm_windows_;
    QTreeView* logview_;

    bool logview_filter_event = true;
    bool logview_filter_info = true;
    bool logview_filter_console = true;
    bool logview_filter_active_states = true;
    bool logview_filter_warning = true;
    bool logview_filter_error = true;
    bool logview_filter_debug = false;
    bool logview_in_stepmode = false;

    QStackedLayout* sm_explorer_treeview_stack_;
    //Actions
    QAction * sm_explorer_new_selection_;
    QAction * sm_explorer_remove_current_selection_;
    QAction * simulation_ctrl_start_sim_;
    QAction * simulation_ctrl_start_all_sims_;
    QAction * logview_ctrl_first_element_;
    QAction * logview_ctrl_last_element_;
    QAction * logview_ctrl_play_resume_;
    QAction * logview_ctrl_filter_;

    //Simulation Control
    QLabel* simulation_ctrl_label_runs_;
    QLabel* simulation_ctrl_errors_;
    QLabel* simulation_ctrl_failures_;



    Ui::MainWindow *ui;
    SvgView * m_view;
    QTreeView * m_treeview;
    ModelSM * m_sm_treeview_model;

    QTabWidget* m_selections_tab;
};

#endif // MAINWINDOW_H
