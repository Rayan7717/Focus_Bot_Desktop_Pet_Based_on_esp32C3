/*
 * Desktop Pet Robot - Custom Interactive Gestures & Animations
 * ESP32-C3 Supermini with Custom Touch & Gyro Gesture Mapping
 *
 * Hardware Configuration:
 * - ESP32-C3 Supermini (Main Controller)
 * - SH1106 128x64 OLED Display (Visual Expression)
 * - MPU6050 6-Axis Gyro/Accelerometer (Motion Detection)
 * - 4x TTP223 Capacitive Touch Sensors (Button 1-4)
 * - Vibration Motor (BC547B transistor driver, 1kΩ base resistor)
 *
 * Pin Configuration:
 * - I2C: SDA=8, SCL=9
 * - Touch: Pins 0(Button 1), 1(Button 2), 3(Button 3), 4(Button 4)
 * - Motor: Pin 6
 *
 * Interaction Mapping:
 * 🎮 Touch Button Interactions:
 *   Button 1: Single tap → Happy | Hold 2s → Sleepy (fade effect)
 *   Button 2: Single tap → Curious | Double tap → Surprised
 *   Button 3: Single tap → Angry | Hold → Affectionate
 *   Button 4: Single tap → Music | Double tap → Laugh
 *   All 4 buttons together → Love
 *
 * 📱 Gyro Gestures:
 *   Shake → Confused
 *   Tilt left-right repeatedly → Playful
 *   Tilt forward (bow) → Proud
 *   Hold still 10s → Bored
 *   Quick spin (rotation) → Frustrated
 *   Tilt backward → Relaxed
 *
 * 🧠 Dynamic Reactions:
 *   No interaction 2 mins → Sleepy (auto)
 *   Shake after sleeping → Surprised (wake up)
 *   Touched repeatedly → Embarrassed
 *   Random button taps → Excited
 *
 * Required Libraries:
 * - Adafruit GFX Library
 * - Adafruit SH110X
 * - MPU6050 by Electronic Cats
 * - LittleFS (built-in)
 *
 * Author: Interactive Desktop Pet System
 * Version: 3.0
 * Date: 2025
 */

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <MPU6050.h>
#include "FS.h"
#include "LittleFS.h"

// ============================================================================
// PIN DEFINITIONS
// ============================================================================
#define SDA_PIN 8
#define SCL_PIN 9
#define TOUCH1_PIN 0    // Button 1
#define TOUCH2_PIN 1    // Button 2
#define TOUCH3_PIN 3    // Button 3
#define TOUCH4_PIN 4    // Button 4
#define MOTOR_PIN 6     // Vibration Motor

// Display Configuration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

// Animation Configuration
#define MAX_FRAMES 100
#define MAX_COMPRESSED_SIZE 1024

// Timing thresholds
#define DOUBLE_TAP_TIMEOUT 500
#define LONG_PRESS_DURATION 2000
#define IDLE_TIMEOUT 120000  // 2 minutes for auto-sleep
#define STILLNESS_TIMEOUT 10000  // 10 seconds for bored

// ============================================================================
// EMOTION STATE SYSTEM
// ============================================================================
enum EmotionState {
    HAPPY = 0,
    EXCITED = 1,
    SLEEPY = 2,
    PLAYFUL = 3,
    LONELY = 4,
    CURIOUS = 5,
    SURPRISED = 6,
    CONTENT = 7,
    BORED = 8,
    AFFECTIONATE = 9,
    ANGRY = 10,
    CONFUSED = 11,
    DETERMINED = 12,
    EMBARRASSED = 13,
    FRUSTRATED = 14,
    LAUGH = 15,
    LOVE = 16,
    MUSIC = 17,
    PROUD = 18,
    RELAXED = 19
};

const char* emotionNames[] = {
    "Happy", "Excited", "Sleepy", "Playful", "Lonely",
    "Curious", "Surprised", "Content", "Bored", "Affectionate",
    "Angry", "Confused", "Determined", "Embarrassed", "Frustrated",
    "Laugh", "Love", "Music", "Proud", "Relaxed"
};

// ============================================================================
// ANIMATION SYSTEM
// ============================================================================
struct Animation {
  String filename;
  String displayName;
  uint16_t frameCount;
  uint16_t fps;
  uint16_t maxCompressedSize;
  uint16_t frameDelay;
};

