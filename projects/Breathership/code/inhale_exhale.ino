#include <Arduino_LSM6DS3.h>
#include <MadgwickAHRS.h>
#include <math.h>
#include "Servo.h"

Madgwick filter;
const float sensorRate = 104.0;

float inhaleBaseline = 0;
float exhaleBaseline = 0;
float prevDiff = 0;
bool inhaleFlag = false;
bool exhaleFlag = true;

float emaDiff = 0;        // smoothed version of currentDiff
const float emaAlpha = 0.2;   // smoothing strength (0.1–0.3 is good)
const float slopeThreshold = 0.05; 

// Parameters for stability-based calibration
const int windowSize = 50;
const float stabilityThreshold = 0.5;
float diffWindow[windowSize];
int windowIndex = 0;
int numValidReadings = 0;

void setup() {
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(2, OUTPUT);

  //Checking LED
  for(int i = 0; i < 5; i++){
    digitalWrite(2, HIGH);
    delay(500);
    digitalWrite(2, LOW);
    delay(500);
  }

  // Arduino Blinks before Calibration
  for(int i = 0; i < 5; i++){
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);
    delay(500);
  }

  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");
    while (1);
  }

  filter.begin(sensorRate);

  // --- CALIBRATION ---
  Serial.println("Calibrating... breathe normally until values stabilize");

  bool stable = false;
  while (!stable) {
    float diff = calculateRollAndPitchDiff();
    if (diff > 0) {

      diffWindow[windowIndex] = diff;
      windowIndex = (windowIndex + 1) % windowSize;
      if (numValidReadings < windowSize) numValidReadings++;

      float minDiff = diffWindow[0];
      float maxDiff = diffWindow[0];
      for (int i = 1; i < numValidReadings; i++) {
        if (diffWindow[i] < minDiff) minDiff = diffWindow[i];
        if (diffWindow[i] > maxDiff) maxDiff = diffWindow[i];
      }

      if ((maxDiff - minDiff) < stabilityThreshold && numValidReadings == windowSize) {
        inhaleBaseline = minDiff;
        exhaleBaseline = maxDiff;
        stable = true;
      }

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

  delay(500);
}

void loop() {
  float currentDiff = calculateRollAndPitchDiff();

  if (emaDiff == 0) emaDiff = currentDiff;

  emaDiff = emaAlpha * currentDiff + (1 - emaAlpha) * emaDiff;

  // *** SERIAL PLOTTER VERSION ***
  Serial.print(currentDiff);
  Serial.print(",");
  Serial.print(emaDiff);
  Serial.print(",");
  Serial.println(emaDiff - prevDiff);

  if (currentDiff <= 0) return;

  float delta = emaDiff - prevDiff;

  // Detect inhale: EMA is dropping and crosses a local minimum
  if (delta < -slopeThreshold && !inhaleFlag && exhaleFlag) {
    inhaleFlag = true;
    exhaleFlag = false;  // reset
    Serial.println("INHALE");
    digitalWrite(2, HIGH);
  }

  // Detect exhale: EMA rising and crosses a local peak
  if (inhaleFlag && delta > slopeThreshold && !exhaleFlag) {
    exhaleFlag = true;
    inhaleFlag = false;
    Serial.println("EXHALE");
    digitalWrite(2, LOW);
  }

  prevDiff = emaDiff;
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

  return -1;
}

