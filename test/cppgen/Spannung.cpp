#include "common.h"
#include "Spannung.h"

//---------------------------------------------------------------------------
// Bordnetzspannung evaluieren; Spannung in [0,1 V]
//---------------------------------------------------------------------------
int Evaluiere_Bordnetz_Spannung(double v)
{
    // grau == 0
    if(v <= 0.0) 
        return 0;
    else
    // gelb  == 1
    if((v >= 220.0) && (v < 240.0))
        return 1;
    else        
    // gruen == 2
    if((v >= 240.0) && (v < 300.0))
        return 2;
    else
    // rot   == 3
    return 3;
}