// Animation mappings for each emotion state
Animation emotionAnimations[] = {
  {"/happy.anim", "Happy", 0, 0, 0, 0},              // 0 - HAPPY
  {"/excited_2.anim", "Excited", 0, 0, 0, 0},        // 1 - EXCITED
  {"/sleepy.anim", "Sleepy", 0, 0, 0, 0},            // 2 - SLEEPY
  {"/happy_2.anim", "Playful", 0, 0, 0, 0},          // 3 - PLAYFUL
  {"/content.anim", "Lonely", 0, 0, 0, 0},           // 4 - LONELY (using content as fallback)
  {"/confused_2.anim", "Curious", 0, 0, 0, 0},       // 5 - CURIOUS
  {"/embarrassed.anim", "Surprised", 0, 0, 0, 0},    // 6 - SURPRISED
  {"/content.anim", "Content", 0, 0, 0, 0},          // 7 - CONTENT
  {"/relaxed.anim", "Bored", 0, 0, 0, 0},            // 8 - BORED
  {"/love.anim", "Affectionate", 0, 0, 0, 0},        // 9 - AFFECTIONATE
  {"/angry.anim", "Angry", 0, 0, 0, 0},              // 10 - ANGRY
  {"/confused_2.anim", "Confused", 0, 0, 0, 0},      // 11 - CONFUSED
  {"/determined.anim", "Determined", 0, 0, 0, 0},    // 12 - DETERMINED
  {"/embarrassed.anim", "Embarrassed", 0, 0, 0, 0},  // 13 - EMBARRASSED
  {"/frustrated.anim", "Frustrated", 0, 0, 0, 0},    // 14 - FRUSTRATED
  {"/laugh.anim", "Laugh", 0, 0, 0, 0},              // 15 - LAUGH
  {"/love.anim", "Love", 0, 0, 0, 0},                // 16 - LOVE
  {"/music.anim", "Music", 0, 0, 0, 0},              // 17 - MUSIC
  {"/proud.anim", "Proud", 0, 0, 0, 0},              // 18 - PROUD
  {"/relaxed.anim", "Relaxed", 0, 0, 0, 0}           // 19 - RELAXED
};

const int animationCount = sizeof(emotionAnimations) / sizeof(emotionAnimations[0]);

// ============================================================================
// TOUCH INPUT STRUCTURES
// ============================================================================
struct ButtonState {
    bool current;
    bool last;
    unsigned long pressTime;
    unsigned long releaseTime;
    uint8_t tapCount;
    bool longPressTriggered;
};

struct TouchInput {
    ButtonState button[4];
    bool allButtonsPressed;
    unsigned long lastInteractionTime;
    uint8_t rapidTapCount;
    unsigned long lastRapidTapTime;
};

// ============================================================================
// MOTION DETECTION STRUCTURES
// ============================================================================
enum MotionType {
    MOTION_STILL,
    MOTION_SHAKE,
    MOTION_TILT_LEFT,
    MOTION_TILT_RIGHT,
    MOTION_TILT_FORWARD,
    MOTION_TILT_BACKWARD,
    MOTION_ROTATION
};

struct MotionState {
    int16_t ax, ay, az;
    int16_t gx, gy, gz;
    MotionType currentMotion;
    MotionType lastMotion;
    unsigned long lastMotionTime;
    unsigned long stillnessDuration;
    int tiltWiggleCount;
    unsigned long lastTiltTime;
};

// ============================================================================
// GLOBAL OBJECTS & VARIABLES
// ============================================================================
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
MPU6050 mpu;

// Core State Variables
EmotionState currentEmotion = CONTENT;
EmotionState previousEmotion = CONTENT;
unsigned long emotionStartTime = 0;
bool isSleeping = false;

// System Components
TouchInput touchInput;
MotionState motionState;

// Timing Variables
unsigned long lastUpdate = 0;
unsigned long systemStartTime = 0;

// Animation Variables
int currentFrame = 0;
unsigned long lastFrameTime = 0;
bool animationLoaded = false;
uint8_t displayBrightness = 255;

// Buffers for animation decompression
uint8_t* compressedBuffer = nullptr;
uint8_t frameBuffer[1024];  // 128*64/8 = 1024 bytes
uint16_t* frameSizes = nullptr;
File currentAnimFile;
bool animFileOpen = false;

// ============================================================================
// FUNCTION DECLARATIONS
// ============================================================================
// Animation system
void loadAnimation(EmotionState emotion);
void unloadAnimation();
bool loadAnimationMetadata(Animation* anim);
void displayCurrentFrame();
void rleDecompress(const uint8_t* compressed, int compressed_size, uint8_t* output);
void listLittleFS();

// Input handling
void updateTouchInputs();
void updateMotionSensors();
void processButton1();
void processButton2();
void processButton3();
void processButton4();
bool processAllButtonsTouch();
void processGyroGestures();

// Emotion system
void transitionToEmotion(EmotionState newEmotion);
void checkAutoReactions();

// Vibration and feedback
void vibrateGentle();
void vibrateRhythmic();
void vibrateSoft();
void vibrateShort();

