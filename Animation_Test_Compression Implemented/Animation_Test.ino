/*
 * ESP32-C3 Animation Display Test - LittleFS Version
 * Displays all available animations in a continuous loop
 * Loads animations on-demand from LittleFS to save memory
 *
 * Hardware:
 * - ESP32-C3 Supermini
 * - SSH1106 128x64 OLED Display
 *
 * Required Libraries:
 * - Adafruit GFX Library
 * - Adafruit SH110X
 * - LittleFS (built-in)
 *
 * Install via Arduino Library Manager
 *
 * IMPORTANT: Before uploading this sketch:
 * 1. Run: python convert_to_spiffs.py  (creates binary .anim files)
 * 2. Upload data folder: Tools -> ESP32 Sketch Data Upload
 * 3. Then upload this sketch
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include "FS.h"
#include "LittleFS.h"

// Pin Definitions for ESP32-C3 Supermini
#define SDA_PIN 8        // I2C Data (default for ESP32-C3)
#define SCL_PIN 9        // I2C Clock (default for ESP32-C3)

// Display Settings (SSH1106 compatible)
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

#define MOTOR_PIN 6      // Vibration Motor (via BC547B + 1kΩ resistor)

// Maximum values across all animations
#define MAX_FRAMES 100
#define MAX_COMPRESSED_SIZE 1024

// Objects
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Animation structure for SPIFFS-loaded animations
struct Animation {
  String filename;
  String displayName;
  uint16_t frameCount;
  uint16_t fps;
  uint16_t maxCompressedSize;
  uint16_t frameDelay;
};

// Animation list - metadata only
Animation animations[] = {
  {"/intro.anim", "Intro", 0, 0, 0, 0},
  {"/happy.anim", "Happy", 0, 0, 0, 0},
  {"/happy_2.anim", "Happy 2", 0, 0, 0, 0},
  {"/angry.anim", "Angry", 0, 0, 0, 0},
  {"/angry_2.anim", "Angry 2", 0, 0, 0, 0},
  {"/confused_2.anim", "Confused", 0, 0, 0, 0},
  {"/content.anim", "Content", 0, 0, 0, 0},
  {"/determined.anim", "Determined", 0, 0, 0, 0},
  {"/embarrassed.anim", "Embarrassed", 0, 0, 0, 0},
  {"/excited_2.anim", "Excited", 0, 0, 0, 0},
  {"/frustrated.anim", "Frustrated", 0, 0, 0, 0},
  {"/laugh.anim", "Laugh", 0, 0, 0, 0},
  {"/love.anim", "Love", 0, 0, 0, 0},
  {"/music.anim", "Music", 0, 0, 0, 0},
  {"/proud.anim", "Proud", 0, 0, 0, 0},
  {"/relaxed.anim", "Relaxed", 0, 0, 0, 0},
  {"/sleepy.anim", "Sleepy", 0, 0, 0, 0},
  {"/sleepy_3.anim", "Sleepy 2", 0, 0, 0, 0}
};

const int animationCount = sizeof(animations) / sizeof(animations[0]);

// Animation state variables
int currentAnimation = 0;
int currentFrame = 0;
unsigned long lastFrameTime = 0;

// Buffers for decompression (allocated once, reused)
uint8_t* compressedBuffer = nullptr;  // Will be allocated based on max size
uint8_t frameBuffer[1024];            // Buffer for decompressed frame (128*64/8 = 1024 bytes)

// Frame sizes for current animation (dynamically allocated)
uint16_t* frameSizes = nullptr;
File currentAnimFile;
bool animFileOpen = false;

void setup() {
  Serial.begin(115200);
  pinMode(MOTOR_PIN, OUTPUT);
  digitalWrite(MOTOR_PIN, LOW);
  delay(1000);

  Serial.println("\n========================================");
  Serial.println("ESP32-C3 Animation Display Test");
  Serial.println("LittleFS On-Demand Loading Version");
  Serial.println("========================================");
  Serial.println("Hardware:");
  Serial.println("- SSH1106 128x64 OLED Display");
  Serial.println("========================================\n");

  // Initialize LittleFS
  Serial.print("Initializing LittleFS... ");

  // First attempt: try to mount existing LittleFS
  if(!LittleFS.begin(false)) {
    Serial.println("No filesystem found.");
    Serial.println("Formatting LittleFS (this may take a minute)...");

    // Format LittleFS
    if(!LittleFS.begin(true)) {
      Serial.println("FAILED!");
      Serial.println("\nERROR: LittleFS format/mount failed!");
      Serial.println("\nPossible issues:");
      Serial.println("1. Partition scheme doesn't include filesystem partition");
      Serial.println("   Fix: Tools -> Partition Scheme -> 'Default 4MB with spiffs' (works for LittleFS too)");
      Serial.println("2. Hardware issue with flash memory");
      Serial.println("\nCurrent partition scheme must support filesystem!");
      while(1) {
        delay(1000);
      }
    }
    Serial.println("LittleFS formatted successfully!");
  }
  Serial.println("OK");

  // List LittleFS contents
  listLittleFS();

  // Load animation metadata
  Serial.println("\nLoading animation metadata...");
  int validAnims = 0;
  for(int i = 0; i < animationCount; i++) {
    if(loadAnimationMetadata(&animations[i])) {
      validAnims++;
      Serial.print("  [");
      Serial.print(i);
      Serial.print("] ");
      Serial.print(animations[i].displayName);
      Serial.print(": ");
      Serial.print(animations[i].frameCount);
      Serial.print(" frames @ ");
      Serial.print(animations[i].fps);
      Serial.println(" FPS");
    }
  }
  Serial.print("Loaded ");
  Serial.print(validAnims);
  Serial.println(" animations");

  // Allocate buffers
  compressedBuffer = (uint8_t*)malloc(MAX_COMPRESSED_SIZE);
  frameSizes = (uint16_t*)malloc(MAX_FRAMES * sizeof(uint16_t));

  if(!compressedBuffer || !frameSizes) {
    Serial.println("ERROR: Failed to allocate buffers!");
    while(1) delay(1000);
  }

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
  display.println("Animation Test");
  display.println("SPIFFS Loading");
  display.println();
  display.print("Animations: ");
  display.println(validAnims);
  display.display();
  Serial.println("OK");
  delay(2000);

  Serial.println("\n========================================");
  Serial.println("Starting Animation Display");
  Serial.println("========================================\n");

  // Initialize animation playback
  currentAnimation = 0;
  currentFrame = 0;
  lastFrameTime = millis();
}

void loop() {
  // Check if we've displayed all animations
  if(currentAnimation >= animationCount) {
    // Close current animation file
    if(animFileOpen) {
      currentAnimFile.close();
      animFileOpen = false;
    }

    // Restart animation sequence
    currentAnimation = 0;
    currentFrame = 0;
    lastFrameTime = millis();
    Serial.println("\n>>> Restarting animation loop...\n");
    delay(1000);
    return;
  }

  Animation* anim = &animations[currentAnimation];

  // Load animation if starting new one
  if(currentFrame == 0) {
    // Close previous animation
    if(animFileOpen) {
      currentAnimFile.close();
      animFileOpen = false;
    }

    // Load new animation
    if(!loadAnimation(anim)) {
      Serial.print("ERROR: Failed to load animation: ");
      Serial.println(anim->displayName);
      currentAnimation++;
      return;
    }

    Serial.print(">>> Playing Animation: ");
    Serial.print(anim->displayName);
    Serial.print(" (");
    Serial.print(anim->frameCount);
    Serial.print(" frames @ ");
    Serial.print(anim->fps);
    Serial.println(" FPS)");
  }

  // Check if it's time to update frame
  if(millis() - lastFrameTime >= anim->frameDelay) {
    // Display current frame from LittleFS
    displayFrameFromLittleFS(anim, currentFrame);

    // Move to next frame
    currentFrame++;
    lastFrameTime = millis();

    // Check if animation complete
    if(currentFrame >= anim->frameCount) {
      Serial.print(">>> Animation Complete: ");
      Serial.println(anim->displayName);
      Serial.println();

      currentAnimation++;
      currentFrame = 0;

      // Short pause between animations
      delay(500);
    }
  }
}

// List LittleFS contents
void listLittleFS() {
  Serial.println("\nLittleFS Contents:");
  File root = LittleFS.open("/");
  File file = root.openNextFile();
  int fileCount = 0;
  size_t totalSize = 0;

  while(file) {
    Serial.print("  ");
    Serial.print(file.name());
    Serial.print(" (");
    Serial.print(file.size());
    Serial.println(" bytes)");

    fileCount++;
    totalSize += file.size();
    file = root.openNextFile();
  }

  Serial.print("Total files: ");
  Serial.print(fileCount);
  Serial.print(", Total size: ");
  Serial.print(totalSize);
  Serial.println(" bytes");

  size_t totalBytes = LittleFS.totalBytes();
  size_t usedBytes = LittleFS.usedBytes();
  Serial.print("LittleFS: ");
  Serial.print(usedBytes);
  Serial.print(" / ");
  Serial.print(totalBytes);
  Serial.print(" bytes used (");
  Serial.print((usedBytes * 100) / totalBytes);
  Serial.println("%)");

  // Warn if no files found
  if(fileCount == 0) {
    Serial.println("\n*** WARNING: No files found in LittleFS! ***");
    Serial.println("You need to upload animation data:");
    Serial.println("1. Run: python convert_to_spiffs.py");
    Serial.println("2. Tools -> ESP32 Sketch Data Upload");
    Serial.println("3. Re-upload this sketch");
  }
}

// Load animation metadata from LittleFS
bool loadAnimationMetadata(Animation* anim) {
  File file = LittleFS.open(anim->filename, "r");
  if(!file) {
    return false;
  }

  // Read header (12 bytes)
  // Format: frame_count (2 bytes), fps (2 bytes), max_compressed_size (2 bytes), reserved (6 bytes)
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

// Load animation data from LittleFS
bool loadAnimation(Animation* anim) {
  // Open animation file
  currentAnimFile = LittleFS.open(anim->filename, "r");
  if(!currentAnimFile) {
    return false;
  }

  // Skip header (12 bytes)
  currentAnimFile.seek(12);

  // Read frame sizes array
  for(int i = 0; i < anim->frameCount; i++) {
    uint8_t sizeBytes[2];
    if(currentAnimFile.read(sizeBytes, 2) != 2) {
      currentAnimFile.close();
      return false;
    }
    frameSizes[i] = sizeBytes[0] | (sizeBytes[1] << 8);
  }

  animFileOpen = true;
  return true;
}

// Display a frame from LittleFS
void displayFrameFromLittleFS(Animation* anim, int frameIndex) {
  unsigned long startTime = micros();

  // Calculate file position for this frame
  // Header (12 bytes) + Frame sizes array (frameCount * 2 bytes) + Frame data offset
  size_t frameDataStart = 12 + (anim->frameCount * 2);
  size_t frameOffset = frameDataStart + (frameIndex * anim->maxCompressedSize);

  // Seek to frame position
  currentAnimFile.seek(frameOffset);

  // Read compressed frame data
  int compressedSize = frameSizes[frameIndex];
  int bytesRead = currentAnimFile.read(compressedBuffer, compressedSize);

  if(bytesRead != compressedSize) {
    Serial.print("ERROR: Failed to read frame ");
    Serial.println(frameIndex);
    return;
  }

  unsigned long readTime = micros() - startTime;

  // Decompress the frame using RLE
  rleDecompress(compressedBuffer, compressedSize, frameBuffer);

  unsigned long decompressTime = micros() - startTime - readTime;

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

  unsigned long totalTime = micros() - startTime;

  // Debug output every 10th frame
  if(frameIndex % 10 == 0) {
    Serial.print("  [Frame ");
    Serial.print(frameIndex);
    Serial.print("] Compressed: ");
    Serial.print(compressedSize);
    Serial.print("B | Read: ");
    Serial.print(readTime);
    Serial.print("us | Decompress: ");
    Serial.print(decompressTime);
    Serial.print("us | Total: ");
    Serial.print(totalTime);
    Serial.println("us");
  }
}

// RLE Decompression function
// Decompresses RLE data into output buffer
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
