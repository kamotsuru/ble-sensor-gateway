/*
  MyAdvertiser

  This example creates a BLE peripheral advertising A0 pin value.

  The circuit:
  - Arduino MKR WiFi 1010, Arduino Uno WiFi Rev2 board, Arduino Nano 33 IoT,
    Arduino Nano 33 BLE, or Arduino Nano 33 BLE Sense board.

  You can use a generic BLE central app, like LightBlue (iOS and Android) or
  nRF Connect (Android), to interact with the services and characteristics
  created in this sketch.

  This example code is in the public domain.
*/

#include <ArduinoBLE.h>
#include <Wire.h>
#include <Adafruit_INA219.h>

#define DATA_SIZE 7
#define LED 7        // the PWM pin the LED is attached to

int oldA0Level = 0;  // last a0 level reading from analog input
//    led = 7;         // the PWM pin the LED is attached to
byte data[DATA_SIZE] = {0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00};
Adafruit_INA219 ina219;

void setup() {
  // declare pin 7 to be an output:
  pinMode(LED, OUTPUT);
  
  Serial.begin(9600);    // initialize serial communication
  delay(500);

  // begin initialization
  if (!BLE.begin()) {
    if(Serial)
      Serial.println("starting BLE failed!");

    while (1);
  }
  if (!ina219.begin()) {
    if(Serial)
      Serial.println("Failed to find INA219 chip!");

    while (1);
  }

  /* Set a local name for the BLE device
     This name will appear in advertising packets
     and can be used by remote devices to identify this BLE device
     The name can be changed but maybe be truncated based on space left in advertisement packet
  */
  BLE.setLocalName("ArduinoNano33BLE");

  BLE.setManufacturerData(data, DATA_SIZE);
  BLE.setAdvertisingInterval(3200); // 2000ms = 3200*0.625ms

  /* Start advertising BLE.  It will start continuously transmitting BLE
     advertising packets and will be visible to remote BLE central devices
     until it receives a new connection */

  // start advertising
  BLE.advertise();

  if(Serial)
    Serial.println("Bluetooth device active, waiting for connections...");
}

void loop() {
  /* Read the current voltage level on the A0 analog input pin.
     This is used here to simulate the charge level of a battery.
  */
  int a0val = analogRead(A0);
  int a0Level = map(a0val, 0, 1023, 0, 3300);
  int a0val = analogRead(A0);
  int a0Level = map(a0val, 0, 1023, 0, 3300);
  float shuntvoltage = ina219.getShuntVoltage_mV();
  float busvoltage = ina219.getBusVoltage_V();
  float current_mA = ina219.getCurrent_mA();
  float loadvoltage = busvoltage + (shuntvoltage / 1000);
  float power_mW = ina219.getPower_mW();
  float current_A = ((float)a0Level/1000.0 - 2.61567)*70.71; // 5.22*461/(459+461), 1/20.0*2000.0/1.4142

  if(Serial) {
    Serial.print("Bus Voltage:   "); Serial.print(busvoltage); Serial.println(" V");
    Serial.print("Shunt Voltage: "); Serial.print(shuntvoltage); Serial.println(" mV");
    Serial.print("Load Voltage:  "); Serial.print(loadvoltage); Serial.println(" V");
    Serial.print("Current:       "); Serial.print(current_mA); Serial.println(" mA");
    Serial.print("Power:         "); Serial.print(power_mW); Serial.println(" mW");
    Serial.println("");
  }
  
  analogWrite(LED, (int)map(a0Level, 0, 3300, 0, 255));
//  analogWrite(LED, (int)map(busvoltage, 0, 32, 0, 255));
  if(Serial) {
    Serial.printf("a0 Level is now: %f\n", a0Level/1000.0);    
  }

  if (a0Level != oldA0Level) {      // if the battery level has changed
    oldA0Level = a0Level;           // save the level for next comparison
    
    BLE.stopAdvertise();
    data[2] = 0x01; // sensor id
    int val = floorf(current_mA); //current before decimal point
    data[7] = (byte)val;
    data[8] = (byte)floorf((current_mA - (float)val)*100.0); //current after decimal point
    val = floorf(busvoltage); //voltage before decimal point
    data[3] = (byte)val;
    data[4] = (byte)floorf((busvoltage - (float)val)*100.0); //voltage after decimal point
    val = floorf(current_A);
    data[5] = (byte)val;
    data[6] = (byte)floorf((current_A - (float)val)*100.0);
    if(Serial)      
      Serial.printf("data: %d.%02d, %d.%02d, %d.%02d\n", (int)data[3], (int)data[4], (int)data[5], (int)data[6], (int)data[7], (int)data[8]);
    BLE.setManufacturerData(data, DATA_SIZE);
    BLE.advertise();
  }
  
  delay(random(2000,3000));
}
