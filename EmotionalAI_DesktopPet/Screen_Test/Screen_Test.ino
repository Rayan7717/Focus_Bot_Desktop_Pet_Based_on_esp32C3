/*
 * ESP32-C3 Screen Test - Sensor Test & Animation Display
 * Tests all sensors then displays all available animations
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
 *
 * Test Sequence:
 * 1. Display initialization test
 * 2. MPU6050 sensor test
 * 3. Touch sensor test
 * 4. Vibration motor test
 * 5. Animation playback (all .h files in folder)
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <MPU6050.h>

// Include all animation files
#include "angry.h"
#include "angry_2.h"
#include "confused_2.h"
#include "content.h"
#include "determined.h"
#include "embarrassed.h"
#include "excited_2.h"
#include "frustrated.h"
#include "happy.h"
#include "happy_2.h"
#include "intro.h"
#include "laugh.h"
#include "love.h"
#include "music.h"
#include "proud.h"
#include "relaxed.h"
#include "sleepy.h"
#include "sleepy_3.h"

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

// Animation structure to hold animation data
struct Animation {
  const char* name;
  const uint8_t (*frames)[1024];
  int frameCount;
  int fps;
  int frameDelay;
};

// Animation list - all available animations
Animation animations[] = {
  {"Intro", intro_frames, intro_FRAME_COUNT, intro_FPS, intro_FRAME_DELAY},
  {"Happy", happy_frames, happy_FRAME_COUNT, happy_FPS, happy_FRAME_DELAY},
  {"Happy 2", happy_2_frames, happy_2_FRAME_COUNT, happy_2_FPS, happy_2_FRAME_DELAY},
  {"Angry", angry_frames, angry_FRAME_COUNT, angry_FPS, angry_FRAME_DELAY},
  {"Angry 2", angry_2_frames, angry_2_FRAME_COUNT, angry_2_FPS, angry_2_FRAME_DELAY},
  {"Confused", confused_2_frames, confused_2_FRAME_COUNT, confused_2_FPS, confused_2_FRAME_DELAY},
  {"Content", content_frames, content_FRAME_COUNT, content_FPS, content_FRAME_DELAY},
  {"Determined", determined_frames, determined_FRAME_COUNT, determined_FPS, determined_FRAME_DELAY},
  {"Embarrassed", embarrassed_frames, embarrassed_FRAME_COUNT, embarrassed_FPS, embarrassed_FRAME_DELAY},
  {"Excited", excited_2_frames, excited_2_FRAME_COUNT, excited_2_FPS, excited_2_FRAME_DELAY},
  {"Frustrated", frustrated_frames, frustrated_FRAME_COUNT, frustrated_FPS, frustrated_FRAME_DELAY},
  {"Laugh", laugh_frames, laugh_FRAME_COUNT, laugh_FPS, laugh_FRAME_DELAY},
  {"Love", love_frames, love_FRAME_COUNT, love_FPS, love_FRAME_DELAY},
  {"Music", music_frames, music_FRAME_COUNT, music_FPS, music_FRAME_DELAY},
  {"Proud", proud_frames, proud_FRAME_COUNT, proud_FPS, proud_FRAME_DELAY},
  {"Relaxed", relaxed_frames, relaxed_FRAME_COUNT, relaxed_FPS, relaxed_FRAME_DELAY},
  {"Sleepy", sleepy_frames, sleepy_FRAME_COUNT, sleepy_FPS, sleepy_FRAME_DELAY},
  {"Sleepy 2", sleepy_3_frames, sleepy_3_FRAME_COUNT, sleepy_3_FPS, sleepy_3_FRAME_DELAY}
};

const int animationCount = sizeof(animations) / sizeof(animations[0]);

// Test state variables
int16_t ax, ay, az, gx, gy, gz;
bool touch1State = false, touch2State = false, touch3State = false, touch4State = false;
enum TestPhase { INIT, DISPLAY_TEST, MPU_TEST, TOUCH_TEST, MOTOR_TEST, ANIMATION_TEST, COMPLETE };
TestPhase currentPhase = INIT;
unsigned long phaseStartTime = 0;
int currentAnimation = 0;
int currentFrame = 0;
unsigned long lastFrameTime = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n========================================");
  Serial.println("ESP32-C3 Screen Test");
  Serial.println("Sensor Test + Animation Display");
  Serial.println("========================================");
  Serial.println("Hardware:");
  Serial.println("- SSH1106 128x64 OLED Display");
  Serial.println("- MPU6050 Gyroscope/Accelerometer");
  Serial.println("- 4x TTP223 Touch Sensors (Pins 0,1,3,4)");
  Serial.println("- Vibration Motor (Pin 6)");
  Serial.println("========================================\n");

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
  display.println("Screen Test");
  display.println("Starting...");
  display.display();
  Serial.println("OK");
  delay(1000);

  // Initialize Motor
  Serial.print("Initializing Vibration Motor... ");
  pinMode(MOTOR_PIN, OUTPUT);
  digitalWrite(MOTOR_PIN, LOW);
  Serial.println("OK");
  delay(500);

  // Initialize MPU6050
  Serial.print("Initializing MPU6050... ");
  mpu.initialize();
  delay(100);

  bool mpuConnected = false;
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

  if(ax != 0 || ay != 0 || az != 0 || gx != 0 || gy != 0 || gz != 0) {
    mpuConnected = true;
  }

  if(!mpuConnected) {
    mpuConnected = mpu.testConnection();
  }

  if(mpuConnected) {
    Serial.println("OK");
  } else {
    Serial.println("FAILED!");
  }

  // Initialize Touch Sensors
  Serial.println("Initializing Touch Sensors...");
  pinMode(TOUCH1_PIN, INPUT);
  pinMode(TOUCH2_PIN, INPUT);
  pinMode(TOUCH3_PIN, INPUT);
  pinMode(TOUCH4_PIN, INPUT);
  Serial.println("OK");

  Serial.println("\n========================================");
  Serial.println("Starting Test Sequence...");
  Serial.println("========================================\n");

  // Start first test phase
  currentPhase = DISPLAY_TEST;
  phaseStartTime = millis();
}

void loop() {
  switch(currentPhase) {
    case DISPLAY_TEST:
      runDisplayTest();
      break;

    case MPU_TEST:
      runMPUTest();
      break;

    case TOUCH_TEST:
      runTouchTest();
      break;

    case MOTOR_TEST:
      runMotorTest();
      break;

    case ANIMATION_TEST:
      runAnimationTest();
      break;

    case COMPLETE:
      runCompleteScreen();
      break;

    default:
      break;
  }
}

void runDisplayTest() {
  unsigned long elapsed = millis() - phaseStartTime;

  if(elapsed < 3000) {
    // Display test pattern
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(0, 0);
    display.println("DISPLAY");
    display.setTextSize(1);
    display.println();
    display.print("Resolution: ");
    display.print(SCREEN_WIDTH);
    display.print("x");
    display.println(SCREEN_HEIGHT);
    display.println();
    display.print("Test: ");
    display.print(3 - (elapsed / 1000));
    display.println("s");

    // Animated progress bar
    int barWidth = (elapsed * SCREEN_WIDTH) / 3000;
    display.drawRect(0, 56, SCREEN_WIDTH, 8, SH110X_WHITE);
    display.fillRect(2, 58, barWidth, 4, SH110X_WHITE);

    display.display();

    if(elapsed < 100) {
      Serial.println(">>> Display Test Started");
    }
  } else {
    // Move to next phase
    Serial.println(">>> Display Test Complete\n");
    currentPhase = MPU_TEST;
    phaseStartTime = millis();
  }
}

void runMPUTest() {
  unsigned long elapsed = millis() - phaseStartTime;

  if(elapsed < 5000) {
    // Read MPU6050 data
    mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

    float accelX = ax / 16384.0;
    float accelY = ay / 16384.0;
    float accelZ = az / 16384.0;

    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("MPU6050 TEST");
    display.print("Time: ");
    display.print(5 - (elapsed / 1000));
    display.println("s");
    display.println();

    display.print("Acc X: ");
    display.println(accelX, 2);
    display.print("Acc Y: ");
    display.println(accelY, 2);
    display.print("Acc Z: ");
    display.println(accelZ, 2);
    display.println();
    display.println("Tilt the device!");

    display.display();

    if(elapsed < 100) {
      Serial.println(">>> MPU6050 Test Started");
    }
  } else {
    // Move to next phase
    Serial.println(">>> MPU6050 Test Complete\n");
    currentPhase = TOUCH_TEST;
    phaseStartTime = millis();
  }
}

void runTouchTest() {
  unsigned long elapsed = millis() - phaseStartTime;

  if(elapsed < 7000) {
    // Read touch sensors
    touch1State = digitalRead(TOUCH1_PIN);
    touch2State = digitalRead(TOUCH2_PIN);
    touch3State = digitalRead(TOUCH3_PIN);
    touch4State = digitalRead(TOUCH4_PIN);

    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("TOUCH TEST");
    display.print("Time: ");
    display.print(7 - (elapsed / 1000));
    display.println("s");
    display.println();
    display.println("Touch sensors:");

    display.print("1: ");
    display.println(touch1State ? "PRESSED" : "Released");
    display.print("2: ");
    display.println(touch2State ? "PRESSED" : "Released");
    display.print("3: ");
    display.println(touch3State ? "PRESSED" : "Released");
    display.print("4: ");
    display.println(touch4State ? "PRESSED" : "Released");

    if(touch1State || touch2State || touch3State || touch4State) {
      display.fillCircle(120, 58, 5, SH110X_WHITE);
    }

    display.display();

    if(elapsed < 100) {
      Serial.println(">>> Touch Test Started");
      Serial.println("Touch any sensor...");
    }

    if(touch1State || touch2State || touch3State || touch4State) {
      Serial.print("Touch: ");
      if(touch1State) Serial.print("1 ");
      if(touch2State) Serial.print("2 ");
      if(touch3State) Serial.print("3 ");
      if(touch4State) Serial.print("4 ");
      Serial.println();
    }
  } else {
    // Move to next phase
    Serial.println(">>> Touch Test Complete\n");
    currentPhase = MOTOR_TEST;
    phaseStartTime = millis();
  }
}

void runMotorTest() {
  static int motorPattern = 0;
  static unsigned long lastPatternChange = 0;
  unsigned long elapsed = millis() - phaseStartTime;

  if(elapsed < 12000) {
    // Change pattern every 3 seconds
    if(millis() - lastPatternChange > 3000) {
      motorPattern++;
      if(motorPattern > 3) motorPattern = 0;
      lastPatternChange = millis();
      Serial.print("Motor Pattern: ");
      Serial.println(motorPattern);
    }

    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("MOTOR TEST");
    display.print("Time: ");
    display.print(12 - (elapsed / 1000));
    display.println("s");
    display.println();
    display.print("Pattern ");
    display.print(motorPattern);
    display.println(":");
    display.println();

    switch(motorPattern) {
      case 0:
        display.println("OFF");
        digitalWrite(MOTOR_PIN, LOW);
        break;
      case 1:
        display.println("CONSTANT");
        digitalWrite(MOTOR_PIN, HIGH);
        break;
      case 2:
        display.println("PULSE (500ms)");
        digitalWrite(MOTOR_PIN, (millis() / 500) % 2);
        break;
      case 3:
        display.println("FAST (100ms)");
        digitalWrite(MOTOR_PIN, (millis() / 100) % 2);
        break;
    }

    // Progress bar
    int patternProgress = ((millis() - lastPatternChange) * SCREEN_WIDTH) / 3000;
    display.drawRect(0, 56, SCREEN_WIDTH, 8, SH110X_WHITE);
    display.fillRect(2, 58, constrain(patternProgress, 0, SCREEN_WIDTH-4), 4, SH110X_WHITE);

    display.display();

    if(elapsed < 100) {
      Serial.println(">>> Motor Test Started");
      lastPatternChange = millis();
    }
  } else {
    // Turn off motor and move to next phase
    digitalWrite(MOTOR_PIN, LOW);
    Serial.println(">>> Motor Test Complete\n");
    Serial.println("========================================");
    Serial.println("Starting Animation Display");
    Serial.println("========================================\n");
    currentPhase = ANIMATION_TEST;
    phaseStartTime = millis();
    currentAnimation = 0;
    currentFrame = 0;
    lastFrameTime = millis();
  }
}

void runAnimationTest() {
  // Check if we've displayed all animations
  if(currentAnimation >= animationCount) {
    currentPhase = COMPLETE;
    phaseStartTime = millis();
    return;
  }

  Animation* anim = &animations[currentAnimation];

  // Display animation name at start
  if(currentFrame == 0 && millis() - lastFrameTime < 100) {
    Serial.print(">>> Playing Animation: ");
    Serial.print(anim->name);
    Serial.print(" (");
    Serial.print(anim->frameCount);
    Serial.print(" frames @ ");
    Serial.print(anim->fps);
    Serial.println(" FPS)");
  }

  // Check if it's time to update frame
  if(millis() - lastFrameTime >= anim->frameDelay) {
    // Display current frame
    displayFrame(anim->frames[currentFrame]);

    // Move to next frame
    currentFrame++;
    lastFrameTime = millis();

    // Check if animation complete
    if(currentFrame >= anim->frameCount) {
      Serial.print(">>> Animation Complete: ");
      Serial.println(anim->name);
      Serial.println();

      currentAnimation++;
      currentFrame = 0;

      // Short pause between animations
      delay(500);
    }
  }
}

void runCompleteScreen() {
  unsigned long elapsed = millis() - phaseStartTime;

  if(elapsed < 5000) {
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(10, 10);
    display.println("TEST");
    display.println(" COMPLETE!");
    display.setTextSize(1);
    display.println();
    display.print("Animations: ");
    display.println(animationCount);

    // Checkmark animation
    if((elapsed / 500) % 2 == 0) {
      display.fillCircle(64, 56, 6, SH110X_WHITE);
    }

    display.display();

    if(elapsed < 100) {
      Serial.println("========================================");
      Serial.println("ALL TESTS COMPLETE!");
      Serial.println("========================================");
      Serial.print("Total Animations Displayed: ");
      Serial.println(animationCount);
      Serial.println("========================================\n");
    }
  } else {
    // Restart animation sequence
    currentPhase = ANIMATION_TEST;
    currentAnimation = 0;
    currentFrame = 0;
    phaseStartTime = millis();
    lastFrameTime = millis();

    Serial.println(">>> Restarting animation display...\n");
  }
}

// Display a single frame from PROGMEM
void displayFrame(const uint8_t* frameData) {
  display.clearDisplay();

  // Read frame data from PROGMEM and draw to display
  for(int y = 0; y < SCREEN_HEIGHT; y++) {
    for(int x = 0; x < SCREEN_WIDTH; x++) {
      int byteIndex = (y * SCREEN_WIDTH + x) / 8;
      int bitIndex = 7 - (x % 8);

      uint8_t byte = pgm_read_byte(&frameData[byteIndex]);

      if(byte & (1 << bitIndex)) {
        display.drawPixel(x, y, SH110X_WHITE);
      }
    }
  }

  display.display();
}