// Utility
void fadeToSleep();
void wakeUp();

// ============================================================================
// SETUP FUNCTION
// ============================================================================
void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println(F("\n========================================"));
    Serial.println(F("Interactive Desktop Pet - Custom Input"));
    Serial.println(F("========================================"));

    // Initialize Motor FIRST
    pinMode(MOTOR_PIN, OUTPUT);
    digitalWrite(MOTOR_PIN, LOW);
    delay(100);

    // Initialize I2C
    Wire.begin(SDA_PIN, SCL_PIN);
    Wire.setClock(400000);

    // Initialize Display
    Serial.print(F("Initializing Display... "));
    if (!display.begin(0x3C, true)) {
        Serial.println(F("FAILED!"));
    } else {
        Serial.println(F("OK"));
    }

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    display.setCursor(0, 0);
    display.println(F("Interactive Pet v3.0"));
    display.println(F("Booting..."));
    display.display();
    delay(500);

    // Initialize MPU6050
    Serial.print(F("Initializing MPU6050... "));
    mpu.initialize();
    delay(100);

    int16_t ax, ay, az, gx, gy, gz;
    mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

    bool mpuConnected = false;
    if(ax != 0 || ay != 0 || az != 0 || gx != 0 || gy != 0 || gz != 0) {
        mpuConnected = true;
    }

    if(!mpuConnected) {
        mpuConnected = mpu.testConnection();
    }

    if (mpuConnected) {
        Serial.println(F("OK"));
        display.println(F("Motion: OK"));
    } else {
        Serial.println(F("FAILED"));
        display.println(F("Motion: FAIL"));
    }
    display.display();
    delay(500);

    // Initialize Touch Sensors
    pinMode(TOUCH1_PIN, INPUT);
    pinMode(TOUCH2_PIN, INPUT);
    pinMode(TOUCH3_PIN, INPUT);
    pinMode(TOUCH4_PIN, INPUT);
    Serial.println(F("Touch Sensors: OK"));
    display.println(F("Touch: OK"));
    display.display();
    delay(500);

    // Initialize LittleFS
    Serial.print(F("Initializing LittleFS... "));
    if(!LittleFS.begin(false)) {
        Serial.println(F("No filesystem found."));
        Serial.println(F("Formatting LittleFS..."));

        if(!LittleFS.begin(true)) {
            Serial.println(F("FAILED!"));
            display.println(F("LittleFS: FAIL"));
            display.display();
            while(1) delay(1000);
        }
    }
    Serial.println(F("OK"));
    display.println(F("LittleFS: OK"));
    display.display();
    delay(500);

    // List filesystem contents
    listLittleFS();

    // Load animation metadata
    Serial.println(F("\nLoading animations..."));
    int validAnims = 0;
    for(int i = 0; i < animationCount; i++) {
        if(loadAnimationMetadata(&emotionAnimations[i])) {
            validAnims++;
            Serial.print(F("  ["));
            Serial.print(i);
            Serial.print(F("] "));
            Serial.print(emotionAnimations[i].displayName);
            Serial.print(F(": "));
            Serial.print(emotionAnimations[i].frameCount);
            Serial.print(F(" frames @ "));
            Serial.print(emotionAnimations[i].fps);
            Serial.println(F(" FPS"));
        }
    }
    Serial.print(F("Loaded "));
    Serial.print(validAnims);
    Serial.println(F(" animations"));

    display.clearDisplay();
    display.setCursor(0, 0);
    display.print(F("Animations: "));
    display.println(validAnims);
    display.display();
    delay(1000);

    // Allocate animation buffers
    compressedBuffer = (uint8_t*)malloc(MAX_COMPRESSED_SIZE);
    frameSizes = (uint16_t*)malloc(MAX_FRAMES * sizeof(uint16_t));

    if(!compressedBuffer || !frameSizes) {
        Serial.println(F("ERROR: Buffer allocation failed!"));
        while(1) delay(1000);
    }

    // Initialize touch input structure
    for(int i = 0; i < 4; i++) {
        touchInput.button[i].current = false;
        touchInput.button[i].last = false;
        touchInput.button[i].tapCount = 0;
        touchInput.button[i].longPressTriggered = false;
    }
    touchInput.lastInteractionTime = millis();
    touchInput.rapidTapCount = 0;

    // Welcome Vibration
    for(int i = 0; i < 3; i++) {
        digitalWrite(MOTOR_PIN, HIGH);
        delay(100);
        digitalWrite(MOTOR_PIN, LOW);
        delay(100);
    }

    delay(1000);

    // Initialize State
    systemStartTime = millis();
    emotionStartTime = millis();
    currentEmotion = CONTENT;

    // Load initial animation
    loadAnimation(currentEmotion);

    Serial.println(F("\n=== System Ready ==="));
    Serial.println(F("Starting Interactive Loop...\n"));
}

