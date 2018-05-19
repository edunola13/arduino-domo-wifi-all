#include <SPI.h>
#include <EEPROM.h>
//#include <MemoryFree.h>
//Serial.println(freeMemory());

// Uncomment to enable printing out nice debug messages.
#define DOMO_DEBUG
//#define DOMO_SPEED 9600
//WATCHDOG NO FUNCIONA CON ETHERNET SHIELD
//#define USE_WDT 
//#define WDT_TIME WDTO_8S
#include "vendor/igniteit/arduino-general-use/common_initial.h"
#include "messages.h"

#define resetPin 2
#define resetPoint 0
#include "vendor/igniteit/arduino-general-use/reset_manage.h"

#include "vendor/igniteit/arduino-sensors-oo/AnalogSensor.h"
#include "vendor/igniteit/arduino-sensors-oo/AnalogSensor.cpp"
#include "vendor/igniteit/arduino-sensors-oo/HumTempDHT.h"
#include "vendor/igniteit/arduino-sensors-oo/HumTempDHT.cpp"
#include "vendor/igniteit/arduino-sensors-oo/TempBase.h"
#include "vendor/igniteit/arduino-sensors-oo/TempBase.cpp"
#include "vendor/igniteit/arduino-sensors-oo/DistanceHCSR04.h"
#include "vendor/igniteit/arduino-sensors-oo/DistanceHCSR04.cpp"
#include "vendor/igniteit/arduino-sensors-oo/DigitalSensor.h"
#include "vendor/igniteit/arduino-sensors-oo/DigitalSensor.cpp"

#include "vendor/igniteit/arduino-actuators-oo/Relay.h"
#include "vendor/igniteit/arduino-actuators-oo/Relay.cpp"
#include "vendor/igniteit/arduino-actuators-oo/Buzzer.h"
#include "vendor/igniteit/arduino-actuators-oo/Buzzer.cpp"
#include "vendor/igniteit/arduino-actuators-oo/LedAlert.h"
#include "vendor/igniteit/arduino-actuators-oo/LedAlert.cpp"
#include "vendor/igniteit/arduino-actuators-oo/DigitalControl.h"
#include "vendor/igniteit/arduino-actuators-oo/DigitalControl.cpp"
#include "vendor/igniteit/arduino-basic-oo/JsonHelper.cpp"
#include "vendor/igniteit/arduino-basic-oo/ElementAbstract.cpp"

#include "vendor/igniteit/arduino-http/HttpServer.h"
#include "vendor/igniteit/arduino-http/HttpServer.cpp"
#include "vendor/igniteit/arduino-http/HttpClientAr.h"
#include "vendor/igniteit/arduino-http/HttpClientAr.cpp"

#define USE_SERVER
#define USE_NOTIFICATIONS
#define AP_SSID "AP WiFi"
#define AP_PASS "12345678"
#define AP_CHAN 10

//Sensores - Tipo 1X
#define analogSize 4
AnalogSensor analogSen[analogSize]= {};
#define humTempSize 2
HumTempDHT humTemps[humTempSize]= {};
#define tempSize 2
TempBase temps[tempSize]= {};
#define distSize 2
DistanceHCSR04 distances[distSize]= {};
#define digSenSize 4
DigitalSensor digitalSen[digSenSize]= {};

//Actuadores - Tipo 2X
#define relaysSize 3
Relay relays[relaysSize]= {};
#define buzzersSize 2
Buzzer buzzers[buzzersSize]= {};
#define ledsSize 2
LedAlert leds[ledsSize]= {};
#define ledsRgbSize 2
LedRGBAlert ledsRgb[ledsSize]= {};
#define digitalSize 6
DigitalControl digitals[digitalSize]= {DigitalControl(7, 2)};

//Controladores - Tipo 3X
#define controlRelaySize 2
ControlRelayInterval controlRelay[controlRelaySize]= {};
#define controlBuzzerSize 1
ControlBuzzerInterval controlBuzzer[controlBuzzerSize]= {};

