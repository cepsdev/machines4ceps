
#include "core/include/state_machine.hpp"
#include "core/include/state_machine_simulation_core.hpp"

#include "Antriebe.h"
#include "math.h"
CAntriebX AntriebX;
CAntriebY AntriebY;
CAntriebZ AntriebZ;

/***************************************************************************/
/***************************   Antrieb X   *********************************/
int X_SollPosition_Berechnen(double Winkel){
  AntriebX.SollPosition_Berechnen(Winkel);
  return AntriebX.getSollPos();
}
double X_IstPosition_Berechnen(void){
  AntriebX.IstPosition_Berechnen();
  return AntriebX.getIstWinkel();
}

/***************************************************************************/
/***************************   Antrieb Y   *********************************/
int Y_SollPosition_Berechnen(double Winkel){
  AntriebY.SollPosition_Berechnen(Winkel);
  return AntriebY.getSollPos();
}
int Y_IstPosition_Berechnen(void){
  AntriebY.IstPosition_Berechnen();
  return AntriebY.getIstWinkel();
}

/***************************************************************************/
/***************************   Antrieb Z   *********************************/
int Z_SollPosition_Berechnen(double Winkel){
  AntriebZ.SollPosition_Berechnen(Winkel);
  return AntriebZ.getSollPos();
}
int Z_IstPosition_Berechnen(void){
  AntriebZ.IstPosition_Berechnen();
  return AntriebZ.getIstMasthoehe();
}



//extern "C" void init_plugin(void*,void*) {}

extern "C" void init_plugin(State_machine_simulation_core* smc, 
			     void (*register_plugin)(State_machine_simulation_core*,std::string const&,ceps::ast::Nodebase_ptr (ceps::ast::Call_parameters* )))
{
  smc->regfn("X_SollPosition_Berechnen",X_SollPosition_Berechnen);
  smc->regfn("X_IstPosition_Berechnen", X_IstPosition_Berechnen);
  
  smc->regfn("Y_SollPosition_Berechnen",Y_SollPosition_Berechnen);
  smc->regfn("Y_IstPosition_Berechnen", Y_IstPosition_Berechnen);
  
  smc->regfn("Z_SollPosition_Berechnen",Z_SollPosition_Berechnen);
  smc->regfn("Z_IstPosition_Berechnen", Z_IstPosition_Berechnen);
}







//---------------------------------------------------------------------------
/*// 0x590 for Baudrate                           Wert      Beschreibung

const unsigned char CAntriebX::Baudrate =       0x02;   // 0: 1000 kBit/s
                                                        // 1:  500 kBit/s
                                                        // 2:  250 kBit/s
                                                        // 3:  125 kBit/s
                                                        // 4:   20 kBit/s
                                                        // 5:   50 kBit/s

//---------------------------------------------------------------------------
// 0x591 for Node Identifier                    Wert       Beschreibung

const unsigned char CAntriebX::NodeID =         0x10;    // 2-63 Slave (default NodeID 2)
*/