// ============================================================================
// MAIN LOOP
// ============================================================================
void loop() {
    unsigned long currentTime = millis();

    // Update sensor inputs
    updateTouchInputs();
    updateMotionSensors();

    // Process button inputs (custom mapping)
    // Check all-button press FIRST before individual buttons
    if (!processAllButtonsTouch()) {
        // Only process individual buttons if all-4 wasn't triggered
        processButton1();
        processButton2();
        processButton3();
        processButton4();
    }

    // Process gyro gestures
    processGyroGestures();

    // Check for automatic reactions
    checkAutoReactions();

    // Update display with animation
    if(animationLoaded && !isSleeping) {
        Animation* anim = &emotionAnimations[currentEmotion];

        if(currentTime - lastFrameTime >= anim->frameDelay) {
            displayCurrentFrame();
            currentFrame++;

            // Loop animation
            if(currentFrame >= anim->frameCount) {
                currentFrame = 0;
            }

            lastFrameTime = currentTime;
        }
    } else if (isSleeping) {
        // Sleep mode - dim display
        if (currentTime - lastUpdate >= 500) {
            displayCurrentFrame();
            lastUpdate = currentTime;
        }
    } else {
        // Fallback display if animation not loaded
        if (currentTime - lastUpdate >= 100) {
            display.clearDisplay();
            display.setTextSize(2);
            display.setCursor(0, 20);
            display.println(emotionNames[currentEmotion]);
            display.display();
            lastUpdate = currentTime;
        }
    }

    delay(10);  // Small delay for stability
}

// ============================================================================
// ANIMATION SYSTEM IMPLEMENTATION
// ============================================================================
void loadAnimation(EmotionState emotion) {
    unloadAnimation();

    Animation* anim = &emotionAnimations[emotion];

    currentAnimFile = LittleFS.open(anim->filename, "r");
    if(!currentAnimFile) {
        Serial.print(F("Failed to load animation: "));
        Serial.println(anim->filename);
        animationLoaded = false;
        return;
    }

    // Skip header (12 bytes)
    currentAnimFile.seek(12);

    // Read frame sizes array
    for(int i = 0; i < anim->frameCount; i++) {
        uint8_t sizeBytes[2];
        if(currentAnimFile.read(sizeBytes, 2) != 2) {
            currentAnimFile.close();
            animationLoaded = false;
            return;
        }
        frameSizes[i] = sizeBytes[0] | (sizeBytes[1] << 8);
    }

    animFileOpen = true;
    animationLoaded = true;
    currentFrame = 0;
    lastFrameTime = millis();

    Serial.print(F("Loaded animation: "));
    Serial.println(anim->displayName);
}

void unloadAnimation() {
    if(animFileOpen) {
        currentAnimFile.close();
        animFileOpen = false;
    }
    animationLoaded = false;
}

bool loadAnimationMetadata(Animation* anim) {
    File file = LittleFS.open(anim->filename, "r");
    if(!file) {
        return false;
    }

    // Read header (12 bytes)
    uint8_t header[12];
    if(file.read(header, 12) != 12) {
        file.close();
        return false;
    }

    anim->frameCount = header[0] | (header[1] << 8);
    anim->fps = header[2] | (header[3] << 8);
    anim->maxCompressedSize = header[4] | (header[5] << 8);
    anim->frameDelay = 1000 / anim->fps;

    file.close();
    return true;
}

void displayCurrentFrame() {
    Animation* anim = &emotionAnimations[currentEmotion];

    // Calculate file position for this frame
    size_t frameDataStart = 12 + (anim->frameCount * 2);
    size_t frameOffset = frameDataStart + (currentFrame * anim->maxCompressedSize);

    // Seek to frame position
    currentAnimFile.seek(frameOffset);

    // Read compressed frame data
    int compressedSize = frameSizes[currentFrame];
    int bytesRead = currentAnimFile.read(compressedBuffer, compressedSize);

    if(bytesRead != compressedSize) {
        Serial.print(F("ERROR: Failed to read frame "));
        Serial.println(currentFrame);
        return;
    }

    // Decompress the frame
    rleDecompress(compressedBuffer, compressedSize, frameBuffer);

    // Display the decompressed frame
    display.clearDisplay();

    // Draw bitmap from decompressed buffer
    for(int y = 0; y < SCREEN_HEIGHT; y++) {
        for(int x = 0; x < SCREEN_WIDTH; x++) {
            int byteIndex = (y * SCREEN_WIDTH + x) / 8;
            int bitIndex = 7 - (x % 8);

            if(frameBuffer[byteIndex] & (1 << bitIndex)) {
                display.drawPixel(x, y, SH110X_WHITE);
            }
        }
    }

    display.display();
}

