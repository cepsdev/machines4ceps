int X_Set_Speed(int speed);
int X_SollPosition_Berechnen(double Winkel);
int X_SollPosition_Berechnen(int Winkel);
double X_IstPosition_Berechnen(signed int Position);
int Y_Set_Speed(int speed);
int Y_SollPosition_Berechnen(double Winkel);
int Y_SollPosition_Berechnen(int Winkel);
double Y_IstPosition_Berechnen(signed int Position);
int Z_Set_Speed(int speed);
int Z_Set_Speed(double speed);
int Z_SollPosition_Berechnen(int sollposition);
int Z_SollPosition_Berechnen(double sollposition);
double Z_IstPosition_Berechnen(signed int Position);
extern double Get_Pressure(int raw_pressure);
extern int    Set_Pressure(double pressure);
int Evaluiere_Bordnetz_Spannung(double v);
extern double Get_Talin_Axis(int raw_axis);
extern int    Set_Talin_Axis(double axis);

extern double Get_Vehicle_Angle(int raw_angle);
extern int    Set_Vehicle_Angle(double angle);

