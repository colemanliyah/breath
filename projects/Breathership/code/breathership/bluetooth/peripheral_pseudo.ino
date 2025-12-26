// Sample Peripheral Code - Programmable Air
#include <ArduinoBLE.h>

BLEService airService("19B10000-E8F2-537E-4F6C-D104768A1214");
BLEStringCharacteristic commandChar("19B10001-E8F2-537E-4F6C-D104768A1214", 
                                    BLEWrite, 20);

void setup() {
  Serial.begin(9600);
  BLE.begin();

  BLE.setLocalName("ProgrammableAir");
  BLE.setAdvertisedService(airService);

  airService.addCharacteristic(commandChar);
  BLE.addService(airService);

  commandChar.writeValue("idle");

  BLE.advertise();
  Serial.println("Air module advertising...");
}

void loop() {
  BLEDevice central = BLE.central();

  if (central) {
    Serial.println("Connected!");

    while (central.connected()) {
      if (commandChar.written()) {
        String cmd = commandChar.value();
        Serial.print("Command: ");
        Serial.println(cmd);

        if (cmd == "inflate") {
          // control pumps here
        }
        if (cmd == "deflate") {
          // control valves here
        }
      }
    }

    Serial.println("Disconnected");
  }
}