//Routes
void controllerInfo(HttpRequest &httpRequest);
void controllerAllComponents(HttpRequest &httpRequest);
void controllerGeneric(HttpRequest &httpRequest);
void controllerControlRelays(HttpRequest &httpRequest);
void controllerControlBuzzers(HttpRequest &httpRequest);
//El primer parametro(la ruta) es lo que usariamos como clave para acceder al determinado tipo de componente.
//Luego en base a los parametros es que se hace con ese tipo de componente y con cual en especifico.
#define routesSize 14
Route routes[routesSize]= {
  {"/info", controllerInfo},
  {"/all", controllerAllComponents},
  
  {"/s/analogSen", controllerGeneric, 1, analogSize},
  {"/s/humTemps", controllerGeneric, 2, humTempSize},
  {"/s/temps", controllerGeneric, 3, tempSize},
  {"/s/dists", controllerGeneric, 4, distSize},
  {"/s/digitals", controllerGeneric, 5, digSenSize},
  
  {"/a/relays", controllerGeneric, 101, relaysSize},
  {"/c/relays", controllerControlRelays, 201, controlRelaySize},

  {"/a/buzzers", controllerGeneric, 102, buzzersSize},
  {"/c/buzzers", controllerControlBuzzers, 202, controlBuzzerSize},

  {"/a/leds", controllerGeneric, 103, ledsSize},
  {"/a/ledsRgb", controllerGeneric, 104, ledsRgbSize},

  {"/a/digitals", controllerGeneric, 105, digitalSize},
};

//CONFIGURABLE
//Tiempos de Actualizacion
uint16_t timeSensors= 30000;
uint16_t timeControls= 1000;
//USO INTERNO
//Tiempos de Actualizacion
unsigned long lasttimeSensors= 0;
unsigned long lasttimeControls= 0;
//Notificaciones
bool notification= false;

#define USE_ACTUAL_ITEM
#include "vendor/igniteit/arduino-general-use/api/json_interface_api.h"
#include "vendor/igniteit/arduino-general-use/wifi/wifi_manage.h"
#include "vendor/igniteit/arduino-general-use/api/general_controllers_api.h"

//CODE PROGRAM
#define codePro "wifi-all-1.0"

void setup() {
  //BASIC
  initialGeneric();  
  //RESET
  initialReset();
  //CONFIGURATION
  loadConfiguration(); 
  //WIFI
  initialWifi();
}

void loop() {
  actualizeIp();
  
  //ACTUALIZA SENSORES Y CONTROLADORES  
  actualizeSensors();
  actualizeControls();
  
  if(notification){
    sendNotification();
    //Si no pongo este delay entra muy rapido a la linea /*client = server.available();*/ y seguramente como que no termina de responder u algo y se clava.
    //delay(200);
  }else{
    //REVISA PETICIONES HTTP
    analizeHttpServer();
  }
}

