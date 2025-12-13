// Arduino nano iot 33 acts as a router. Two arduinos connect to it via wifi. If two people complete a breath in sync, signal is sent to pa to inflate

// ============================================================================
// Board-specific WiFi Library
// ============================================================================
#include <WiFiNINA.h>   // SAMD family (e.g., Nano 33 IoT)


// Wi-Fi settings
const char* WIFI_SSID = "sandbox370";       // WiFi SSID
const char* WIFI_PASS = "YOUR_WIFI_PASSWORD";   // WiFi password

const uint16_t SERVER_PORT = 80;

// ============================================================================
// Global Variables
// ============================================================================
WiFiServer server(SERVER_PORT);
static bool inhaleOn = false;  // LED 1 (D12) state
static bool exhaleOn = false;  // LED 2 (D7) state

bool personOneInhale = false;
bool personOneExhale = false;
bool personTwoInhale = false;
bool personTwoExhale = false;

// No client management needed - clients send TOGGLE1 or TOGGLE2 directly

// ============================================================================
// Function Prototypes
// ============================================================================
void connectWiFi();
void runServer();
void setLed(uint8_t breathNum, bool on);
void toggleLed(uint8_t breathNum);
void processClient(WiFiClient client);

#define PA 2 // (Programmable-Air control pin)
#define LED 3 // LED
bool isInhaling = false;   // State machine variable

/**
 * Initialize the device and WiFi communication
 */
void setup() {
  //Serial.begin(115200);
  //Serial1.begin(9600);
  Serial.println();
  Serial.println("=== WiFi Example (SAMD, Server) ===");

  pinMode(LED, OUTPUT);
  pinMode(PA, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  digitalWrite(PA, LOW);

  // Connect to WiFi network
  connectWiFi();

  // Start server and display connection info
  Serial.println("[MODE] SERVER");
  server.begin();
  Serial.print("Server port: ");
  Serial.println(SERVER_PORT);
  Serial.print("This board IP: ");
  Serial.println(WiFi.localIP());
  
  // Display LED control mode
  Serial.println("\n[LED Control Mode]");
  Serial.println("Clients send TOGGLE1 or TOGGLE2 to control LED 1 or LED 2");
  Serial.println("Waiting for client connections...");
  Serial.println();

  // Waiting for Clients Message
  for(int i = 0; i < 10; i++){
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);
    delay(500);
  }
}

// ============================================================================
// Main Loop
// ============================================================================

/**
 * Main program loop
 * Handles client connections and processes received messages
 */
void loop() {
  runServer();
}

// ============================================================================
// WiFi Connection Functions
// ============================================================================

/**
 * Connect to WiFi network
 * Restarts device if connection fails
 */
void connectWiFi() {
  Serial.print("Connecting to WiFi... SSID: ");
  Serial.println(WIFI_SSID);

  uint8_t retry = 0;
  while (WiFi.status() != WL_CONNECTED) {

    WiFi.begin(WIFI_SSID, WIFI_PASS);
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("Wifi Network");
  Serial.println(WIFI_SSID);
}

// ============================================================================
// Server Mode Functions
// ============================================================================

/**
 * Handle server connections and process client messages
 * Processes clients as they become available
 */
void runServer() {
  // Process any available client
  WiFiClient client = server.available();
  if (client) {
    processClient(client);
  }
}

/**
 * Process messages from a client
 * Handles TOGGLE1 and TOGGLE2 commands
 */
void processClient(WiFiClient client) {
  if (!client || !client.connected()) return;
  if (!client.available()) return;
  
  client.setTimeout(50);
  
  String msg = client.readStringUntil('\n');
  msg.trim();
  if (msg.length() == 0) return;
  
  msg.toUpperCase();
  
  // Handles msgs sent from clients
  if (msg == "TOGGLEINHALE1") {
    personOneInhale = true;
    personOneExhale = false;
    Serial.print("[SERVER] Client ");
    Serial.print(client.remoteIP());
    Serial.println(" toggled Person One INHALE");
    client.println("Person One INHALE toggled");
  } else if (msg == "TOGGLEEXHALE1") {
    personOneInhale = false;
    personOneExhale = true;
    Serial.print("[SERVER] Client ");
    Serial.print(client.remoteIP());
    Serial.println(" toggled Person One EXHALE");
    client.println("Person One EXHALE toggled");
  } else if (msg == "TOGGLEINHALE2") {
    personTwoInhale = true;
    personTwoExhale = false;
    Serial.print("[SERVER] Client ");
    Serial.print(client.remoteIP());
    Serial.println("Person Two toggled INHALE");
    client.println("Person Two INHALE toggled");
  } else if (msg == "TOGGLEEXHALE2") {
    personTwoInhale = false;
    personTwoExhale = true;
    Serial.print("[SERVER] Client ");
    Serial.print(client.remoteIP());
    Serial.println("Person Two toggled EXHALE");
    client.println("Person Two EXHALE toggled");
  }

  if (personOneInhale && personTwoInhale) {
    Serial.println("INHALE");
    digitalWrite(PA, HIGH);
    digitalWrite(LED, HIGH);
    digitalWrite(LED_BUILTIN, HIGH);
  } 
  else if (personOneExhale && personTwoExhale) {
      Serial.println("EXHALE");
      digitalWrite(PA, LOW);
      digitalWrite(LED, LOW);
      digitalWrite(LED_BUILTIN, LOW);
  }

}

// // ============================================================================
// // LED Control Functions
// // ============================================================================

// /**
//  * Set breathing output state (INHALE = HIGH, EXHALE = LOW)
//  */
// void setBreathState(bool inhale) {
//   isInhaling = inhale;

//   if (inhale) {
//     digitalWrite(LED, HIGH);
//     Serial.println("[BREATH] INHALE (pin HIGH)");
//     delay(10);
//   } else {
//     digitalWrite(LED, LOW);
//     Serial.println("[BREATH] EXHALE (pin LOW)");
//     delay(10);
//   }
// }

// /**
//  * Toggle breathing state
//  * If currently INHALE, switch to EXHALE. If EXHALE, switch to INHALE.
//  */
// void toggleBreathState() {
//   setBreathState(!isInhaling);
// }

