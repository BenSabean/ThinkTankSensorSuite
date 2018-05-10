/*
   Reading Temperature Data using Arduino Nano
   Date: Apr 30, 2018
*/

#include <OneWire.h>                // Library for One-Wire interface
#include <DallasTemperature.h>      // Library for DS18B20 temp. sensor
#include <stdio.h>

#define DEBUG true
#define SAMPLE_RATE 5000
#define ADDR_LENGTH 8
#define DELIM ":"

//-----------------Dallas_Library-----------------//
#define ONE_WIRE_GPIO 2               // Starting data pin of the bus
#define ONE_WIRE_LENGTH 4             // The number of pins used by the bus
#define CALLIBRATION 2                // Used to callibrate the sensors (in C)
volatile uint8_t oneWire_count = 0;
// Setting up the interface for OneWire communication
//OneWire oneWire = OneWire(ONE_WIRE_GPIO);

OneWire oneWire[]  = {
  OneWire(ONE_WIRE_GPIO),
  OneWire(ONE_WIRE_GPIO + 1),
  OneWire(ONE_WIRE_GPIO + 2),
  OneWire(ONE_WIRE_GPIO + 3)
};

// Creating an instans of DallasTemperature Class with reference to OneWire interface
DallasTemperature Dallas_Library[] = {
  DallasTemperature(&oneWire[0]),
  DallasTemperature(&oneWire[1]),
  DallasTemperature(&oneWire[2]),
  DallasTemperature(&oneWire[3])
};
// Sensor structure with address and value
struct oneWire_struct {
  byte address[ADDR_LENGTH];
  float value;
};

oneWire_struct *pOneWire;

/*
   Initialized oneWire sensors + gets the number of sensors
*/
uint8_t TempSensors_init () {
  uint8_t _count = 0;
  for (int i = 0; i <  ONE_WIRE_LENGTH; i++) {
    Dallas_Library[i].begin();               // initializing the sensors
    _count += Dallas_Library[i].getDeviceCount();// getting number of sensors
    Dallas_Library[i].requestTemperatures(); // requesting data
  }
  return _count;
}

/*
   Reading one sensor
   Modifing pointer for one sensor with address and value
*/
void TempSensors_getTemp( oneWire_struct **_sensor) {
  int index = 0;
  for (int i = 0; i <= ONE_WIRE_LENGTH; i++) {
    while (oneWire[i].search((*_sensor + index)->address)) {
      //Serial.println("index: " + String(index));
      (*_sensor + index)->value = Dallas_Library[i].getTempC((*_sensor + index)->address);
      (*_sensor + index)->value -= CALLIBRATION;
      index++;
    }
  }
}

void ReadSensors(oneWire_struct TempSensor[]) {
  // Create a pointer to pass in the full array of structures
  pOneWire = &TempSensor[0];

  // Getting temperature
  TempSensors_getTemp(&pOneWire);
}

void PrintValues(oneWire_struct TempSensor[]) {
  Serial.print("DeviceCount" + String(DELIM) + String(oneWire_count) + "\n");
  delay(500);
  // Display current sensor readings and addresses
  for (uint8_t i = 0; i < oneWire_count; i++) {
    Serial.print("[");
    Serial.print(int(TempSensor[i].address[0]), HEX);
    for (int x = 1; x < ADDR_LENGTH; x++) {
      Serial.print(", ");
      Serial.print(int(TempSensor[i].address[x]), HEX);
    }
    Serial.print("]" + String(DELIM));
    Serial.print(String(TempSensor[i].value) + "\n");
    delay(500);
  }
}

// ------------------SETUP--------------------//
void setup() {
  /* Initializing OneWire Sensors */
  TempSensors_init();


  Serial.begin(9600);

  // Getting Sensor count
  oneWire_count = TempSensors_init();       // Getting number of sensors
  oneWire_struct TempSensor[oneWire_count]; // structure that holds the sensors

  ReadSensors(TempSensor);
  PrintValues(TempSensor);
}

void loop() {
  oneWire_count = TempSensors_init();       // Getting number of sensors
  oneWire_struct TempSensor[oneWire_count]; // structure that holds the sensors
  ReadSensors(TempSensor);

  // Display current sensor readings and addresses
  PrintValues(TempSensor);

  // Enforce sample rate
  delay(SAMPLE_RATE);
}
