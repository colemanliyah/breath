// Programmable-Air
#include "programmable_air.h"

int prev_val; 

#define DEBUG 0

void setup() {
  initializePins();
  Serial.begin(9600);
  pinMode(A5, INPUT_PULLUP);
  Serial.println("Nano Reciever Ready");
}

void loop() {

  int curr_val = digitalRead(A5); 
  Serial.println(curr_val);

  // if(curr_val == 0) {
  //   switchOffPumps();
  //   vent();
  // } else if (curr_val == 1) {
  //   switchOnPumps(100);
  //   blow();
  // } 
  if (curr_val == 1) {
    Serial.println("pop");
    switchOnPumps(100);
    blow();
  } else if (readBtn(BLUE)){
    Serial.println("sizzle");
    //switchOffPumps();
    vent();
    //suck();
  } else if (readBtn(RED)) {
    Serial.println("drum");
    switchOnPumps(100);
    blow();
  } else {
    Serial.println("taco");
    switchOffPumps();
    closeAllValves();
  }
   

//   if (readBtn(BLUE)){
//     switchOffPumps();
//     vent();
//  }
//  if (readBtn(RED)){
//     switchOnPumps(100);
//     blow();
//  }

// Debugging: 
  // if (readBtn(RED)){
  // //   // inflate for x seconds
  //     switchOnPumps(100);
  //     blow();
  
  //  } else if(readBtn(BLUE)){
  //   // switchOnPumps(80);  
  //   // suck();
  //   switchOffPumps();
  //   vent();

  // } else {
  //   closeAllValves();
  // }
}
