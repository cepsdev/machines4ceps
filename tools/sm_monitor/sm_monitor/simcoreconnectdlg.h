#ifndef SIMCORECONNECTDLG_H
#define SIMCORECONNECTDLG_H

#include <QDialog>

namespace Ui {
  class SimCoreConnectDlg;
}

class SimCoreConnectDlg : public QDialog
{
  Q_OBJECT

public:
  explicit SimCoreConnectDlg(QWidget *parent = 0);
  ~SimCoreConnectDlg();

private slots:
  void on_lineEdit_textChanged(const QString &arg1);

  void on_pushButton_clicked();

  void on_lineEdit_2_textChanged(const QString &arg1);

  void check_comm_thread();

  void on_pushButton_2_clicked();

private:
  Ui::SimCoreConnectDlg *ui;
};

#endif // SIMCORECONNECTDLG_H
