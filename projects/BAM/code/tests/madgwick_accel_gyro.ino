#include <Arduino_LSM6DS3.h> // For the IMU
#include <MadgwickAHRS.h>    // For the Madgwick filter

// Initialize a Madgwick filter object
Madgwick filter;

// Define the sensor's sample rate (adjust if needed, default for LSM6DS3 is often 104 Hz)
const float sensorRate = 104.0; 
// Beta parameter for the Madgwick filter (adjust for desired filter performance)
const float beta = 0.1; 

void setup() {
  Serial.begin(9600); // Initialize serial communication

  // Initialize the IMU
  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");
    while (true); // Halt if IMU fails to initialize
  }

  // Initialize the Madgwick filter with the sample rate and beta
  filter.begin(sensorRate);
}

void loop() {
  // Declare variables to store accelerometer and gyroscope data
  float xAcc, yAcc, zAcc;
  float xGyro, yGyro, zGyro;

  // Check if new IMU data is available
  if (IMU.accelerationAvailable() && IMU.gyroscopeAvailable()) {
    // Read accelerometer and gyroscope data
    IMU.readAcceleration(xAcc, yAcc, zAcc);
    IMU.readGyroscope(xGyro, yGyro, zGyro);

    // Update the Madgwick filter with the raw sensor data
    // Note: Gyroscope values typically need to be in radians per second for the filter
    filter.updateIMU(xGyro * PI / 180.0, yGyro * PI / 180.0, zGyro * PI / 180.0, xAcc, yAcc, zAcc);

    // Get the calculated roll, pitch, and yaw from the filter
    float roll = filter.getRoll();
    float pitch = filter.getPitch();
    float yaw = filter.getYaw();

    // Print the orientation values (in degrees)
    // Serial.print("Roll: ");
    // Serial.print(roll);
    // Serial.print(" Pitch: ");
    // Serial.print(pitch);
    // Serial.print(" Yaw: ");
    // Serial.println(yaw);
    Serial.print(roll);
    Serial.print(",");
    Serial.print(pitch);
    Serial.print(",");
    Serial.println(abs(pitch - roll));
    // Serial.print(",");
    // Serial.println(yaw);

    delay(20);
  }
}
