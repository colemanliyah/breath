// Central Code on Nano 33 IoT
#include <ArduinoBLE.h>

BLECharacteristic commandChar;

void setup() {
  Serial.begin(9600);
  BLE.begin();

  Serial.println("Scanning...");
}

void loop() {
  BLE.scan();

  BLEDevice airDevice = BLE.available();

  if (airDevice && airDevice.localName() == "ProgrammableAir") {
    Serial.println("Found device, connecting...");
    BLE.stopScan();

    if (BLE.connect(airDevice)) {
      Serial.println("Connected!");

      BLEService airService = airDevice.service("19B10000-E8F2-537E-4F6C-D104768A1214");
      commandChar = airService.characteristic("19B10001-E8F2-537E-4F6C-D104768A1214");

      while (BLE.connected()) {
        // Send test commands for now
        commandChar.writeValue("inflate");
        delay(1000);
        commandChar.writeValue("deflate");
        delay(1000);
      }
    }
  }
}