//
//CONFIGURACION
void loadConfiguration(){
  DEB_DO_PRINTLN(loCo);
  int pos= 0;
  //Compruebo que se hayan grabado datos, si no utilizo valores por defecto
  uint8_t load= EEPROM.read(pos++);
  if(load == 1){
    //Lee configuracion global
    readConfWifi(pos);
    timeSensors= EEPROMRInt(pos); pos+= 4;
    timeControls= EEPROMRInt(pos); pos+= 4;
    
    //Lee configuracion de componentes
    pos= 200;
    //AnalogSensor
    for (uint8_t i= 0; i < analogSize; i++) {
      analogSen[i].readFromEeprom(pos);
      pos+= analogSen[i].positions();
    } 
    //HumedadTemperatura
    for (uint8_t i= 0; i < humTempSize; i++) {
      humTemps[i].readFromEeprom(pos);
      pos+= humTemps[i].positions();
    }
    //Temperatura
    for (uint8_t i= 0; i < tempSize; i++) {
      temps[i].readFromEeprom(pos);
      pos+= temps[i].positions();
    }
    //Sensor Distancia
    for (uint8_t i= 0; i < distSize; i++) {
      distances[i].readFromEeprom(pos);
      pos+= distances[i].positions();
    }
    //Sensor Digital
    for (uint8_t i= 0; i < digSenSize; i++) {
      digitalSen[i].readFromEeprom(pos);
      pos+= digitalSen[i].positions();
    }
  
    //Relays
    for (uint8_t i= 0; i < relaysSize; i++) {
      relays[i].readFromEeprom(pos);
      pos+= relays[i].positions();
    }
    //Buzzer
    for (uint8_t i= 0; i < buzzersSize; i++) {
      buzzers[i].readFromEeprom(pos);
      pos+= buzzers[i].positions();
    }
    //LedAlert
    for (uint8_t i= 0; i < ledsSize; i++) {
      leds[i].readFromEeprom(pos);
      pos+= leds[i].positions();
    }
    //LedRGBAlert
    for (uint8_t i= 0; i < ledsSize; i++) {
      ledsRgb[i].readFromEeprom(pos);
      pos+= ledsRgb[i].positions();
    }
    //DigitalControl
    for (uint8_t i= 0; i < digitalSize; i++) {
      digitals[i].readFromEeprom(pos);
      pos+= digitals[i].positions();
    }
    
    //Control Relay
    for (uint8_t i= 0; i < controlRelaySize; i++) {
      controlRelay[i].readFromEeprom(pos);
      if(controlRelay[i].getRelayCode() != 0){
        controlRelay[i].setRelay(&relays[controlRelay[i].getRelayId()], controlRelay[i].getRelayId());
      }
      if(controlRelay[i].getSensorCode() != 0){
        controlRelay[i].setSensor(getSensor(controlRelay[i].getSensorCode(), controlRelay[i].getSensorId()), controlRelay[i].getSensorId());
      }    
      pos+= controlRelay[i].positions();
    }
    //Control Buzzer
    for (uint8_t i= 0; i < controlBuzzerSize; i++) {
      controlBuzzer[i].readFromEeprom(pos);
      if(controlBuzzer[i].getBuzzerCode() != 0){
        controlBuzzer[i].setBuzzer(&buzzers[controlBuzzer[i].getBuzzerId()], controlBuzzer[i].getBuzzerId());
      }
      if(controlBuzzer[i].getSensorCode() != 0){
        controlBuzzer[i].setSensor(getSensor(controlBuzzer[i].getSensorCode(), controlBuzzer[i].getSensorId()), controlBuzzer[i].getSensorId());
      }
      pos+= controlBuzzer[i].positions();
    }    
  }else{
    DEB_DO_PRINTLN(deVa);
  }  
  DEB_DO_PRINTLN(enCo);  
}
void saveConfiguration(){
  DEB_DO_PRINTLN(saCo);
  int pos= 0;
  EEPROM.update(pos++, 1);
  //Escribo configuracion global
  saveConfWifi(pos);
  EEPROMWInt(pos, timeSensors); pos+= 4;
  EEPROMWInt(pos, timeControls); pos+= 4;  

  pos= 200;
  //AnalogSensor
  for (uint8_t i= 0; i < analogSize; i++) {
    analogSen[i].saveInEeprom(pos);
    pos+= analogSen[i].positions();
  } 
  //HumedadTemperatura
  for (uint8_t i= 0; i < humTempSize; i++) {
    humTemps[i].saveInEeprom(pos);
    pos+= humTemps[i].positions();
  }
  //Temperatura
  for (uint8_t i= 0; i < tempSize; i++) {
    temps[i].saveInEeprom(pos);
    pos+= temps[i].positions();
  }
  //Sensor Distancia
  for (uint8_t i= 0; i < distSize; i++) {
    distances[i].saveInEeprom(pos);
    pos+= distances[i].positions();
  }
  //Sensor Digital
  for (uint8_t i= 0; i < digSenSize; i++) {
    digitalSen[i].saveInEeprom(pos);
    pos+= digitalSen[i].positions();
  }

  //Relays
  for (uint8_t i= 0; i < relaysSize; i++) {
    relays[i].saveInEeprom(pos);
    pos+= relays[i].positions();
  }
  //Buzzer
  for (uint8_t i= 0; i < buzzersSize; i++) {
    buzzers[i].saveInEeprom(pos);
    pos+= buzzers[i].positions();
  }
  //LedAlert
  for (uint8_t i= 0; i < ledsSize; i++) {
    leds[i].saveInEeprom(pos);
    pos+= leds[i].positions();
  }
  //LedRGBAlert
  for (uint8_t i= 0; i < ledsSize; i++) {
    ledsRgb[i].saveInEeprom(pos);
    pos+= ledsRgb[i].positions();
  }
  //DigitalControl
  for (uint8_t i= 0; i < digitalSize; i++) {
    digitals[i].saveInEeprom(pos);
    pos+= digitals[i].positions();
  }
  
  //Control Relay
  for (uint8_t i= 0; i < controlRelaySize; i++) {
    controlRelay[i].saveInEeprom(pos);
    pos+= controlRelay[i].positions();
  }
  //Control Buzzer
  for (uint8_t i= 0; i < controlBuzzerSize; i++) {
    controlBuzzer[i].saveInEeprom(pos);
    pos+= controlBuzzer[i].positions();
  } 

char url2[50];
    EEPROM.get(44, url2);//pos+= 50;
    Serial.println(url2);
  
  DEB_DO_PRINTLN(enSa);
}

