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
public slots:
  void process_event(int data);

private slots:
  void on_verticalSlider_valueChanged(int value);

private:
  Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
