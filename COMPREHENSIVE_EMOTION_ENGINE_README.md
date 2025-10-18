# Comprehensive Emotion Engine for ESP32-C3 Desktop Pet

## Overview

This is a fully integrated emotional AI system that combines:
- **20 Emotion States** with animated facial expressions
- **LittleFS Animation System** with RLE compression
- **Multi-Modal Sensor Integration** (Touch, Motion, Vibration)
- **Adaptive Personality System** that learns user preferences
- **Social Bonding Mechanics** with interaction history
- **Circadian Rhythm** simulation for natural behavior changes

## Hardware Requirements

### Components
- **ESP32-C3 Supermini** - Main controller
- **SH1106 128x64 OLED Display** - Visual expressions (I2C)
- **MPU6050** - 6-axis gyroscope/accelerometer (I2C)
- **4x TTP223 Capacitive Touch Sensors** - User interaction
- **Vibration Motor** - Haptic feedback (via BC547B transistor + 1kΩ resistor)

### Pin Connections

| Component | Pin | Description |
|-----------|-----|-------------|
| I2C SDA | GPIO 8 | Display & MPU6050 data line |
| I2C SCL | GPIO 9 | Display & MPU6050 clock line |
| Touch 1 | GPIO 0 | Front Left touch sensor |
| Touch 2 | GPIO 1 | Back Left touch sensor |
| Touch 3 | GPIO 3 | Front Right touch sensor |
| Touch 4 | GPIO 4 | Back Right touch sensor |
| Vibration Motor | GPIO 6 | Motor control (via transistor) |

### Wiring Diagram - Vibration Motor
```
ESP32-C3 GPIO6 ----[1kΩ]---- BC547B Base
                              BC547B Collector ---- Motor ---- VCC (3.3V or 5V)
                              BC547B Emitter ------ GND
```

## Software Requirements

### Arduino IDE Setup
1. Install **Arduino IDE** (version 1.8.19+ or 2.x)
2. Add ESP32 board support:
   - File → Preferences → Additional Board Manager URLs
   - Add: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
3. Install **ESP32 boards** via Board Manager

### Required Libraries
Install via Arduino Library Manager (Tools → Manage Libraries):
- **Adafruit GFX Library** (by Adafruit)
- **Adafruit SH110X** (by Adafruit)
- **MPU6050** (by Electronic Cats)
- **LittleFS** (built-in with ESP32 core)
- **EEPROM** (built-in with ESP32 core)

### Board Configuration
- **Board**: "ESP32C3 Dev Module"
- **Partition Scheme**: "Default 4MB with spiffs (1.2MB APP/1.5MB SPIFFS)"
- **Flash Mode**: "QIO"
- **Flash Frequency**: "80MHz"
- **Upload Speed**: "921600"

## Installation Steps

### Step 1: Upload Animation Data
1. **Ensure animation files are ready**:
   - The `EmotionalAI_DesktopPet/data/` folder should contain all `.anim` files
   - Files included: happy.anim, excited_2.anim, sleepy.anim, love.anim, etc.