//
//SENSORES
void actualizeSensors(){
  if(millis() - lasttimeSensors >= timeSensors || millis() - lasttimeSensors < 0){   
    //AnalogSensor
    for (uint8_t i= 0; i < analogSize; i++) {
      if(analogSen[i].getStarted()){
        analogSen[i].updateSensor();
      }      
    } 
    //HumedadTemperatura
    for (uint8_t i= 0; i < humTempSize; i++) {
      if(humTemps[i].getStarted()){
        humTemps[i].updateSensor();
      }
    }
    //Temperatura
    for (uint8_t i= 0; i < tempSize; i++) {
      if(temps[i].getStarted()){
        temps[i].updateSensor();
      }
    }
    //Sensor Distancia
    for (uint8_t i= 0; i < distSize; i++) {
      if(distances[i].getStarted()){
        distances[i].updateSensor();
      }
    }
    //Sensor Digital
    for (uint8_t i= 0; i < digSenSize; i++) {
      if(digitalSen[i].getStarted()){
        digitalSen[i].updateSensor();
      }
    }
    //Para que envie la notificacion al servidor level 1
    notification= true;
    
    //Actualizo el tiempo    
    lasttimeSensors= millis(); 
  }
}
//
//CONTROLADORES
void actualizeControls(){
  bool changed= false;
  //Los controlleres me indican si hubo cambios y en ese caso activo las notificaciones
  if(millis() - lasttimeControls >= timeControls || millis() - lasttimeControls < 0){
    for (uint8_t i= 0; i < controlRelaySize; i++) {
      if(controlRelay[i].getStarted()){
        changed= changed || controlRelay[i].updateStatus();
      }      
    }

    for (uint8_t i= 0; i < controlBuzzerSize; i++) {
      if(controlBuzzer[i].getStarted()){
        changed= changed || controlBuzzer[i].updateStatus();
      }      
    }
    if(changed){
      notification= true;
    }
    
    //Actualizo el tiempo    
    lasttimeControls= millis();
  }
}

