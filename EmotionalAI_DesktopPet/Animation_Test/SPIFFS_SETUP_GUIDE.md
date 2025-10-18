# SPIFFS On-Demand Animation Loading - Setup Guide

## Overview

This guide will help you upload animations to SPIFFS and run the memory-efficient version that loads animations on-demand instead of keeping them all in memory at once.

## Why SPIFFS?

The original sketch tried to load all 18 animations (~4MB) into memory simultaneously, causing the ESP32C3 to crash. This SPIFFS-based approach:
- Stores animations in the filesystem (flash storage)
- Loads only the current animation into RAM
- Supports all 18 animations without memory issues
- Uses only ~2-3KB of RAM vs ~756KB before

## Prerequisites

### 1. Install ESP32 SPIFFS Upload Tool

**For Arduino IDE 1.x:**
1. Download the [ESP32FS Plugin](https://github.com/me-no-dev/arduino-esp32fs-plugin/releases)
2. Extract to `<Arduino>/tools/ESP32FS/tool/esp32fs.jar`
3. Restart Arduino IDE
4. You should see "ESP32 Sketch Data Upload" in Tools menu

**For Arduino IDE 2.x:**
1. Install `arduino-esp32fs-plugin` via Extension Manager
2. Or use command line tool: `arduino-cli upload --fqbn esp32:esp32:esp32c3 --port COM_PORT`

**For PlatformIO:**
- SPIFFS upload is built-in, use `pio run -t uploadfs`

### 2. Required Python Libraries

The conversion script requires Python 3.x (already installed on most systems).

## Step-by-Step Instructions

### Step 1: Convert Animations to Binary Format

Navigate to the Animation_Test directory and run the Python script:

```bash
cd "a:\OneDrive\Documents\Programming\ESp32C3 Dasai Mochi\esp32C3_dasai_mochi_clone\EmotionalAI_DesktopPet\Animation_Test"
python convert_to_spiffs.py
```

This will:
- Read all `.h` animation files
- Convert them to binary `.anim` files
- Store them in the `data/` folder
- Create a manifest file
- Show total size (should be ~3.8MB)

**Expected Output:**
```
============================================================
Animation Header to SPIFFS Binary Converter
============================================================

Processing angry.h...
  Frame count: 60
  FPS: 10
  Max compressed size: 610
  Extracted 60 frames
  Created data/angry.anim (36612 bytes)

[... repeats for all 18 animations ...]

============================================================
Conversion complete!
Total animations: 18
Total size: 3,847,234 bytes (3756.1 KB)
Output directory: data
============================================================
```

### Step 2: Upload Animations to SPIFFS

**Arduino IDE:**
1. Open `Animation_Test_SPIFFS.ino` in Arduino IDE
2. Select your ESP32C3 board and COM port
3. Go to **Tools -> ESP32 Sketch Data Upload**
4. Wait for upload to complete (~2-3 minutes)

**PlatformIO:**
```bash
pio run -t uploadfs
```

**Manual Upload (if plugin doesn't work):**
```bash
python -m esptool --chip esp32c3 --port COM_PORT --baud 460800 write_flash 0x310000 spiffs.bin
```

### Step 3: Upload the Sketch

1. Open `Animation_Test_SPIFFS.ino`
2. Select ESP32C3 board
3. Upload normally (Ctrl+U or Upload button)

### Step 4: Verify Operation

1. Open Serial Monitor (115200 baud)
2. You should see:

```
========================================
ESP32-C3 Animation Display Test
SPIFFS On-Demand Loading Version
========================================
Hardware:
- SSH1106 128x64 OLED Display
========================================

Initializing SPIFFS... OK

SPIFFS Contents:
  /angry.anim (36612 bytes)
  /happy.anim (30804 bytes)
  [... all 18 animations listed ...]
Total files: 18, Total size: 3847234 bytes
SPIFFS: 3847234 / 4194304 bytes used (91%)

Loading animation metadata...
  [0] Intro: 60 frames @ 10 FPS
  [1] Happy: 60 frames @ 10 FPS
  [... all animations ...]
Loaded 18 animations

Initializing SSH1106 Display... OK

========================================
Starting Animation Display
========================================

>>> Playing Animation: Intro (60 frames @ 10 FPS)
  [Frame 0] Compressed: 682B | Read: 234us | Decompress: 156us | Total: 1234us
  [Frame 10] Compressed: 678B | Read: 231us | Decompress: 154us | Total: 1201us
  ...
>>> Animation Complete: Intro

>>> Playing Animation: Happy (60 frames @ 10 FPS)
  ...
```

## Troubleshooting

### "SPIFFS mount failed!"
- Make sure you uploaded the data folder using ESP32 Sketch Data Upload
- Verify partition scheme supports SPIFFS (Tools -> Partition Scheme -> Default 4MB with spiffs)

### "Failed to load animation"
- Check that all .anim files are in SPIFFS (look at Serial Monitor listing)
- Verify conversion script completed successfully
- Re-run conversion and re-upload data

### Sketch too large
- This should no longer be an issue! The sketch is now ~50KB vs ~800KB+
- If still too large, check you're not including the old .h files

### Display shows garbage
- This indicates decompression issue
- Verify the binary format matches between Python script and Arduino code
- Check RLE compression format in original .h files

### Animation playback is choppy
- SPIFFS read speed is slower than PROGMEM
- Consider reducing frame rate in Python script
- Or use lower resolution images

## Memory Usage Comparison

**Original (PROGMEM):**
- Flash: ~800KB+ (doesn't fit!)
- RAM: ~3KB
- Result: Crashes on startup

**SPIFFS Version:**
- Flash: ~50KB sketch + ~3.8MB data
- RAM: ~3KB
- Result: Runs perfectly!

## File Structure

```
Animation_Test/
├── Animation_Test_SPIFFS.ino    # Main sketch (use this!)
├── Animation_Test.ino            # Original (too large - don't use)
├── convert_to_spiffs.py          # Conversion script
├── data/                         # SPIFFS upload folder
│   ├── angry.anim
│   ├── happy.anim
│   ├── ... (all 18 animations)
│   └── manifest.txt
├── angry.h                       # Original headers (kept for reference)
├── happy.h
└── ... (all animation .h files)
```

## Binary File Format

Each `.anim` file contains:

```
Header (12 bytes):
  - frame_count: uint16 (2 bytes)
  - fps: uint16 (2 bytes)
  - max_compressed_size: uint16 (2 bytes)
  - reserved: 6 bytes

Frame Sizes Array (frame_count * 2 bytes):
  - size[0]: uint16
  - size[1]: uint16
  - ... for each frame

Frame Data (frame_count * max_compressed_size bytes):
  - frame[0]: RLE compressed data (padded to max_compressed_size)
  - frame[1]: RLE compressed data (padded to max_compressed_size)
  - ... for each frame
```

## Next Steps

Once everything is working:
1. Integrate into your main EmotionalAI_DesktopPet project
2. Add emotion selection logic
3. Add interactive features (sensors, WiFi, etc.)
4. Optimize frame rates for smoother playback

## Support

If you encounter issues:
1. Check Serial Monitor output
2. Verify SPIFFS contents are correct
3. Ensure partition scheme supports SPIFFS
4. Re-run conversion script and re-upload data