void rleDecompress(const uint8_t* compressed, int compressed_size, uint8_t* output) {
    int out_idx = 0;
    for (int i = 0; i < compressed_size; i += 2) {
        uint8_t count = compressed[i];
        uint8_t value = compressed[i + 1];
        for (uint8_t j = 0; j < count; j++) {
            output[out_idx++] = value;
        }
    }
}

void listLittleFS() {
    Serial.println(F("\nLittleFS Contents:"));
    File root = LittleFS.open("/");
    File file = root.openNextFile();
    int fileCount = 0;

    while(file) {
        Serial.print(F("  "));
        Serial.print(file.name());
        Serial.print(F(" ("));
        Serial.print(file.size());
        Serial.println(F(" bytes)"));
        fileCount++;
        file = root.openNextFile();
    }

    Serial.print(F("Total files: "));
    Serial.println(fileCount);
}

// ============================================================================
// INPUT PROCESSING - TOUCH BUTTONS
// ============================================================================
void updateTouchInputs() {
    unsigned long now = millis();

    for (int i = 0; i < 4; i++) {
        touchInput.button[i].last = touchInput.button[i].current;

        // Read button state based on pin
        switch(i) {
            case 0: touchInput.button[i].current = digitalRead(TOUCH1_PIN); break;
            case 1: touchInput.button[i].current = digitalRead(TOUCH2_PIN); break;
            case 2: touchInput.button[i].current = digitalRead(TOUCH3_PIN); break;
            case 3: touchInput.button[i].current = digitalRead(TOUCH4_PIN); break;
        }

        // Detect press events (rising edge)
        if (touchInput.button[i].current && !touchInput.button[i].last) {
            touchInput.button[i].pressTime = now;
            touchInput.button[i].longPressTriggered = false;
            touchInput.lastInteractionTime = now;

            // Count taps for double-tap detection
            if (now - touchInput.button[i].releaseTime < DOUBLE_TAP_TIMEOUT) {
                touchInput.button[i].tapCount++;
            } else {
                touchInput.button[i].tapCount = 1;
            }

            // Count rapid taps across all buttons
            if (now - touchInput.lastRapidTapTime < 1000) {
                touchInput.rapidTapCount++;
            } else {
                touchInput.rapidTapCount = 1;
            }
            touchInput.lastRapidTapTime = now;
        }

        // Detect release events (falling edge)
        if (!touchInput.button[i].current && touchInput.button[i].last) {
            touchInput.button[i].releaseTime = now;
        }
    }

    // Check if all 4 buttons are pressed simultaneously
    touchInput.allButtonsPressed = touchInput.button[0].current &&
                                    touchInput.button[1].current &&
                                    touchInput.button[2].current &&
                                    touchInput.button[3].current;
}

// Button 1: Single tap → Happy | Hold 2s → Sleepy
void processButton1() {
    unsigned long now = millis();
    ButtonState* btn = &touchInput.button[0];
    static unsigned long waitUntil = 0;
    static bool waitingForDoubleTap = false;

    // Long press detection (2 seconds)
    if (btn->current && !btn->longPressTriggered) {
        unsigned long pressDuration = now - btn->pressTime;
        if (pressDuration >= LONG_PRESS_DURATION) {
            btn->longPressTriggered = true;
            waitingForDoubleTap = false;
            Serial.println(F("Button 1: Long Press - Going to sleep"));
            fadeToSleep();
            btn->tapCount = 0;
        }
    }

    // Single tap detection (on release)
    if (!btn->current && btn->last && !btn->longPressTriggered) {
        unsigned long pressDuration = btn->releaseTime - btn->pressTime;
        if (pressDuration < LONG_PRESS_DURATION) {
            waitingForDoubleTap = true;
            waitUntil = now + DOUBLE_TAP_TIMEOUT;
        }
    }

    // Check if double-tap timeout expired
    if (waitingForDoubleTap && now >= waitUntil) {
        if (btn->tapCount == 1) {
            Serial.println(F("Button 1: Single Tap - Happy"));
            wakeUp();
            transitionToEmotion(HAPPY);
            vibrateGentle();
        }
        btn->tapCount = 0;
        waitingForDoubleTap = false;
    }
}

