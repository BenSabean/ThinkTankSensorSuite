/*
   Think Tank ESP Communication
   Date: May 1, 2018
*/

#include <AERClient.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>

// This device's unique ID
#define DEVICE_ID 8

#define BUFFSIZE 20
#define TIMEOUT 60
#define BUTTON 14
#define STATUS_LIGHT 13
#define AP_SSID  "ESP Network Setup"

/* Wifi setup */
char ssid[BUFFSIZE];
char password[BUFFSIZE];

char msg[BUFFSIZE];  // Container for serial message
bool apMode = false; // Flag to dermine current mode of operation

/* Create Library Object password */
AERClient aerServer(DEVICE_ID);
ESP8266WebServer server(80);

//------------------------------------------------------
// Get a string from EEPROM
// @param startAddr the starting address of the string
//            in EEPROM
// @return char* the current string at the address given
//            by startAddr
//-------------------------------------------------------
char* getString(int startAddr) {
  char str[BUFFSIZE];
  memset(str, NULL, BUFFSIZE);
  
  for (int i = 0; i < BUFFSIZE; i ++) {
    str[i] = char(EEPROM.read(startAddr + i));
  }
  return str;
}

//------------------------------------------------------
// Page to inform the user they successfully changed
// Wi-Fi credentials
// @param none
// @return void
//-------------------------------------------------------
void handleSubmit() {
  String content = "<html><body><H2>WiFi information updated!</H2><br>";
  server.send(200, "text/html", content);
  digitalWrite(STATUS_LIGHT, LOW);
  WiFi.mode(WIFI_STA);
  //getString(0);         // Username
  //getString(BUFFSIZE);   // Password
  //waitForConnect();
  strcpy(ssid, getString(0));            // SSID
  strcpy(password, getString(BUFFSIZE)); // Password
  aerServer.enableReconnect();           // Allow AERClient to reconnect to Wi-Fi
  if (aerServer.init(ssid, password)) {
    Serial.println("Connected!");
    aerServer.debug();
  }
  else {
    Serial.println("Connection timed out");
    Serial.print("SSID: ");    Serial.println(ssid);
    Serial.print("Password: ");    Serial.println(password);
  }
  apMode = false;  
  Serial.readString();          // Empty serial buffer
}

//------------------------------------------------------
// Page to enter new Wi-Fi credentials
// @param none
// @return void
//-------------------------------------------------------
void handleRoot() {
  String msg;
  if (server.hasArg("SSID") && server.hasArg("PASSWORD")) {
    char newSSID[BUFFSIZE];
    char newPw[BUFFSIZE];
    memset(newSSID, NULL, BUFFSIZE);
    memset(newPw, NULL, BUFFSIZE);
    server.arg("SSID").toCharArray(newSSID, BUFFSIZE);
    server.arg("PASSWORD").toCharArray(newPw, BUFFSIZE);
    Serial.println("SSID: " + String(newSSID));
    Serial.println("Password: " + String(newPw));

    for (int i = 0; i < BUFFSIZE; i++) {
      EEPROM.write(i, newSSID[i]);              // Write SSID to EEPROM
      EEPROM.write((i + BUFFSIZE), newPw[i]);   // Write Password to EEPROM
    }
    EEPROM.commit();

    server.sendContent("HTTP/1.1 301 OK\r\nLocation: /success\r\nCache-Control: no-cache\r\n\r\n");
    return;
  }
  String content = "<html><body><form action='/' method='POST'>Please enter new SSID and password.<br>";
  content += "SSID:<input type='text' name='SSID' placeholder='SSID'><br>";
  content += "Password:<input type='password' name='PASSWORD' placeholder='password'><br>";
  content += "<input type='submit' name='SUBMIT' value='Submit'></form>" + msg + "<br>";
  server.send(200, "text/html", content);
}

//------------------------------------------------------
// Page displayed on HTTP 404 not found error
// @param none
// @return void
//-------------------------------------------------------
void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

//------------------------------------------------------
// Hardware interrupt triggered by button press. Starts
// soft AP mode
// @param none
// @return void
//-------------------------------------------------------
void btnHandler() {
  digitalWrite(STATUS_LIGHT, HIGH);
  Serial.println("\nButton pressed!");

  if (!apMode) {
    // AP SERVER
    Serial.print("Setting soft-AP ... ");
    WiFi.mode(WIFI_AP_STA);
    if (WiFi.softAP(AP_SSID)) {
      Serial.println("Ready");
    }
    else {
      Serial.println("Failed");
      return;
    }
    Serial.println("");

    // Wait for connection
    // waitForConnect();

    server.on("/", handleRoot);
    server.on("/success", handleSubmit);
    server.on("/inline", []() {
      server.send(200, "text/plain", "this works without need of authentification");
    });

    server.onNotFound(handleNotFound);
    //here the list of headers to be recorded
    const char * headerkeys[] = {
      "User-Agent", "Cookie"
    } ;
    size_t headerkeyssize = sizeof(headerkeys) / sizeof(char*);
    //ask server to track these headers
    server.collectHeaders(headerkeys, headerkeyssize );
    server.begin();
    Serial.println("HTTP server started");

    // Stop AERClient from reconnecting to Wi-Fi so that HTTP server
    // (in soft AP mode) will be more responsive when Wi-Fi credentials
    // are incorrect.
    aerServer.disableReconnect();
    apMode = true;
  }
  else {
    Serial.println("Already in soft AP mode!");
  }
}

//------------------------------------------------------
// Clears all of EEPROM
// @param none
// @return void
//-------------------------------------------------------
void clearEEPROM() {
  // write a 0 to all 512 bytes of the EEPROM
  for (int i = 0; i < 512; i++)
    EEPROM.write(i, 0);
}

void setup()
{
  // Set pins and baud rate
  Serial.begin(9600);
  delay(10);  // Wait for serial port to connect
  pinMode(BUTTON, INPUT_PULLUP);
  pinMode(STATUS_LIGHT, OUTPUT);
  Serial.println("\nStart");

  // Empty ssid and password buffers
  memset(ssid, NULL, BUFFSIZE);
  memset(password, NULL, BUFFSIZE);

  // Start communication with EEPROM
  EEPROM.begin(512);
  //delay(500);
  strcpy(ssid, getString(0));
  strcpy(password, getString(BUFFSIZE));
  // Initialization and connection to WiFi
  if (aerServer.init(ssid, password)) {
    Serial.println("Connected!");
    aerServer.debug();
  }
  else {
    Serial.println("Connection timed out");
    Serial.print("SSID: ");    Serial.println(ssid);
    Serial.print("Password: ");    Serial.println(password);
  }
  attachInterrupt(digitalPinToInterrupt(BUTTON), btnHandler, FALLING);
}

void loop()
{
  if (apMode) {
    server.handleClient();
  }
  else {
    // Empty Serial container
    memset(msg, NULL, BUFFSIZE);

    // Publish Data
    if (Serial.available()) {
      Serial.readString().toCharArray(msg, BUFFSIZE);
      Serial.print("Got: ");    Serial.println(msg);
      if (aerServer.publish("Test", msg)) {
        Serial.println("Msg Sent!");
      }
      else {
        Serial.println("Msg Failed!");
      }
    }
  }
}





