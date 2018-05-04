/*
   Think Tank ESP Communication
   Date: May 1, 2018
*/

#include <AERClient.h>
#include <ESP8266WebServer.h>

// This device's unique ID
#define DEVICE_ID 8

#define BUFFSIZE 20
#define BUTTON 14
#define STATUS_LIGHT 13
#define AP_SSID  "ESP Network Setup"

/* Wifi setup */
char ssid[] =        "";
char password[] =    "";

char msg[BUFFSIZE];  // Container for serial message
bool apMode = false;

/* Create Library Object password */
AERClient aerServer(DEVICE_ID);
ESP8266WebServer server(80);

//login page, also called for disconnect
void handleSubmit() {
  String content = "<html><body><H2>WiFi information updated!</H2><br>";
  server.send(200, "text/html", content);
  digitalWrite(STATUS_LIGHT, LOW);
  WiFi.mode(WIFI_STA);
  waitForConnect();
  apMode = false;
}

//root page can be accessed only if authentification is ok
void handleRoot() {
  String msg;
  if (server.hasArg("SSID") && server.hasArg("PASSWORD")) {
    Serial.println("SSID: " + server.arg("SSID"));
    Serial.println("Password: " + server.arg("PASSWORD"));
    server.sendContent("HTTP/1.1 301 OK\r\nLocation: /success\r\nCache-Control: no-cache\r\n\r\n");
    return;
  }
  String content = "<html><body><form action='/' method='POST'>Please enter new SSID and password.<br>";
  content += "SSID:<input type='text' name='SSID' placeholder='SSID'><br>";
  content += "Password:<input type='password' name='PASSWORD' placeholder='password'><br>";
  content += "<input type='submit' name='SUBMIT' value='Submit'></form>" + msg + "<br>";
  server.send(200, "text/html", content);
}

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
    waitForConnect();

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

    apMode = true;
  }
  else {
    Serial.println("Already in soft AP mode!");
  }
}

void waitForConnect() {
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // Wi-Fi debug info
  Serial.println("Connected!");
  Serial.println("------- WiFi DEBUG ------- ");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  WiFi.printDiag(Serial);
  Serial.println();
}

void setup()
{
  // Set pins and baud rate
  Serial.begin(9600);
  delay(10);  // Wait for serial port to connect
  pinMode(BUTTON, INPUT_PULLUP);
  pinMode(STATUS_LIGHT, OUTPUT);

  Serial.println("\nStart");
  // Initialization and connection to WiFi
  aerServer.init(ssid, password);
  Serial.println("Connected!");
  aerServer.debug();
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





