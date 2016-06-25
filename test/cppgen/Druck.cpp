#include "math.h"
#include "common.h"
#include "Druck.h"

// //---------------------------------------------------------------------------
// float trgs_round(float f)
// {
//     //return floor(f * 5 + 0.5) / 5;
//     return std::round(f * 5) / 5; // C++11
// }
//---------------------------------------------------------------------------
double Get_Pressure(int raw_pressure)
{
        // Pneumatikdruck
        // 4mA  bei  0 bar  (Spannungsabfall an 196 Ohm:  784 mV)
        // 20mA bei 10 bar  (Spannungsabfall an 196 Ohm: 3920 mV)
        // 20 / (3920-784) = 0,006377
        // Ergebis in 0,5 bar

        double fSpannung;

        fSpannung = (raw_pressure * MAX_ADC_SPANNUNG_FLOAT) / 1023.0;
        if(fSpannung < 784.0) 
	  return 0.0;
        else
           return (fSpannung - 784.0) * 0.006377;
}

//---------------------------------------------------------------------------
int Set_Pressure(double pressure)
{
        // Pneumatikdruck
        // 4mA  bei  0 bar  (Spannungsabfall an 196 Ohm:  784 mV)
        // 20mA bei 10 bar  (Spannungsabfall an 196 Ohm: 3920 mV)
        // 20 / (3920-784) = 0,006377
        // Ergebis in 0,5 bar

	return round((pressure / 0.006377 + 784.0) * 1023.0 / MAX_ADC_SPANNUNG_FLOAT);
}