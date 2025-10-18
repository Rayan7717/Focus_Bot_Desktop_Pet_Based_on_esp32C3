# Desktop Pet Robot - Comprehensive Logic Guide

## Table of Contents
1. [System Overview](#system-overview)
2. [Core Architecture](#core-architecture)
3. [Emotion State System](#emotion-state-system)
4. [Input Processing](#input-processing)
5. [Animation System](#animation-system)
6. [Personality & Adaptation](#personality--adaptation)
7. [Data Flow](#data-flow)
8. [Timing & State Management](#timing--state-management)

---

## System Overview

This is an **Emotional AI Desktop Pet** running on an ESP32-C3 microcontroller that creates an interactive companion with adaptive personality traits. The pet responds to touch, movement, and time, displaying emotions through animated expressions on an OLED screen and haptic feedback via a vibration motor.

### Key Components
- **ESP32-C3 Supermini**: Main controller
- **SH1106 OLED Display (128x64)**: Visual feedback with animations
- **MPU6050 IMU**: Detects motion (tilt, shake, rotation)
- **4x TTP223 Touch Sensors**: Front/back left/right touch detection
- **Vibration Motor**: Haptic feedback patterns
- **LittleFS Storage**: Compressed animation files

---

## Core Architecture

### Main Program Flow

```
setup()
  ├─ Hardware Initialization
  │   ├─ I2C Bus (Display + IMU)
  │   ├─ Touch Sensors (4 GPIO pins)
  │   ├─ Vibration Motor (PWM capable)
  │   └─ LittleFS Filesystem
  │
  ├─ Data Loading
  │   ├─ Animation Metadata (20 emotions)
  │   ├─ Personality Traits (EEPROM)
  │   └─ Interaction History (EEPROM)
  │
  └─ Initial State
      ├─ Default Emotion: CONTENT
      └─ Load first animation

loop()  [runs ~100 Hz]
  ├─ Sensor Updates
  │   ├─ updateTouchSensors() → Read 4 touch pins
  │   └─ updateMotionSensors() → Read MPU6050 accel/gyro
  │
  ├─ Pattern Recognition
  │   ├─ processTouchPatterns() → Detect tap types
  │   └─ processMotionEvents() → Classify motion
  │
  ├─ Emotion Update (every 500ms)
  │   └─ updateEmotionState()
  │       ├─ Check emotion duration timeout
  │       ├─ Loneliness check (5 min no interaction)
  │       └─ Boredom check (120s same state)
  │
  ├─ Animation Rendering (FPS dependent)
  │   └─ displayCurrentFrame()
  │       ├─ Load compressed frame from LittleFS
  │       ├─ RLE decompress
  │       ├─ Draw to display buffer
  │       └─ Overlay bond level indicator
  │
  └─ Background Tasks
      ├─ Circadian rhythm (every 10s)
      └─ Auto-save personality (every 5 min)
```

---

## Emotion State System

### 20 Emotion States

The system supports 20 distinct emotions, each mapped to a unique animation:

| Emotion | ID | Base Weight | Typical Trigger |
|---------|-----|-------------|----------------|
| HAPPY | 0 | 25 | Single touch, positive interaction |
| EXCITED | 1 | 15 | Double tap, rapid motion |
| SLEEPY | 2 | 10 | Prolonged stillness (60s) |
| PLAYFUL | 3 | 20 | High playfulness trait |
| LONELY | 4 | 5 | 5 min no interaction |
| CURIOUS | 5 | 15 | Tilt detection |
| SURPRISED | 6 | 8 | Multi-sensor touch, shake |
| CONTENT | 7 | 30 | Default/neutral state |
| BORED | 8 | 8 | Same state too long |
| AFFECTIONATE | 9 | 12 | Long press touch |
| ANGRY | 10 | 3 | Harsh shake (low energy) |
| CONFUSED | 11 | 7 | Rotation, tilt |
| DETERMINED | 12 | 10 | - |
| EMBARRASSED | 13 | 5 | - |
| FRUSTRATED | 14 | 4 | - |
| LAUGH | 15 | 15 | Double tap variation |
| LOVE | 16 | 12 | Long press variation |
| MUSIC | 17 | 10 | - |
| PROUD | 18 | 10 | - |
| RELAXED | 19 | 15 | High stillness duration |

### Emotion Transition Logic

**[EmotionalAI_DesktopPet_Comprehensive.ino:1022-1041](EmotionalAI_DesktopPet_Comprehensive.ino#L1022-L1041)**

```cpp
void updateEmotionState() {
    // 1. Duration-based transition (15-60 seconds per emotion)
    if (now - emotionStartTime >= emotionDuration) {
        nextEmotion = selectNextEmotion();  // Weighted random
        transitionToEmotion(nextEmotion);
    }

    // 2. Context-triggered transitions
    if (noInteractionFor5Min && notLonely) {
        if (random(100) < (10 - sociability) * 10) {
            transitionToEmotion(LONELY);
        }
    }

    // 3. Boredom from stagnation
    if (sameEmotionFor2Min && noRecentInteraction) {
        if (random(100) < 30) {
            transitionToEmotion(BORED);
        }
    }
}
```

### Weighted Emotion Selection

**[EmotionalAI_DesktopPet_Comprehensive.ino:1043-1066](EmotionalAI_DesktopPet_Comprehensive.ino#L1043-L1066)**

The next emotion is chosen using a **personality-modified weighted random selection**:

1. Start with base weights from `baseEmotionWeights[]`
2. Apply personality modifiers:
   - **PLAYFUL**: +3 per point of playfulness above 5
   - **AFFECTIONATE/LOVE**: +3 per point of affection above 5
   - **CURIOUS**: +3 per point of curiosity above 5
   - **SLEEPY/RELAXED**: +2 per point of low energy (10-energy)
   - **EXCITED/LAUGH**: +2 per point of high energy
   - **LONELY**: +2 per point of low sociability
3. Generate random number within total probability
4. Select emotion using cumulative probability distribution

**Example**: If playfulness=8 and energy=7:
- PLAYFUL: 20 + (8-5)*3 = **29** (boosted)
- EXCITED: 15 + (7-5)*2 = **19** (boosted)
- SLEEPY: 10 + (10-7)*2 = **16** (reduced)

---

## Input Processing

### Touch Pattern Recognition

**[EmotionalAI_DesktopPet_Comprehensive.ino:803-868](EmotionalAI_DesktopPet_Comprehensive.ino#L803-L868)**

The system detects **5 touch patterns** from 4 sensors:

| Pattern | Detection Logic | Response |
|---------|----------------|----------|
| **SINGLE** | Press < 500ms, 1 tap, released | HAPPY (40%), gentle pulse |
| **DOUBLE** | 2 taps < 500ms apart | EXCITED/LAUGH (60/40%), rapid burst |
| **LONG_PRESS** | Held > 3000ms | AFFECTIONATE/LOVE (70/30%), wave pattern |
| **RAPID_TAPS** | 5 taps in 2 seconds | EXCITED/SURPRISED, rapid burst |
| **MULTI_SENSOR** | 2+ sensors active simultaneously | SURPRISED, wave pattern |

**Touch State Tracking** (per sensor):
```cpp
struct TouchState {
    bool current[4];           // Current reading
    bool last[4];              // Previous reading (edge detection)
    unsigned long pressTime[4];    // When pressed
    unsigned long releaseTime[4];  // When released
    uint8_t tapCount[4];          // Tap counter (reset after 2s)
}
```

**Processing Flow**:
1. **Edge Detection**: Compare current vs last state for press/release events
2. **Timing Analysis**: Calculate press duration = now - pressTime
3. **Pattern Matching**: Check conditions in priority order:
   - Long press (highest priority)
   - Rapid taps (5 in 2s)
   - Double tap (2 in 500ms)
   - Multi-sensor (parallel)
   - Single tap (default)

### Motion Detection

**[EmotionalAI_DesktopPet_Comprehensive.ino:754-798](EmotionalAI_DesktopPet_Comprehensive.ino#L754-L798)**

The MPU6050 provides 6-axis data (ax, ay, az, gx, gy, gz) processed into motion types:

| Motion Type | Detection Threshold | Emotion Response |
|-------------|-------------------|-----------------|
| **SHAKE** | accelMagnitude > 2.0g | EXCITED (high energy) / ANGRY (low energy) / SURPRISED |
| **ROTATION** | gyroMagnitude > 100°/s | CONFUSED |
| **TILT** | abs(accelZ - 1.0) > 0.3g | CURIOUS (50%) / CONFUSED (50%) |
| **STILL** | gyro < 5°/s && accel < 1.2g | (triggers SLEEPY after 60s) |

**Calculation Example**:
```cpp
accelMagnitude = sqrt(ax² + ay² + az²)
gyroMagnitude = sqrt(gx² + gy² + gz²)
```

---

## Animation System

### File Format (Binary .anim)

**[EmotionalAI_DesktopPet_Comprehensive.ino:616-636](EmotionalAI_DesktopPet_Comprehensive.ino#L616-L636)**

Each animation is stored as a binary file with this structure:

```
┌─────────────────────────────────────────┐
│ HEADER (12 bytes)                       │
│  - frameCount    (2 bytes, uint16)      │
│  - fps           (2 bytes, uint16)      │
│  - maxCompressedSize (2 bytes, uint16)  │
│  - unused        (6 bytes)              │
├─────────────────────────────────────────┤
│ FRAME SIZES ARRAY                       │
│  - size[0]       (2 bytes)              │
│  - size[1]       (2 bytes)              │
│  - ...                                  │
│  - size[N-1]     (2 bytes)              │
├─────────────────────────────────────────┤
│ COMPRESSED FRAME DATA                   │
│  - Frame 0 (RLE compressed)             │
│  - Frame 1 (RLE compressed)             │
│  - ...                                  │
│  - Frame N-1                            │
└─────────────────────────────────────────┘
```

### RLE Compression

**[EmotionalAI_DesktopPet_Comprehensive.ino:686-695](EmotionalAI_DesktopPet_Comprehensive.ino#L686-L695)**

Frames use **Run-Length Encoding** to compress the 1024-byte bitmap:

```
Format: [count, value] pairs
Example: [5, 0xFF] [3, 0x00] [2, 0xFF]
Decompresses to: 0xFF 0xFF 0xFF 0xFF 0xFF 0x00 0x00 0x00 0xFF 0xFF
```

### Animation Playback Pipeline

**[EmotionalAI_DesktopPet_Comprehensive.ino:638-684](EmotionalAI_DesktopPet_Comprehensive.ino#L638-L684)**

```
loadAnimation(emotion)
  ├─ Open .anim file from LittleFS
  ├─ Seek to offset 12 (skip header)
  ├─ Read frameSizes[] array into RAM
  └─ Keep file handle open

displayCurrentFrame()  [called at FPS rate]
  ├─ Calculate frame offset:
  │   frameOffset = 12 + (frameCount*2) + (currentFrame * maxCompressedSize)
  ├─ Seek to frame position
  ├─ Read compressed data → compressedBuffer
  ├─ Decompress RLE → frameBuffer (1024 bytes)
  ├─ Render frameBuffer to display:
  │   for each pixel (x,y):
  │     byteIndex = (y * 128 + x) / 8
  │     bitIndex = 7 - (x % 8)
  │     if (frameBuffer[byteIndex] & (1 << bitIndex))
  │       drawPixel(x, y, WHITE)
  ├─ Draw bond level overlay (bottom bar)
  └─ Display to screen

currentFrame++
if (currentFrame >= frameCount)
    currentFrame = 0  // Loop animation
```

**Memory Efficiency**:
- Only **one frame** is decompressed at a time
- File streaming reduces RAM usage
- Typical compression: 1024 bytes → 200-400 bytes

---

## Personality & Adaptation

### Personality Traits

**[EmotionalAI_DesktopPet_Comprehensive.ino:147-153](EmotionalAI_DesktopPet_Comprehensive.ino#L147-L153)**

Five traits (range 0-10) that evolve based on user interaction:

```cpp
struct PersonalityTraits {
    uint8_t playfulness;   // Affects PLAYFUL emotion probability
    uint8_t affection;     // Affects response to touch
    uint8_t curiosity;     // Affects exploration behaviors
    uint8_t energy;        // Affects sleep/activity balance
    uint8_t sociability;   // Affects loneliness threshold
}
```

**Default Values**: All traits start at 5 (neutral), except energy=7

### Adaptation Mechanisms

**[EmotionalAI_DesktopPet_Comprehensive.ino:1175-1202](EmotionalAI_DesktopPet_Comprehensive.ino#L1175-L1202)**

The personality **gradually evolves** based on usage patterns:

| User Behavior | Trait Change | Probability |
|--------------|--------------|-------------|
| Rapid tap patterns | playfulness++ | 20% per event |
| Long press touches | affection++ | 30% per event |
| Frequent interactions | sociability++ | Every 50 interactions |
| Playing while PLAYFUL | energy++ | 10% per occurrence |
| Motion exploration | curiosity++ | 5% per motion event |

**Example Evolution**:
```
User frequently pets with long presses (20 times)
→ affection increases from 5 to 7-8
→ AFFECTIONATE/LOVE states become 6-9 points more likely
→ Pet shows more affectionate behavior
```

### Interaction History

**[EmotionalAI_DesktopPet_Comprehensive.ino:158-167](EmotionalAI_DesktopPet_Comprehensive.ino#L158-L167)**

```cpp
struct InteractionHistory {
    unsigned long lastInteraction;     // Timestamp
    unsigned long totalInteractions;   // Lifetime count
    uint16_t leftFrontTouches;        // Per-sensor counters
    uint16_t leftBackTouches;
    uint16_t rightFrontTouches;
    uint16_t rightBackTouches;
    uint8_t favoriteLocation;         // 0-3 (most touched sensor)
    uint8_t bondLevel;                // 0-100 (increases with interaction)
}
```

**Bond Level Progression**:
- Each touch pattern increments bondLevel by 1 (max 100)
- Displayed as progress bar on screen (30 pixels wide)
- Higher bond level doesn't change behavior yet (future expansion point)

### Circadian Rhythm

**[EmotionalAI_DesktopPet_Comprehensive.ino:1225-1243](EmotionalAI_DesktopPet_Comprehensive.ino#L1225-L1243)**

Simulates day/night cycle based on uptime:

```cpp
hours = (uptime / 3600000) % 24

Night (22:00-06:00):
  - Energy decreases (10% chance per check)
  - SLEEPY emotion more likely (30% chance)

Morning (06:00-10:00):
  - Energy increases (10% chance per check)
```

---

## Data Flow

### Complete Interaction Sequence

```
User touches front-left sensor
  │
  ├─ updateTouchSensors()
  │   ├─ Read GPIO pins → touchState.current[]
  │   ├─ Detect rising edge (pressed)
  │   │   └─ Set pressTime[0] = millis()
  │   └─ tapCount[0]++
  │
  ├─ processTouchPatterns()
  │   ├─ Analyze press duration
  │   ├─ Detect TOUCH_SINGLE pattern
  │   └─ Call handleTouchPattern(SINGLE, 0)
  │
  ├─ handleTouchPattern()
  │   ├─ Update history:
  │   │   ├─ lastInteraction = now
  │   │   ├─ totalInteractions++
  │   │   ├─ leftFrontTouches++
  │   │   └─ bondLevel++ (if < 100)
  │   ├─ updateFavoriteLocation()
  │   ├─ adaptPersonality()
  │   └─ respondToTouch()
  │
  ├─ respondToTouch(TOUCH_SINGLE, 0)
  │   ├─ 40% chance → transitionToEmotion(HAPPY)
  │   └─ playVibrationPattern(VIB_SHORT_TAPS)
  │
  ├─ transitionToEmotion(HAPPY)
  │   ├─ currentEmotion = HAPPY
  │   ├─ emotionDuration = random(15-60s)
  │   └─ loadAnimation(HAPPY)
  │
  ├─ loadAnimation(HAPPY)
  │   ├─ Open /happy.anim from LittleFS
  │   └─ Load frame metadata
  │
  └─ [Next frame cycle]
      └─ displayCurrentFrame()
          ├─ Read compressed frame
          ├─ RLE decompress
          ├─ Render to OLED
          └─ currentFrame++
```

### Persistence (EEPROM)

**Saved Data** (auto-save every 5 minutes):

```
Address 0-49: Personality Traits
  0: playfulness
  1: affection
  2: curiosity
  3: energy
  4: sociability

Address 50-99: Interaction History
  50: bondLevel
  51: favoriteLocation
  52-53: leftFrontTouches (uint16)
  54-55: leftBackTouches
  56-57: rightFrontTouches
  58-59: rightBackTouches

Address 500: Initialization Flag (0xAA)
```

---

## Timing & State Management

### Update Frequencies

| System | Rate | Implementation |
|--------|------|----------------|
| Main loop | ~100 Hz | `delay(10)` at loop end |
| Sensor reading | Every loop | `updateTouchSensors()` + `updateMotionSensors()` |
| Emotion update | 2 Hz (500ms) | `if (now - lastEmotionUpdate >= 500)` |
| Animation frame | Variable FPS | Based on animation metadata (10-30 FPS typical) |
| Circadian check | 0.1 Hz (10s) | Static timer variable |
| Auto-save | 0.0033 Hz (5 min) | Static timer variable |

### Critical Timing Sequences

**Touch Pattern Detection Windows**:
```cpp
SINGLE:       Press < 500ms
DOUBLE:       2 taps within 500ms
LONG_PRESS:   Press > 3000ms
RAPID_TAPS:   5 taps within 2000ms
TAP_RESET:    2000ms timeout between tap sequences
```

**Emotion Duration**:
- Random range: 15,000 - 60,000 ms (15-60 seconds)
- Set on each transition with `emotionDuration = random(15000, 60000)`

**Loneliness Trigger**:
- No interaction for 300,000 ms (5 minutes)
- Probability: `(10 - sociability) * 10%`
  - High sociability (9): 10% chance
  - Low sociability (1): 90% chance

**Sleep Trigger**:
- Stillness duration > 60,000 ms (60 seconds)
- 20% chance to transition to SLEEPY

---

## Vibration Feedback Patterns

**[EmotionalAI_DesktopPet_Comprehensive.ino:1122-1170](EmotionalAI_DesktopPet_Comprehensive.ino#L1122-L1170)**

Five distinct haptic patterns synchronized with emotions:

```cpp
VIB_GENTLE_PULSE:      // 200ms burst
  HIGH → 200ms → LOW

VIB_RAPID_BURST:       // 3 quick pulses
  HIGH → 100ms → LOW → 100ms (x3)

VIB_LONG_SUSTAINED:    // 1 second continuous
  HIGH → 1000ms → LOW

VIB_WAVE_PATTERN:      // PWM ramp up/down
  analogWrite: 0→255 (15 steps, 20ms each)
  analogWrite: 255→0 (15 steps, 20ms each)

VIB_SHORT_TAPS:        // 5 quick taps
  HIGH → 50ms → LOW → 100ms (x5)
```

---

## Key Design Patterns

### State Machine Architecture
- **Current State**: `currentEmotion` (20 states)
- **Transition Logic**: Time-based + event-driven
- **State Memory**: `previousEmotion` (for context)

### Event-Driven Input
- **Polling**: Sensors read every loop
- **Edge Detection**: State change detection for touch
- **Pattern Recognition**: Temporal analysis of inputs

### Adaptive Behavior
- **Learning**: Personality traits evolve
- **Memory**: Interaction history persists
- **Prediction**: Weighted random with personality bias

### Resource Management
- **Streaming**: Animations loaded frame-by-frame
- **Compression**: RLE reduces storage by 60-80%
- **Persistence**: EEPROM for critical data only

---

## Summary of Logic Flow

1. **Initialization** → Load personality + animations
2. **Continuous Sensing** → Touch + motion inputs every 10ms
3. **Pattern Recognition** → Classify input into meaningful patterns
4. **Event Response** → Immediate emotion + vibration feedback
5. **State Evolution** → Natural emotion transitions every 15-60s
6. **Personality Adaptation** → Gradual trait changes based on usage
7. **Visual Output** → Animated emotions at native FPS
8. **Persistence** → Save learned behavior every 5 minutes

The system creates an **illusion of life** through:
- **Responsiveness**: Sub-second reaction to touch/motion
- **Unpredictability**: Weighted randomness prevents repetition
- **Memory**: Learns favorite touch locations and interaction styles
- **Autonomy**: Transitions states independently
- **Multi-modal Feedback**: Visual + haptic creates immersive experience

---

## Future Expansion Points

Based on code structure, these systems are designed for extension:

1. **Bond Level Effects**: Currently tracked but not used for behavior
2. **Favorite Location Preferences**: Detected but could influence responses
3. **Additional Emotions**: Easy to add more states (expand enum + animations)
4. **Sound Output**: No audio yet, but vibration system could be adapted
5. **Wireless Communication**: ESP32-C3 has BLE/WiFi capabilities unused
6. **Advanced Personality**: More complex trait interactions possible
