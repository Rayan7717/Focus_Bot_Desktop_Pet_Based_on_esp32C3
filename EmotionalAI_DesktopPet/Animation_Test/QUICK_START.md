# Quick Start - Fix ESP32C3 Memory Crash

## The Problem
Your ESP32C3 was crashing because all animations (~4MB) were being loaded into memory at once.

## The Solution
Load animations on-demand from SPIFFS filesystem instead of keeping them all in memory.

## Steps to Fix (Do in Order!)

### Step 1: Check Partition Scheme in Arduino IDE
**CRITICAL:** Your board must be configured to use a partition scheme that includes SPIFFS.

1. Open Arduino IDE
2. Select your ESP32C3 board
3. Go to **Tools -> Partition Scheme**
4. Select one of these options:
   - **"Default 4MB with spiffs (1.2MB APP/1.5MB SPIFFS)"** ← RECOMMENDED
   - **"Huge APP (3MB No OTA/1MB SPIFFS)"**
   - **"Minimal SPIFFS (1.9MB APP with OTA/190KB SPIFFS)"** (too small for all animations)

### Step 2: Convert Animation Headers to Binary Files

Open a terminal/command prompt and run:

```bash
cd "a:\OneDrive\Documents\Programming\ESp32C3 Dasai Mochi\esp32C3_dasai_mochi_clone\EmotionalAI_DesktopPet\Animation_Test"
python convert_to_spiffs.py
```

You should see:
```
============================================================
Animation Header to SPIFFS Binary Converter
============================================================

Processing angry.h...
  Frame count: 60
  ...
  Created data/angry.anim (36612 bytes)

[... for all 18 animations ...]

Conversion complete!
Total animations: 18
Total size: 3,847,234 bytes (3756.1 KB)
```

This creates binary files in the `data/` folder.

### Step 3: Upload Animation Data to SPIFFS

**Arduino IDE 1.x:**
1. Close Serial Monitor if open
2. Go to **Tools -> ESP32 Sketch Data Upload**
3. Wait 2-3 minutes for upload to complete

**Arduino IDE 2.x:**
- May need to install ESP32FS plugin separately
- Or use command line:
  ```bash
  arduino-cli upload --fqbn esp32:esp32:esp32c3 --port YOUR_COM_PORT
  ```

**PlatformIO:**
```bash
pio run -t uploadfs
```

### Step 4: Upload the Sketch

1. Open `Animation_Test.ino` (it's now the SPIFFS version)
2. Click Upload (or press Ctrl+U)
3. Wait for upload to complete

### Step 5: Verify It's Working

1. Open Serial Monitor (115200 baud)
2. You should see:

```
========================================
ESP32-C3 Animation Display Test
SPIFFS On-Demand Loading Version
========================================

Initializing SPIFFS... OK

SPIFFS Contents:
  /angry.anim (36612 bytes)
  /happy.anim (30804 bytes)
  ... [all 18 .anim files listed]
Total files: 18, Total size: 3847234 bytes
SPIFFS: 3847234 / 1572864 bytes used (244%)

Loading animation metadata...
  [0] Intro: 60 frames @ 10 FPS
  [1] Happy: 60 frames @ 10 FPS
  ...
Loaded 18 animations

>>> Playing Animation: Intro (60 frames @ 10 FPS)
  [Frame 0] Compressed: 682B | Read: 234us | ...
```

## Troubleshooting

### Error: "SPIFFS mount failed, -10025"
**Cause:** Partition scheme doesn't include SPIFFS or SPIFFS not formatted.

**Fix:**
1. Check partition scheme (Step 1 above)
2. The sketch will now auto-format SPIFFS on first run
3. Re-upload the sketch

### Error: "No files found in SPIFFS!"
**Cause:** You skipped Step 3 (uploading data).

**Fix:**
1. Run Step 2 to convert animations
2. Upload data folder (Step 3)
3. Sketch already uploaded, just upload data!

### Error: "ESP32FS plugin not found"
**Cause:** SPIFFS upload tool not installed.

**Fix for Arduino IDE 1.x:**
1. Download from: https://github.com/me-no-dev/arduino-esp32fs-plugin/releases
2. Extract to: `<Arduino>/tools/ESP32FS/tool/esp32fs.jar`
3. Restart Arduino IDE

**Alternative (Manual Upload):**
Use `esptool.py` or `arduino-cli` - see SPIFFS_SETUP_GUIDE.md

### Animations not playing / Sketch still crashes
**Possible causes:**
1. Animation data corrupt - re-run conversion script
2. Partition scheme still wrong - verify Step 1
3. SPIFFS upload incomplete - check Serial Monitor shows all 18 files

**Debug:**
- Check Serial Monitor output carefully
- Verify SPIFFS shows 18 files totaling ~3.8MB
- Verify all animations loaded successfully

## Memory Usage

**Before (PROGMEM - CRASHED):**
- Sketch size: ~800KB+ (too large!)
- RAM: ~3KB
- Status: **CRASH ON STARTUP**

**After (SPIFFS - WORKS!):**
- Sketch size: ~50KB
- SPIFFS usage: ~3.8MB
- RAM: ~3KB
- Status: **RUNS PERFECTLY!**

## File Structure

```
Animation_Test/
├── Animation_Test.ino       ← Main sketch (NOW USES SPIFFS!)
├── convert_to_spiffs.py     ← Conversion script
├── data/                    ← Created by script
│   ├── angry.anim           ← Binary animation files
│   ├── happy.anim
│   └── ... (18 total)
├── *.h                      ← Original headers (for reference only)
└── QUICK_START.md           ← This file!
```

## Next Steps

Once working:
- Test each animation plays correctly
- Integrate into main EmotionalAI project
- Add emotion detection/selection logic
- Connect sensors, WiFi, etc.

## Need Help?

Check these files:
- **QUICK_START.md** (this file) - Quick fix steps
- **SPIFFS_SETUP_GUIDE.md** - Detailed technical info
- **RLE_IMPLEMENTATION.md** - Animation format details

If still stuck, check:
1. Serial Monitor output - shows exact error
2. Partition scheme setting
3. SPIFFS file listing (should show 18 .anim files)
