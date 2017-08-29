/* ---------------------------------------------------------------------
   CAN                                             KRAUSS-MAFFEI WEGMANN
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
   PROJEKT           : Multibus
   ---------------------------------------------------------------------
   Funktion          : DLL-Funktionen
   Dateiname         : Multibus.h
   Ablage_ort        : Entwicklung\6_sw\Multibus\Interface
   Dokumenten_Nr     : SK000-000.000.000.0

   Aenderungsindex   : 0.01
   Datum             : 12.04.2010
   Bearbeiter        : EW201 Pullwitt

   ---------------------------------------------------------------------

   Aenderungsstand   :

   0.01   Ersterstellung                            12.04.2010  Pullwitt

   --------------------------------------------------------------------- */

/* ---------------------------------------------------------------------
   Modulbeschreibung
   ---------------------------------------------------------------------
   Definiert die Schnittstelle der DLL, welche die Funktionen fuer den 
   Austausch von CANbus-Informationen zwischen einer Anwendung und dem 
   Prozess Multi-CANbus-Verwaltung enthaelt.
   --------------------------------------------------------------------- */

#ifndef MultibusH
#define MultibusH
#ifdef WIN32
#if defined(MULTIBUS_EXPORT)
#define DLL_API __declspec(dllexport)
#elif defined(MULTIBUS_IMPORT)
#define DLL_API __declspec(dllimport)
#else
#define DLL_API
#endif
#else
#define DLL_API
#endif
#pragma pack(1)


/* CanBusStatus
   Status von CANbus-Hardware und Prozess Multi-CANbus-Verwaltung
      processTermination               Serviceprocess existiert nicht
                                          mehr oder kann nicht erzeugt
                                          werden?
      configurationError               CANbus nicht oder fehlerhaft
                                          konfiguriert?
      hwFailure                        Defekt der CANbus-Hardware?
      simulation                       CANbus-Hardware nicht gefunden,
                                          Bus wird PC-intern simuliert?
      light                            Fehlerzaehler des CAN-Controllers
                                          hat Fehlerstufe "Light"
                                          erreicht?
      heavy                            Fehlerzaehler des CAN-Controllers
                                          hat Fehlerstufe "Heavy"
                                          erreicht?
      busoff                           CAN-Controller hat Buszustand
                                          "Bus-off" festgestellt?
      reset                            eigene oder andere Anwendung
                                          hatte CAN-Controller
                                          zurueckgesetzt?
      txOverrun                        Sendewarteschlange war voll, zu
                                          sendende Nachrichten wurden
                                          nicht uebertragen?
      rxOverrun                        Empfangswarteschlange war voll,
                                          empfangene Nachrichten gingen
                                          verloren?
      authorizationError               Kopierschutz nicht
                                          gewaehrleistet?
      busload                          Auslastung des Busses
                                          (Verhaeltnis der uebertragenen
                                          Datenmenge zur maximal
                                          uebertragbaren Datenmenge in
                                          der letzten Zeiteinheit) [%]   */

typedef struct
{
   unsigned char processTermination : 1;
   unsigned char configurationError : 1;
   unsigned char hwFailure : 1;
   unsigned char simulation : 1;
   unsigned char light : 1;
   unsigned char heavy : 1;
   unsigned char busoff : 1;
   unsigned char reset : 1;
   unsigned char txOverrun : 1;
   unsigned char rxOverrun : 1;
   unsigned char authorizationError : 1;
   unsigned char busload;
} CanBusStatus;


/* CanFormat
   Format des Nachrichten-Identifikators                                 */
   
typedef enum 
{
   CanFormatConfiguration,
   CanFormatStandard,
   CanFormatExtended,
} CanFormat;


/* CanMessage
   Sende- oder Empfangsnachricht der CANbus-Uebertragung
      id                               CAN-Nachrichten-Identifikator
      rtr                              Nachricht ist ein Remote-Request?
      length                           Anzahl der Nutzdaten [Byte]
                                          (0...8)
      format                           Format des Nachrichten-
                                          Identifikators (Werte von 
                                          CanFormat)
      data                             Nutzdaten                         */

typedef struct
{
   unsigned long id;
   unsigned char rtr : 1;
   unsigned char length : 4;
   unsigned char format : 2;
   unsigned char data[8];
} CanMessage;


/* CanEvent
   Ereignismeldung fuer Nachrichtenempfang oder Statusaenderung
      time                             Systemzeit des Ereignisses ([ms]
                                          seit Systemstart)
      statusChanged                    Status hat sich seit der letzten 
                                          Statusabfrage geaendert?
      messageValid                     Eine neue CAN-Nachricht wurde 
                                          empfangen?
      status                           aktueller Status (auch gueltig,
                                          wenn status keine Aenderung
                                          anzeigt)
      message                          letzte CAN-Nachricht (nur
                                          gueltig, wenn message Empfang
                                          anzeigt)                       */

typedef struct
{
   unsigned long time;
   unsigned char statusChanged : 1;
   unsigned char messageValid : 1;
   CanBusStatus status;
   CanMessage message;
} CanEvent;


