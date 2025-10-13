/*
 * Desktop Pet Robot - Advanced Emotional AI System
 * ESP32-C3 Supermini with Adaptive Personality & Multi-Modal Feedback
 *
 * Hardware Configuration:
 * - ESP32-C3 Supermini (Main Controller)
 * - SH1106 128x64 OLED Display (Visual Expression)
 * - MPU6050 6-Axis Gyro/Accelerometer (Motion Detection)
 * - 4x TTP223 Capacitive Touch Sensors (Head, Left, Right, Back)
 * - Vibration Motor (BC547B transistor driver, 1kΩ base resistor)
 *
 * Pin Configuration:
 * - I2C: SDA=8, SCL=9
 * - Touch: Pins 0(Head), 1(Left), 3(Right), 4(Back)
 * - Motor: Pin 6
 *
 * Features:
 * - 10 Distinct Emotion States with Natural Transitions
 * - Touch Pattern Recognition (Single, Double, Long Press, Rapid Taps)
 * - Motion-Based Responses (Tilt, Shake, Rotation, Stillness)
 * - Adaptive Personality System (Learns User Preferences)
 * - Circadian Rhythm (Activity Varies by Time)
 * - Social Bonding Mechanics (Interaction History)
 * - Multi-Modal Feedback (Visual + Haptic Coordination)
 *
 * Required Libraries:
 * - Adafruit GFX Library
 * - Adafruit SH110X
 * - MPU6050 by Electronic Cats
 *
 * Author: Advanced Desktop Pet AI System
 * Version: 1.0
 * Date: 2025
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <MPU6050.h>
#include <EEPROM.h>

// ============================================================================
// PIN DEFINITIONS
// ============================================================================
#define SDA_PIN 8
#define SCL_PIN 9
#define TOUCH1_PIN 0    // Head
#define TOUCH2_PIN 1    // Left Side
#define TOUCH3_PIN 3    // Right Side
#define TOUCH4_PIN 4    // Back
#define MOTOR_PIN 6

// Display Configuration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

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
    AFFECTIONATE = 9
};

const char* emotionNames[] = {
    "Happy", "Excited", "Sleepy", "Playful", "Lonely",
    "Curious", "Surprised", "Content", "Bored", "Affectionate"
};

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
    uint16_t headTouches;
    uint16_t leftTouches;
    uint16_t rightTouches;
    uint16_t backTouches;
    uint8_t favoriteLocation;  // 0=Head, 1=Left, 2=Right, 3=Back
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
int animationFrame = 0;
unsigned long lastAnimationUpdate = 0;

// EEPROM Addresses for Persistence
#define EEPROM_SIZE 512
#define EEPROM_PERSONALITY_ADDR 0
#define EEPROM_HISTORY_ADDR 50
#define EEPROM_INIT_FLAG_ADDR 500

// ============================================================================
// EMOTION TRANSITION PROBABILITY MATRIX
// ============================================================================
// Rows = current state, Columns = next state
// Values represent base probability (0-100) of transitioning
const uint8_t PROGMEM emotionTransitionMatrix[10][10] = {
    // HAPPY transitions
    {30, 20, 5, 25, 2, 8, 3, 15, 2, 10},
    // EXCITED transitions
    {25, 20, 8, 30, 1, 10, 5, 5, 1, 5},
    // SLEEPY transitions
    {5, 2, 40, 1, 3, 2, 15, 30, 10, 2},
    // PLAYFUL transitions
    {20, 35, 3, 25, 1, 12, 8, 5, 1, 5},
    // LONELY transitions
    {15, 5, 10, 3, 30, 5, 10, 8, 15, 20},
    // CURIOUS transitions
    {10, 15, 5, 20, 3, 25, 20, 8, 5, 4},
    // SURPRISED transitions
    {20, 25, 5, 15, 5, 15, 10, 10, 2, 3},
    // CONTENT transitions
    {25, 5, 15, 10, 5, 8, 3, 30, 8, 10},
    // BORED transitions
    {10, 8, 20, 15, 20, 18, 5, 8, 20, 1},
    // AFFECTIONATE transitions
    {30, 5, 8, 8, 2, 5, 3, 25, 3, 25}
};

// ============================================================================
// FUNCTION DECLARATIONS
// ============================================================================
void handleTouchPattern(TouchPattern pattern, uint8_t location);
void respondToTouch(TouchPattern pattern, uint8_t location);
void handleMotionEvent(MotionType motion);
void transitionToEmotion(EmotionState newEmotion);
void playVibrationPattern(VibrationPattern pattern);
void adaptPersonality(TouchPattern pattern, uint8_t location);
void updateFavoriteLocation();
void applyCircadianRhythm();
void loadOrInitializeData();
void initializeDefaultPersonality();
void initializeHistory();
void savePersonalityData();
void loadPersonalityData();
void saveHistoryData();
void loadHistoryData();
void printPersonality();

// Face drawing functions
void drawHappyFace();
void drawExcitedFace();
void drawSleepyFace();
void drawPlayfulFace();
void drawLonelyFace();
void drawCuriousFace();
void drawSurprisedFace();
void drawContentFace();
void drawBoredFace();
void drawAffectionateFace();
void drawStatusBar();
void drawArc(int16_t x0, int16_t y0, int16_t r, int16_t startAngle, int16_t endAngle, uint16_t color);

// ============================================================================
// SETUP FUNCTION
// ============================================================================
void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println(F("\n========================================"));
    Serial.println(F("Desktop Pet Robot - Emotional AI System"));
    Serial.println(F("========================================"));

    // Initialize EEPROM
    EEPROM.begin(EEPROM_SIZE);

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
    display.println(F("Emotional AI"));
    display.println(F("Booting..."));
    display.display();

    // Initialize Motor
    pinMode(MOTOR_PIN, OUTPUT);
    digitalWrite(MOTOR_PIN, LOW);
    delay(100);

    // Initialize MPU6050
    Serial.print(F("Initializing MPU6050... "));
    mpu.initialize();
    delay(100);

    if (mpu.testConnection()) {
        Serial.println(F("OK"));
        display.println(F("Motion: OK"));
    } else {
        Serial.println(F("FAILED"));
        display.println(F("Motion: FAIL"));
    }
    display.display();

    // Initialize Touch Sensors
    pinMode(TOUCH1_PIN, INPUT);
    pinMode(TOUCH2_PIN, INPUT);
    pinMode(TOUCH3_PIN, INPUT);
    pinMode(TOUCH4_PIN, INPUT);
    Serial.println(F("Touch Sensors: OK"));
    display.println(F("Touch: OK"));
    display.display();

    // Load or Initialize Personality & History
    loadOrInitializeData();

    // Welcome Vibration
    playVibrationPattern(VIB_GENTLE_PULSE);

    delay(2000);

    // Initialize State
    systemStartTime = millis();
    emotionStartTime = millis();
    currentEmotion = CONTENT;

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

    // Update display and feedback (30 FPS)
    if (currentTime - lastUpdate >= 33) {
        expressEmotion();
        lastUpdate = currentTime;
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
        lastSave = currentTime;
    }

    delay(10);  // Small delay for stability
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
            touchState.tapCount[i] = 0;  // Reset to prevent multiple triggers
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
            if (now - touchState.releaseTime[i] < 500 && !touchState.current[i]) {
                touchState.pattern = TOUCH_DOUBLE;
                handleTouchPattern(TOUCH_DOUBLE, i);
                touchState.tapCount[i] = 0;
                return;
            }
        }

        // Single Tap (released quickly)
        if (!touchState.current[i] && touchState.last[i]) {
            if (pressDuration < 500) {
                // Wait a bit to see if it's a double tap
                delay(150);
                if (touchState.tapCount[i] == 1) {
                    touchState.pattern = TOUCH_SINGLE;
                    handleTouchPattern(TOUCH_SINGLE, i);
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
        case 0: history.headTouches++; break;
        case 1: history.leftTouches++; break;
        case 2: history.rightTouches++; break;
        case 3: history.backTouches++; break;
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
    Serial.print(F("Touch: "));
    Serial.print(pattern);
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
            }
            playVibrationPattern(VIB_RAPID_BURST);
            break;

        case TOUCH_LONG_PRESS:
            // Affectionate bonding
            transitionToEmotion(AFFECTIONATE);
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
            // Surprise or excitement
            if (personality.energy > 5) {
                transitionToEmotion(EXCITED);
            } else {
                transitionToEmotion(SURPRISED);
            }
            playVibrationPattern(VIB_RAPID_BURST);
            Serial.println(F("Motion: SHAKE detected"));
            break;

        case MOTION_TILT:
            // Curiosity
            if (random(100) < 40) {
                transitionToEmotion(CURIOUS);
            }
            Serial.println(F("Motion: TILT detected"));
            break;

        case MOTION_ROTATION:
            // Dizzy/surprised
            transitionToEmotion(SURPRISED);
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
    // Get transition probabilities for current emotion
    uint16_t probabilities[10];
    uint16_t totalProb = 0;

    for (int i = 0; i < 10; i++) {
        // Read base probability from PROGMEM
        uint8_t baseProb = pgm_read_byte(&emotionTransitionMatrix[currentEmotion][i]);

        // Apply personality modifiers
        probabilities[i] = applyPersonalityModifier((EmotionState)i, baseProb);
        totalProb += probabilities[i];
    }

    // Weighted random selection
    uint16_t randValue = random(totalProb);
    uint16_t cumulative = 0;

    for (int i = 0; i < 10; i++) {
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
            modified += (personality.affection - 5) * 3;
            break;
        case CURIOUS:
            modified += (personality.curiosity - 5) * 3;
            break;
        case SLEEPY:
            modified += (10 - personality.energy) * 2;
            break;
        case EXCITED:
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

    // Reset animation
    animationFrame = 0;

    Serial.print(F("Emotion: "));
    Serial.print(emotionNames[previousEmotion]);
    Serial.print(F(" -> "));
    Serial.println(emotionNames[currentEmotion]);
}

// ============================================================================
// EXPRESSION OUTPUT SYSTEM
// ============================================================================
void expressEmotion() {
    display.clearDisplay();

    // Draw emotion-specific animation
    switch(currentEmotion) {
        case HAPPY:
            drawHappyFace();
            break;
        case EXCITED:
            drawExcitedFace();
            break;
        case SLEEPY:
            drawSleepyFace();
            break;
        case PLAYFUL:
            drawPlayfulFace();
            break;
        case LONELY:
            drawLonelyFace();
            break;
        case CURIOUS:
            drawCuriousFace();
            break;
        case SURPRISED:
            drawSurprisedFace();
            break;
        case CONTENT:
            drawContentFace();
            break;
        case BORED:
            drawBoredFace();
            break;
        case AFFECTIONATE:
            drawAffectionateFace();
            break;
    }

    // Draw status bar
    drawStatusBar();

    display.display();

    // Update animation frame
    if (millis() - lastAnimationUpdate > 100) {
        animationFrame++;
        lastAnimationUpdate = millis();
    }
}

// ============================================================================
// EMOTION FACE ANIMATIONS
// ============================================================================
void drawHappyFace() {
    // Eyes (open and happy)
    display.fillCircle(45, 28, 8, SH110X_WHITE);
    display.fillCircle(83, 28, 8, SH110X_WHITE);
    display.fillCircle(45, 28, 5, SH110X_BLACK);
    display.fillCircle(83, 28, 5, SH110X_BLACK);

    // Smiling mouth (arc)
    int bounce = (animationFrame % 20 < 10) ? 0 : 1;
    for (int x = 48; x < 80; x++) {
        int y = 42 + bounce + (int)(6 * sin((x - 48) * 3.14159 / 32));
        display.drawPixel(x, y, SH110X_WHITE);
        display.drawPixel(x, y + 1, SH110X_WHITE);
    }

    // Cheeks (optional decorative)
    display.drawCircle(30, 38, 4, SH110X_WHITE);
    display.drawCircle(98, 38, 4, SH110X_WHITE);
}

void drawExcitedFace() {
    // Wide excited eyes
    display.fillCircle(45, 26, 10, SH110X_WHITE);
    display.fillCircle(83, 26, 10, SH110X_WHITE);
    display.fillCircle(45, 26, 6, SH110X_BLACK);
    display.fillCircle(83, 26, 6, SH110X_BLACK);

    // Pupils looking around (animated)
    int offsetX = (animationFrame % 40 < 20) ? -2 : 2;
    display.fillCircle(45 + offsetX, 26, 3, SH110X_WHITE);
    display.fillCircle(83 + offsetX, 26, 3, SH110X_WHITE);

    // Open mouth (excited O shape)
    display.drawCircle(64, 44, 8, SH110X_WHITE);
    display.drawCircle(64, 44, 7, SH110X_WHITE);

    // Sparkles (excitement indicators)
    if (animationFrame % 10 < 5) {
        display.drawPixel(25, 18, SH110X_WHITE);
        display.drawPixel(103, 22, SH110X_WHITE);
    }
}

void drawSleepyFace() {
    // Closed/half-closed eyes
    int eyeHeight = 2 + abs((animationFrame % 60) - 30) / 10;  // Slow blink
    display.fillRect(38, 28, 14, eyeHeight, SH110X_WHITE);
    display.fillRect(76, 28, 14, eyeHeight, SH110X_WHITE);

    // Small curved mouth (peaceful)
    display.drawLine(50, 44, 78, 44, SH110X_WHITE);

    // Sleeping Z's (animated)
    if (animationFrame % 40 < 20) {
        display.setTextSize(1);
        display.setCursor(95, 15);
        display.print(F("z"));
        display.setCursor(100, 10);
        display.print(F("Z"));
    }
}

void drawPlayfulFace() {
    // One eye winking (alternating)
    bool wink = (animationFrame % 30) < 15;

    if (wink) {
        display.fillCircle(45, 28, 8, SH110X_WHITE);
        display.fillCircle(45, 28, 5, SH110X_BLACK);
        display.fillRect(76, 28, 14, 3, SH110X_WHITE);  // Wink
    } else {
        display.fillRect(38, 28, 14, 3, SH110X_WHITE);  // Wink
        display.fillCircle(83, 28, 8, SH110X_WHITE);
        display.fillCircle(83, 28, 5, SH110X_BLACK);
    }

    // Playful sideways smile
    display.drawLine(50, 42, 70, 44, SH110X_WHITE);
    display.drawLine(70, 44, 78, 42, SH110X_WHITE);

    // Tongue (playful)
    display.fillRect(62, 45, 4, 3, SH110X_WHITE);
}

void drawLonelyFace() {
    // Sad eyes
    display.fillCircle(45, 30, 7, SH110X_WHITE);
    display.fillCircle(83, 30, 7, SH110X_WHITE);
    display.fillCircle(45, 30, 4, SH110X_BLACK);
    display.fillCircle(83, 30, 4, SH110X_BLACK);

    // Teardrops (animated)
    int tearY = 38 + (animationFrame % 20);
    if (animationFrame % 40 < 20) {
        display.fillCircle(45, tearY, 2, SH110X_WHITE);
    }

    // Sad mouth (inverted arc)
    for (int x = 50; x < 78; x++) {
        int y = 48 - (int)(4 * sin((x - 50) * 3.14159 / 28));
        display.drawPixel(x, y, SH110X_WHITE);
    }
}

void drawCuriousFace() {
    // One eyebrow raised, asymmetric eyes
    display.fillCircle(45, 28, 7, SH110X_WHITE);
    display.fillCircle(45, 28, 4, SH110X_BLACK);

    display.fillCircle(83, 26, 9, SH110X_WHITE);
    display.fillCircle(83, 26, 5, SH110X_BLACK);

    // Raised eyebrow
    display.drawLine(75, 18, 90, 16, SH110X_WHITE);
    display.drawLine(75, 19, 90, 17, SH110X_WHITE);

    // Small O mouth (questioning)
    display.drawCircle(64, 44, 4, SH110X_WHITE);

    // Question mark indicator
    if (animationFrame % 20 < 10) {
        display.setTextSize(1);
        display.setCursor(100, 10);
        display.print(F("?"));
    }
}

void drawSurprisedFace() {
    // Wide open eyes
    display.drawCircle(45, 28, 10, SH110X_WHITE);
    display.drawCircle(45, 28, 9, SH110X_WHITE);
    display.fillCircle(45, 28, 6, SH110X_WHITE);

    display.drawCircle(83, 28, 10, SH110X_WHITE);
    display.drawCircle(83, 28, 9, SH110X_WHITE);
    display.fillCircle(83, 28, 6, SH110X_WHITE);

    // Large O mouth
    display.drawCircle(64, 46, 9, SH110X_WHITE);
    display.drawCircle(64, 46, 8, SH110X_WHITE);

    // Exclamation marks
    if (animationFrame % 10 < 5) {
        display.drawLine(20, 15, 20, 25, SH110X_WHITE);
        display.drawPixel(20, 28, SH110X_WHITE);
        display.drawLine(108, 15, 108, 25, SH110X_WHITE);
        display.drawPixel(108, 28, SH110X_WHITE);
    }
}

void drawContentFace() {
    // Peaceful closed eyes (gentle curves)
    display.drawArc(45, 28, 8, 8, 0, 180, SH110X_WHITE);
    display.drawArc(83, 28, 8, 8, 0, 180, SH110X_WHITE);

    // Gentle smile
    for (int x = 50; x < 78; x++) {
        int y = 42 + (int)(3 * sin((x - 50) * 3.14159 / 28));
        display.drawPixel(x, y, SH110X_WHITE);
    }

    // Breathing animation (gentle pulse)
    int pulseSize = 60 + (animationFrame % 40) / 4;
    display.drawCircle(64, 32, pulseSize / 2, SH110X_WHITE);
}

void drawBoredFace() {
    // Half-lidded unamused eyes
    display.drawRect(38, 28, 14, 6, SH110X_WHITE);
    display.fillRect(38, 32, 14, 2, SH110X_BLACK);
    display.fillCircle(45, 30, 3, SH110X_WHITE);

    display.drawRect(76, 28, 14, 6, SH110X_WHITE);
    display.fillRect(76, 32, 14, 2, SH110X_BLACK);
    display.fillCircle(83, 30, 3, SH110X_WHITE);

    // Straight line mouth
    display.drawLine(50, 44, 78, 44, SH110X_WHITE);

    // Ellipsis (...)
    if (animationFrame % 60 < 20) {
        display.setTextSize(1);
        display.setCursor(92, 40);
        display.print(F("."));
    } else if (animationFrame % 60 < 40) {
        display.setTextSize(1);
        display.setCursor(92, 40);
        display.print(F(".."));
    } else {
        display.setTextSize(1);
        display.setCursor(92, 40);
        display.print(F("..."));
    }
}

void drawAffectionateFace() {
    // Heart-shaped eyes
    // Left heart
    display.fillCircle(40, 26, 4, SH110X_WHITE);
    display.fillCircle(50, 26, 4, SH110X_WHITE);
    display.fillTriangle(36, 28, 54, 28, 45, 36, SH110X_WHITE);

    // Right heart
    display.fillCircle(78, 26, 4, SH110X_WHITE);
    display.fillCircle(88, 26, 4, SH110X_WHITE);
    display.fillTriangle(74, 28, 92, 28, 83, 36, SH110X_WHITE);

    // Big smile
    for (int x = 48; x < 80; x++) {
        int y = 42 + (int)(8 * sin((x - 48) * 3.14159 / 32));
        display.drawPixel(x, y, SH110X_WHITE);
        display.drawPixel(x, y + 1, SH110X_WHITE);
    }

    // Floating hearts (animated)
    int heartY = 10 - (animationFrame % 15);
    if (animationFrame % 30 < 15) {
        display.drawPixel(20, heartY + 2, SH110X_WHITE);
        display.drawPixel(22, heartY + 2, SH110X_WHITE);
        display.drawPixel(21, heartY + 3, SH110X_WHITE);

        display.drawPixel(106, heartY + 5, SH110X_WHITE);
        display.drawPixel(108, heartY + 5, SH110X_WHITE);
        display.drawPixel(107, heartY + 6, SH110X_WHITE);
    }
}

void drawStatusBar() {
    // Draw thin status bar at bottom
    display.drawLine(0, 56, SCREEN_WIDTH, 56, SH110X_WHITE);

    // Bond level indicator
    int bondBarWidth = (history.bondLevel * (SCREEN_WIDTH - 4)) / 100;
    display.drawRect(2, 58, SCREEN_WIDTH - 4, 4, SH110X_WHITE);
    display.fillRect(3, 59, bondBarWidth, 2, SH110X_WHITE);
}

// Helper function for drawing arcs (if not available in library)
void drawArc(int16_t x0, int16_t y0, int16_t r, int16_t startAngle, int16_t endAngle, uint16_t color) {
    for (int angle = startAngle; angle < endAngle; angle += 5) {
        float rad = angle * 3.14159 / 180.0;
        int x = x0 + r * cos(rad);
        int y = y0 + r * sin(rad);
        display.drawPixel(x, y, color);
    }
}

// ============================================================================
// VIBRATION PATTERN SYSTEM
// ============================================================================
void playVibrationPattern(VibrationPattern pattern) {
    switch(pattern) {
        case VIB_GENTLE_PULSE:
            // 200ms on, 500ms off
            digitalWrite(MOTOR_PIN, HIGH);
            delay(200);
            digitalWrite(MOTOR_PIN, LOW);
            break;

        case VIB_RAPID_BURST:
            // 3x quick pulses
            for (int i = 0; i < 3; i++) {
                digitalWrite(MOTOR_PIN, HIGH);
                delay(100);
                digitalWrite(MOTOR_PIN, LOW);
                delay(100);
            }
            break;

        case VIB_LONG_SUSTAINED:
            // 1 second sustained
            digitalWrite(MOTOR_PIN, HIGH);
            delay(1000);
            digitalWrite(MOTOR_PIN, LOW);
            break;

        case VIB_WAVE_PATTERN:
            // Increasing intensity using PWM
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
            // 5x very quick taps
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
    uint16_t maxTouches = history.headTouches;
    history.favoriteLocation = 0;

    if (history.leftTouches > maxTouches) {
        maxTouches = history.leftTouches;
        history.favoriteLocation = 1;
    }
    if (history.rightTouches > maxTouches) {
        maxTouches = history.rightTouches;
        history.favoriteLocation = 2;
    }
    if (history.backTouches > maxTouches) {
        maxTouches = history.backTouches;
        history.favoriteLocation = 3;
    }
}

// ============================================================================
// CIRCADIAN RHYTHM SYSTEM
// ============================================================================
void applyCircadianRhythm() {
    // Simulate time-based energy levels
    // Based on system uptime (could use RTC for real time)
    unsigned long uptime = millis() - systemStartTime;
    unsigned long hours = (uptime / 3600000) % 24;  // Simulated hours

    // Adjust personality.energy based on "time of day"
    if (hours >= 22 || hours <= 6) {  // Night time
        if (personality.energy > 3 && random(100) < 10) {
            personality.energy--;
        }
        // More likely to be sleepy
        if (currentEmotion != SLEEPY && random(100) < 30) {
            transitionToEmotion(SLEEPY);
        }
    } else if (hours >= 6 && hours <= 10) {  // Morning
        if (personality.energy < 7 && random(100) < 10) {
            personality.energy++;
        }
    } else if (hours >= 10 && hours <= 18) {  // Daytime
        // Peak activity - energy stays moderate to high
        if (personality.energy < 6 && random(100) < 5) {
            personality.energy++;
        }
    }
}

// ============================================================================
// DATA PERSISTENCE (EEPROM)
// ============================================================================
void loadOrInitializeData() {
    // Check initialization flag
    uint8_t initFlag = EEPROM.read(EEPROM_INIT_FLAG_ADDR);

    if (initFlag == 0xAA) {
        // Data exists, load it
        loadPersonalityData();
        loadHistoryData();
        Serial.println(F("Loaded existing personality & history"));
    } else {
        // First boot, initialize with defaults
        initializeDefaultPersonality();
        initializeHistory();
        savePersonalityData();
        saveHistoryData();
        EEPROM.write(EEPROM_INIT_FLAG_ADDR, 0xAA);
        EEPROM.commit();
        Serial.println(F("Initialized new personality & history"));
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
    history.headTouches = 0;
    history.leftTouches = 0;
    history.rightTouches = 0;
    history.backTouches = 0;
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

    // Save bond level and favorite location
    EEPROM.write(addr++, history.bondLevel);
    EEPROM.write(addr++, history.favoriteLocation);

    // Save touch counts (16-bit values)
    EEPROM.write(addr++, history.headTouches >> 8);
    EEPROM.write(addr++, history.headTouches & 0xFF);
    EEPROM.write(addr++, history.leftTouches >> 8);
    EEPROM.write(addr++, history.leftTouches & 0xFF);
    EEPROM.write(addr++, history.rightTouches >> 8);
    EEPROM.write(addr++, history.rightTouches & 0xFF);
    EEPROM.write(addr++, history.backTouches >> 8);
    EEPROM.write(addr++, history.backTouches & 0xFF);

    EEPROM.commit();
}

void loadHistoryData() {
    int addr = EEPROM_HISTORY_ADDR;

    history.bondLevel = EEPROM.read(addr++);
    history.favoriteLocation = EEPROM.read(addr++);

    history.headTouches = (EEPROM.read(addr++) << 8) | EEPROM.read(addr++);
    history.leftTouches = (EEPROM.read(addr++) << 8) | EEPROM.read(addr++);
    history.rightTouches = (EEPROM.read(addr++) << 8) | EEPROM.read(addr++);
    history.backTouches = (EEPROM.read(addr++) << 8) | EEPROM.read(addr++);

    history.lastInteraction = millis();
    history.totalInteractions = history.headTouches + history.leftTouches +
                                history.rightTouches + history.backTouches;
}

// ============================================================================
// DEBUG & UTILITY FUNCTIONS
// ============================================================================
void printPersonality() {
    Serial.print(F("  Playfulness: ")); Serial.println(personality.playfulness);
    Serial.print(F("  Affection: ")); Serial.println(personality.affection);
    Serial.print(F("  Curiosity: ")); Serial.println(personality.curiosity);
    Serial.print(F("  Energy: ")); Serial.println(personality.energy);
    Serial.print(F("  Sociability: ")); Serial.println(personality.sociability);
    Serial.print(F("  Bond Level: ")); Serial.println(history.bondLevel);
}
