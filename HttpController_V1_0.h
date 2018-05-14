//
//ULTIMA MODIFICACION: 16-10-2017

#ifdef USE_ACTUAL_ITEM
  //Componente actual
  uint8_t actualType;
  uint8_t actualSize;

  void sendListToJson(String name, uint8_t type, uint8_t size){
    actualType= type;
    actualSize= size;
    server.partialSendApiRest(client, "{\"type\": \""+name+"\",");
    server.partialSendApiRest(client, "\"data\": [");
    for(int i= 0; i < actualSize; i++){
      server.partialSendApiRest(client, toJson(i));
      if(i < actualSize - 1){
        server.partialSendApiRest(client, ",");
      }    
    }  
    server.partialSendApiRest(client, "]}");
  }
#endif

void controllerCentral(HttpRequest &httpRequest){
  if(httpRequest.getMethod() == "OPTIONS"){
    server.sendApiRest(client, 200, msjOk);
  }else{
    //A este se le pasa el json que llego y decide a que metodo le pasa el control  
    //Armo un switch de METHOD+URL y en base a eso veo que metodo llamo
    for(uint8_t i= 0; i < routesSize; i++){
      if(String(routes[i].url) == httpRequest.getUrl()){ 
        #ifdef USE_ACTUAL_ITEM
          actualType= routes[i].type;
          actualSize= routes[i].cant;    
        #endif
        routes[i].functionPointer(httpRequest);
        break;
      }
    }
  }
}

void analizeHttpServer(){
  //listen for incoming clients
  client = server.available();
  if (client) { 
    DEB_DO_PRINTLN(cliAv);
    HttpRequest httpRequest= server.httpRequest(client); 
    controllerCentral(httpRequest);    
    // give the web browser time to receive the data
    delay(10);
    // close the connection:
    client.stop();
    DEB_DO_PRINTLN(cliDi);
  }
}

//
//GENERIC CONTROLLER
#ifdef USE_ACTUAL_ITEM
  void controllerGeneric(HttpRequest &httpRequest){  
    int id= httpRequest.getParam("id").toInt();
    if(httpRequest.getMethod() == "GET" && id >= 0 && id < actualSize){
      server.sendApiRest(client, 200, toJson(id));
    }
    if(httpRequest.getMethod() == "GET" && id == -1){
      String json= "{\"data\": [";
      server.sendApiRest(client, 200, json);
      for(uint8_t i= 0; i < actualSize; i++){      
        json= toJson(i);
        if(i < actualSize - 1){
          json+= ",";
        }
        client.println(json);
      }
      json= "]}";
      client.println(json);
    }
    if(httpRequest.getMethod() == "PUT" && id >= 0 && id < actualSize){
      parseJson(id, httpRequest.getPunteroBody());    
      server.sendApiRest(client, 200, msjOk);
    }
  }
#endif
