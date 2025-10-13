
/*
 * OLED Animation Advanced Example (24FPS)
 * Tạo bởi OLED Frame Generator
 * @khoi2mai
 * 
 * Số lượng frame: 772
 * Kích thước: 128x64 pixels
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "all_frames.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Cấu hình animation
#define FRAME_DELAY 42  // 24FPS

void setup() {
  Serial.begin(115200);
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  display.clearDisplay();
  display.display();
  Serial.println("OLED initialized");
  Serial.println("@khoi2mai");
}

void loop() {
  static unsigned long lastFrameTime = 0;
  static int currentFrame = 0;
  unsigned long currentTime = millis();
  if (currentTime - lastFrameTime >= FRAME_DELAY) {
    lastFrameTime = currentTime;
    display.clearDisplay();
    display.drawBitmap(0, 0, frames[currentFrame], FRAME_WIDTH, FRAME_HEIGHT, SSD1306_WHITE);
    display.display();
    currentFrame = (currentFrame + 1) % TOTAL_FRAMES;
  }
}