// Button 2: Single tap → Curious | Double tap → Surprised
void processButton2() {
    unsigned long now = millis();
    ButtonState* btn = &touchInput.button[1];
    static unsigned long waitUntil = 0;
    static bool waitingForDoubleTap = false;

    // On button release, start waiting for potential double tap
    if (!btn->current && btn->last) {
        unsigned long pressDuration = btn->releaseTime - btn->pressTime;
        if (pressDuration < DOUBLE_TAP_TIMEOUT) {
            if (btn->tapCount >= 2) {
                // Double tap detected immediately
                Serial.println(F("Button 2: Double Tap - Surprised"));
                wakeUp();
                transitionToEmotion(SURPRISED);
                vibrateShort();
                btn->tapCount = 0;
                waitingForDoubleTap = false;
            } else {
                // First tap, wait for potential second
                waitingForDoubleTap = true;
                waitUntil = now + DOUBLE_TAP_TIMEOUT;
            }
        }
    }

    // Check if double-tap timeout expired
    if (waitingForDoubleTap && now >= waitUntil) {
        if (btn->tapCount == 1) {
            Serial.println(F("Button 2: Single Tap - Curious"));
            wakeUp();
            transitionToEmotion(CURIOUS);
            vibrateGentle();
        }
        btn->tapCount = 0;
        waitingForDoubleTap = false;
    }
}

// Button 3: Single tap → Angry | Hold → Affectionate
void processButton3() {
    unsigned long now = millis();
    ButtonState* btn = &touchInput.button[2];
    static unsigned long waitUntil = 0;
    static bool waitingForDoubleTap = false;

    // Long press detection (2 seconds)
    if (btn->current && !btn->longPressTriggered) {
        unsigned long pressDuration = now - btn->pressTime;
        if (pressDuration >= LONG_PRESS_DURATION) {
            btn->longPressTriggered = true;
            waitingForDoubleTap = false;
            Serial.println(F("Button 3: Long Press - Affectionate"));
            wakeUp();
            transitionToEmotion(AFFECTIONATE);
            vibrateSoft();
            btn->tapCount = 0;
        }
    }

    // Single tap detection (on release)
    if (!btn->current && btn->last && !btn->longPressTriggered) {
        unsigned long pressDuration = btn->releaseTime - btn->pressTime;
        if (pressDuration < LONG_PRESS_DURATION) {
            waitingForDoubleTap = true;
            waitUntil = now + DOUBLE_TAP_TIMEOUT;
        }
    }

    // Check if double-tap timeout expired
    if (waitingForDoubleTap && now >= waitUntil) {
        if (btn->tapCount == 1) {
            Serial.println(F("Button 3: Single Tap - Angry"));
            wakeUp();
            transitionToEmotion(ANGRY);
            vibrateShort();
        }
        btn->tapCount = 0;
        waitingForDoubleTap = false;
    }
}

// Button 4: Single tap → Music | Double tap → Laugh
void processButton4() {
    unsigned long now = millis();
    ButtonState* btn = &touchInput.button[3];
    static unsigned long waitUntil = 0;
    static bool waitingForDoubleTap = false;

    // On button release, start waiting for potential double tap
    if (!btn->current && btn->last) {
        unsigned long pressDuration = btn->releaseTime - btn->pressTime;
        if (pressDuration < DOUBLE_TAP_TIMEOUT) {
            if (btn->tapCount >= 2) {
                // Double tap detected immediately
                Serial.println(F("Button 4: Double Tap - Laugh"));
                wakeUp();
                transitionToEmotion(LAUGH);
                vibrateRhythmic();
                btn->tapCount = 0;
                waitingForDoubleTap = false;
            } else {
                // First tap, wait for potential second
                waitingForDoubleTap = true;
                waitUntil = now + DOUBLE_TAP_TIMEOUT;
            }
        }
    }

    // Check if double-tap timeout expired
    if (waitingForDoubleTap && now >= waitUntil) {
        if (btn->tapCount == 1) {
            Serial.println(F("Button 4: Single Tap - Music"));
            wakeUp();
            transitionToEmotion(MUSIC);
            vibrateRhythmic();
        }
        btn->tapCount = 0;
        waitingForDoubleTap = false;
    }
}

// All 4 buttons together → Love
bool processAllButtonsTouch() {
    static bool allButtonsProcessed = false;

    if (touchInput.allButtonsPressed && !allButtonsProcessed) {
        Serial.println(F("All Buttons: Love"));
        wakeUp();
        transitionToEmotion(LOVE);
        vibrateSoft();

        // Reset all tap counts
        for(int i = 0; i < 4; i++) {
            touchInput.button[i].tapCount = 0;
            touchInput.button[i].longPressTriggered = true;  // Prevent individual triggers
        }
        allButtonsProcessed = true;
        return true;  // All buttons processed, skip individual button handling
    }

    if (!touchInput.allButtonsPressed) {
        allButtonsProcessed = false;
    }

    return false;  // Individual buttons can be processed
}

