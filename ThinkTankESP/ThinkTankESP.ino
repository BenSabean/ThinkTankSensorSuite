/*
   AERClient library Example Code
   Date: Feb 24, 2018
*/

#include <ESP8266WiFi.h>          // ESP WiFi Libarary
#include <PubSubClient.h>         // MQTT publisher/subscriber client 
#include <AERClient.h>

//////////////////////////////////////
//    CHANGE TO YOUR UNIQUE ID      //
#define DEVICE_ID 8                 //
//                                  //
//////////////////////////////////////

#define BUFFSIZE 20

/* Wifi setup */
const char ssid[] =        "";
const char password[] =    "";

char msg[BUFFSIZE];  // Container for serial message

/* Create Library Object password */
AERClient server(DEVICE_ID);

void setup()
{
  Serial.begin(9200);
  /* Initialization and connection to WiFi */
  server.init(ssid, password);
  Serial.println("Connected!");
}

void loop()
{
  // Empty Serial container
  memset(msg, NULL, BUFFSIZE);
  
  //server.debug();
  // Publish Data
  if(Serial.available()) {
    Serial.readString().toCharArray(msg, BUFFSIZE);
    Serial.print("Got: ");    Serial.println(msg);
    if(server.publish("Test", msg)) {
      Serial.println("Msg Sent!");
    }
    else {
      Serial.println("Msg Failed!");
    }
  }
}