/***************************************************************************/
/***************************   Antrieb X   *********************************/
/***************************************************************************/
//---------------------------------------------------------------------------
CAntriebX::CAntriebX()
{
        Control = 0x00;
        Drehrichtung = 1.0;
        Hublaenge = 64.5;
        WinkelMax = 16.0;
        Steigung = 4.0;
}
//---------------------------------------------------------------------------
int CAntriebX::getSollPos()
{
  return SollPos;
}
//---------------------------------------------------------------------------
int CAntriebX::getIstWinkel()
{
  return IstWinkel;
}
//---------------------------------------------------------------------------
void CAntriebX::Get_PDO()
{
        //Global.AntrX.Get_PDO(IstPos, IstGeschw, IstStrom, Status);
        //IstPosition_Berechnen();
}
//---------------------------------------------------------------------------
void CAntriebX::Set_PDO()
{
        //Global.AntrX.Set_PDO(SollPos, SollGeschw, SollStrom, Control);
}
//---------------------------------------------------------------------------
void CAntriebX::SollPosition_Berechnen(double Winkel)
/*
Die Hublänge für Horizentierung X beträgt:              90mm
Bei -8° beträgt der Hub:                                -33,0mm
Bei +8° beträgt der Hub:                                +31,5mm
Untersetzung Zwischenstufe (geht nicht ein in die Berechnung):  10
Spindelsteigung:                                        4mm
Anzahl Umdrehungen vom Inkrementalgeber für Hublänge:   22,5 (für Hublänge 90mm)
Berechnung dafür: (Hublänge/Steigung)

Anzahl Umdrehungen als Ganzzahl in Byte 2 und 3 von T_IstPos
Soll Position Inkrementalg. in Bit 4,5,6,7 von Byte 0 und in Byte 1 von T_IstPos

Berechnung von SollUmdrehungen: SollInkr/4096 + AnzahlUmdrehungen
*************************************************************************
*/
{
        double AnzahlUmdr;
        int SollUmdr;

        AnzahlUmdr = Drehrichtung * 4096.0 * Hublaenge / WinkelMax * Winkel / Steigung;
        SollUmdr = floor(AnzahlUmdr);

        SollPos = SollUmdr << 4;
        Set_PDO();
}
//---------------------------------------------------------------------------
void CAntriebX::IstPosition_Berechnen(void)
/*
*************************************************************************
Die Hublänge für Horizentierung X beträgt:              90mm
Bei -8° beträgt der Hub:                                -33,0mm
Bei +8° beträgt der Hub:                                +31,5mm
Untersetzung Zwischenstufe (geht nicht ein in die Berechnung):     10
Spindelsteigung:                                        4mm
Anzahl Umdrehungen vom Inkrementalgeber für Hublänge:   22,5 (für Hublänge 90mm)
Berechnung dafür: (Hublänge/Steigung)

Anzahl Umdrehungen als Ganzzahl in Byte 2 und 3 von IstPos
Ist Position Inkrementalg. in Bit 4,5,6,7 von Byte 0 und in Byte 1 von IstPos
*************************************************************************
*/
{
        int IstUmdr;

        IstUmdr = IstPos;
        IstUmdr = IstUmdr >> 4;
        IstWinkel = Drehrichtung * IstUmdr * Steigung * WinkelMax  / Hublaenge / 4096.0;
}
//---------------------------------------------------------------------------
void CAntriebX::Get_Ist(double & Winkel, unsigned char & St)
{
        Winkel = IstWinkel;
        St = Status;
}
//---------------------------------------------------------------------------
void CAntriebX::Set_Control(unsigned char Contr)
{
        Control |= Contr;
        Set_PDO();
}
//---------------------------------------------------------------------------
void CAntriebX::Reset_Control(unsigned char Contr)
{
        Control &= ~Contr;
        Set_PDO();
}
//---------------------------------------------------------------------------
void CAntriebX::Reset_AllControl()
{
        Control = 0;  
        Set_PDO();
}
//---------------------------------------------------------------------------
void CAntriebX::Set_Speed(short Speed)
{
//        if ((Speed > 0) && (Speed < 300)) Speed = 300;
        if (Speed > 2047) Speed = 2047;
//        if ((Speed < 0) && (Speed > -300)) Speed = -300;
        if (Speed < -2048) Speed = -2048;
        SollGeschw = Speed;
        Set_PDO();
}

