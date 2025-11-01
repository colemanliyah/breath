#include <Dictionary.h>
#include <Arduino_LSM6DS3.h>
#include <MadgwickAHRS.h>
#include <math.h>
#include "Servo.h"

Madgwick filter;
const float sensorRate = 104.0;

Servo servoMotor;
int servoPin = 9;

int breathCount=0;
int darkCount=4;
int brightCount=8;
int completed=12;
int servoAngle=0;

float inhaleBaseline = 0;
float exhaleBaseline = 0;
float prevDiff = 0;
bool inhaleFlag = false;
bool exhaleFlag = false;

// Parameters for stability-based calibration
const int windowSize = 50;           // Number of readings to consider
const float stabilityThreshold = 0.5; // Max variation to consider stable
float diffWindow[windowSize];        // Circular buffer for recent diffs
int windowIndex = 0;
int numValidReadings = 0;

void setup() {
  Serial.begin(9600);
  //while(!Serial);
  pinMode(LED_BUILTIN, OUTPUT);
  servoMotor.attach(servoPin);

  // --- Before CALIBRATION ---
  Serial.println("Before Calibration, LED Blink");
  Serial.println("HWEORIWHE");
  for(int i = 0; i < 5; i++){
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);
    delay(500);
  }

  // Initialize IMU
  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");
    while (1);
  }

  filter.begin(sensorRate);

  // Calibrating Motor
  servoMotor.write(0);
  delay(10000);

  Serial.println("before next");
  for(int i = 0; i < 5; i++){
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
  }


  // --- CALIBRATION ---
  Serial.println("Calibrating... breathe normally until values stabilize");

  bool stable = false;
  while (!stable) {
    float diff = calculateRollAndPitchDiff();
    if (diff > 0) {
      // Add diff to circular buffer
      diffWindow[windowIndex] = diff;
      windowIndex = (windowIndex + 1) % windowSize;
      if (numValidReadings < windowSize) numValidReadings++;

      // Calculate max and min in the window
      float minDiff = diffWindow[0];
      float maxDiff = diffWindow[0];
      for (int i = 1; i < numValidReadings; i++) {
        if (diffWindow[i] < minDiff) minDiff = diffWindow[i];
        if (diffWindow[i] > maxDiff) maxDiff = diffWindow[i];
      }

      // Check if stable
      if ((maxDiff - minDiff) < stabilityThreshold && numValidReadings == windowSize) {
        inhaleBaseline = minDiff;
        exhaleBaseline = maxDiff;
        stable = true;
      }

      // Debug print
      Serial.print(diff);
      Serial.print(",");
      Serial.print(minDiff);
      Serial.print(",");
      Serial.println(maxDiff);
    }
    delay(20);
  }

  for(int i = 0; i < 5; i++){
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);
    delay(500);
  }

  Serial.println("Calibration complete!");
  Serial.print("Inhale baseline: "); Serial.println(inhaleBaseline);
  Serial.print("Exhale baseline: "); Serial.println(exhaleBaseline);
}

void loop() {
  float currentDiff = calculateRollAndPitchDiff();
  if (currentDiff <= 0) return; // Skip invalid readings

  // Compare to previous difference
  float delta = currentDiff - prevDiff;

  // Detect inhale (difference decreasing)
  if (delta < 0) {
    if (currentDiff <= inhaleBaseline) {  
      inhaleFlag = true;  // green 1 ON
      if (!exhaleFlag) exhaleFlag = false; // reset exhale if new inhale starts
    }
  }

  // Detect exhale (difference increasing)
  if (inhaleFlag && delta > 0) {
    if (currentDiff >= exhaleBaseline) {
      exhaleFlag = true;  // green 2 ON
    }
  }

  // TODO - If breath gets caught inbetween finishing the if statement and the next breath then rest somehow
  // Both flags active → breathing cycle complete
  if (inhaleFlag && exhaleFlag) {
    Serial.println("You did it!!!");
    digitalWrite(LED_BUILTIN, HIGH);
    inhaleFlag = false;
    exhaleFlag = false;

    //Serial.println(rotationCount);
    if (breathCount < darkCount) {
      int prevServo = servoAngle;
      servoAngle = servoAngle + floor(90/darkCount);
      Serial.println(servoAngle);

      for (int i=prevServo; i<=servoAngle; i++){
        servoMotor.write(i);
        delay(200);
      }
      digitalWrite(LED_BUILTIN, LOW);
    }

    if ((brightCount <= breathCount) && (breathCount < completed)) {
      Serial.println("in completed circuit");
      digitalWrite(LED_BUILTIN, HIGH);
      int currentServo = servoAngle;
      servoAngle = servoAngle - floor(90/darkCount);
      //Serial.println(servoAngle);

      for (int i=currentServo; i>=servoAngle; i--) {
        servoMotor.write(i);
        delay(200);
      }
      digitalWrite(LED_BUILTIN, LOW);
    }
    
    if (breathCount >= completed){
      servoMotor.write(0);
    }

    breathCount = breathCount + 1;
    Serial.println("breath count");
    Serial.println(breathCount);
    for(int i=0; i<breathCount; i++){
      digitalWrite(LED_BUILTIN, HIGH);
      delay(10);
      digitalWrite(LED_BUILTIN, LOW);
      delay(10);
      digitalWrite(LED_BUILTIN, HIGH);
      delay(10);
      digitalWrite(LED_BUILTIN, LOW);
      delay(10);
    }
  }

  prevDiff = currentDiff;
  delay(20);
}

float calculateRollAndPitchDiff() {
  float xAcc, yAcc, zAcc;
  float xGyro, yGyro, zGyro;

  if (IMU.accelerationAvailable() && IMU.gyroscopeAvailable()) {
    IMU.readAcceleration(xAcc, yAcc, zAcc);
    IMU.readGyroscope(xGyro, yGyro, zGyro);

    filter.updateIMU(
      xGyro * PI / 180.0,
      yGyro * PI / 180.0,
      zGyro * PI / 180.0,
      xAcc, yAcc, zAcc
    );

    float roll = filter.getRoll();
    float pitch = filter.getPitch();

    return abs(pitch - roll);
  }

  return -1; // no new data available
}
