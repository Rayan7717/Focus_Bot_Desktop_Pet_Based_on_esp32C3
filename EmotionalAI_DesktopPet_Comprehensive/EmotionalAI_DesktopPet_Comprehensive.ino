/*
 * Desktop Pet Robot - Comprehensive Emotional AI System with Animation Support
 * ESP32-C3 Supermini with Adaptive Personality & Multi-Modal Feedback
 *
 * Hardware Configuration:
 * - ESP32-C3 Supermini (Main Controller)
 * - SH1106 128x64 OLED Display (Visual Expression)
 * - MPU6050 6-Axis Gyro/Accelerometer (Motion Detection)
 * - 4x TTP223 Capacitive Touch Sensors (Front Left, Back Left, Front Right, Back Right)
 * - Vibration Motor (BC547B transistor driver, 1kΩ base resistor)
 *
 * Pin Configuration:
 * - I2C: SDA=8, SCL=9
 * - Touch: Pins 0(Front Left), 1(Back Left), 3(Front Right), 4(Back Right)
 * - Motor: Pin 6
 *
 * Features:
 * - 10 Distinct Emotion States with Animated Expressions
 * - Touch Pattern Recognition (Single, Double, Long Press, Rapid Taps, Multi-Touch)
 * - Motion-Based Responses (Tilt, Shake, Rotation, Stillness)
 * - Adaptive Personality System (Learns User Preferences)
 * - Circadian Rhythm (Activity Varies by Time)
 * - Social Bonding Mechanics (Interaction History)
 * - Multi-Modal Feedback (Visual Animations + Haptic Coordination)
 * - LittleFS Animation Storage with RLE Compression
 *
 * Required Libraries:
 * - Adafruit GFX Library
 * - Adafruit SH110X
 * - MPU6050 by Electronic Cats
 * - LittleFS (built-in)
 *
 * IMPORTANT: Before uploading:
 * 1. Run: python convert_to_spiffs.py (creates binary .anim files)
 * 2. Upload data folder: Tools -> ESP32 Sketch Data Upload
 * 3. Then upload this sketch
 * 4. Tools -> Partition Scheme -> 'Default 4MB with spiffs'
 *
 * Author: Comprehensive Desktop Pet AI System
 * Version: 2.0
 * Date: 2025
 */

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <MPU6050.h>
#include <EEPROM.h>
#include "FS.h"
#include "LittleFS.h"

// ============================================================================
// PIN DEFINITIONS
// ============================================================================
#define SDA_PIN 8
#define SCL_PIN 9
#define TOUCH1_PIN 0    // Left Front
#define TOUCH2_PIN 1    // Left Back
#define TOUCH3_PIN 3    // Right Front
#define TOUCH4_PIN 4    // Right Back
#define MOTOR_PIN 6     // Vibration Motor

// Display Configuration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

// Animation Configuration
#define MAX_FRAMES 100
#define MAX_COMPRESSED_SIZE 1024

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
// PERSONALITY TRAIT SYSTEM
// ============================================================================
struct PersonalityTraits {
    uint8_t playfulness;    // 0-10: Affects playful state preference
    uint8_t affection;      // 0-10: Affects response to touch
    uint8_t curiosity;      // 0-10: Affects exploration behaviors
    uint8_t energy;         // 0-10: Affects sleep/activity balance
    uint8_t sociability;    // 0-10: Affects loneliness threshold
};

// ============================================================================
// INTERACTION HISTORY
// ============================================================================
struct InteractionHistory {
    unsigned long lastInteraction;
    unsigned long totalInteractions;
    uint16_t leftFrontTouches;
    uint16_t leftBackTouches;
    uint16_t rightFrontTouches;
    uint16_t rightBackTouches;
    uint8_t favoriteLocation;  // 0=LeftFront, 1=LeftBack, 2=RightFront, 3=RightBack
    uint8_t bondLevel;         // 0-100
};

