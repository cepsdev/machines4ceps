#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QModelIndex>
#include <set>
#include "common.h"

class ModelSM ;
class StandardItemSM;

namespace Ui {
class MainWindow;
}

class SvgView;
class QTreeView;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
    void sm_treeview_clicked(const QModelIndex &index);
    void item_checkstate_changed(StandardItemSM*);
private:
    Ui::MainWindow *ui;
    SvgView * m_view;
    QTreeView * m_treeview;
    ModelSM * m_sm_treeview_model;
    std::set<State_machine*> sm_selection;
};

#endif // MAINWINDOW_H
