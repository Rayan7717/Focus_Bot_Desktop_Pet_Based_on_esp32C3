/*
 * ESP32-C3 Animation Display Test
 * Displays all available animations in a continuous loop
 *
 * Hardware:
 * - ESP32-C3 Supermini
 * - SSH1106 128x64 OLED Display
 *
 * Required Libraries:
 * - Adafruit GFX Library
 * - Adafruit SH110X
 *
 * Install via Arduino Library Manager
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

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

// Display Settings (SSH1106 compatible)
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

#define MOTOR_PIN 6      // Vibration Motor (via BC547B + 1kΩ resistor)

// Objects
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Animation structure to hold animation data (RLE compressed)
struct Animation {
  const char* name;
  const uint8_t* frames;            // Pointer to compressed frame array (flattened)
  const int* frameSizes;            // Array of compressed sizes for each frame
  int maxCompressedSize;            // Maximum compressed frame size
  int frameCount;
  int fps;
  int frameDelay;
  void (*decompressFunc)(const uint8_t*, int, uint8_t*);  // Decompression function
};

// Animation list - all available animations (RLE compressed)
Animation animations[] = {
  {"Intro", (const uint8_t*)intro_frames, intro_frame_sizes, intro_MAX_COMPRESSED_SIZE, intro_FRAME_COUNT, intro_FPS, intro_FRAME_DELAY, intro_decompress},
  {"Happy", (const uint8_t*)happy_frames, happy_frame_sizes, happy_MAX_COMPRESSED_SIZE, happy_FRAME_COUNT, happy_FPS, happy_FRAME_DELAY, happy_decompress},
  {"Happy 2", (const uint8_t*)happy_2_frames, happy_2_frame_sizes, happy_2_MAX_COMPRESSED_SIZE, happy_2_FRAME_COUNT, happy_2_FPS, happy_2_FRAME_DELAY, happy_2_decompress},
  {"Angry", (const uint8_t*)angry_frames, angry_frame_sizes, angry_MAX_COMPRESSED_SIZE, angry_FRAME_COUNT, angry_FPS, angry_FRAME_DELAY, angry_decompress},
  {"Angry 2", (const uint8_t*)angry_2_frames, angry_2_frame_sizes, angry_2_MAX_COMPRESSED_SIZE, angry_2_FRAME_COUNT, angry_2_FPS, angry_2_FRAME_DELAY, angry_2_decompress},
  {"Confused", (const uint8_t*)confused_2_frames, confused_2_frame_sizes, confused_2_MAX_COMPRESSED_SIZE, confused_2_FRAME_COUNT, confused_2_FPS, confused_2_FRAME_DELAY, confused_2_decompress},
  {"Content", (const uint8_t*)content_frames, content_frame_sizes, content_MAX_COMPRESSED_SIZE, content_FRAME_COUNT, content_FPS, content_FRAME_DELAY, content_decompress},
  {"Determined", (const uint8_t*)determined_frames, determined_frame_sizes, determined_MAX_COMPRESSED_SIZE, determined_FRAME_COUNT, determined_FPS, determined_FRAME_DELAY, determined_decompress},
  {"Embarrassed", (const uint8_t*)embarrassed_frames, embarrassed_frame_sizes, embarrassed_MAX_COMPRESSED_SIZE, embarrassed_FRAME_COUNT, embarrassed_FPS, embarrassed_FRAME_DELAY, embarrassed_decompress},
  {"Excited", (const uint8_t*)excited_2_frames, excited_2_frame_sizes, excited_2_MAX_COMPRESSED_SIZE, excited_2_FRAME_COUNT, excited_2_FPS, excited_2_FRAME_DELAY, excited_2_decompress},
  {"Frustrated", (const uint8_t*)frustrated_frames, frustrated_frame_sizes, frustrated_MAX_COMPRESSED_SIZE, frustrated_FRAME_COUNT, frustrated_FPS, frustrated_FRAME_DELAY, frustrated_decompress},
  {"Laugh", (const uint8_t*)laugh_frames, laugh_frame_sizes, laugh_MAX_COMPRESSED_SIZE, laugh_FRAME_COUNT, laugh_FPS, laugh_FRAME_DELAY, laugh_decompress},
  {"Love", (const uint8_t*)love_frames, love_frame_sizes, love_MAX_COMPRESSED_SIZE, love_FRAME_COUNT, love_FPS, love_FRAME_DELAY, love_decompress},
  {"Music", (const uint8_t*)music_frames, music_frame_sizes, music_MAX_COMPRESSED_SIZE, music_FRAME_COUNT, music_FPS, music_FRAME_DELAY, music_decompress},
  {"Proud", (const uint8_t*)proud_frames, proud_frame_sizes, proud_MAX_COMPRESSED_SIZE, proud_FRAME_COUNT, proud_FPS, proud_FRAME_DELAY, proud_decompress},
  {"Relaxed", (const uint8_t*)relaxed_frames, relaxed_frame_sizes, relaxed_MAX_COMPRESSED_SIZE, relaxed_FRAME_COUNT, relaxed_FPS, relaxed_FRAME_DELAY, relaxed_decompress},
  {"Sleepy", (const uint8_t*)sleepy_frames, sleepy_frame_sizes, sleepy_MAX_COMPRESSED_SIZE, sleepy_FRAME_COUNT, sleepy_FPS, sleepy_FRAME_DELAY, sleepy_decompress},
  {"Sleepy 2", (const uint8_t*)sleepy_3_frames, sleepy_3_frame_sizes, sleepy_3_MAX_COMPRESSED_SIZE, sleepy_3_FRAME_COUNT, sleepy_3_FPS, sleepy_3_FRAME_DELAY, sleepy_3_decompress}
};

const int animationCount = sizeof(animations) / sizeof(animations[0]);

// Animation state variables
int currentAnimation = 0;
int currentFrame = 0;
unsigned long lastFrameTime = 0;

// Buffers for decompression
uint8_t compressedBuffer[700];  // Buffer for compressed frame data (max 682 bytes)
uint8_t frameBuffer[1024];      // Buffer for decompressed frame (128*64/8 = 1024 bytes)

void setup() {
  Serial.begin(115200);
  pinMode(MOTOR_PIN, OUTPUT);
  digitalWrite(MOTOR_PIN, LOW);
  delay(1000);

  Serial.println("\n========================================");
  Serial.println("ESP32-C3 Animation Display Test");
  Serial.println("========================================");
  Serial.println("Hardware:");
  Serial.println("- SSH1106 128x64 OLED Display");
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
  display.println("Animation Test");
  display.println("Starting...");
  display.display();
  Serial.println("OK");
  delay(1000);

  Serial.println("\n========================================");
  Serial.println("Starting Animation Display");
  Serial.print("Total animations: ");
  Serial.println(animationCount);
  Serial.println("========================================\n");

  // Initialize animation playback
  currentAnimation = 0;
  currentFrame = 0;
  lastFrameTime = millis();
}

void loop() {
  // Check if we've displayed all animations
  if(currentAnimation >= animationCount) {
    // Restart animation sequence
    currentAnimation = 0;
    currentFrame = 0;
    lastFrameTime = millis();
    Serial.println("\n>>> Restarting animation loop...\n");
    delay(1000);
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
    // Display current frame with decompression
    displayCompressedFrame(anim, currentFrame);

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

// Display a compressed frame (with RLE decompression)
void displayCompressedFrame(Animation* anim, int frameIndex) {
  unsigned long startTime = micros();

  // Step 1: Read compressed frame from PROGMEM into RAM buffer
  int compressedSize = anim->frameSizes[frameIndex];

  // Calculate offset into the flattened 2D array
  // The frames array is stored as [frame][max_compressed_size]
  int frameOffset = frameIndex * anim->maxCompressedSize;

  for (int i = 0; i < compressedSize; i++) {
    compressedBuffer[i] = pgm_read_byte(anim->frames + frameOffset + i);
  }

  unsigned long readTime = micros() - startTime;

  // Step 2: Decompress the frame using the animation's decompression function
  anim->decompressFunc(compressedBuffer, compressedSize, frameBuffer);

  unsigned long decompressTime = micros() - startTime - readTime;

  // Step 3: Display the decompressed frame
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