// ============================================================================
// TOUCH PATTERN DETECTION
// ============================================================================
enum TouchPattern {
    TOUCH_NONE,
    TOUCH_SINGLE,
    TOUCH_DOUBLE,
    TOUCH_LONG_PRESS,
    TOUCH_RAPID_TAPS,
    TOUCH_MULTI_SENSOR
};

struct TouchState {
    bool current[4];
    bool last[4];
    unsigned long pressTime[4];
    unsigned long releaseTime[4];
    uint8_t tapCount[4];
    TouchPattern pattern;
    uint8_t activeLocation;  // Which sensor triggered pattern
};

// ============================================================================
// MOTION DETECTION
// ============================================================================
enum MotionType {
    MOTION_STILL,
    MOTION_TILT,
    MOTION_SHAKE,
    MOTION_ROTATION,
    MOTION_DROP
};

struct MotionState {
    int16_t ax, ay, az;
    int16_t gx, gy, gz;
    MotionType currentMotion;
    unsigned long lastMotionTime;
    unsigned long stillnessDuration;
};

// ============================================================================
// VIBRATION PATTERNS
// ============================================================================
enum VibrationPattern {
    VIB_NONE,
    VIB_GENTLE_PULSE,    // Happiness/Content
    VIB_RAPID_BURST,     // Excitement
    VIB_LONG_SUSTAINED,  // Loneliness
    VIB_WAVE_PATTERN,    // Affection
    VIB_SHORT_TAPS       // Curiosity/Playful
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
unsigned long emotionDuration = 30000;  // Default 30 seconds per emotion

// System Components
PersonalityTraits personality;
InteractionHistory history;
TouchState touchState;
MotionState motionState;

// Timing Variables
unsigned long lastUpdate = 0;
unsigned long lastEmotionUpdate = 0;
unsigned long systemStartTime = 0;

// Animation Variables
int currentFrame = 0;
unsigned long lastFrameTime = 0;
bool animationLoaded = false;

// Buffers for animation decompression
uint8_t* compressedBuffer = nullptr;
uint8_t frameBuffer[1024];  // 128*64/8 = 1024 bytes
uint16_t* frameSizes = nullptr;
File currentAnimFile;
bool animFileOpen = false;

// EEPROM Addresses for Persistence
#define EEPROM_SIZE 512
#define EEPROM_PERSONALITY_ADDR 0
#define EEPROM_HISTORY_ADDR 50
#define EEPROM_INIT_FLAG_ADDR 500

// ============================================================================
// EMOTION TRANSITION PROBABILITY MATRIX (Expanded for 20 states)
// ============================================================================
// Simplified for memory - will be calculated dynamically based on personality
const uint8_t PROGMEM baseEmotionWeights[20] = {
    25,  // HAPPY - high probability
    15,  // EXCITED - medium-high
    10,  // SLEEPY - medium
    20,  // PLAYFUL - high
    5,   // LONELY - low
    15,  // CURIOUS - medium-high
    8,   // SURPRISED - medium-low
    30,  // CONTENT - highest (default state)
    8,   // BORED - medium-low
    12,  // AFFECTIONATE - medium
    3,   // ANGRY - low
    7,   // CONFUSED - medium-low
    10,  // DETERMINED - medium
    5,   // EMBARRASSED - low
    4,   // FRUSTRATED - low
    15,  // LAUGH - medium-high
    12,  // LOVE - medium
    10,  // MUSIC - medium
    10,  // PROUD - medium
    15   // RELAXED - medium-high
};

// ============================================================================
// FUNCTION DECLARATIONS
// ============================================================================
// Animation system
void loadAnimation(EmotionState emotion);
void unloadAnimation();
bool loadAnimationMetadata(Animation* anim);
void displayCurrentFrame();
void displayFrameFromLittleFS(Animation* anim, int frameIndex);
void rleDecompress(const uint8_t* compressed, int compressed_size, uint8_t* output);
void listLittleFS();

// Touch and motion
void handleTouchPattern(TouchPattern pattern, uint8_t location);
void respondToTouch(TouchPattern pattern, uint8_t location);
void handleMotionEvent(MotionType motion);

// Emotion system
void transitionToEmotion(EmotionState newEmotion);
void updateEmotionState();
EmotionState selectNextEmotion();
uint16_t applyPersonalityModifier(EmotionState emotion, uint8_t baseProb);

// Vibration and feedback
void playVibrationPattern(VibrationPattern pattern);

// Personality and adaptation
void adaptPersonality(TouchPattern pattern, uint8_t location);
void updateFavoriteLocation();
void applyCircadianRhythm();

// Data persistence
void loadOrInitializeData();
void initializeDefaultPersonality();
void initializeHistory();
void savePersonalityData();
void loadPersonalityData();
void saveHistoryData();
void loadHistoryData();
void printPersonality();

// Sensor processing
void updateTouchSensors();
void updateMotionSensors();
void processTouchPatterns();
void processMotionEvents();

// ============================================================================
// SETUP FUNCTION
// ============================================================================
void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println(F("\n========================================"));
    Serial.println(F("Desktop Pet Robot - Comprehensive AI"));
    Serial.println(F("========================================"));

