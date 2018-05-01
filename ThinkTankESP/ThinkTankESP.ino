/*
   Think Tank ESP Communication 
   Date: May 1, 2018
*/

#include <AERClient.h>


// This device's unique ID    
#define DEVICE_ID 8                

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





