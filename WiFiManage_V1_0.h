//
//ULTIMA MODIFICACION: 16-10-2017

//Servidor HTTP
#ifdef USE_SERVER
HttpWiFiServer server(80);
#endif
//Cliente HTTP
WiFiEspClient client;

//ESTADO
int status = WL_IDLE_STATUS;     // the Wifi radio's status
//CONFIGURABLE
IPAddress ip(192,168,0,51);
//SSID
char ssid[25] = "Fibertel WiFi756";            // your network SSID (name)
char pass[12] = "0143507310";        // your network password

#ifdef USE_NOTIFICATIONS
  //Notificaciones
  HttpWifiClientAr notifier;
  //IPAddress notifierip(0,0,0,0);
  IPAddress notifierip(192,168,0,11);
  String urlNotifier= "/ArduinoTest/simple_json.php";
  String keyNotifier= "";
  bool waitRta= false;
#endif

//
//INICIO
void initialWifi(){
  Serial3.begin(115200);
  // initialize ESP module
  WiFi.init(&Serial3);

  // check for the presence of the shield
  if (WiFi.status() == WL_NO_SHIELD) {
    DEB_DO_PRINTLN("WiFi shield not present");
    // don't continue
    while (true);
  }

  if(ssid[0] != '\0'){
    DEB_DO_PRINTLN("Conectandose a una Red");
    //Conecto a la red
    uint8_t rein= 0;
    while (status != WL_CONNECTED && rein < 3){
      DEB_DO_PRINTLN("Attempting to connect to WPA SSID: ");
      DEB_DO_PRINTLN(ssid);
      // Connect to WPA/WPA2 network
      status = WiFi.begin(ssid, pass);
  
      rein++;
    }
    if(status == WL_CONNECTED){
      if(ip[0] == 0){
        ip= WiFi.localIP();    
      }else if(ip != WiFi.localIP()){
        WiFi.config(ip);
      }
    }
  }else{
    DEB_DO_PRINTLN("Creando AP");
    char ap_ssid[]= AP_SSID;
    char ap_pass[]= AP_PASS;
    status = WiFi.beginAP(ap_ssid, AP_CHAN, ap_pass, ENC_TYPE_WPA2_PSK);
  }
  
  DEB_DO_PRINTLN(WiFi.localIP());
  server.begin();
}

#ifdef USE_NOTIFICATIONS
  //
  //NOTIFICACIONES
  bool sendNotification(){  
    DEB_DO_PRINTLN(seNo);
    if(notifierip[0] == 0){
      notification= false;
      DEB_DO_PRINTLN(noNCon);
      return false;
    }
    notifier.stop();
    if(notifier.connect(notifierip, 80)){
      String method= "GET";
      String url= urlNotifier;
      if(keyNotifier != ""){
        url+= "key=" + keyNotifier;
      }
      String contentType= "application/json";
      notifier.sendRequest(method, url);  
      DEB_DO_PRINTLN(noSe); 
    }else{
      DEB_DO_PRINTLN(noNCo);
    }
    //Espera RTA - En este caso no me importa
    if(waitRta){
      while(notifier.connected() && !notifier.available()){
        delay(1); //waits for data
      }
      if(notifier.available()) {
        notification= false;
        HttpResponse httpResponse= notifier.httpResponse();
        DEB_DO_PRINTLN(httpResponse.getBody());
      }
    }
    notification= false;
    notifier.stop();
    return true;
  }
#endif

//
//CONFIGURACION
void readConfWifi(uint8_t &pos){
  EEPROM.get(pos, ip);pos+= 6;
  EEPROM.get(pos, ssid);pos+= 25;
  EEPROM.get(pos, pass);pos+= 12;  
}
void saveConfWifi(uint8_t &pos){
  EEPROM.put(pos, ip);pos+= 6;
  EEPROM.put(pos, ssid);
  EEPROM.put(pos, pass);pos+= 12; 
}

//
//JSON
void jsonWifi(){
  server.partialSendApiRest(client, "\"ssid\": \"");
  server.partialSendApiRest(client, ssid);
  server.partialSendApiRest(client, "\",");
  server.partialSendApiRest(client, "\"pass\": \"");
  server.partialSendApiRest(client, pass);
  server.partialSendApiRest(client, "\",");
  server.partialSendApiRest(client, "\"ip\": \"" + String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]) + "\"");
}

//
//UPDATE
void updateWifi(HttpRequest &httpRequest){
  //SSID
  String value= parseProperty(httpRequest.getPunteroBody(), "ssid", 25);
  if(isNotNull(value)){ value.toCharArray(ssid, 25); }
  //PASS
  value= parseProperty(httpRequest.getPunteroBody(), "pass", 12);
  if(isNotNull(value)){ value.toCharArray(pass, 12); }
  //IP
  value= parseProperty(httpRequest.getPunteroBody(), "ip1");
  if(isNotNull(value)){ ip[0]= value.toInt(); }    
  value= parseProperty(httpRequest.getPunteroBody(), "ip2");
  if(isNotNull(value)){ ip[1]= value.toInt(); }    
  value= parseProperty(httpRequest.getPunteroBody(), "ip3");
  if(isNotNull(value)){ ip[2]= value.toInt(); }    
  value= parseProperty(httpRequest.getPunteroBody(), "ip4");
  if(isNotNull(value)){ ip[3]= value.toInt(); }    
}

