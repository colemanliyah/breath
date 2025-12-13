// Uses IMU sensor to capture a person's breath. Connects to arduino router to send inhale and exhale signals

#include <Arduino_LSM6DS3.h>
#include <MadgwickAHRS.h>
#include <math.h>
#include "Servo.h"
#include <WiFiNINA.h> 

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

// Wi-Fi settings
const char* WIFI_SSID = "sandbox370";       // WiFi SSID
const char* WIFI_PASS = "YOUR_WIFI_PASSWORD";   // WiFi password
const char* SERVER_IP   = "10.23.11.248";  
const uint16_t SERVER_PORT = 80;
const uint8_t CLIENT_ID = 1; // Change for each arduino

WiFiClient client;

enum BreathState { NONE, INHALE_STATE, EXHALE_STATE };

BreathState lastSentState = NONE;

void setup() {
  // Serial.begin(9600);
  // Serial.begin(115200);
  Serial.println("=== WiFi Example (SAMD, Client) ===");
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(2, OUTPUT);

  // Arduino Blinks before connecting to wifi
  for(int i = 0; i < 5; i++){
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);
    delay(500);
  }

  connectWiFi();

  Serial.println("[MODE] CLIENT");

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

  // Arduino Blinks after Calibration
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
  runClient();

  float currentDiff = calculateRollAndPitchDiff();
  if (currentDiff <= 0) return;

  if (emaDiff == 0) emaDiff = currentDiff;
  emaDiff = emaAlpha * currentDiff + (1 - emaAlpha) * emaDiff;
  float delta = emaDiff - prevDiff;

  // *** SERIAL PLOTTER VERSION ***
  // Serial.print(currentDiff);
  // Serial.print(",");
  // Serial.print(emaDiff);
  // Serial.print(",");
  // Serial.println(emaDiff - prevDiff);

  // Detect inhale: EMA is dropping and crosses a local minimum
  if (delta < -slopeThreshold && !inhaleFlag && exhaleFlag) {
    inhaleFlag = true;
    exhaleFlag = false;  // reset
    Serial.println("INHALE");
    digitalWrite(2, HIGH);
    digitalWrite(LED_BUILTIN, HIGH);
  }

  // Detect exhale: EMA rising and crosses a local peak
  if (inhaleFlag && delta > slopeThreshold && !exhaleFlag) {
    exhaleFlag = true;
    inhaleFlag = false;
    Serial.println("EXHALE");
    digitalWrite(2, LOW);
    digitalWrite(LED_BUILTIN, LOW);
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

    return abs(filter.getPitch() - filter.getRoll());
  }

  return -1;
}

/**
 * Connect to WiFi network
 * Retries connection until successful or timeout (30 seconds)
 * Restarts device if connection fails
 */
void connectWiFi() {
  Serial.print("Connecting to WiFi... SSID: ");
  Serial.println(WIFI_SSID);

  while (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

// ============================================================================
// Client Mode Functions
// ============================================================================

/**
 * Main client loop
 * Handles button press, server connection, and message reception
 * Automatically reconnects on disconnection
 */
void runClient() {
  static BreathState lastSentState = NONE;

  // Determine current breath state
  BreathState currentState = NONE;
  if ( inhaleFlag && !exhaleFlag ) currentState = INHALE_STATE;
  else if ( exhaleFlag && !inhaleFlag ) currentState = EXHALE_STATE;

  // Send only when state changes
  if (client.connected() && currentState != lastSentState) {

    String cmd = (currentState == INHALE_STATE) ? "TOGGLEINHALE" :
                 (currentState == EXHALE_STATE) ? "TOGGLEEXHALE" :
                 "";

    if (cmd.length() > 0) {
      cmd += CLIENT_ID;
      Serial.print("[STATE CHANGE] Sending ");
      Serial.println(cmd);
      client.println(cmd);
      client.flush();
      lastSentState = currentState;  // update history
    }
  }

  // Connect to server if not connected
  if (!client.connected()) {
    // Clean up previous connection before reconnecting
    client.stop();
    
    Serial.print("[CLIENT] Trying to connect to server: ");
    Serial.print(SERVER_IP);
    Serial.print(":");
    Serial.println(SERVER_PORT);

    if (client.connect(SERVER_IP, SERVER_PORT)) {
      Serial.println("[CLIENT] Connected to server!");

      // Connected to server blinks
      for(int i = 0; i < 5; i++){
        digitalWrite(LED_BUILTIN, HIGH);
        delay(500);
        digitalWrite(LED_BUILTIN, LOW);
        delay(500);
      }
      
      // Set shorter timeout for faster response (default is 1000ms)
      client.setTimeout(50);
    } else {
      Serial.print("Current Client IP address: ");
      Serial.println(WiFi.localIP());
      Serial.println("[CLIENT] Failed to connect, retrying in 5 seconds");
      delay(5000);
      return;
    }
  }

  // Receive and process messages from server
  if (client.available()) {
    String resp = client.readStringUntil('\n');
    resp.trim();
    if (resp.length() > 0) {
      Serial.print("[CLIENT] Server response: ");
      Serial.println(resp);
    }
  }
}
