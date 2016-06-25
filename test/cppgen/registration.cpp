
#include <iostream>
#include "Antriebe.h"
#include "Druck.h"
#include "Spannung.h"
#include "Time.h"
#include "Winkel.h"
#include "core/include/state_machine_simulation_core_reg_fun.hpp"

/***************************************************************************/
/*****************************   GLOBALS   *********************************/
CAntriebX AntriebX;
CAntriebY AntriebY;
CAntriebZ AntriebZ;
/***************************************************************************/
/***************************   Antrieb X   *********************************/
int X_Set_Speed(int speed) {
  AntriebX.Set_Speed(speed);
  return AntriebX.getTargetSpeed();
}

int X_SollPosition_Berechnen(double Winkel){
  AntriebX.SollPosition_Berechnen(Winkel);
  return AntriebX.getSollPos();
}
int X_SollPosition_Berechnen(int Winkel){
  AntriebX.SollPosition_Berechnen((double)Winkel);
  return AntriebX.getSollPos();
}

double X_IstPosition_Berechnen(signed int Position){
  AntriebX.Set_IstPos(Position);
  AntriebX.IstPosition_Berechnen();
  return AntriebX.getIstWinkel();
}

/***************************************************************************/
/***************************   Antrieb Y   *********************************/
int Y_Set_Speed(int speed) {
  AntriebY.Set_Speed(speed);
  return AntriebY.getTargetSpeed();
}

int Y_SollPosition_Berechnen(double Winkel){
  AntriebY.SollPosition_Berechnen(Winkel);
  return AntriebY.getSollPos();
}
int Y_SollPosition_Berechnen(int Winkel){
  AntriebY.SollPosition_Berechnen((double)Winkel);
  return AntriebY.getSollPos();
}

double Y_IstPosition_Berechnen(signed int Position){
  AntriebY.Set_IstPos(Position);
  AntriebY.IstPosition_Berechnen();
  return AntriebY.getIstWinkel();
}

/***************************************************************************/
/***************************   Antrieb Z   *********************************/
int Z_Set_Speed(int speed) {
  AntriebZ.Set_Speed(speed);
  return AntriebZ.getTargetSpeed();
}

int Z_Set_Speed(double speed) {
  AntriebZ.Set_Speed((int)speed);
  return AntriebZ.getTargetSpeed();
}

int Z_SollPosition_Berechnen(int sollposition){
  AntriebZ.SollPosition_Berechnen((double)sollposition);
  return AntriebZ.getSollPos();
}

int Z_SollPosition_Berechnen(double sollposition){
  AntriebZ.SollPosition_Berechnen(sollposition);
  return AntriebZ.getSollPos();
}

double Z_IstPosition_Berechnen(signed int Position){
  AntriebZ.Set_IstPos(Position);
  AntriebZ.IstPosition_Berechnen();
  return AntriebZ.getIstMasthoehe();
}

/*
extern "C" void init_plugin(IUserdefined_function_registry* smc){
  smc->regfn("X_Set_Speed",X_Set_Speed);
  smc->regfn("X_SollPosition_Berechnen", (int (*)(int))    X_SollPosition_Berechnen);
  smc->regfn("X_SollPosition_Berechnen", (int (*)(double)) X_SollPosition_Berechnen);
  smc->regfn("X_IstPosition_Berechnen", X_IstPosition_Berechnen);
  
  smc->regfn("Y_Set_Speed",Y_Set_Speed);
  smc->regfn("Y_SollPosition_Berechnen", (int (*)(int))    Y_SollPosition_Berechnen);
  smc->regfn("Y_SollPosition_Berechnen", (int (*)(double)) Y_SollPosition_Berechnen);
  smc->regfn("Y_IstPosition_Berechnen", Y_IstPosition_Berechnen);
  
  smc->regfn("Z_Set_Speed",(int (*)(int)) Z_Set_Speed);
  smc->regfn("Z_Set_Speed",(int (*)(double)) Z_Set_Speed);
 
  smc->regfn("Z_SollPosition_Berechnen", (int (*)(int))    Z_SollPosition_Berechnen);
  smc->regfn("Z_SollPosition_Berechnen", (int (*)(double)) Z_SollPosition_Berechnen);
  smc->regfn("Z_IstPosition_Berechnen", Z_IstPosition_Berechnen);
  
  smc->regfn("Get_Pressure", Get_Pressure);
  smc->regfn("Set_Pressure", Set_Pressure);
  
  smc->regfn("Get_Vehicle_Angle", Get_Vehicle_Angle);
  
  smc->regfn("Get_Talin_Axis", Get_Talin_Axis);
  smc->regfn("Set_Talin_Axis", Set_Talin_Axis);
  
  smc->regfn("Get_Vehicle_Angle", Get_Vehicle_Angle);
  smc->regfn("Set_Vehicle_Angle", Set_Vehicle_Angle);
  
  smc->regfn("Evaluiere_Bordnetz_Spannung", Evaluiere_Bordnetz_Spannung);
  
  smc->regfn("Set_Time", Set_Time);
  smc->regfn("Get_Time", Get_Time);
  
  std::cout << "Plugin functions registered .. [OK]" << std::endl;
}*/
