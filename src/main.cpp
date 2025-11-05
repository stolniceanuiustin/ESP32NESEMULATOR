#include <TFT_eSPI.h>
#include "ps2_driver.h"

TFT_eSPI tft = TFT_eSPI();  // Pins are in User_Setup.h

// //#define TFT_MISO  21  // Automatically assigned with ESP8266 if not defined
// #define TFT_MOSI  21  // Automatically assigned with ESP8266 if not defined
// #define TFT_SCLK  18  // Automatically assigned with ESP8266 if not defined

// #define TFT_CS    15  // Chip select control pin D8
// #define TFT_DC    2  // Data Command control pin
// #define TFT_RST   4  // Reset pin (could connect to NodeMCU RST, see next line)

#define PS2_DAT 5
#define PS2_CMD 6
#define PS2_ATT 7
#define PS2_CLK 8
#define PS2_DELAY 50 

#define SCREEN_W 320
#define SCREEN_H 240


PS2 gamepad(PS2_DAT, PS2_CMD, PS2_ATT, PS2_CLK, PS2_DELAY);
uint16_t frameBuffer[SCREEN_W * SCREEN_H]; 

int rectX = 0, rectY = 0;
int rectW = 50, rectH = 40;
int dx = 2, dy = 1;

void drawRectToBuffer(int X, int Y, int W, int H, uint16_t COLOR) {
  for (int j = 0; j < H; j++) {
    int py = Y + j;
    if (py < 0 || py >= SCREEN_H) continue;
    for (int i = 0; i < W; i++) {
      int px = X + i;
      if (px < 0 || px >= SCREEN_W) continue;
      frameBuffer[py * SCREEN_W + px] = COLOR;
    }
  }
}

void setup() {
  Serial.begin(115200);
  gamepad.begin();
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
}

void loop() {
  memset(frameBuffer, 0, sizeof(frameBuffer));
  drawRectToBuffer(rectX, rectY, rectW, rectH, TFT_RED);
  tft.pushImage(0, 0, SCREEN_W, SCREEN_H, frameBuffer);
  // rectX += dx;
  // rectY += dy;
  gamepad.readController();
  if(gamepad.buttonIsPressed("LEFT"))
    if(rectX > 0)
      rectX -= dx;
  if(gamepad.buttonIsPressed("RIGHT"))
    if(rectX < SCREEN_W)
      rectX += dx;

  if(gamepad.buttonIsPressed("UP"))
    if(rectY > 0)
      rectY -= dy;
  if(gamepad.buttonIsPressed("DOWN"))
    if(rectY < SCREEN_H)
      rectY += dy;
    
  
  // if (rectX < 0 || rectX + rectW > SCREEN_W) dx = -dx;
  // if (rectY < 0 || rectY + rectH > SCREEN_H) dy = -dy;
}