#include "math.h"
#include "common.h"
#include "Winkel.h"

//-------------------------------------------------------------------
// TALIN (NAV Anlage)
//-------------------------------------------------------------------
double Get_Talin_Axis(int raw_axis) 
{
    return (double)raw_axis / (double)0x7FFFFFFF * 180.0;
}

int Set_Talin_Axis(double axis) 
{
    return round(axis * (double)0x7FFFFFFF / 180.0);
}


//---------------------------------------------------------------------------
// Fahrzeugachse {X,Y} vom Neigungsgeber [0.1°]
//---------------------------------------------------------------------------
double Get_Vehicle_Angle(int raw_angle) 
{
    double fSpg = (raw_angle * MAX_ADC_SPANNUNG_FLOAT) / 1023.0;
    return ((fSpg - 2500.0) / 7.0);
}

int Set_Vehicle_Angle(double angle) 
{
    return round(((7.0 * angle) + 2500.0) * 1023.0 / MAX_ADC_SPANNUNG_FLOAT);
}