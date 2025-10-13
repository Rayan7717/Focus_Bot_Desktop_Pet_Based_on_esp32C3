/*
 * ESP32-C3 Supermini Complete System Test
 * Tests: SSH1106 Display, MPU6050 Gyro, 4x TTP223 Touch, Vibration Motor
 * 
 * Hardware:
 * - ESP32-C3 Supermini
 * - SSH1106 128x64 OLED Display
 * - MPU6050 Gyroscope/Accelerometer
 * - 4x TTP223 Touch Sensors
 * - Vibration Motor (BC547B transistor driver with 1kΩ base resistor)
 * 
 * Required Libraries:
 * - Adafruit GFX Library
 * - Adafruit SH110X
 * - MPU6050 by Electronic Cats
 * 
 * Install via Arduino Library Manager
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <MPU6050.h>

// Pin Definitions for ESP32-C3 Supermini
#define SDA_PIN 8        // I2C Data (default for ESP32-C3)
#define SCL_PIN 9        // I2C Clock (default for ESP32-C3)
#define TOUCH1_PIN 0     // TTP223 Touch Sensor 1
#define TOUCH2_PIN 1     // TTP223 Touch Sensor 2
#define TOUCH3_PIN 3     // TTP223 Touch Sensor 3
#define TOUCH4_PIN 4     // TTP223 Touch Sensor 4
#define MOTOR_PIN 6      // Vibration Motor (via BC547B + 1kΩ resistor)

// Display Settings (SSH1106 compatible)
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

// Objects
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
MPU6050 mpu;

// Variables
int16_t ax, ay, az, gx, gy, gz;
bool touch1State = false, touch2State = false, touch3State = false, touch4State = false;
bool lastTouch1 = false, lastTouch2 = false, lastTouch3 = false, lastTouch4 = false;
unsigned long lastUpdate = 0;
int testMode = 0;
unsigned long testStartTime = 0;
bool testRunning = false;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("ESP32-C3 Supermini System Test");
  Serial.println("================================");
  Serial.println("Hardware:");
  Serial.println("- SSH1106 128x64 Display");
  Serial.println("- MPU6050 Gyroscope");
  Serial.println("- 4x TTP223 Touch (Pins 0,1,3,4)");
  Serial.println("- Vibration Motor (Pin 6)");
  Serial.println("================================\n");
  
  // Initialize I2C
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(400000); // 400kHz Fast Mode
  
  // Initialize SSH1106 Display
  Serial.print("Initializing SSH1106 Display... ");
  display.begin(0x3C, true); // Address 0x3C, reset=true
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0, 0);
  display.println("ESP32-C3 Test");
  display.println("SSH1106 OK");
  display.display();
  Serial.println("OK");
  
  // Initialize Motor (BC547B transistor driver)
  Serial.print("Initializing Vibration Motor (BC547B)... ");
  pinMode(MOTOR_PIN, OUTPUT);
  digitalWrite(MOTOR_PIN, LOW);  // Ensure motor is OFF before MPU6050 init
  Serial.println("OK");
  
  delay(500);  // Give time for power to stabilize
  
  // Initialize MPU6050 (AFTER motor is confirmed OFF)
  Serial.print("Initializing MPU6050... ");
  mpu.initialize();
  
  delay(100);  // Give sensor time to stabilize
  
  // Try to read actual sensor data as connection test
  bool mpuConnected = false;
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
  
  // If we get non-zero values, sensor is working
  if(ax != 0 || ay != 0 || az != 0 || gx != 0 || gy != 0 || gz != 0) {
    mpuConnected = true;
  }
  
  // Fallback to testConnection() if reading gave all zeros
  if(!mpuConnected) {
    mpuConnected = mpu.testConnection();
  }
  
  if(mpuConnected) {
   Serial.println("OK");
    display.println("MPU6050: OK");
    display.display();
  } else {
    Serial.println("FAILED!");
    display.println("MPU6050: FAIL");
    display.display();
  }
  
  delay(500);
  
  // Initialize Touch Sensors (TTP223 at pins 0, 1, 3, 4)
  Serial.println("Initializing TTP223 Touch Sensors...");
  pinMode(TOUCH1_PIN, INPUT);  // Pin 0
  pinMode(TOUCH2_PIN, INPUT);  // Pin 1
  pinMode(TOUCH3_PIN, INPUT);  // Pin 3
  pinMode(TOUCH4_PIN, INPUT);  // Pin 4
  Serial.println("TTP223 Sensors (0,1,3,4): OK");
  display.println("Touch: OK");
  display.display();
  
  delay(500);
  
  // Test motor with 3 quick pulses AFTER MPU6050 is initialized
  Serial.println("Testing motor...");
  for(int i = 0; i < 3; i++) {
    digitalWrite(MOTOR_PIN, HIGH);
    delay(100);
    digitalWrite(MOTOR_PIN, LOW);
    delay(100);
  }
  display.println("Motor: OK");
  display.display();
  
  delay(2000);
  
  Serial.println("\n================================");
  Serial.println("Test Modes:");
  Serial.println("Touch 1: Display Test");
  Serial.println("Touch 2: Gyro/Accel Test");
  Serial.println("Touch 3: Touch Sensor Test");
  Serial.println("Touch 4: Motor Test");
  Serial.println("================================\n");
}

void loop() {
  // Read touch sensors
  touch1State = digitalRead(TOUCH1_PIN);
  touch2State = digitalRead(TOUCH2_PIN);
  touch3State = digitalRead(TOUCH3_PIN);
  touch4State = digitalRead(TOUCH4_PIN);
  
  // Detect rising edge (button press) only when not in a test
  if(!testRunning) {
    if(touch1State && !lastTouch1) {
      testMode = 1;
      testRunning = true;
      testStartTime = millis();
      vibrateMotor(50);
      Serial.println("\n>>> Starting Display Test");
    }
    else if(touch2State && !lastTouch2) {
      testMode = 2;
      testRunning = true;
      testStartTime = millis();
      vibrateMotor(50);
      Serial.println("\n>>> Starting Gyro Test");
    }
    else if(touch3State && !lastTouch3) {
      testMode = 3;
      testRunning = true;
      testStartTime = millis();
      vibrateMotor(50);
      Serial.println("\n>>> Starting Touch Test");
    }
    else if(touch4State && !lastTouch4) {
      testMode = 4;
      testRunning = true;
      testStartTime = millis();
      vibrateMotor(50);
      Serial.println("\n>>> Starting Motor Test");
    }
  }
  
  // Store previous states
  lastTouch1 = touch1State;
  lastTouch2 = touch2State;
  lastTouch3 = touch3State;
  lastTouch4 = touch4State;
  
  // Run current test mode or default display
  if(testRunning) {
    switch(testMode) {
      case 1:
        displayTest();
        break;
      case 2:
        gyroTest();
        break;
      case 3:
        touchTest();
        break;
      case 4:
        motorTest();
        break;
    }
  } else {
    defaultDisplay();
  }
  
  delay(50);
}

void defaultDisplay() {
  if(millis() - lastUpdate > 500) {
    // Ensure motor is off when in default menu
    digitalWrite(MOTOR_PIN, LOW);
    
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(0, 0);
    display.println("READY");
    display.setTextSize(1);
    display.println();
    display.println("Touch to test:");
    display.println("1: Display");
    display.println("2: Gyro");
    display.println("3: Touch");
    display.println("4: Motor");
    display.display();
    lastUpdate = millis();
  }
}

void displayTest() {
  // Run test for 5 seconds
  unsigned long testDuration = millis() - testStartTime;
  
  if(testDuration < 5000) {
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(0, 0);
    display.println("DISPLAY");
    display.setTextSize(1);
    display.println();
    display.println("Resolution:");
    display.print("  ");
    display.print(SCREEN_WIDTH);
    display.print("x");
    display.println(SCREEN_HEIGHT);
    display.println();
    
    // Show countdown
    display.print("Ending in: ");
    display.print((5000 - testDuration) / 1000);
    display.println("s");
    
    // Animated progress bar
    int barWidth = (millis() / 20) % SCREEN_WIDTH;
    display.drawRect(0, 56, SCREEN_WIDTH, 8, SH110X_WHITE);
    display.fillRect(2, 58, barWidth, 4, SH110X_WHITE);
    
    display.display();
  } else {
    // Test complete, return to menu
    Serial.println(">>> Display Test Complete\n");
    testRunning = false;
    testMode = 0;
  }
}

void gyroTest() {
  // Run test for 10 seconds
  unsigned long testDuration = millis() - testStartTime;
  
  if(testDuration < 10000) {
    // Read MPU6050 data
    mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
    
    // Convert to readable units
    float accelX = ax / 16384.0;
    float accelY = ay / 16384.0;
    float accelZ = az / 16384.0;
    float gyroX = gx / 131.0;
    float gyroY = gy / 131.0;
    float gyroZ = gz / 131.0;
    
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("GYRO/ACCEL TEST");
    
    display.print("Time: ");
    display.print((10000 - testDuration) / 1000);
    display.println("s");
    
    display.print("Acc X: ");
    display.println(accelX, 2);
    display.print("Acc Y: ");
    display.println(accelY, 2);
    display.print("Acc Z: ");
    display.println(accelZ, 2);
    
    display.print("Gyr X: ");
    display.println(gyroX, 0);
    display.print("Gyr Y: ");
    display.println(gyroY, 0);
    display.print("Gyr Z: ");
    display.println(gyroZ, 0);
    
    display.display();
    
    // Serial output
    if(millis() % 500 < 50) {  // Print every 500ms
      Serial.print("Accel: ");
      Serial.print(accelX); Serial.print(", ");
      Serial.print(accelY); Serial.print(", ");
      Serial.print(accelZ);
      Serial.print(" | Gyro: ");
      Serial.print(gyroX); Serial.print(", ");
      Serial.print(gyroY); Serial.print(", ");
      Serial.println(gyroZ);
    }
  } else {
    // Test complete, return to menu
    Serial.println(">>> Gyro Test Complete\n");
    testRunning = false;
    testMode = 0;
  }
}

void touchTest() {
  // Run test for 10 seconds
  unsigned long testDuration = millis() - testStartTime;
  
  if(testDuration < 10000) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("TOUCH TEST");
    
    display.print("Time: ");
    display.print((10000 - testDuration) / 1000);
    display.println("s");
    display.println("Touch sensors:");
    
    // Show touch states
    display.print("1: ");
    display.println(touch1State ? "PRESSED" : "Released");
    
    display.print("2: ");
    display.println(touch2State ? "PRESSED" : "Released");
    
    display.print("3: ");
    display.println(touch3State ? "PRESSED" : "Released");
    
    display.print("4: ");
    display.println(touch4State ? "PRESSED" : "Released");
    
    // Visual indicator
    if(touch1State || touch2State || touch3State || touch4State) {
      display.fillCircle(120, 58, 5, SH110X_WHITE);
    }
    
    display.display();
    
    // Serial output
    if(touch1State || touch2State || touch3State || touch4State) {
      Serial.print("Touch: ");
      if(touch1State) Serial.print("1 ");
      if(touch2State) Serial.print("2 ");
      if(touch3State) Serial.print("3 ");
      if(touch4State) Serial.print("4 ");
      Serial.println();
    }
  } else {
    // Test complete, return to menu
    Serial.println(">>> Touch Test Complete\n");
    testRunning = false;
    testMode = 0;
  }
}

void motorTest() {
  static int motorPattern = 0;
  static unsigned long lastMotorChange = 0;
  unsigned long testDuration = millis() - testStartTime;
  
  // Run test for 15 seconds (3 seconds per pattern * 5 patterns)
  if(testDuration < 15000) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("MOTOR TEST");
    
    display.print("Time: ");
    display.print((15000 - testDuration) / 1000);
    display.println("s");
    display.println("Testing patterns:");
    display.println();
    
    // Cycle through different patterns every 3 seconds
    if(millis() - lastMotorChange > 3000) {
      motorPattern++;
      if(motorPattern > 4) motorPattern = 0;
      lastMotorChange = millis();
      Serial.print("Motor Pattern: ");
      Serial.println(motorPattern);
    }
    
    switch(motorPattern) {
      case 0:
        display.println("Pattern: OFF");
        analogWrite(MOTOR_PIN, 0);
        digitalWrite(MOTOR_PIN, LOW);
        break;
        
      case 1:
        display.println("Pattern: CONSTANT");
        digitalWrite(MOTOR_PIN, HIGH);
        break;
        
      case 2:
        display.println("Pattern: PULSE");
        digitalWrite(MOTOR_PIN, (millis() / 500) % 2);
        break;
        
      case 3:
        display.println("Pattern: FAST");
        digitalWrite(MOTOR_PIN, (millis() / 100) % 2);
        break;
        
      case 4:
        display.println("Pattern: PWM 50%");
        analogWrite(MOTOR_PIN, 128);
        break;
    }
    
    // Progress indicator for current pattern
    int progress = ((millis() - lastMotorChange) * SCREEN_WIDTH) / 3000;
    display.drawRect(0, 50, SCREEN_WIDTH, 8, SH110X_WHITE);
    display.fillRect(2, 52, constrain(progress, 0, SCREEN_WIDTH-4), 4, SH110X_WHITE);
    
    display.display();
  } else {
    // Test complete, turn off motor and return to menu
    analogWrite(MOTOR_PIN, 0);      // Clear PWM first
    digitalWrite(MOTOR_PIN, LOW);   // Then set LOW
    Serial.println(">>> Motor Test Complete\n");
    testRunning = false;
    testMode = 0;
    motorPattern = 0;
    lastMotorChange = 0;
  }
}

void vibrateMotor(int duration) {
  digitalWrite(MOTOR_PIN, HIGH);
  delay(duration);
  digitalWrite(MOTOR_PIN, LOW);
}

// PWM Motor control (0-255)
void setMotorSpeed(int speed) {
  analogWrite(MOTOR_PIN, constrain(speed, 0, 255));
}