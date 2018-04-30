/*
   Reading Temperature Data using esp8266 12F
   Date: Nov 21, 2017
*/
#include <OneWire.h>                // Library for One-Wire interface
#include <DallasTemperature.h>      // Library for DS18B20 temp. sensor
#include <stdio.h>

#define DEBUG true
#define SAMPLE_RATE 5000
#define ADDR_LENGTH 8

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
  int index;
};

oneWire_struct *pOneWire[ONE_WIRE_LENGTH];

/*
   Initialized oneWire sensors + gets the number of sensors
*/
uint8_t TempSensors_init () {
  uint8_t _count = 0;
  for (int i = 0; i <  ONE_WIRE_LENGTH; i++) {
    Dallas_Library[i].begin();               // initializing the sensors
  }
  for (int i = 0; i <  ONE_WIRE_LENGTH; i++) {
    _count += Dallas_Library[i].getDeviceCount();// getting number of sensors
  }
  for (int i = 0; i <  ONE_WIRE_LENGTH; i++) {
    Dallas_Library[i].requestTemperatures(); // requesting data
  }
  return _count;

}

/*
   Reading one sensor
   Modifing pointer for one sensor with address and value
*/
void TempSensors_getTemp( oneWire_struct **_sensor) {
  for (int i = 0; i < 4; i++) {
    oneWire[0].search((*_sensor + i)->address);
    (*_sensor + i)->value = Dallas_Library[0].getTempC((*_sensor + i)->address);
    (*_sensor + i)->value -= CALLIBRATION;
  }
  (*_sensor + 4)->value = Dallas_Library[1].getTempC((*_sensor + 4)->address);
  (*_sensor + 4)->value -= CALLIBRATION;
  (*_sensor + 5)->value = Dallas_Library[2].getTempC((*_sensor + 5)->address);
  (*_sensor + 5)->value -= CALLIBRATION;
  (*_sensor + 6)->value = Dallas_Library[3].getTempC((*_sensor + 6)->address);
  (*_sensor + 6)->value -= CALLIBRATION;
}

void ReadSensors(oneWire_struct TempSensor[]) {
  // Create a pointer to pass in the full array of structures
  for ( int i = 0; i < ONE_WIRE_LENGTH; i++) {
    pOneWire[i] = &TempSensor[i];
  }

  // Getting temperature
  for (int i = 0; i < ONE_WIRE_LENGTH; i ++) {
    TempSensors_getTemp(&pOneWire[i]);
  }
}
// ------------------SETUP--------------------//
void setup() {
  /* Initializing OneWire Sensors */
  TempSensors_init();

#if DEBUG // ----------
  Serial.begin(9600);
  Serial.println();
#endif  // ----------

  // Getting Sensor count
  oneWire_count = TempSensors_init();       // Getting number of sensors
  oneWire_struct TempSensor[oneWire_count]; // structure that holds the sensors

  ReadSensors(TempSensor);

#if DEBUG // -------------------
  Serial.println("Number of sensors: " + String(oneWire_count));
#endif  // ---------------------
#if DEBUG // -------------------
  /* Print Sensor Data */
  for (uint8_t i = 0; i < oneWire_count; i++) {
    Serial.println("Sensor[" + String(i) + "]: " + String(TempSensor[i].value));
  }
  Serial.println();
#endif  // ---------------------
}

void loop() {
  //Dallas_Library[0].requestTemperatures();
  oneWire_struct TempSensor[oneWire_count]; // structure that holds the sensors
  ReadSensors(TempSensor);

  // Display current sensor readings and addresses
  for (uint8_t i = 0; i < oneWire_count; i++) {
    Serial.println("Sensor[" + String(i) + "]: " + String(TempSensor[i].value));
    Serial.print("[");
    Serial.print(int(TempSensor[i].address[0]), HEX);
    for (int x = 1; x < ADDR_LENGTH; x++) {
      Serial.print(", ");
      Serial.print(int(TempSensor[i].address[x]), HEX);
    }
    Serial.println("]");
  }
  Serial.println();
  delay(SAMPLE_RATE);
}
