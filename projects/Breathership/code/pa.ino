// Programmable-Air
// Author: tinkrmind
// https://github.com/orgs/Programmable-Air
// 
// Basic example of using Blow function of the Programmable-Air board. 
// The neopixels show pressure. Cyan for low pressure and Purple for high pressure
// Brighter color means more extreme pressure. Bright cyan means lower pressure than dim cyan and brighter purple means higher pressure tham dim purple. At atmospheric presure,lights will be off.
//
// PCB v0.3/v0.4/v0.5
// Connect TX pins from iot 33 to u 9/10 ion PA


#include "programmable_air.h"
#include <SoftwareSerial.h>

SoftwareSerial mySerial(9, 8);


#define DEBUG 0

void setup() {
  initializePins();
  Serial.begin(9600);
  mySerial.begin(9600);
  Serial.println("Nano Reciever Ready");
}

void loop() {
  // delayWhileReadingPressure(200);

  // shut off clause
  // if(readBtn(RED) && readBtn(BLUE)){ 

  // if the RED button is pressed blow air into the output, else, vent to atmosphere

  if (mySerial.available()) {
    String msg = mySerial.readStringUntil('\n');
    Serial.println("Received: " + msg);
    switchOnPumps(80);  
    suck();
    float exhale_time = 3.5;
    delayWhileReadingPressure(exhale_time * 1000);
  }  else if (readBtn(RED)){
    // inflate for x seconds
    switchOnPumps(100);
    blow();
    int fill_time = 9;
    delayWhileReadingPressure(fill_time * 1000);
    // once inflated, wait for blue button 
  
  } else if(readBtn(BLUE)){

    for (int i = 0; i < 1; i++){

    switchOnPumps(80);  
    suck();
    float exhale_time = 3.5;
    delayWhileReadingPressure(exhale_time * 1000);

    switchOnPumps(100);
    blow();
    float inhale_time = 2;
    delayWhileReadingPressure(inhale_time * 1000);
    }

    // after blue button begin breathing cycle, red button again stops everything

  } else {
    closeAllValves();
  }
}