2. **Install ESP32 Filesystem Uploader**:
   - Download the plugin from: [ESP32FS Plugin](https://github.com/me-no-dev/arduino-esp32fs-plugin)
   - Extract to Arduino's `tools` folder
   - Restart Arduino IDE

3. **Upload the filesystem**:
   - Open `EmotionalAI_DesktopPet_Comprehensive.ino`
   - Tools → ESP32 Sketch Data Upload
   - Wait for upload to complete (may take 1-2 minutes)

### Step 2: Upload the Sketch
1. Open `EmotionalAI_DesktopPet_Comprehensive.ino`
2. Select correct board and port
3. Click **Upload**
4. Wait for compilation and upload to complete

### Step 3: First Boot
1. Open Serial Monitor (115200 baud)
2. Watch the initialization sequence:
   - Display initialization
   - MPU6050 sensor check
   - Touch sensor setup
   - LittleFS mounting and file listing
   - Animation metadata loading
   - Personality initialization
3. The pet should start with "Content" emotion

## Features Explanation

### 20 Emotion States

| Emotion | Animation File | Triggered By |
|---------|---------------|--------------|
| Happy | happy.anim | Single touch, positive interactions |
| Excited | excited_2.anim | Double tap, shake motion |
| Sleepy | sleepy.anim | Long stillness, night time |
| Playful | happy_2.anim | Rapid taps, high energy |
| Lonely | content.anim | No interaction for 5+ minutes |
| Curious | confused_2.anim | Tilt motion, exploration |
| Surprised | embarrassed.anim | Shake, multi-touch |
| Content | content.anim | Default calm state |
| Bored | relaxed.anim | Same state too long |
| Affectionate | love.anim | Long press (>3s) |
| Angry | angry.anim | Rough shake |
| Confused | confused_2.anim | Rotation |
| Determined | determined.anim | Random transition |
| Embarrassed | embarrassed.anim | Random transition |
| Frustrated | frustrated.anim | Random transition |
| Laugh | laugh.anim | Double tap variant |
| Love | love.anim | Long press variant |
| Music | music.anim | Random transition |
| Proud | proud.anim | Random transition |
| Relaxed | relaxed.anim | Low energy state |

### Touch Pattern Recognition

The system recognizes 5 distinct touch patterns:

1. **Single Touch** - Quick tap (<500ms)
   - Response: Happy emotion, short vibration

2. **Double Touch** - Two quick taps (<500ms apart)
   - Response: Excited/Laugh emotion, rapid burst vibration

3. **Long Press** - Hold for >3 seconds
   - Response: Affectionate/Love emotion, wave pattern vibration

4. **Rapid Taps** - 5+ taps in 2 seconds
   - Response: Excited or Surprised (based on energy level)

5. **Multi-Touch** - 2+ sensors pressed simultaneously
   - Response: Surprised emotion, wave pattern vibration

### Motion Detection

The MPU6050 detects 4 motion types:

1. **Shake** - High acceleration (>2g)
   - Response: Excited, Angry, or Surprised

2. **Tilt** - Deviation from vertical (>0.3g)
   - Response: Curious or Confused

3. **Rotation** - High gyro activity (>100°/s)
   - Response: Confused

4. **Stillness** - No motion for >60 seconds
   - Response: Sleepy (20% chance)

### Vibration Patterns

5 distinct haptic feedback patterns:

1. **Gentle Pulse** - 200ms pulse (Happiness/Content)
2. **Rapid Burst** - 3x quick pulses (Excitement)
3. **Long Sustained** - 1 second continuous (Loneliness)
4. **Wave Pattern** - PWM fade in/out (Affection)
5. **Short Taps** - 5x quick taps (Curiosity)

### Adaptive Personality System

The pet has 5 personality traits (0-10 scale):

1. **Playfulness** - Increases with rapid tap interactions
2. **Affection** - Increases with long press touches
3. **Curiosity** - Increases with motion/exploration
4. **Energy** - Affects sleep/activity balance
5. **Sociability** - Increases with frequent interactions

These traits influence:
- Emotion transition probabilities
- Response to interactions
- Circadian rhythm effects

### Social Bonding

The system tracks:
- Total interactions count
- Touch location preferences (4 sensors)
- Favorite touch location
- Bond level (0-100)
  - Displayed as progress bar at bottom of screen
  - Increases with each positive interaction
  - Affects emotion responses

### Circadian Rhythm

Simulated time-of-day effects (based on uptime):
- **Night (22:00-06:00)**: Lower energy, more sleepy states
- **Morning (06:00-10:00)**: Energy increases
- **Day (10:00-18:00)**: Peak activity levels

### Data Persistence

Saved to EEPROM (survives reboots):
- Personality traits
- Bond level
- Favorite touch location
- Touch count statistics

Auto-saves every 5 minutes during operation.

## Usage Tips

### Initial Bonding
1. Touch the pet frequently in the first few minutes
2. Try different touch patterns to see responses
3. Watch the bond level bar fill up at the bottom of the screen

### Personality Development
- Use **rapid taps** to make the pet more playful
- Use **long presses** to make the pet more affectionate
- **Frequent interactions** increase sociability
- **Motion/movement** increases curiosity

### Interaction Ideas
- **Pet it gently**: Single touches on different sensors
- **Tickle it**: Rapid taps to make it laugh
- **Cuddle it**: Long press for affectionate response
- **Play with it**: Shake and tilt for playful reactions
- **Let it rest**: Leave still to let it sleep

## Troubleshooting

### Display Not Working
- Check I2C connections (SDA=GPIO8, SCL=GPIO9)
- Verify display address is 0x3C
- Check power supply (3.3V)

### MPU6050 Not Detected
- **CRITICAL**: Ensure vibration motor is OFF during MPU6050 initialization
- Check I2C connections
- Try slower I2C clock (100kHz instead of 400kHz)
- Verify MPU6050 power connections

### Animations Not Loading
- Verify filesystem upload completed successfully
- Check Serial Monitor for file listing
- Ensure partition scheme includes SPIFFS/LittleFS space
- Re-upload data folder if needed

### Touch Sensors Not Responding
- Check pull-up resistors on TTP223 modules
- Verify GPIO connections (0, 1, 3, 4)
- Test individual sensors with Serial Monitor debug output

### Motor Not Vibrating
- Check transistor (BC547B) connections
- Verify 1kΩ base resistor is connected
- Test motor with direct power
- Ensure motor is off during MPU6050 initialization

### Memory Issues
- If sketch is too large, reduce MAX_FRAMES or MAX_COMPRESSED_SIZE
- Consider removing less-used emotion states
- Optimize String usage

## Serial Monitor Commands

The Serial Monitor provides detailed debug information:

```
========================================
Desktop Pet Robot - Comprehensive AI
========================================
Initializing Display... OK
Initializing MPU6050... OK
Touch Sensors: OK
Initializing LittleFS... OK

LittleFS Contents:
  /happy.anim (31212 bytes)
  /excited_2.anim (35652 bytes)
  ...

Loading animations...
  [0] Happy: 24 frames @ 15 FPS
  [1] Excited: 28 frames @ 15 FPS
  ...

=== System Ready ===
Personality Profile:
  Playfulness: 5
  Affection: 5
  Curiosity: 5
  Energy: 7
  Sociability: 5
  Bond Level: 0

Starting Emotional Loop...

Loaded animation: Content
Touch: SINGLE @ Location: 0 | Bond: 1
Emotion: Content -> Happy
Motion: SHAKE detected
Emotion: Happy -> Excited
```

## Advanced Customization

### Adding New Emotions
1. Add emotion to `EmotionState` enum
2. Add animation mapping to `emotionAnimations[]` array
3. Update `emotionNames[]` array
4. Add transition logic in `selectNextEmotion()`

### Adjusting Transition Probabilities
Modify `baseEmotionWeights[]` array to change how often each emotion appears.

### Custom Touch Responses
Edit the `respondToTouch()` function to create custom behavior patterns.

### Tuning Motion Sensitivity
Adjust thresholds in `updateMotionSensors()`:
- Shake: `accelMagnitude > 2.0` (reduce for more sensitive)
- Rotation: `gyroMagnitude > 100.0`
- Tilt: `abs(accelZ - 1.0) > 0.3`

## File Structure

```
esp32C3_dasai_mochi_clone/
├── EmotionalAI_DesktopPet_Comprehensive.ino  (Main sketch)
├── EmotionalAI_DesktopPet/
│   └── data/
│       ├── happy.anim
│       ├── excited_2.anim
│       ├── sleepy.anim
│       ├── love.anim
│       ├── angry.anim
│       ├── confused_2.anim
│       ├── determined.anim
│       ├── embarrassed.anim
│       ├── frustrated.anim
│       ├── laugh.anim
│       ├── music.anim
│       ├── proud.anim
│       ├── relaxed.anim
│       └── ... (all animation files)
└── COMPREHENSIVE_EMOTION_ENGINE_README.md  (This file)
```

## Credits

**Emotion Engine**: Comprehensive AI System v2.0
**Animation System**: LittleFS with RLE Compression
**Hardware Integration**: Full sensor suite support

Based on:
- Focus_Bot_V2_with_MPU6050_Vibration_4_Touch_TEST.ino (Hardware testing)
- Animation_Test.ino (Animation playback system)
- EmotionalAI_DesktopPet_Logic_V1.ino (Emotion state machine)

## License

Open source - feel free to modify and expand!

## Support

For issues, questions, or feature requests:
- Check Serial Monitor debug output
- Review troubleshooting section
- Verify all hardware connections
- Ensure all required libraries are installed

Happy building! Your desktop pet awaits! 🤖