/* CanQueueStatus
   Status einer Nachrichten-Warteschlange
      capacity                         Kapazitaet der Warteschlange
      length                           aktuelle Laenge der Warteschlange
      totalCount                       Anzahl insgesamt uebertragener
                                          Nachrichten                   
      overrun                          Anzahl insgesamt verlorener
                                          Nachrichten                    */

typedef struct
{
   unsigned int capacity;
   unsigned int length;
   unsigned int totalCount;
   unsigned int overrun;
} CanQueueStatus;


/* CanFilterType
   Typ des Filters                                                       */
   
typedef enum 
{
   CanFilterTypeBlock,
   CanFilterTypePass,
   CanFilterTypeTime,
} CanFilterType;


/* CanFilterMask
   Teilbereichsdefinition fuer einen Durchlass- oder Sperrfilter. Diese 
   Filtervariante basiert auf Bitmaskierung des CANbus-Message-
   Identifikators.
      type                             Typ des Filters
      format                           Format des Nachrichten-
                                          Identifikators
      idMask                           Bitmaske fuer Selektion 
                                          relevanter Bits im 
                                          Nachrichten-Identifikator
      idPattern                        Werte relevanter Bits im           
                                          Nachrichten-Identifikator     
      time_ms                          Zeit, die nach dem Durchlass 
                                          einer Message vergehen muss,
                                          bis die naechste Message den
                                          Filter passieren darf
                                          (nur bei type = 
                                          CanFilterTypeTime)             */

typedef struct 
{
   CanFilterType type;
   CanFormat format;
   unsigned long idMask;
   unsigned long idPattern;
   unsigned long time_ms;
} CanFilterMask;


/* CanReadCallback
   Prozedur, die aufgerufen wird, wenn der Status sich geaendert hat
   oder eine neue CAN-Nachricht empfangen wurde
      queue                            Identifikator der Warteschlange   */

typedef void (*CanReadCallback)(
   unsigned char queue);


/* CanFlushedCallback
   Prozedur, die aufgerufen wird, wenn die Sendewarteschlange leer oder
   nicht mehr leer ist (Achtung: Prozedur darf nur Signale setzen und 
   ruecksetzen, nicht aber auf den CANbus zugreifen)
      queue                            Identifikator der Warteschlange
      flushed                          Sendewarteschlange ist leer?      */

typedef void (*CanFlushedCallback)(
   unsigned char queue,
   bool flushed);


/* ---------------------------------------------------------------------
   Funktion CanStart

   Eingangsparameter:
      Keine

   Ausgangsparameter:
      Keine

   ---------------------------------------------------------------------
   Fuehrt die Initialisierung der DLL durch.
   Muss nach dem Laden der DLL sowie nach Aufruf der Routine CanStop
   aufgerufen werden, bevor eine der anderen DLL-Funktion ausgefuehrt
   werden kann.
   --------------------------------------------------------------------- */

extern "C" DLL_API void CanStart(void);


/* ---------------------------------------------------------------------
   Funktion CanStop

   Eingangsparameter:
      Keine

   Ausgangsparameter:
      Keine

   ---------------------------------------------------------------------
   Fuehrt die Endebehandlung der DLL durch.
   Muss aufgerufen werden, bevor die DLL entladen wird. Kann aufgerufen
   werden, wenn die CANbus-Kommunikation nicht mehr gebraucht wird.
   --------------------------------------------------------------------- */

extern "C" DLL_API void CanStop(void);


/* ---------------------------------------------------------------------
   Funktion CanVersion

   Eingangsparameter:
      Keine

   Ausgangsparameter:
      return                           Version der der CANbus-Software

   ---------------------------------------------------------------------
   Ermittelt die Versionsnummer der CANbus-Software.
   --------------------------------------------------------------------- */

extern "C" DLL_API unsigned long CanVersion(void);


/* ---------------------------------------------------------------------
   Funktion CanConfigure

   Eingangsparameter:
      bus                              Identifikator des CANbus

   Ausgangsparameter:
      Keine

   ---------------------------------------------------------------------
   Veranlasst die Neuinitialisierung der CANbus-Hardware und der
   Warteschlangen.
   Sollte aufgerufen werden, wenn die CANbus-Parameter in der Windows-
   Registry bzw. in der INI-Datei geaendert wurden.
   --------------------------------------------------------------------- */

extern "C" DLL_API void CanConfigure(
   unsigned char bus);


/* ---------------------------------------------------------------------
   Funktion CanReset

   Eingangsparameter:
      bus                              Identifikator des CANbus

   Ausgangsparameter:
      Keine

   ---------------------------------------------------------------------
   Loest das Ruecksetzen des CAN-Controllers aus.
   --------------------------------------------------------------------- */

extern "C" DLL_API void CanReset(
   unsigned char bus);


/* ---------------------------------------------------------------------
   Funktion CanOpen

   Eingangsparameter:
      bus                              Identifikator des CANbus

   Ausgangsparameter:
      return                           Identifikator der Warteschlange

   ---------------------------------------------------------------------
   Oeffnet eine weitere Warteschlange fuer das Senden und Empfangen von 
   CANbus-Nachrichten.
   --------------------------------------------------------------------- */

