#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include "common.h"
#include <QMainWindow>
class QCoreApplication;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow, public MapSelectionEventConsumer
{
    Q_OBJECT

public:
    explicit MainWindow(QCoreApplication* app,QWidget *parent = 0);
    ~MainWindow();
    bool event(QEvent *event);
    void mapping_selection_changed(bool);
private slots:
    void on_pushButton_3_clicked();

    void on_connect_btn_clicked();

    void on_new_downstream_mapping_btn_clicked();

    void on_delete_downstream_mapping_btn_clicked();
    void on_apply_mappings_btn_clicked();

public:
    std::vector<Stream_Mapping> get_valid_downstream_mappings();
    void update_status_apply_btn();

private:
    Ui::MainWindow *ui;
    QCoreApplication* app_core;
};

#endif // MAINWINDOW_H