    // Initialize Motor FIRST (before MPU6050)
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
    display.println(F("Emotional AI v2.0"));
    display.println(F("Booting..."));
    display.display();
    delay(500);

    // Initialize MPU6050 (AFTER motor is OFF)
    Serial.print(F("Initializing MPU6050... "));
    mpu.initialize();
    delay(100);

    // Test MPU6050 with actual data read
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

    // Initialize EEPROM
    EEPROM.begin(EEPROM_SIZE);

    // Load or Initialize Personality & History
    loadOrInitializeData();

    // Welcome Vibration (3 quick pulses AFTER MPU6050 init)
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
    Serial.println(F("Personality Profile:"));
    printPersonality();
    Serial.println(F("\nStarting Emotional Loop...\n"));
}

// ============================================================================
// MAIN LOOP
// ============================================================================
void loop() {
    unsigned long currentTime = millis();

    // Update sensor inputs
    updateTouchSensors();
    updateMotionSensors();

    // Process touch patterns
    processTouchPatterns();

    // Process motion events
    processMotionEvents();

    // Update emotion state (every 500ms)
    if (currentTime - lastEmotionUpdate >= 500) {
        updateEmotionState();
        lastEmotionUpdate = currentTime;
    }

    // Update display with animation (based on FPS)
    if(animationLoaded) {
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

    // Circadian rhythm check (every 10 seconds)
    static unsigned long lastCircadianCheck = 0;
    if (currentTime - lastCircadianCheck >= 10000) {
        applyCircadianRhythm();
        lastCircadianCheck = currentTime;
    }

    // Auto-save personality data (every 5 minutes)
    static unsigned long lastSave = 0;
    if (currentTime - lastSave >= 300000) {
        savePersonalityData();
        saveHistoryData();
        lastSave = currentTime;
    }

    delay(10);  // Small delay for stability
}

// ============================================================================
// ANIMATION SYSTEM IMPLEMENTATION
// ============================================================================
void loadAnimation(EmotionState emotion) {
    // Unload previous animation
    unloadAnimation();

    Animation* anim = &emotionAnimations[emotion];

    // Open animation file
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

    unsigned long startTime = micros();

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

    // Draw status bar overlay
    int bondBarWidth = (history.bondLevel * 30) / 100;
    display.drawRect(0, 60, 32, 4, SH110X_WHITE);
    display.fillRect(1, 61, bondBarWidth, 2, SH110X_WHITE);

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
// SENSOR INPUT PROCESSING
// ============================================================================
void updateTouchSensors() {
    // Store previous states
    for (int i = 0; i < 4; i++) {
        touchState.last[i] = touchState.current[i];
    }

    // Read current states
    touchState.current[0] = digitalRead(TOUCH1_PIN);
    touchState.current[1] = digitalRead(TOUCH2_PIN);
    touchState.current[2] = digitalRead(TOUCH3_PIN);
    touchState.current[3] = digitalRead(TOUCH4_PIN);

    unsigned long now = millis();

    // Detect press/release events
    for (int i = 0; i < 4; i++) {
        // Rising edge - button pressed
        if (touchState.current[i] && !touchState.last[i]) {
            touchState.pressTime[i] = now;
            touchState.tapCount[i]++;

            // Reset tap count if too much time passed
            if (now - touchState.releaseTime[i] > 2000) {
                touchState.tapCount[i] = 1;
            }
        }

        // Falling edge - button released
        if (!touchState.current[i] && touchState.last[i]) {
            touchState.releaseTime[i] = now;
        }
    }
}

void updateMotionSensors() {
    // Read MPU6050 data
    mpu.getMotion6(&motionState.ax, &motionState.ay, &motionState.az,
                   &motionState.gx, &motionState.gy, &motionState.gz);

    unsigned long now = millis();

    // Convert to g-force
    float accelX = motionState.ax / 16384.0;
    float accelY = motionState.ay / 16384.0;
    float accelZ = motionState.az / 16384.0;

    // Convert to degrees/second
    float gyroX = motionState.gx / 131.0;
    float gyroY = motionState.gy / 131.0;
    float gyroZ = motionState.gz / 131.0;

    // Detect motion types
    float accelMagnitude = sqrt(accelX*accelX + accelY*accelY + accelZ*accelZ);
    float gyroMagnitude = sqrt(gyroX*gyroX + gyroY*gyroY + gyroZ*gyroZ);

    // Shake detection (high acceleration)
    if (accelMagnitude > 2.0) {
        motionState.currentMotion = MOTION_SHAKE;
        motionState.lastMotionTime = now;
    }
    // Rotation detection (high gyro)
    else if (gyroMagnitude > 100.0) {
        motionState.currentMotion = MOTION_ROTATION;
        motionState.lastMotionTime = now;
    }
    // Tilt detection (deviation from vertical)
    else if (abs(accelZ - 1.0) > 0.3) {
        motionState.currentMotion = MOTION_TILT;
        motionState.lastMotionTime = now;
    }
    // Stillness
    else if (gyroMagnitude < 5.0 && accelMagnitude < 1.2) {
        motionState.currentMotion = MOTION_STILL;
        motionState.stillnessDuration = now - motionState.lastMotionTime;
    }
    else {
        motionState.currentMotion = MOTION_STILL;
    }
}

// ============================================================================
// TOUCH PATTERN RECOGNITION
// ============================================================================
void processTouchPatterns() {
    unsigned long now = millis();
    touchState.pattern = TOUCH_NONE;

    // Check each sensor
    for (int i = 0; i < 4; i++) {
        if (!touchState.current[i]) continue;

        unsigned long pressDuration = now - touchState.pressTime[i];
        touchState.activeLocation = i;

        // Long Press Detection (>3000ms)
        if (pressDuration > 3000 && touchState.last[i]) {
            touchState.pattern = TOUCH_LONG_PRESS;
            handleTouchPattern(TOUCH_LONG_PRESS, i);
            touchState.tapCount[i] = 0;
            return;
        }

        // Rapid Taps Detection (5 taps in 2 seconds)
        if (touchState.tapCount[i] >= 5) {
            if (now - touchState.releaseTime[i] < 2000) {
                touchState.pattern = TOUCH_RAPID_TAPS;
                handleTouchPattern(TOUCH_RAPID_TAPS, i);
                touchState.tapCount[i] = 0;
                return;
            }
        }

        // Double Tap Detection (2 taps < 500ms)
        if (touchState.tapCount[i] == 2) {
            if (now - touchState.releaseTime[i] < 500) {
                touchState.pattern = TOUCH_DOUBLE;
                handleTouchPattern(TOUCH_DOUBLE, i);
                touchState.tapCount[i] = 0;
                return;
            }
        }
    }

    // Check for single tap on released buttons
    for (int i = 0; i < 4; i++) {
        if (!touchState.current[i] && touchState.last[i]) {
            unsigned long pressDuration = millis() - touchState.pressTime[i];
            if (pressDuration < 500) {
                delay(150);
                if (touchState.tapCount[i] == 1) {
                    touchState.pattern = TOUCH_SINGLE;
                    handleTouchPattern(TOUCH_SINGLE, i);
                    touchState.tapCount[i] = 0;
                }
            }
        }
    }

    // Multi-Sensor Detection
    int activeSensors = 0;
    for (int i = 0; i < 4; i++) {
        if (touchState.current[i]) activeSensors++;
    }

    if (activeSensors >= 2) {
        touchState.pattern = TOUCH_MULTI_SENSOR;
        handleTouchPattern(TOUCH_MULTI_SENSOR, 0);
    }
}

void handleTouchPattern(TouchPattern pattern, uint8_t location) {
    unsigned long now = millis();

    // Update interaction history
    history.lastInteraction = now;
    history.totalInteractions++;

    switch(location) {
        case 0: history.leftFrontTouches++; break;
        case 1: history.leftBackTouches++; break;
        case 2: history.rightFrontTouches++; break;
        case 3: history.rightBackTouches++; break;
    }

    // Update favorite location
    updateFavoriteLocation();

    // Update bond level
    if (history.bondLevel < 100) {
        history.bondLevel++;
    }

    // Adapt personality based on interaction
    adaptPersonality(pattern, location);

    // Trigger emotional response
    respondToTouch(pattern, location);

    // Debug output
    const char* patternNames[] = {"NONE", "SINGLE", "DOUBLE", "LONG", "RAPID", "MULTI"};
    Serial.print(F("Touch: "));
    Serial.print(patternNames[pattern]);
    Serial.print(F(" @ Location: "));
    Serial.print(location);
    Serial.print(F(" | Bond: "));
    Serial.println(history.bondLevel);
}

void respondToTouch(TouchPattern pattern, uint8_t location) {
    // Different responses based on pattern and personality
    switch(pattern) {
        case TOUCH_SINGLE:
            // Quick acknowledgment
            if (random(100) < 40) {
                transitionToEmotion(HAPPY);
            }
            playVibrationPattern(VIB_SHORT_TAPS);
            break;

        case TOUCH_DOUBLE:
            // Excited response
            if (random(100) < 60) {
                transitionToEmotion(EXCITED);
            } else {
                transitionToEmotion(LAUGH);
            }
            playVibrationPattern(VIB_RAPID_BURST);
            break;

        case TOUCH_LONG_PRESS:
            // Affectionate bonding
            if(random(100) < 70) {
                transitionToEmotion(AFFECTIONATE);
            } else {
                transitionToEmotion(LOVE);
            }
            playVibrationPattern(VIB_WAVE_PATTERN);
            break;

        case TOUCH_RAPID_TAPS:
            // Overwhelmed or very excited
            if (personality.energy > 6) {
                transitionToEmotion(EXCITED);
            } else {
                transitionToEmotion(SURPRISED);
            }
            playVibrationPattern(VIB_RAPID_BURST);
            break;

        case TOUCH_MULTI_SENSOR:
            // Special combined response
            transitionToEmotion(SURPRISED);
            playVibrationPattern(VIB_WAVE_PATTERN);
            break;
    }
}

// ============================================================================
// MOTION EVENT PROCESSING
// ============================================================================
void processMotionEvents() {
    static MotionType lastMotion = MOTION_STILL;

    // Detect motion change
    if (motionState.currentMotion != lastMotion) {
        handleMotionEvent(motionState.currentMotion);
        lastMotion = motionState.currentMotion;
    }

    // Check for prolonged stillness (triggers sleep)
    if (motionState.stillnessDuration > 60000) {  // 60 seconds
        if (currentEmotion != SLEEPY && random(100) < 20) {
            transitionToEmotion(SLEEPY);
        }
    }
}

void handleMotionEvent(MotionType motion) {
    switch(motion) {
        case MOTION_SHAKE:
            // Surprise or excitement or anger
            if (personality.energy > 5) {
                transitionToEmotion(EXCITED);
            } else if (random(100) < 30) {
                transitionToEmotion(ANGRY);
            } else {
                transitionToEmotion(SURPRISED);
            }
            playVibrationPattern(VIB_RAPID_BURST);
            Serial.println(F("Motion: SHAKE detected"));
            break;

        case MOTION_TILT:
            // Curiosity or confusion
            if (random(100) < 50) {
                transitionToEmotion(CURIOUS);
            } else {
                transitionToEmotion(CONFUSED);
            }
            Serial.println(F("Motion: TILT detected"));
            break;

        case MOTION_ROTATION:
            // Dizzy/confused
            transitionToEmotion(CONFUSED);
            Serial.println(F("Motion: ROTATION detected"));
            break;

        case MOTION_STILL:
            // Calm down
            break;
    }
}

// ============================================================================
// EMOTION STATE MACHINE
// ============================================================================
void updateEmotionState() {
    unsigned long now = millis();
    unsigned long timeSinceLastInteraction = now - history.lastInteraction;

    // Check if emotion duration has expired
    if (now - emotionStartTime >= emotionDuration) {
        // Natural emotion transition
        EmotionState nextEmotion = selectNextEmotion();
        transitionToEmotion(nextEmotion);
    }

    // Loneliness check (no interaction for 5 minutes)
    if (timeSinceLastInteraction > 300000 && currentEmotion != LONELY) {
        if (random(100) < (10 - personality.sociability) * 10) {
            transitionToEmotion(LONELY);
        }
    }

    // Boredom check (in same state too long without interaction)
    if (currentEmotion == previousEmotion && timeSinceLastInteraction > 120000) {
        if (random(100) < 30) {
            transitionToEmotion(BORED);
        }
    }
}

EmotionState selectNextEmotion() {
    // Weighted random selection based on base weights and personality
    uint16_t probabilities[20];
    uint16_t totalProb = 0;

    for (int i = 0; i < 20; i++) {
        uint8_t baseProb = pgm_read_byte(&baseEmotionWeights[i]);
        probabilities[i] = applyPersonalityModifier((EmotionState)i, baseProb);
        totalProb += probabilities[i];
    }

    // Weighted random selection
    uint16_t randValue = random(totalProb);
    uint16_t cumulative = 0;

    for (int i = 0; i < 20; i++) {
        cumulative += probabilities[i];
        if (randValue < cumulative) {
            return (EmotionState)i;
        }
    }

    return CONTENT;  // Fallback
}

uint16_t applyPersonalityModifier(EmotionState emotion, uint8_t baseProb) {
    int16_t modified = baseProb;

    // Apply personality trait influences
    switch(emotion) {
        case PLAYFUL:
            modified += (personality.playfulness - 5) * 3;
            break;
        case AFFECTIONATE:
        case LOVE:
            modified += (personality.affection - 5) * 3;
            break;
        case CURIOUS:
            modified += (personality.curiosity - 5) * 3;
            break;
        case SLEEPY:
        case RELAXED:
            modified += (10 - personality.energy) * 2;
            break;
        case EXCITED:
        case LAUGH:
            modified += (personality.energy - 5) * 2;
            break;
        case LONELY:
            modified += (10 - personality.sociability) * 2;
            break;
    }

    // Ensure valid range
    return constrain(modified, 0, 200);
}

void transitionToEmotion(EmotionState newEmotion) {
    if (newEmotion == currentEmotion) return;

    previousEmotion = currentEmotion;
    currentEmotion = newEmotion;
    emotionStartTime = millis();

    // Randomize emotion duration (15-60 seconds)
    emotionDuration = random(15000, 60000);

    // Load new animation
    loadAnimation(currentEmotion);

    Serial.print(F("Emotion: "));
    Serial.print(emotionNames[previousEmotion]);
    Serial.print(F(" -> "));
    Serial.println(emotionNames[currentEmotion]);
}

// ============================================================================
// VIBRATION PATTERN SYSTEM
// ============================================================================
void playVibrationPattern(VibrationPattern pattern) {
    switch(pattern) {
        case VIB_GENTLE_PULSE:
            digitalWrite(MOTOR_PIN, HIGH);
            delay(200);
            digitalWrite(MOTOR_PIN, LOW);
            break;

        case VIB_RAPID_BURST:
            for (int i = 0; i < 3; i++) {
                digitalWrite(MOTOR_PIN, HIGH);
                delay(100);
                digitalWrite(MOTOR_PIN, LOW);
                delay(100);
            }
            break;

        case VIB_LONG_SUSTAINED:
            digitalWrite(MOTOR_PIN, HIGH);
            delay(1000);
            digitalWrite(MOTOR_PIN, LOW);
            break;

        case VIB_WAVE_PATTERN:
            for (int i = 0; i < 255; i += 15) {
                analogWrite(MOTOR_PIN, i);
                delay(20);
            }
            for (int i = 255; i > 0; i -= 15) {
                analogWrite(MOTOR_PIN, i);
                delay(20);
            }
            digitalWrite(MOTOR_PIN, LOW);
            break;

        case VIB_SHORT_TAPS:
            for (int i = 0; i < 5; i++) {
                digitalWrite(MOTOR_PIN, HIGH);
                delay(50);
                digitalWrite(MOTOR_PIN, LOW);
                delay(100);
            }
            break;

        default:
            digitalWrite(MOTOR_PIN, LOW);
            break;
    }
}

// ============================================================================
// PERSONALITY ADAPTATION SYSTEM
// ============================================================================
void adaptPersonality(TouchPattern pattern, uint8_t location) {
    // Gradually adjust personality based on user preferences

    // If user frequently uses rapid taps, increase playfulness
    if (pattern == TOUCH_RAPID_TAPS && personality.playfulness < 10) {
        if (random(100) < 20) personality.playfulness++;
    }

    // Long presses increase affection
    if (pattern == TOUCH_LONG_PRESS && personality.affection < 10) {
        if (random(100) < 30) personality.affection++;
    }

    // Frequent interactions increase sociability
    if (history.totalInteractions % 50 == 0 && personality.sociability < 10) {
        personality.sociability++;
    }

    // Regular play increases energy
    if (currentEmotion == PLAYFUL && personality.energy < 10) {
        if (random(100) < 10) personality.energy++;
    }

    // Exploration behaviors increase curiosity
    if (motionState.currentMotion != MOTION_STILL && personality.curiosity < 10) {
        if (random(100) < 5) personality.curiosity++;
    }
}

void updateFavoriteLocation() {
    uint16_t maxTouches = history.leftFrontTouches;
    history.favoriteLocation = 0;

    if (history.leftBackTouches > maxTouches) {
        maxTouches = history.leftBackTouches;
        history.favoriteLocation = 1;
    }
    if (history.rightFrontTouches > maxTouches) {
        maxTouches = history.rightFrontTouches;
        history.favoriteLocation = 2;
    }
    if (history.rightBackTouches > maxTouches) {
        maxTouches = history.rightBackTouches;
        history.favoriteLocation = 3;
    }
}

// ============================================================================
// CIRCADIAN RHYTHM SYSTEM
// ============================================================================
void applyCircadianRhythm() {
    // Simulate time-based energy levels
    unsigned long uptime = millis() - systemStartTime;
    unsigned long hours = (uptime / 3600000) % 24;

    // Adjust personality.energy based on "time of day"
    if (hours >= 22 || hours <= 6) {  // Night time
        if (personality.energy > 3 && random(100) < 10) {
            personality.energy--;
        }
        if (currentEmotion != SLEEPY && random(100) < 30) {
            transitionToEmotion(SLEEPY);
        }
    } else if (hours >= 6 && hours <= 10) {  // Morning
        if (personality.energy < 7 && random(100) < 10) {
            personality.energy++;
        }
    }
}

// ============================================================================
// DATA PERSISTENCE (EEPROM)
// ============================================================================
void loadOrInitializeData() {
    uint8_t initFlag = EEPROM.read(EEPROM_INIT_FLAG_ADDR);

    if (initFlag == 0xAA) {
        loadPersonalityData();
        loadHistoryData();
        Serial.println(F("Loaded existing data"));
    } else {
        initializeDefaultPersonality();
        initializeHistory();
        savePersonalityData();
        saveHistoryData();
        EEPROM.write(EEPROM_INIT_FLAG_ADDR, 0xAA);
        EEPROM.commit();
        Serial.println(F("Initialized new data"));
    }
}

void initializeDefaultPersonality() {
    personality.playfulness = 5;
    personality.affection = 5;
    personality.curiosity = 5;
    personality.energy = 7;
    personality.sociability = 5;
}

void initializeHistory() {
    history.lastInteraction = millis();
    history.totalInteractions = 0;
    history.leftFrontTouches = 0;
    history.leftBackTouches = 0;
    history.rightFrontTouches = 0;
    history.rightBackTouches = 0;
    history.favoriteLocation = 0;
    history.bondLevel = 0;
}

void savePersonalityData() {
    int addr = EEPROM_PERSONALITY_ADDR;
    EEPROM.write(addr++, personality.playfulness);
    EEPROM.write(addr++, personality.affection);
    EEPROM.write(addr++, personality.curiosity);
    EEPROM.write(addr++, personality.energy);
    EEPROM.write(addr++, personality.sociability);
    EEPROM.commit();
}

void loadPersonalityData() {
    int addr = EEPROM_PERSONALITY_ADDR;
    personality.playfulness = EEPROM.read(addr++);
    personality.affection = EEPROM.read(addr++);
    personality.curiosity = EEPROM.read(addr++);
    personality.energy = EEPROM.read(addr++);
    personality.sociability = EEPROM.read(addr++);
}

void saveHistoryData() {
    int addr = EEPROM_HISTORY_ADDR;
    EEPROM.write(addr++, history.bondLevel);
    EEPROM.write(addr++, history.favoriteLocation);
    EEPROM.write(addr++, history.leftFrontTouches >> 8);
    EEPROM.write(addr++, history.leftFrontTouches & 0xFF);
    EEPROM.write(addr++, history.leftBackTouches >> 8);
    EEPROM.write(addr++, history.leftBackTouches & 0xFF);
    EEPROM.write(addr++, history.rightFrontTouches >> 8);
    EEPROM.write(addr++, history.rightFrontTouches & 0xFF);
    EEPROM.write(addr++, history.rightBackTouches >> 8);
    EEPROM.write(addr++, history.rightBackTouches & 0xFF);
    EEPROM.commit();
}

void loadHistoryData() {
    int addr = EEPROM_HISTORY_ADDR;
    history.bondLevel = EEPROM.read(addr++);
    history.favoriteLocation = EEPROM.read(addr++);
    history.leftFrontTouches = (EEPROM.read(addr++) << 8) | EEPROM.read(addr++);
    history.leftBackTouches = (EEPROM.read(addr++) << 8) | EEPROM.read(addr++);
    history.rightFrontTouches = (EEPROM.read(addr++) << 8) | EEPROM.read(addr++);
    history.rightBackTouches = (EEPROM.read(addr++) << 8) | EEPROM.read(addr++);
    history.lastInteraction = millis();
    history.totalInteractions = history.leftFrontTouches + history.leftBackTouches +
                                history.rightFrontTouches + history.rightBackTouches;
}

void printPersonality() {
    Serial.print(F("  Playfulness: ")); Serial.println(personality.playfulness);
    Serial.print(F("  Affection: ")); Serial.println(personality.affection);
    Serial.print(F("  Curiosity: ")); Serial.println(personality.curiosity);
    Serial.print(F("  Energy: ")); Serial.println(personality.energy);
    Serial.print(F("  Sociability: ")); Serial.println(personality.sociability);
    Serial.print(F("  Bond Level: ")); Serial.println(history.bondLevel);
}