extern "C" DLL_API unsigned char CanOpen(
   unsigned char bus);


/* ---------------------------------------------------------------------
   Funktion CanClose

   Eingangsparameter:
      queue                            Identifikator der Warteschlange

   Ausgangsparameter:
      Keine

   ---------------------------------------------------------------------
   Schliesst eine Warteschlange.
   --------------------------------------------------------------------- */

extern "C" DLL_API void CanClose(
   unsigned char queue);


/* ---------------------------------------------------------------------
   Funktion CanInstall

   Eingangsparameter:
      queue                            Identifikator der Warteschlange
      readCallback                     Routine fuer Ereignisbehandlung
                                          von Statusaenderung oder 
                                          Datenempfang
                                          NULL: Routine deinstallieren
      flushedCallback                  Routine fuer Ereignisbehandlung
                                          von Zustandsaenderungen der 
                                          Sendewarteschlange 
                                          NULL: Routine deinstallieren

   Ausgangsparameter:
      Keine

   ---------------------------------------------------------------------
   Installiert oder deinstalliert Ereignisbehandlungsroutinen. 
   Der Prozess Multi-CANbus-Verwaltung veranlasst den Aufruf der 
   Routine readCallback, wenn er den CANbus-Status geaendert oder eine 
   neue CAN-Nachricht empfangen hat. Die Callbackfunktion wird nur dann 
   ausgefuehrt, wenn die Ereigniswarteschlange vor dem Ereigniszeitpunkt 
   leer war.
   Routine flushedCallback wird aufgerufen, wenn die Sendewarteschlange 
   leer oder nicht mehr leer ist.
   Es koennen eigene Routinen fuer jeden CANbus oder gemeinsame Routine 
   fuer alle Busse installiert werden.
   --------------------------------------------------------------------- */

extern "C" DLL_API void CanInstall(
   unsigned char queue,
   CanReadCallback readCallback,
   CanFlushedCallback flushedCallback);


/* ---------------------------------------------------------------------
   Funktion CanStatus

   Eingangsparameter:
      queue                            Identifikator der Warteschlange

   Ausgangsparameter:
      busStatus                        aktueller CANbus-Status
      txQueueStatus                    Status der Sendewarteschlange
      rxQueueStatus                    Status der Empfangswarteschlange

   ---------------------------------------------------------------------
   Liefert den aktuellen CANbus-Status.
   --------------------------------------------------------------------- */

extern "C" DLL_API void CanStatus(
   unsigned char queue,
   CanBusStatus* busStatus,
   CanQueueStatus* txQueueStatus,
   CanQueueStatus* rxQueueStatus);


/* ---------------------------------------------------------------------
   Funktion CanWrite

   Eingangsparameter:
      queue                            Identifikator der Warteschlange
      message                          zu sendende Nachricht

   Ausgangsparameter:
      return                           Kennzeichen, dass die Nachricht
                                          gesendet wird (Warteschlange
                                          war nicht voll)

   ---------------------------------------------------------------------
   Veranlasst das Senden einer Nachricht ueber einen CANbus.
   --------------------------------------------------------------------- */

extern "C" DLL_API unsigned char CanWrite(
   unsigned char queue,
   const CanMessage* message);


/* ---------------------------------------------------------------------
   Funktion CanRead

   Eingangsparameter:
      queue                            Identifikator der Warteschlange

   Ausgangsparameter:
      event                            Status und eventuell empfangene
                                          Nachricht

   ---------------------------------------------------------------------
   Liefert den aktuellen CANbus-Status und die naechste empfangene CAN-
   Nachricht, falls eine vorliegt.
   --------------------------------------------------------------------- */

extern "C" DLL_API void CanRead(
   unsigned char queue,
   CanEvent* event);


/* ---------------------------------------------------------------------
   Funktion CanFilterMaskDefine

   Eingangsparameter:
      queue                            Identifikator der Warteschlange
      reset                            Gesamtfilter zuvor 
                                          zuruecksetzen?
      filter                           Teilbereichsfilter

   Ausgangsparameter:
      Keine

   ---------------------------------------------------------------------
   Fuegt der Empfangswarteschlange einen Teilbereich als Durchlass- oder
   Sperrfilter hinzu. Diese Filtervariante basiert auf Bitmaskierung des 
   CANbus-Message-Identifikators.
   Ueber mehrfachen Aufruf der Funktion wird der Gesamtfilter 
   festfestgelegt. Damit eine empfangene Message von der Applikation 
   gelesen werden kann, darf sie zunaechst von keinem der 
   Sperrteilbereichsfilter ausgeschlossen werden. Danach muss sie von 
   irgendeinem Durchlassteilbereichsfilter eingeschlossen werden.
   Das Ruecksetzen des Gesamtfilters loescht alle Teilbereichsfilter und
   schliesst damit den Gesamtfilter fuer alle Messages.
   --------------------------------------------------------------------- */

extern "C" DLL_API void CanFilterMaskDefine(
   unsigned char queue,
   unsigned char reset,
   const CanFilterMask* filter);


#pragma pack()
#undef DLL_API
#endif