// ============================================================================
// INPUT PROCESSING - MOTION SENSORS
// ============================================================================
void updateMotionSensors() {
    unsigned long now = millis();

    // Read MPU6050 data
    mpu.getMotion6(&motionState.ax, &motionState.ay, &motionState.az,
                   &motionState.gx, &motionState.gy, &motionState.gz);

    // Convert to g-force
    float accelX = motionState.ax / 16384.0;
    float accelY = motionState.ay / 16384.0;
    float accelZ = motionState.az / 16384.0;

    // Convert to degrees/second
    float gyroX = motionState.gx / 131.0;
    float gyroY = motionState.gy / 131.0;
    float gyroZ = motionState.gz / 131.0;

    float accelMagnitude = sqrt(accelX*accelX + accelY*accelY + accelZ*accelZ);
    float gyroMagnitude = sqrt(gyroX*gyroX + gyroY*gyroY + gyroZ*gyroZ);

    motionState.lastMotion = motionState.currentMotion;

    // Detect motion types
    if (accelMagnitude > 2.0) {
        // Shake detected
        motionState.currentMotion = MOTION_SHAKE;
        motionState.lastMotionTime = now;
    } else if (gyroMagnitude > 150.0) {
        // Quick rotation/spin
        motionState.currentMotion = MOTION_ROTATION;
        motionState.lastMotionTime = now;
    } else if (accelX > 0.5) {
        // Tilt left
        motionState.currentMotion = MOTION_TILT_LEFT;
        motionState.lastMotionTime = now;
    } else if (accelX < -0.5) {
        // Tilt right
        motionState.currentMotion = MOTION_TILT_RIGHT;
        motionState.lastMotionTime = now;
    } else if (accelY > 0.6) {
        // Tilt forward (bow)
        motionState.currentMotion = MOTION_TILT_FORWARD;
        motionState.lastMotionTime = now;
    } else if (accelY < -0.6) {
        // Tilt backward
        motionState.currentMotion = MOTION_TILT_BACKWARD;
        motionState.lastMotionTime = now;
    } else if (gyroMagnitude < 5.0 && abs(accelMagnitude - 1.0) < 0.2) {
        // Still
        motionState.currentMotion = MOTION_STILL;
        if (motionState.lastMotion != MOTION_STILL) {
            motionState.stillnessDuration = 0;
        } else {
            motionState.stillnessDuration = now - motionState.lastMotionTime;
        }
    } else {
        motionState.currentMotion = MOTION_STILL;
    }
}

void processGyroGestures() {
    unsigned long now = millis();
    static unsigned long lastGestureTime = 0;

    // Debounce gestures (minimum 1 second between)
    if (now - lastGestureTime < 1000) {
        return;
    }

    // Shake → Confused
    if (motionState.currentMotion == MOTION_SHAKE && motionState.lastMotion != MOTION_SHAKE) {
        Serial.println(F("Gyro: Shake - Confused"));
        wakeUp();
        transitionToEmotion(CONFUSED);
        vibrateShort();
        lastGestureTime = now;
    }

    // Quick spin (rotation) → Frustrated
    if (motionState.currentMotion == MOTION_ROTATION && motionState.lastMotion != MOTION_ROTATION) {
        Serial.println(F("Gyro: Quick Spin - Frustrated"));
        wakeUp();
        transitionToEmotion(FRUSTRATED);
        vibrateShort();
        lastGestureTime = now;
    }

    // Tilt forward (bow) → Proud
    if (motionState.currentMotion == MOTION_TILT_FORWARD && motionState.lastMotion != MOTION_TILT_FORWARD) {
        Serial.println(F("Gyro: Tilt Forward - Proud"));
        wakeUp();
        transitionToEmotion(PROUD);
        vibrateGentle();
        lastGestureTime = now;
    }

    // Tilt backward → Relaxed
    if (motionState.currentMotion == MOTION_TILT_BACKWARD && motionState.lastMotion != MOTION_TILT_BACKWARD) {
        Serial.println(F("Gyro: Tilt Backward - Relaxed"));
        wakeUp();
        transitionToEmotion(RELAXED);
        vibrateSoft();
        lastGestureTime = now;
    }

    // Tilt left-right repeatedly → Playful
    if ((motionState.currentMotion == MOTION_TILT_LEFT || motionState.currentMotion == MOTION_TILT_RIGHT) &&
        (motionState.lastMotion == MOTION_TILT_RIGHT || motionState.lastMotion == MOTION_TILT_LEFT) &&
        motionState.currentMotion != motionState.lastMotion) {

        if (now - motionState.lastTiltTime < 1000) {
            motionState.tiltWiggleCount++;
            if (motionState.tiltWiggleCount >= 3) {
                Serial.println(F("Gyro: Wiggle (Left-Right) - Playful"));
                wakeUp();
                transitionToEmotion(PLAYFUL);
                vibrateRhythmic();
                motionState.tiltWiggleCount = 0;
                lastGestureTime = now;
            }
        } else {
            motionState.tiltWiggleCount = 1;
        }
        motionState.lastTiltTime = now;
    }

    // Hold still for 10 seconds → Bored
    if (motionState.currentMotion == MOTION_STILL && motionState.stillnessDuration >= STILLNESS_TIMEOUT) {
        if (!isSleeping && currentEmotion != BORED) {
            Serial.println(F("Gyro: Still for 10s - Bored"));
            transitionToEmotion(BORED);
            motionState.lastMotionTime = now;  // Reset to prevent repeated triggers
        }
    }
}

