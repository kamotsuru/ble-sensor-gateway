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

#define DATA_SIZE 7
#define LED 7        // the PWM pin the LED is attached to

int oldA0Level = 0;  // last a0 level reading from analog input
//    led = 7;         // the PWM pin the LED is attached to
byte data[DATA_SIZE] = {0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00};

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
  int a0Level = map(a0val, 0, 1023, 0, 9999);

  analogWrite(LED, (int)map(a0Level, 0, 9999, 0, 255));
//  if(Serial)  
//    Serial.printf("a0 Level is now: %f\n", a0Level/100.0);  

  if (a0Level != oldA0Level) {      // if the battery level has changed
    oldA0Level = a0Level;           // save the level for next comparison
    
    BLE.stopAdvertise();
    data[2] = 0x01; // sensor id
    data[3] = (byte)floorf(a0Level/100.0); //current before decimal point
    data[4] = (byte)(a0Level%100); //current after decimal point
    data[5] = (byte)floorf(a0Level/100.0); //voltage before decimal point
    data[6] = (byte)(a0Level%100); //voltage after decimal point
    if(Serial)      
      Serial.printf("a0 Level is now: %d.%02d\n", (int)data[3], (int)data[4]);
    BLE.setManufacturerData(data, DATA_SIZE);
    BLE.advertise();
  }
  
  delay(random(2000,3000));
}
