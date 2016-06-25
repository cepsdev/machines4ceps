/* ---------------------------------------------------------------------
   Dingo2 BÜR                                      KRAUSS-MAFFEI WEGMANN
   ---------------------------------------------------------------------


   ---------------------------------------------------------------------
                           Schutzvermerk
                       nach DIN 34 beachten

          Weitergabe sowie Vervielfaeltigung dieser Unterlage,
          Verwertung ihres Inhalts und Mitteilung ihres Inhalts
          nicht gestattet, soweit nicht ausdruecklich zugestanden.
          Zuwiderhandlungen verpflichten zu Schadensersatz.
          Alle Rechte fuer den Fall der Patenterteilung oder
          Gebrauchsmuster-Eintragung vorbehalten.
   ---------------------------------------------------------------------


   ---------------------------------------------------------------------
   PROJEKT           : Dingo2 BÜR
   ---------------------------------------------------------------------
   Funktion          :
   Dateiname         : ANTRIEBE.H
   Ablage_ort        :
   Dokumenten_Nr     : SK000-000.000.000.0

   Aenderungsindex   : 0.01
   Datum             : 12.12.2008
   Bearbeiter        : EW201 Bachmann

   ---------------------------------------------------------------------

   Aenderungsstand   :

   0.01   Ersterstellung                            12.12.2008  Bachmann

   --------------------------------------------------------------------- */

   
/* ---------------------------------------------------------------------
   Modulbeschreibung
   ---------------------------------------------------------------------
   Das Modul...
   --------------------------------------------------------------------- */

//---------------------------------------------------------------------------
#ifndef __ANTRIEBE_H__
#define __ANTRIEBE_H__
//---------------------------------------------------------------------------
class CAntriebX
{
public:
        CAntriebX();

        // Funktionen für SDO
        //void SetInitParameter(void);
        //unsigned char InitParameterGesetzt(void);
        //void SetSpeedLimit(unsigned char Limit);
        //unsigned char SpeedLimitGesetzt(void);
        
        int  getSollPos();
        double getIstWinkel();
	int  getTargetSpeed();
        void Get_PDO();
        void Set_PDO();
        void SollPosition_Berechnen(double Winkel);
        void IstPosition_Berechnen(void);
        void Get_Ist(double & Winkel, unsigned char & Status);
        void Set_Control(unsigned char Control);
        void Reset_Control(unsigned char Control);
        void Reset_AllControl();
        void Set_Speed(int Speed);
	void Set_IstPos(signed int IstPos);
private:
        static const unsigned char Baudrate;
        static const unsigned char NodeID;
        unsigned char SpeedLimit;

        double Drehrichtung;
        double Hublaenge;
        double WinkelMax;
        double Steigung;

        signed int IstPos;
        short IstGeschw;
        unsigned short IstStrom;
        unsigned char Status;

        signed int SollPos;
        short SollGeschw;
        unsigned short SollStrom;
        unsigned char Control;

        double IstWinkel;
};

//---------------------------------------------------------------------------
class CAntriebY
{
public:
        CAntriebY();

        // Funktionen für SDO
        void SetInitParameter(void);
        unsigned char InitParameterGesetzt(void);
        void SetSpeedLimit(unsigned char Limit);
        unsigned char SpeedLimitGesetzt(void);
          
        int  getSollPos();
        double  getIstWinkel();
	int  getTargetSpeed();
        void Get_PDO();
        void Set_PDO();
        void SollPosition_Berechnen(double Winkel);
        void IstPosition_Berechnen(void);
        void Get_Ist(double & Winkel, unsigned char & Status);
        void Set_Control(unsigned char Control);
        void Reset_Control(unsigned char Control);  
        void Reset_AllControl();
        void Set_Speed(int Speed);
	void Set_IstPos(signed int IstPos);
private:
        static const unsigned char Baudrate;
        static const unsigned char NodeID;
        unsigned char SpeedLimit;

        double Drehrichtung;
        double Hublaenge;
        double WinkelMax;
        double Steigung;

        signed int IstPos;
        short IstGeschw;
        unsigned short IstStrom;
        unsigned char Status;

        signed int SollPos;
        short SollGeschw;
        unsigned short SollStrom;
        unsigned char Control;

        double IstWinkel;
};

//---------------------------------------------------------------------------
class CAntriebZ
{
public:
        CAntriebZ();

        // Funktionen für SDO
        void SetInitParameter(void);
        unsigned char InitParameterGesetzt(void);
        void SetSpeedLimit(unsigned char Limit);
        unsigned char SpeedLimitGesetzt(void);
          
        int  getSollPos();
        int  getIstMasthoehe();
	int  getTargetSpeed();
        void Get_PDO();
        void Set_PDO();
        void SollPosition_Berechnen(double MastHoehe);
        void IstPosition_Berechnen(void);
        void Get_Ist(signed int & Masthoehe, unsigned char & Status);
        void Set_Control(unsigned char Control);
        void Reset_Control(unsigned char Control);  
        void Reset_AllControl();
        void Set_Speed(int Speed);
	void Set_IstPos(signed int IstPos);

private:
        static const unsigned char Baudrate;
        static const unsigned char NodeID;
        unsigned char SpeedLimit;

        double Drehrichtung;
        double Hublaenge;
        double Getriebe;
        double Zwischenstufe;
        double Steigung;

        signed int IstPos;
        short IstGeschw;
        unsigned short IstStrom;
        unsigned char Status;

        signed int SollPos;
        short SollGeschw;
        unsigned short SollStrom;
        unsigned char Control;

        signed int IstMasthoehe;
};
//---------------------------------------------------------------------------
extern CAntriebX AntriebX;
extern CAntriebY AntriebY;
extern CAntriebZ AntriebZ;
//---------------------------------------------------------------------------
#endif // __ANTRIEBE_H__
 