// ============================================================================
// AUTOMATIC REACTIONS
// ============================================================================
void checkAutoReactions() {
    unsigned long now = millis();
    unsigned long timeSinceInteraction = now - touchInput.lastInteractionTime;

    // No interaction for 2 minutes → Auto-sleep
    if (timeSinceInteraction >= IDLE_TIMEOUT && !isSleeping) {
        Serial.println(F("Auto: No interaction 2min - Sleepy"));
        fadeToSleep();
    }

    // Touched repeatedly (many rapid taps) → Embarrassed
    if (touchInput.rapidTapCount >= 8) {
        Serial.println(F("Auto: Rapid taps - Embarrassed"));
        wakeUp();
        transitionToEmotion(EMBARRASSED);
        vibrateShort();
        touchInput.rapidTapCount = 0;
    }

    // Random button taps (varied buttons quickly) → Excited
    static unsigned long lastButtonPressed[4] = {0, 0, 0, 0};
    int recentButtonCount = 0;
    for(int i = 0; i < 4; i++) {
        if (touchInput.button[i].current && now - lastButtonPressed[i] > 2000) {
            recentButtonCount++;
            lastButtonPressed[i] = now;
        }
    }
    if (recentButtonCount >= 3) {
        Serial.println(F("Auto: Random taps - Excited"));
        wakeUp();
        transitionToEmotion(EXCITED);
        vibrateRhythmic();
    }
}

// ============================================================================
// EMOTION TRANSITIONS
// ============================================================================
void transitionToEmotion(EmotionState newEmotion) {
    if (newEmotion == currentEmotion && !isSleeping) return;

    previousEmotion = currentEmotion;
    currentEmotion = newEmotion;
    emotionStartTime = millis();

    loadAnimation(currentEmotion);

    Serial.print(F("Emotion: "));
    Serial.print(emotionNames[previousEmotion]);
    Serial.print(F(" -> "));
    Serial.println(emotionNames[currentEmotion]);
}

// ============================================================================
// SLEEP / WAKE FUNCTIONS
// ============================================================================
void fadeToSleep() {
    Serial.println(F("Fading to sleep..."));
    transitionToEmotion(SLEEPY);

    // Gentle fade effect
    for(int i = 0; i < 5; i++) {
        displayCurrentFrame();
        delay(500);
    }

    isSleeping = true;
    displayBrightness = 50;  // Dim display
}

void wakeUp() {
    if (isSleeping) {
        Serial.println(F("Waking up - Surprised"));
        isSleeping = false;
        displayBrightness = 255;
        transitionToEmotion(SURPRISED);
        vibrateShort();
    }
}

// ============================================================================
// VIBRATION PATTERNS
// ============================================================================
void vibrateGentle() {
    digitalWrite(MOTOR_PIN, HIGH);
    delay(200);
    digitalWrite(MOTOR_PIN, LOW);
}

void vibrateRhythmic() {
    for (int i = 0; i < 3; i++) {
        digitalWrite(MOTOR_PIN, HIGH);
        delay(100);
        digitalWrite(MOTOR_PIN, LOW);
        delay(100);
    }
}

void vibrateSoft() {
    for (int i = 0; i < 255; i += 20) {
        analogWrite(MOTOR_PIN, i);
        delay(10);
    }
    for (int i = 255; i > 0; i -= 20) {
        analogWrite(MOTOR_PIN, i);
        delay(10);
    }
    // Properly disable PWM and set pin LOW
    analogWrite(MOTOR_PIN, 0);
    pinMode(MOTOR_PIN, OUTPUT);
    digitalWrite(MOTOR_PIN, LOW);
}

void vibrateShort() {
    for (int i = 0; i < 2; i++) {
        digitalWrite(MOTOR_PIN, HIGH);
        delay(50);
        digitalWrite(MOTOR_PIN, LOW);
        delay(100);
    }
}
