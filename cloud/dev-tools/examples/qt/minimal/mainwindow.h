#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "cepscloud_streaming_common.h"

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
    void update_frame(int id, int len, char ch0, char ch1, char ch2, char ch3 , char ch4, char ch5, char ch6, char ch7);

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
