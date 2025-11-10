#include <Arduino.h>
#include <TFT_eSPI.h>
#include "ps2_driver.h"

#include "bus.h"
#include "cartridge.h"
#include "cpu.h"
#include "emulator_config.h"
#include "mapper.h"
#include "ppu.h"

#define CPU_FREQ_MHZ 240UL
typedef uint8_t byte;

TFT_eSPI tft = TFT_eSPI();
#define PS2_DAT 5
#define PS2_CMD 6
#define PS2_ATT 7
#define PS2_CLK 8
#define PS2_DELAY 50

#define SCREEN_W 320
#define SCREEN_H 240

PS2 gamepad(PS2_DAT, PS2_CMD, PS2_ATT, PS2_CLK, PS2_DELAY);
uint16_t frameBuffer[256 * 240]; // NES resolution
byte controller_input_buffer = 0;
/*
// int rectX = 0, rectY = 0;
// int rectW = 50, rectH = 40;
// int dx = 2, dy = 1;

// void drawRectToBuffer(int X, int Y, int W, int H, uint16_t COLOR) {
//   for (int j = 0; j < H; j++) {
//     int py = Y + j;
//     if (py < 0 || py >= SCREEN_H) continue;
//     for (int i = 0; i < W; i++) {
//       int px = X + i;
//       if (px < 0 || px >= SCREEN_W) continue;
//       frameBuffer[py * SCREEN_W + px] = COLOR;
//     }
//   }
// } */

Screen screen; // this is the virtual screen
Config config = Config("/smb.bin");
CARTRIDGE *cartridge;
CPU cpu;
TRACER tracer(cpu);
PPU ppu(cpu, screen);
BUS *bus;
int frames = 0;

void render_frame()
{
    tft.pushImage(0, 0, 256, 240, screen.pixels);
}
void setup()
{
    Serial.begin(115200);
    if (!LittleFS.begin(true))
    { // `true` means format if mount fails
        Serial.println("Failed to mount LittleFS");
        while (1)
            ; // stop
    }
    Serial.println("LittleFS mounted successfully");

    gamepad.begin();
    screen.generate_16bit_pallet();
    tft.init();
    tft.setRotation(0);
    tft.fillScreen(TFT_BLACK);

    cartridge = new CARTRIDGE(config);
    if (!cartridge->read_file())
        exit(-1);

    Serial.println("READ FILE SUCCESFULLY\n");
    bus = new BUS(cpu, ppu, *cartridge);
    cpu.connect_bus(bus);
    ppu.connect_bus(bus);
    ppu.connect_cartridge(cartridge);
    cpu.reset();
    cpu.init();
    Serial.println(cpu.read_abs_address(0xFFFC), HEX);
    cpu.PC = cpu.read_abs_address(0xFFFC);
    int frames = 0;
}
uint32_t start_time = 0;
uint32_t end_time = 0;
uint32_t previous_time_s = 0;
uint32_t current_time_s = 0;
void loop()
{
    bus->controller[0] = controller_input_buffer;
    current_time_s = millis();
    if(current_time_s - previous_time_s >= 1000){
        previous_time_s = current_time_s;
        Serial.println("FPS: " + String(frames));
        frames = 0;
    }
    start_time = xthal_get_ccount();
    bus->clock(); // does one clock systemwide. debug logs disabled
    end_time = xthal_get_ccount();
    
    uint32_t cycles = end_time- start_time;
    float time_ns = (float)cycles * (1e9 / (float)CPU_FREQ_MHZ / 1e6);
    Serial.printf("Clock took %u cycles (~%.2f ns)\n", cycles, time_ns);
    delay(100);
    if (screen.RENDER_ENABLED)
    {
        render_frame();
        //Serial.println("Frame rendered");
        // frames++;
        // end_time = millis();
        screen.RENDER_ENABLED = false;
    }

    // if(frames == 4) {
    //     cpu.hexdump();
    //     frames = 0;
    // }

    // memset(frameBuffer, 0, sizeof(frameBuffer));
    // tft.pushImage(0, 0, 256, 240, frameBuffer); // This is the method to push framebuffer to screen
}