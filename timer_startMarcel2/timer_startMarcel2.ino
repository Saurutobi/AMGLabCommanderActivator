#include <ArduinoBLE.h>


#define TIMER_MAC "60:09:c3:82:1d:8d"
#define SERVICE_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define RX_SERVICE_UUID "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define TX_SERVICE_UUID "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"
bool deviceFound = false;

BLEDevice commander;

//BLEService uartService = BLEService("6E400001-B5A3-F393-E0A9-E50E24DCCA9E");
//BLECharacteristic receiveCharacteristic = BLECharacteristic("6E400002-B5A3-F393-E0A9-E50E24DCCA9E", BLEWriteWithoutResponse, BLE_ATTRIBUTE_MAX_VALUE_LENGTH);
//BLECharacteristic transmitCharacteristic = BLECharacteristic("6E400003-B5A3-F393-E0A9-E50E24DCCA9E", BLENotify, BLE_ATTRIBUTE_MAX_VALUE_LENGTH);


void setup() {
  Serial.begin(9600);

  // begin initialization
  if (!BLE.begin()) {
    Serial.println("starting Bluetooth® Low Energy module failed!");
    while (1)  //infinite loop cuz broke
      ;
  }

  Serial.println("Bluetooth® Low Energy Central scan callback");

  // set the discovered event handle
  BLE.setEventHandler(BLEDiscovered, bleCentralDiscoverHandler);

  // start scanning for peripherals with duplicates
  BLE.scan(true);
}

void loop() {
  if (!deviceFound) {
    Serial.println("Polling");
    BLE.poll();
    delay(1000);
  }  else {
    Serial.println("Trying to talk to the AMGLab");
    commander.connect();
    tryWriteCommand(commander);
    commander.disconnect();
    deviceFound = false;
    delay(5000);
  }
}

void bleCentralDiscoverHandler(BLEDevice device) {
  //Serial.println("Discovered a peripheral");
  //Serial.println("-----------------------");
  //Serial.println("Address: " + device.address());

  if (device.address() == TIMER_MAC) {
    

    Serial.println("Found AMGLab Commander");
    commander = device;
    deviceFound = true;
  }
}


void tryWriteCommand(BLEDevice device) {
  Serial.println("Trying To Write Start");
  Serial.println("-----------------------");

  // retrieve the characteristic
  
  Serial.println("deviceaddress: " + device.address());

  Serial.println("advertisedCount: " + device.advertisedServiceUuidCount());
  Serial.println("name: " + device.localName());


  Serial.println("charcount: " + device.characteristicCount());
  Serial.println("service: " + device.serviceCount());

  //BLECharacteristic amgCharacteristic = device.characteristic(SERVICE_UUID);
  //BLEService aService = device.service(SERVICE_UUID);
  //aService.characteristic()

  
  //String commandAsString = "COM START";
  //amgCharacteristic.writeValue(commandAsString.c_str(), false);
}
