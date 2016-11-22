#include "mdibase.h"

int Mdibase::instance_counter = 0;

Mdibase::Mdibase(QWidget * parent, Qt::WindowFlags flags):QMdiSubWindow(parent,flags){
    this_instance_id = Mdibase::instance_counter++;
}
