# Focus Bot - Emotional AI Desktop Pet

**By Rayan Singh**

An interactive, emotionally intelligent desktop companion powered by ESP32-C3, featuring adaptive personality, touch sensing, motion detection, and expressive OLED animations.

![Version](https://img.shields.io/badge/version-1.0-blue)
![Platform](https://img.shields.io/badge/platform-ESP32--C3-green)
![License](https://img.shields.io/badge/license-MIT-orange)

---

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Hardware Requirements](#hardware-requirements)
- [Pin Configuration](#pin-configuration)
- [Software Architecture](#software-architecture)
- [Installation](#installation)
- [Animation System](#animation-system)
- [Emotional AI System](#emotional-ai-system)
- [Usage](#usage)
- [Development Files](#development-files)
- [Technical Details](#technical-details)
- [Troubleshooting](#troubleshooting)
- [Contributing](#contributing)
- [License](#license)

---

## Overview

**Focus Bot** is an advanced emotional AI desktop pet that responds to touch, motion, and time-based patterns. Built on the ESP32-C3 Supermini microcontroller, it features:

- **10 distinct emotional states** with adaptive transitions
- **18 expressive animations** optimized with RLE compression (52% size reduction)
- **Multi-modal feedback** combining visual displays and haptic vibration
- **Persistent personality learning** that evolves based on user interaction
- **Advanced sensor fusion** using touch and motion detection

The device learns your interaction patterns over time, developing a unique personality and building a social bond that persists across power cycles.

---

## Features

### >� Emotional Intelligence
- **10 Emotion States**: Happy, Excited, Sleepy, Playful, Lonely, Curious, Surprised, Content, Bored, Affectionate
- **Adaptive Personality**: 5 personality traits (Playfulness, Affection, Curiosity, Energy, Sociability)
- **Learning System**: Tracks interaction history and adjusts behavior accordingly
- **Bond Level System**: Develops stronger bonds with frequent, varied interactions
- **Circadian Rhythm**: Activity patterns vary based on time-of-day simulation

### =F Touch Interaction
- **4 Capacitive Touch Sensors** with location-specific responses
- **5 Touch Patterns**: Single tap, double tap, long press, rapid taps, multi-sensor
- **Favorite Location Learning**: Remembers and responds to your preferred touch locations
- **Pattern Recognition**: Detects complex interaction sequences

### <� Motion Detection
- **5 Motion Types**: Still, Tilt, Shake, Rotation, Drop (free-fall)
- **6-Axis Sensor**: MPU6050 gyroscope + accelerometer fusion
- **Context-Aware Responses**: Different reactions based on current emotional state

### <� Visual & Haptic Feedback
- **18 Expressive Animations** (60 frames each @ 10 FPS)
- **128�64 OLED Display** (SH1106) with smooth animation playback
- **6 Vibration Patterns**: Gentle pulse, rapid burst, sustained, wave, short taps, none
- **Synchronized Multi-Modal Output**: Coordinated visual + haptic responses

### =� Persistent Memory
- **EEPROM Storage**: Personality traits and interaction history
- **LittleFS/SPIFFS**: Compressed animation data
- **State Preservation**: Remembers personality across reboots

---

## Hardware Requirements

### Core Components

| Component | Model/Type | Quantity | Purpose |
|-----------|------------|----------|---------|
| **Microcontroller** | ESP32-C3 Supermini | 1 | Main processor |
| **Display** | SH1106 128�64 OLED (I2C) | 1 | Visual output |
| **Motion Sensor** | MPU6050 (6-axis) | 1 | Motion detection |
| **Touch Sensors** | TTP223 Capacitive Touch | 4 | User interaction |
| **Vibration Motor** | DC Vibration Motor | 1 | Haptic feedback |
| **Transistor** | BC547B NPN BJT | 1 | Motor driver |
| **Resistor** | 1k� | 1 | Base current limiting |

### Additional Requirements
- USB-C cable for programming
- Power supply (3.3V from USB or battery)
- PCB or breadboard for assembly
- Connecting wires

---

## Pin Configuration

### ESP32-C3 Pinout

```cpp
// I2C Bus (shared by OLED and MPU6050)
#define SDA_PIN 8      // I2C Data
#define SCL_PIN 9      // I2C Clock

// Capacitive Touch Sensors
#define TOUCH1_PIN 0   // Front Left
#define TOUCH2_PIN 1   // Back Left
#define TOUCH3_PIN 3   // Front Right
#define TOUCH4_PIN 4   // Back Right

// Vibration Motor (via BC547B transistor)
#define MOTOR_PIN 6    // PWM/Digital control
```

### I2C Device Addresses
- **SH1106 OLED**: `0x3C`
- **MPU6050 Sensor**: `0x68`

### Motor Driver Circuit

```
ESP32-C3 GPIO6 
