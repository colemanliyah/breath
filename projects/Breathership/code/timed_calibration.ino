#include <Arduino_LSM6DS3.h>
#include <MadgwickAHRS.h>
#include <math.h>

Madgwick filter;
const float sensorRate = 104.0; 
float inhaleBaseline = 0;
float exhaleBaseline = 0;
float prevDiff = 0;
bool inhaleFlag = false;
bool exhaleFlag = false;

const unsigned long startupDelay = 15000; // ignore first 5 seconds
unsigned long calibrationStart = 0;      // reference time for calibration start

void setup() {
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);

  // --- Before CALIBRATION ---
  Serial.println("Before Calibration, LED Blink");
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

  // --- CALIBRATION ---
  calibrationStart = millis(); // set reference for startup delay
  float minDiff = 9999;
  float maxDiff = -9999;

  Serial.println("Calibrating... please breathe normally for 20 seconds");

  while (millis() - calibrationStart < 20000) { // 10-second calibration
    float diff = calculateRollAndPitchDiff();

    if (diff > 0) {
      // Update min/max for calibration
      if (diff < minDiff) minDiff = diff;
      if (diff > maxDiff) maxDiff = diff;

      // Print for debugging
      // Serial.print("Time: "); Serial.print(millis() - calibrationStart);
      // Serial.print(" ms | Diff: "); Serial.print(diff);
      // Serial.print(" | Min: "); Serial.print(minDiff);
      // Serial.print(" | Max: "); Serial.println(maxDiff);
      Serial.print(diff);
      Serial.print(",");
      Serial.print(minDiff);
      Serial.print(",");
      Serial.println(maxDiff);
    }

    delay(20); // small delay between readings
  }

  inhaleBaseline = minDiff;
  exhaleBaseline = maxDiff;

  Serial.print("Inhale baseline: "); Serial.println(inhaleBaseline);
  Serial.print("Exhale baseline: "); Serial.println(exhaleBaseline);
}

void loop() {
  float currentDiff = calculateRollAndPitchDiff();

  // Compare to previous difference
  float delta = currentDiff - prevDiff;
  //Serial.println(delta);

  // Detect inhale (difference decreasing)
  if (delta < 0) {
    //Serial.println("HERE");
    // if distance is smaller than baseline or staying near baseline
    if (currentDiff <= inhaleBaseline) {  
      //Serial.println("HEREAGAIN");
      inhaleFlag = true;  // green 1 ON
      // (optional) exhale resets if new inhale starts before completing previous
      if (!exhaleFlag) {
        exhaleFlag = false;
      }
    } else {
      // Serial.println("current diff inhale is: ");
      // Serial.println(currentDiff);
      // Serial.println("baseline inhale is: ");
      // Serial.println(inhaleBaseline);
    }
  }

  // Detect exhale (difference increasing)
  if (inhaleFlag && delta > 0) {
    //Serial.println("THERE");
    // if diff is greater than baseline or increasing steadily
    if (currentDiff >= exhaleBaseline) {
      //Serial.println("THEREAGAIN");
      exhaleFlag = true;  // green 2 ON
    } else {
      // Serial.println("current diff exhale is: ");
      // Serial.println(currentDiff);
      // Serial.println("baseline exhale is: ");
      // Serial.println(exhaleBaseline);
    }
  }

  // Both flags active → breathing cycle complete
  if (inhaleFlag && exhaleFlag) {
    Serial.println("You did it!!!");
    
    // reset for next cycle
    inhaleFlag = false;
    exhaleFlag = false;
  }

  // Update for next loop
  prevDiff = currentDiff;
  delay(20);
}

float calculateRollAndPitchDiff() {
  float xAcc, yAcc, zAcc;
  float xGyro, yGyro, zGyro;

  // Check if new IMU data is available
  if (IMU.accelerationAvailable() && IMU.gyroscopeAvailable()) {
    IMU.readAcceleration(xAcc, yAcc, zAcc);
    IMU.readGyroscope(xGyro, yGyro, zGyro);

    // Update the Madgwick filter
    filter.updateIMU(
      xGyro * PI / 180.0,
      yGyro * PI / 180.0,
      zGyro * PI / 180.0,
      xAcc, yAcc, zAcc
    );

    // Skip readings during initial startup delay
    if (millis() - calibrationStart < startupDelay) {
      return -1; // invalid reading
    }

    float roll = filter.getRoll();
    float pitch = filter.getPitch();

    return abs(pitch - roll);
  }

  return -1; // no new data available
}
