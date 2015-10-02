#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
  class MainWindow;
}

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = 0);
  ~MainWindow();

private slots:
  void on_actionConnect_triggered(bool checked);
  void check_comm_thread();

  void on_actionStop_update_triggered();

  void on_actionStart_update_triggered();

private:
  Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