/***************************************************************************/
/***************************   Antrieb Y   *********************************/
/***************************************************************************/
//---------------------------------------------------------------------------
CAntriebY::CAntriebY()
{
        Control = 0x00;
        Drehrichtung = 1.0;
        Hublaenge = 283.0 - 5.0;
        WinkelMax = 98.0;
        Steigung = 5.0;
}
//---------------------------------------------------------------------------
int CAntriebY::getSollPos()
{
  return SollPos;
}
//---------------------------------------------------------------------------
int CAntriebY::getIstWinkel()
{
  return IstWinkel;
}
//---------------------------------------------------------------------------
void CAntriebY::Get_PDO()
{
//        Global.AntrY.Get_PDO(IstPos, IstGeschw, IstStrom, Status);
//        IstPosition_Berechnen();
}
//---------------------------------------------------------------------------
void CAntriebY::Set_PDO()
{
//        Global.AntrY.Set_PDO(SollPos, SollGeschw, SollStrom, Control);
}
//---------------------------------------------------------------------------
void CAntriebY::SollPosition_Berechnen(double Winkel)
/*
Die Hublänge für Plattform Y beträgt:                   290mm
Bei 0° beträgt der Hub:                                 5mm
Bei 98° beträgt der Hub:                                283mm
Untersetzung Zwischenstufe:                             16
Spindelsteigung:                                        5mm
Anzahl Umdrehungen vom Inkrementalgeber für Hublänge:   58,0
Berechnung dafür: (Hublänge/Steigung)

Anzahl Umdrehungen als Ganzzahl in Byte 2 und 3 von T_IstPos
Soll Position Inkrementalg. in Bit 4,5,6,7 von Byte 0 und in Byte 1 von T_IstPos

Berechnung von SollUmdrehungen: SollInkr/4096 + AnzahlUmdrehungen
*************************************************************************
*/
{
/*
        double AnzahlUmdr;
        int SollUmdr;

        AnzahlUmdr = Drehrichtung * 4096.0 * Hublaenge / WinkelMax * Winkel / Steigung;
        SollUmdr = floor(AnzahlUmdr);

        SollPos = SollUmdr << 4;
        Set_PDO();
*/
        float fLaenge = 0.0f;
        // 1. Winkel Laenge -> Stellstab
        // fuer die Anzeige am ExtBG genügt: 3 Bereiche
        // 1.     0 mm .. 248.3 mm    0..82 Grad    Faktor : 3.0285  mm/Grad
        // 2. 248.3 mm .. 266.7 mm   82..90 Grad    Faktor : 2.29725 mm/Grad
        // 3. 266.7 mm .. 283.3 mm   90..88 Grad    Faktor : 2.0786  mm/Grad
        if(Winkel < 82.0)
        {
            fLaenge = Winkel * 3.0285;    //  [mm]
        }
        else if((Winkel >= 82.0) &&
                (Winkel <  90.0))
        {
           fLaenge = ((Winkel - 82.0)* 2.29725) + 248.3;   //  [mm]
        }
        else if(Winkel >= 90.0)
        {
           fLaenge = ((Winkel - 90.0) * 2.0786) + 266.7;    //  [mm]
        }
        // 2. Laenge Stellstab -> Umdrehungen
        SollPos = (int)(fLaenge * 65536.0);     // ganze Umdrehungen
        SollPos /= 5;                 // 1 Umdrehungen sind 5 mm
        Set_PDO();

}
//---------------------------------------------------------------------------
void CAntriebY::IstPosition_Berechnen(void)
/*
*************************************************************************
Die Hublänge für Plattform Y beträgt:                   290mm
Bei 0° beträgt der Hub:                                 5mm
Bei 98° beträgt der Hub:                                283mm
Untersetzung Zwischenstufe:                             16
Spindelsteigung:                                        5mm
Anzahl Umdrehungen vom Inkrementalgeber für Hublänge:   58,0
Berechnung dafür: (Hublänge/Steigung)

Anzahl Umdrehungen als Ganzzahl in Byte 2 und 3 von IstPos
Ist Position Inkrementalg. in Bit 4,5,6,7 von Byte 0 und in Byte 1 von IstPos
*************************************************************************
*/   
{
/*
        int IstUmdr;

        IstUmdr = IstPos;
        IstUmdr = IstUmdr >> 4;
        IstWinkel = Drehrichtung * IstUmdr * Steigung * WinkelMax / Hublaenge / 4096.0;
*/
float fLaenge,fHORIZ_WinkelY = 0.0f;
                // 1. Umdrehungen -> Laenge Stellstab
        fLaenge = (float)IstPos / 65536.0;     // ganze Umdrehungen
        fLaenge *= 5.0;                       // 1 Umdrehungen sind 5 mm
        // 2. Laenge Stellstab -> Winkel
        // fuer die Anzeige am ExtBG genügt: 3 Bereiche
        // 1.     0 mm .. 248.3 mm    0..82 Grad    Faktor : 3.0285  mm/Grad
        // 2. 248.3 mm .. 266.7 mm   82..90 Grad    Faktor : 2.29725 mm/Grad
        // 3. 266.7 mm .. 283.3 mm   90..88 Grad    Faktor : 2.0786  mm/Grad
        if(fLaenge < 248.3)
        {
           fHORIZ_WinkelY = fLaenge / 3.0285;    //  [1 Grad]
        }
        else if((fLaenge >= 248.3) &&
                (fLaenge <  266.7))
        {
           fHORIZ_WinkelY = 82.0 + (fLaenge-248.3) / 2.29725;   //  [1 Grad]
        }
        else if(fLaenge >= 266.7)
        {
           fHORIZ_WinkelY = 90.0 + (fLaenge-266.7) / 2.0786;    //  [1 Grad]
        }
        IstWinkel  = fHORIZ_WinkelY;

}
//---------------------------------------------------------------------------
void CAntriebY::Get_Ist(double & Winkel, unsigned char & St)
{
        Winkel = IstWinkel;
        St = Status;
}
//---------------------------------------------------------------------------
void CAntriebY::Set_Control(unsigned char Contr)
{
        Control |= Contr;
        Set_PDO();
}
//---------------------------------------------------------------------------
void CAntriebY::Reset_Control(unsigned char Contr)
{
        Control &= ~Contr;
        Set_PDO();
}   
//---------------------------------------------------------------------------
void CAntriebY::Reset_AllControl()
{
        Control = 0;
        Set_PDO();
}
//---------------------------------------------------------------------------
void CAntriebY::Set_Speed(short Speed)
{
//        if ((Speed > 0) && (Speed < 300)) Speed = 300;
        if (Speed > 2047) Speed = 2047;
//        if ((Speed < 0) && (Speed > -300)) Speed = -300;
        if (Speed < -2048) Speed = -2048;
        SollGeschw = Speed;
        Set_PDO();
}

