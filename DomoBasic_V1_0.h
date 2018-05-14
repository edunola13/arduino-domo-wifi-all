//
//ULTIMA MODIFICACION: 16-10-2017

#ifdef USE_WDT
  #include <MemoryFree.h>
#endif
//Serial.println(freeMemory());

//MENSAJES JSON
#define msjOk "{\"msj\": \"ok\"}"
#define msjOkTy2 "msj=ok|"
//MENSAJES PRINT
#define chaIp "Cambiando IP"
#define cliAv "client avalaible"
#define cliDi "client disconnected"
#define seNo "Enviando Notificacion"
#define noNCo "Notificador sin Configurar"
#define noSe "Notificacion enviada"
#define noNCon "No se pudo conectar al notificador"
#define loCo "Cargando Configuracion"
#define deVa "Valores por defecto"
#define enCo "Configuracion Finalizada"
#define reCo "Reseteando Configuracion"
#define enRe "Reseteo Finalizado"
#define saCo "Guardando Configuracion"
#define enSa "Configuracion Guardada"

// Define where debug output will be printed.
#define DEB_DO_PRINTER Serial
// Setup debug printing macros.
#ifdef DOMO_DEBUG
  #define DEB_DO_PRINT(...) { DEB_DO_PRINTER.print(__VA_ARGS__); }
  #define DEB_DO_PRINTLN(...) { DEB_DO_PRINTER.println(__VA_ARGS__); }
#else
  #define DEB_DO_PRINT(...) {}
  #define DEB_DO_PRINTLN(...) {}
#endif

void initialDomo(){
  #ifdef USE_WDT
    wdt_disable(); //Siempre es bueno empezar deshabilitando
    wdt_enable(WDTO_8S); //Seteo el Watch Dog en 8s
  #endif
  #ifdef DOMO_DEBUG
    Serial.begin(115200);  
    while (!Serial) {; // wait for serial port to connect. Needed for native USB port only
    }
  #endif
}
