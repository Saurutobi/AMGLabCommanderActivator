#include <ArduinoBLE.h>


#define TIMER_MAC "60:09:c3:82:1d:8d"

#define SERVICE_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"

void setup() {
  Serial.begin(9600);
  while (!Serial)
    ;

  // begin initialization
  if (!BLE.begin()) {
    Serial.println("starting Bluetooth® Low Energy module failed!");

    while (1)
      ;
  }

  Serial.println("Bluetooth® Low Energy Central scan callback");

  // set the discovered event handle
  BLE.setEventHandler(BLEDiscovered, bleCentralDiscoverHandler);

  // start scanning for peripherals with duplicates
  BLE.scan(true);
}

void loop() {
  // poll the central for events
  BLE.poll();
}

void bleCentralDiscoverHandler(BLEDevice peripheral) {
  // discovered a peripheral
  Serial.println("Discovered a peripheral");
  Serial.println("-----------------------");

  // print address
  Serial.print("Address: ");
  Serial.println(peripheral.address());

  // print the local name, if present
  if (peripheral.hasLocalName()) {
    Serial.print("Local Name: ");
    Serial.println(peripheral.localName());
  }

  // print the advertised service UUIDs, if present
  if (peripheral.hasAdvertisedServiceUuid()) {
    Serial.print("Service UUIDs: ");
    for (int i = 0; i < peripheral.advertisedServiceUuidCount(); i++) {
      Serial.print(peripheral.advertisedServiceUuid(i));
      Serial.print(" ");
    }
    Serial.println();
  }

  // print the RSSI
  Serial.print("RSSI: ");
  Serial.println(peripheral.rssi());

  Serial.println();
  amglabtesting(peripheral);
}

void amglabtesting(BLEDevice peripheral) {
  //Discovered a peripheral
  //-----------------------
  //Address: 60:09:c3:82:1d:8d
  //Local Name: AMG Lab COMM 1D8D
  //Service UUIDs: 6e400001-b5a3-f393-e0a9-e50e24dcca9e
  //RSSI: -40

  //FOUND THE AMGLAB
  if (peripheral.address() == TIMER_MAC) {
    // connect to the peripheral
    Serial.println("Connecting ...");


    if (peripheral.connect()) {
      Serial.println("Connected");
    } else {
      Serial.println("Failed to connect!");
    }


    // retrieve the characteristic
    BLECharacteristic amgCharacteristic = peripheral.characteristic(SERVICE_UUID);

    if (!amgCharacteristic) {
      Serial.println("Peripheral does not have characteristic!");
      peripheral.disconnect();
    } else if (!amgCharacteristic.canWrite()) {
      Serial.println("Peripheral does not have a writable characteristic!");
      peripheral.disconnect();
    }

    String commandAsString = "COM START";
    amgCharacteristic.writeValue(commandAsString.c_str());

    delay(5000);
  }
}