/***************************************************************************/
/***************************   Antrieb Z   *********************************/
/***************************************************************************/
//---------------------------------------------------------------------------
CAntriebZ::CAntriebZ()
{
        Control = 0x00;
        Drehrichtung = 1.0;
        Hublaenge = 3290.0;
        Getriebe = 25.0;         // Untersetzung
        Zwischenstufe = 2.85;    // Untersetzung
        Steigung = 40.0;
}
//---------------------------------------------------------------------------
int CAntriebZ::getSollPos()
{
  return SollPos;
}
//---------------------------------------------------------------------------
int CAntriebZ::getIstMasthoehe()
{
  return IstMasthoehe;
}
//---------------------------------------------------------------------------
void CAntriebZ::Get_PDO()
{
//         Global.AntrZ.Get_PDO(IstPos, IstGeschw, IstStrom, Status);
//         IstPosition_Berechnen();
}
//---------------------------------------------------------------------------  
void CAntriebZ::Set_PDO()
{
//        Global.AntrZ.Set_PDO(SollPos, SollGeschw, SollStrom, Control);
}
//---------------------------------------------------------------------------
void CAntriebZ::SollPosition_Berechnen(int MastHoehe)
/*
Die Hublänge vom Mast beträgt:                          3290mm
Untersetzung Zwischenstufe:                             2,85
Untersetzung Getriebe:                                  25
Spindelsteigung:                                        40mm
Anzahl Umdrehungen vom Inkrementalgeber für Hublänge:   5963,13
Berechnung dafür: (Hublänge/Steigung) * iGetr * iZw

Anzahl Umdrehungen als Ganzzahl in Byte 2 und 3 von T_IstPos
Soll Position Inkrementalg. in Bit 4,5,6,7 von Byte 0 und in Byte 1 von T_IstPos

Berechnung von SollUmdrehungen: SollInkr/4096 + AnzahlUmdrehungen
*************************************************************************
*/  
{
        double AnzahlUmdr;
        int SollUmdr;

        AnzahlUmdr = Drehrichtung * 4096.0 * MastHoehe / Steigung * Zwischenstufe * Getriebe;
        SollUmdr = floor(AnzahlUmdr);

        SollPos = SollUmdr << 4;
        Set_PDO();
}
//---------------------------------------------------------------------------
void CAntriebZ::IstPosition_Berechnen(void)
/*
*************************************************************************
Die Hublänge vom Mast beträgt:                          3290mm
Untersetzung Zwischenstufe:                             2,9
Untersetzung Getriebe:                                  25
Spindelsteigung:                                        40mm
Anzahl Umdrehungen vom Inkrementalgeber für Hublänge:   5963,13
Berechnung dafür: (Hublänge/Steigung) * iGetr * iZw

Anzahl Umdrehungen als Ganzzahl in Byte 2 und 3 von IstPos
Ist Position Inkrementalg. in Bit 4,5,6,7 von Byte 0 und in Byte 1 von IstPos
*************************************************************************
*/    
{
        int IstUmdr;
        double IstHub;

        IstUmdr = IstPos;
        IstUmdr = IstUmdr >> 4;
        IstHub = Drehrichtung * IstUmdr * Steigung / Zwischenstufe / Getriebe / 4096.0;
        IstMasthoehe = floor(IstHub);
}
//---------------------------------------------------------------------------
void CAntriebZ::Get_Ist(signed int & Masthoehe, unsigned char & St)
{
        Masthoehe = IstMasthoehe;
        St = Status;
}
//---------------------------------------------------------------------------
void CAntriebZ::Set_Control(unsigned char Contr)
{
        Control |= Contr;
        Set_PDO();
}
//---------------------------------------------------------------------------
void CAntriebZ::Reset_Control(unsigned char Contr)
{
        Control &= ~Contr;
        Set_PDO();
} 
//---------------------------------------------------------------------------
void CAntriebZ::Reset_AllControl()
{
        Control = 0; 
        Set_PDO();
}
//---------------------------------------------------------------------------
void CAntriebZ::Set_Speed(short Speed)
{
//        if ((Speed > 0) && (Speed < 300)) Speed = 300;
        if (Speed > 2047) Speed = 2047;
//        if ((Speed < 0) && (Speed > -300)) Speed = -300;
        if (Speed < -2048) Speed = -2048;
        SollGeschw = Speed;
        Set_PDO();
}

