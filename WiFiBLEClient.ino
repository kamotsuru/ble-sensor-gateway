/*
   Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleScan.cpp
   Ported to Arduino ESP32 by Evandro Copercini
   Modified by kamotsuru
*/

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <Syslog.h>
#include <UIPEthernet.h>

#define MANUFACTURER_ID 0xffff // test manufacturer ID
#define SYSLOG_PORT 514
#define SCAN_TIME 5

//#define ENC28J60

BLEScan* pBLEScan;
char* server = "192.168.0.8";
//char* server = "10.0.1.1";

#ifndef ENC28J60
char* ssid     = "Wifi Network";
char* password = "guest";
IPAddress wifiIp;
WiFiUDP udpClient;
#else
EthernetUDP udpClient;
uint8_t mac[] = { 0x00, 0x01, 0x02, 0x04, 0x04, 0x05 };
uint8_t IP[] = { 10, 0, 1, 2 };
uint8_t DNS[] = { 10, 0, 1, 1 };
uint8_t MASK[] = { 255, 255,255, 0 };
uint8_t GW[] = { 10, 0, 1, 1 };
#endif

Syslog syslog(udpClient, server, SYSLOG_PORT, "esp32", LOG_KERN);

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
      Serial.printf("Advertised Device: %s \n", advertisedDevice.toString().c_str());
    }
};

void setup() {
  Serial.begin(115200);
  if(Serial)
    Serial.println("Scanning...");

  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan(); //create new scan
  pBLEScan->setActiveScan(false); //active scan uses more power, but get results faster

#ifndef ENC28J60
  // We start by connecting to a WiFi network
  delay(10);
  if(Serial) {
    Serial.print("Connecting to ");
    Serial.println(ssid);
  }

  WiFi.begin(ssid, password);
  while(WiFi.status() != WL_CONNECTED) {
      delay(500);
      if(Serial)
        Serial.print(".");
  }
  
  wifiIp = WiFi.localIP();


  if(Serial) {
    Serial.println("");
    Serial.print("Wifi connected. IP address: ");
    Serial.println(WiFi.localIP());
  }
#else
//  Ethernet.begin(mac, IP, DNS, GW, MASK);
  Ethernet.begin(mac);
  if(Serial) {
    Serial.println("");
    Serial.print("ENC28J60 IP address: ");
    Serial.println(Ethernet.localIP());
    Serial.print("Subnet Mask: ");
    Serial.println(Ethernet.subnetMask());
    Serial.print("Gateway: ");
    Serial.println(Ethernet.gatewayIP());
  }
#endif

}

void loop() {
  bool found = false;
  int manufacturerId, id;
  float current, busvoltage, current_mA;

  if (Serial.available() > 0) {
    String str = Serial.readStringUntil('\n');
    if (str[0] == 'R') {
#ifndef ENC28J60
      Serial.printf("%s ", ssid);
      Serial.println(wifiIp);
#else
      Serial.println(Ethernet.localIP());
#endif
    } else if (str[0] == 'A') {
      std::string para = myOffset(str, 2);
      const char* param = "testapp";
      Serial.println(strcmp(para.c_str(), param));
//      syslog.appName((const char*)(para.c_str()));
      syslog.appName(param);      
    }
  }
  
  // put your main code here, to run repeatedly:
  BLEScanResults foundDevices = pBLEScan->start(SCAN_TIME);
  int count = foundDevices.getCount();
  if(Serial) {
    Serial.print("Devices found: ");
    Serial.println(count);
  }
  for (int i = 0; i < count; i++) {
    BLEAdvertisedDevice d = foundDevices.getDevice(i);
    if (d.haveManufacturerData()) {
      std::string data = d.getManufacturerData();
      manufacturerId = data[1] << 8 | data[0];
      if (manufacturerId == MANUFACTURER_ID) {
        found = true;
        id = (int)data[2];
        current = (float)data[3] + (float)data[4]/100.0;
	if (current > 128)
	  current = current - 256.0;
        busvoltage = (float)data[5] + (float)data[6]/100.0;
	if (busvoltage > 128)
	  busvoltage = busvoltage - 256.0;
        current_mA = (float)data[7] + (float)data[8]/100.0;
	if (current_mA > 128)
	  current_mA = current_mA - 256.0;
        if(Serial)
	  Serial.printf("%s %d %f %f %f\n", d.getAddress().toString().c_str(), id, current, busvoltage, current_mA);	
        syslog.logf(LOG_INFO, "%s %d %f %f %f", d.getAddress().toString().c_str(), id, current, busvoltage, current_mA);
      }
    }
  }
  pBLEScan->clearResults();   // delete results fromBLEScan buffer to release memory
  
delay(random(2000,3000));
}

std::string myOffset (String string, int offset) {
  std::string para = "";
  for (int i = offset; i < string.length(); i++) {
     para += string[i];
  }
  return para;
}