//
//METODOS HELPER
SensorInterface* getSensor(uint8_t id){
  return getSensor(actualType, id);
}
SensorInterface* getSensor(uint8_t type, uint8_t id){
  switch (type){
    case 1:
      return &analogSen[id];
    case 2:
      return &humTemps[id];
    case 3:
      return &temps[id];
    case 4:
      return &distances[id];
    case 5:
      return &digitalSen[id];
    default:
      //Sensor por defecto
      return &analogSen[id];
  }
}
//
//API REST HELPER
String toJson(uint8_t id){
  switch (actualType){
    case 1:
      return analogSen[id].toJson();
    case 2:
      return humTemps[id].toJson();
    case 3:
      return temps[id].toJson();
    case 4:
      return distances[id].toJson();
    case 5:
      return digitalSen[id].toJson();
      
    case 101:
      return relays[id].toJson();
    case 102:
      return buzzers[id].toJson();
    case 103:
      return leds[id].toJson();
    case 104:
      return ledsRgb[id].toJson();
    case 105:
      return digitals[id].toJson();
      
    case 201:
      return controlRelay[id].toJson();
    case 202:
      return controlBuzzer[id].toJson();
     
    default:
      return "{\"empty\": \"\"}";
  }
}
void parseJson(uint8_t id, String* body){
  switch (actualType){
    case 1:
      analogSen[id].parseJson(body);
      break;
    case 2:
      humTemps[id].parseJson(body);
      break;
    case 3:
      temps[id].parseJson(body);
      break;
    case 4:
      distances[id].parseJson(body);
      break;
    case 5:
      digitalSen[id].parseJson(body);
      break;
      
    case 101:
      relays[id].parseJson(body);
      break;
    case 102:
      buzzers[id].parseJson(body);
      break;
    case 103:
      leds[id].parseJson(body);
      break;
    case 104:
      ledsRgb[id].parseJson(body);
      break;
    case 105:
      digitals[id].parseJson(body);
      break;
      
    case 201:
      controlRelay[id].parseJson(body);
      break;
    case 202:
      controlBuzzer[id].parseJson(body);
      break;
      
    default:
      break;
  }  
}

