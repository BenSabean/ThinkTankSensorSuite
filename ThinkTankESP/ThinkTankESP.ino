/*
   Think Tank ESP Communication
   Date: May 1, 2018
*/

// MQTT, HTTP & Soft AP
#include <AERClient.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
// Display
#include <ESP_SSD1306.h>    // Modification of Adafruit_SSD1306 for ESP8266 compatibility
#include <Adafruit_GFX.h>   // Needs a little change in original Adafruit library (See README.txt file)
#include <SPI.h>            // For SPI comm (needed for not getting compile error)
#include <Wire.h>           // For I2C comm

// This device's unique ID
#define DEVICE_ID 8

// Constants definition
#define BUFFSIZE 80
#define TIMEOUT 60        // Timeout for putting ESP into soft AP mode
#define AP_SSID  "Think Tank"
#define DELIM ":"

// Pin defines
#define BUTTON 14         // Interrupt to trigger soft AP mode
#define STATUS_LIGHT 13   // Light to indicate that HTTP server is running
#define OLED_RESET  16    // Pin 15 -RESET digital signal

/* Wifi setup */
char ssid[BUFFSIZE];
char password[BUFFSIZE];
char ip[BUFFSIZE] = "192.168.4.1";
//char msg[BUFFSIZE];  // Container for serial message
bool apMode = false; // Flag to dermine current mode of operation

/* Create Library Object password */
AERClient aerServer(DEVICE_ID);
ESP8266WebServer server(80);

ESP_SSD1306 display(OLED_RESET); // FOR I2C

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

void writeToDisplay(char* header, char* msg) {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println(header);
  display.println(msg);
  display.display();
}

//------------------------------------------------------
// Page to inform the user they successfully changed
// Wi-Fi credentials
// @param none
// @return void
//-------------------------------------------------------
void handleSubmit() {
  apMode = false;
  String content = "<html><body><H2>WiFi information updated!</H2><br>";
  server.send(200, "text/html", content);
  digitalWrite(STATUS_LIGHT, LOW);
  //delay(500);
  ESP.restart();
  WiFi.mode(WIFI_STA);
  strcpy(ssid, getString(0));            // SSID
  strcpy(password, getString(BUFFSIZE)); // Password
  writeToDisplay("Connecting", ssid);
  aerServer.enableReconnect();           // Allow AERClient to reconnect to Wi-Fi
  if (aerServer.init(ssid, password)) {
    Serial.println("Connected!");
    writeToDisplay("Connected!", ssid);
    aerServer.debug();
  }
  else {
    Serial.println("Connection timed out");
    writeToDisplay("Timed out", ssid);
    Serial.print("SSID: ");    Serial.println(ssid);
    Serial.print("Password: ");    Serial.println(password);
  }
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
    WiFi.disconnect(true);
    //delay(500);

    // AP SERVER
    Serial.println("Setting soft-AP");
    WiFi.mode(WIFI_AP_STA);

    for (int i = 0; i < TIMEOUT; i++) {
      if (WiFi.softAP(AP_SSID)) {
        break;
      }
      delay(10);
    }
    Serial.println("Ready");


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

  // SSD1306 OLED display init
  display.begin(SSD1306_SWITCHCAPVCC);
  display.display();
  delay(2000);
  display.clearDisplay();  //

  // Empty ssid and password buffers
  memset(ssid, NULL, BUFFSIZE);
  memset(password, NULL, BUFFSIZE);

  // Start communication with EEPROM
  EEPROM.begin(512);
  strcpy(ssid, getString(0));
  strcpy(password, getString(BUFFSIZE));
  // Initialization and connection to WiFi
  writeToDisplay("Connecting", ssid);
  if (aerServer.init(ssid, password)) {
    Serial.println("Connected!");
    writeToDisplay("Connected!", ssid);
    aerServer.debug();
  }
  else {
    Serial.println("Connection timed out");
    writeToDisplay("Timed out", ssid);
    Serial.print("SSID: ");    Serial.println(ssid);
    Serial.print("Password: ");    Serial.println(password);
  }
  attachInterrupt(digitalPinToInterrupt(BUTTON), btnHandler, FALLING);
  Serial.readString();   // Empty Serial buffer
}

void loop()
{
  if (apMode) {
    char buf[BUFFSIZE];
    String tmp = "";
    memset(buf, NULL, BUFFSIZE);
    tmp = String(AP_SSID) + "\n" + String(ip);
    tmp.toCharArray(buf, BUFFSIZE);
    writeToDisplay("Soft-AP", buf);

    server.handleClient();
  }
  else {
    // Publish Data
    char ch;          // character pointer for strtok
    char sAddr[BUFFSIZE], sVal[BUFFSIZE];
    char msg[BUFFSIZE] = "";

    while (Serial.available()) {
      for (int i = 0; i < BUFFSIZE; i ++) {
        ch = Serial.read();
        if (String(ch) == String(DELIM)) {
          break;
        }
        msg[i] = ch;         // Get Address
        Serial.print(ch);
        delay(10);           // Wait for Serial buffer
      }
      Serial.println();
      strcpy(sAddr, msg);
      Serial.print("Header: "); Serial.println(sAddr);

      memset(msg, NULL, BUFFSIZE);
      for (int i = 0; i < BUFFSIZE; i ++) {
        ch = Serial.read();
        if (String(ch) == String("\n")) {
          break;
        }
        msg[i] = ch;         // Get Data
        Serial.print(ch);
        delay(10);           // Wait for Serial buffer
      }
      Serial.println();
      strcpy(sVal, msg);
      Serial.print("Data: "); Serial.println(sVal);
      
      if (sAddr[0] == '[') {         // Data
        char topic[BUFFSIZE];
        memset(topic, NULL, BUFFSIZE);
        strcat(topic, "DATA/");
        strcat(topic, sAddr);
        Serial.println(topic);
        Serial.println(sVal);
        if (aerServer.publish(topic, sVal)) {
          Serial.println("Msg Sent!");
        }
        else {
          Serial.println("Msg Failed!");
        }
      }
      else {                         // Runtime Info
        char topic[BUFFSIZE];
        memset(topic, NULL, BUFFSIZE);
        strcat(topic, "System/");
        strcat(topic, sAddr);
        //Serial.println(topic);
        //Serial.println(sVal);
        if (aerServer.publish(topic, sVal)) {
          Serial.println("Msg Sent!");
        }
        else {
          Serial.println("Msg Failed!");
        }
      }
    }
  }
}





