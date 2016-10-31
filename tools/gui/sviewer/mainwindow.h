#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
namespace sm4ceps{
 class Render_statemachine_context;
}
class State_machine_simulation_core;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(State_machine_simulation_core* , QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    sm4ceps::Render_statemachine_context* sm_render_ctxt;
};

#endif // MAINWINDOW_H