//
//CONTROLADORES ESPECIFICOS POR RUTAS
//Controladores
void controllerControlRelays(HttpRequest &httpRequest){
  if(httpRequest.getMethod() == "GET"){
    controllerGeneric(httpRequest);
  }
  if(httpRequest.getMethod() == "PUT"){
    uint8_t id= httpRequest.getParam("id").toInt();
    if(id >= 0 && id < actualSize){
      uint8_t relayId= httpRequest.getParam("acId").toInt();
      if(relayId >= 0){
        controlRelay[id].setRelay(&relays[relayId], relayId); 
      }
      uint8_t type= httpRequest.getParam("seCo").toInt();
      uint8_t sensorId= httpRequest.getParam("seId").toInt();
      if(sensorId >= 0){
        controlRelay[id].setSensor(getSensor(type, sensorId), sensorId);
      }
      
      parseJson(id, httpRequest.getPunteroBody());    
      server.sendApiRest(client, 200, msjOk);
    } 
  }  
}
void controllerControlBuzzers(HttpRequest &httpRequest){
  if(httpRequest.getMethod() == "GET"){
    controllerGeneric(httpRequest);
  }
  if(httpRequest.getMethod() == "PUT"){
    uint8_t id= httpRequest.getParam("id").toInt();
    if(id >= 0 && id < actualSize){
      uint8_t buzzerId= httpRequest.getParam("acId").toInt();
      if(buzzerId >= 0){
        controlBuzzer[id].setBuzzer(&buzzers[buzzerId], buzzerId); 
      }
      uint8_t type= httpRequest.getParam("seCo").toInt();
      uint8_t sensorId= httpRequest.getParam("seId").toInt();
      if(sensorId >= 0){      
        controlBuzzer[id].setSensor(getSensor(type, sensorId), sensorId);
      }
      
      parseJson(id, httpRequest.getPunteroBody());    
      server.sendApiRest(client, 200, msjOk);
    }
  }
}
void controllerInfo(HttpRequest &httpRequest){
  if(httpRequest.getMethod() == "POST"){
    saveConfiguration();
    server.sendApiRest(client, 200, msjOk);
  }
  if(httpRequest.getMethod() == "PUT"){
    String value= parseProperty(httpRequest.getPunteroBody(), "tiS");
    if(isNotNull(value)){ timeSensors= value.toInt(); }  
    value= parseProperty(httpRequest.getPunteroBody(), "tiC");
    if(isNotNull(value)){ timeControls= value.toInt(); }
    
    updateWifi(httpRequest);
    
    server.sendApiRest(client, 200, "{\"msj\": \"ok\"}");
  }
  if(httpRequest.getMethod() == "GET"){
    server.sendApiRest(client, 200, "{");
      server.partialSendApiRest(client, "\"api\": 1.0,");
      server.partialSendApiRest(client, "\"mother\": \"arduino uno\",");
      server.partialSendApiRest(client, "\"code\": \"");
      server.partialSendApiRest(client, codePro);
      server.partialSendApiRest(client, "\",");
      server.partialSendApiRest(client, "\"sensors\": [");
        server.partialSendApiRest(client, "\"/s/analogSen\",");
        server.partialSendApiRest(client, "\"/s/humTemps\",");
        server.partialSendApiRest(client, "\"/s/temps\",");
        server.partialSendApiRest(client, "\"/s/dists\",");
        server.partialSendApiRest(client, "\"/s/digitalSen\"");
      server.partialSendApiRest(client, "],");
      server.partialSendApiRest(client, "\"actuators\": [");
        server.partialSendApiRest(client, "\"/a/relays\",");
        server.partialSendApiRest(client, "\"/a/buzzers\",");
        server.partialSendApiRest(client, "\"/a/leds\",");
        server.partialSendApiRest(client, "\"/a/ledsRgb\",");
        server.partialSendApiRest(client, "\"/a/digitals\"");
      server.partialSendApiRest(client, "],");
      server.partialSendApiRest(client, "\"controls\": [");
        server.partialSendApiRest(client, "\"/c/relays\"");
        server.partialSendApiRest(client, ",");
        server.partialSendApiRest(client, "\"/c/buzzers\"");
      server.partialSendApiRest(client, "],");
      server.partialSendApiRest(client, "\"config\": {");
        server.partialSendApiRest(client, "\"tiS\": " + String(timeSensors) + ",");
        server.partialSendApiRest(client, "\"tiC\": " + String(timeControls) + ",");
        jsonWifi();
      server.partialSendApiRest(client, "}");
    server.partialSendApiRest(client, "}");
  }
}
void controllerAllComponents(HttpRequest &httpRequest){
  server.sendApiRest(client, 200, "{");
  server.partialSendApiRest(client, "\"sensors\": [");
    sendListToJson("/s/analogSen", 1, analogSize);
    server.partialSendApiRest(client, ",");
    sendListToJson("/s/humTemps", 2, humTempSize);
    server.partialSendApiRest(client, ",");
    sendListToJson("/s/temps", 3, tempSize);
    server.partialSendApiRest(client, ",");
    sendListToJson("/s/dists", 4, distSize);
    server.partialSendApiRest(client, ",");
    sendListToJson("/s/digitalSen", 5, digSenSize);
  server.partialSendApiRest(client, "],");
  server.partialSendApiRest(client, "\"actuators\": [");
    sendListToJson("/a/relays", 101, relaysSize);
    server.partialSendApiRest(client, ",");
    sendListToJson("/a/buzzers", 102, buzzersSize);
    server.partialSendApiRest(client, ",");
    sendListToJson("/a/leds", 103, ledsSize);
    server.partialSendApiRest(client, ",");
    sendListToJson("/a/ledsRgb", 104, ledsRgbSize);
    server.partialSendApiRest(client, ",");
    sendListToJson("/a/digitals", 105, digitalSize);
  server.partialSendApiRest(client, "],");
  server.partialSendApiRest(client, "\"controls\": [");
    sendListToJson("/c/relays", 201, controlRelaySize);
    server.partialSendApiRest(client, ",");
    sendListToJson("/c/buzzers", 202, controlBuzzerSize);    
  server.partialSendApiRest(client, "]");
  server.partialSendApiRest(client, "}");
}
