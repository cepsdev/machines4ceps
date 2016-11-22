#ifndef MDIBASE_H
#define MDIBASE_H

#include <QMdiSubWindow>

class Mdibase : public QMdiSubWindow{
    Q_OBJECT
protected:
    int this_instance_id = 0;
public:
    Mdibase(QWidget * parent = 0, Qt::WindowFlags flags = 0);
private:
    static int instance_counter;
};

#endif // MDIBASE_H
