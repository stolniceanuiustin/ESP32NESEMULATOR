#include <Arduino.h>
#include <TFT_eSPI.h>
#include "ps2_driver.h"

#include "bus.h"
#include "cartridge.h"
#include "cpu.h"
#include "emulator_config.h"
#include "ppu.h"
#include <WiFi.h>
#include <LittleFS.h>

#define CPU_FREQ_MHZ 240UL
typedef uint8_t byte;

TFT_eSPI tft = TFT_eSPI();
#define PS2_DAT 40
#define PS2_CMD 39
#define PS2_ATT 38
#define PS2_CLK 37
#define PS2_DELAY 30

#define SCREEN_W 320
#define SCREEN_H 240

PS2 gamepad(PS2_DAT, PS2_CMD, PS2_ATT, PS2_CLK, PS2_DELAY);
uint16_t frameBuffer[256 * 240]; // NES resolution
byte controller_input_buffer = 0;

TaskHandle_t Core0Task;
int start_cycles_d = 0;
int end_cycles_d = 0;

Config configClass = Config("/smb.bin");
uint32_t frames = 0;
void Core0Loop(void *parameter);

uint8_t div3 = 0;
bool dma_phase = false;

inline void bus_clock_t()
{
    ppu_execute(); // same as before
    if (++div3 == 3)
    {
        div3 = 0;

        if (!dma_transfer)
        {
            cpu_clock();
        }
        else
        {
            if (dma_first_clock)
            {
                if (dma_phase == false)
                {
                    dma_first_clock = false;
                }
            }
            else
            {
                if (dma_phase == true)
                {
                    oam_dma_data = cpu_read((oam_dma_page << 8) | oam_dma_addr);
                }
                else
                {
                    pOAM[oam_dma_addr] = oam_dma_data;
                    oam_dma_addr++;
                    if (oam_dma_addr == 0x00)
                    {
                        dma_transfer = false;
                        dma_first_clock = true;
                    }
                }
            }

            dma_phase = !dma_phase;
        }
    }
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

    btStop();
    WiFi.mode(WIFI_OFF);

    gamepad.begin();
    delay(500);
    tft.init();
    tft.setRotation(0);
    tft.fillScreen(TFT_BLACK);

    if (!cartridge_read_file("/smb.bin"))
        exit(-1);

    Serial.println("READ FILE SUCCESFULLY\n");
    screen_init();
    bus_init();
    cpu_init();
    cpu_reset();
    ppu_init();
    // PC = read_abs_address(0xFFFC);
    int frames = 0;

    xTaskCreatePinnedToCore(Core0Loop, "Core0Loop", 10000, NULL, configMAX_PRIORITIES - 1, &Core0Task, 0);

    // vTaskDelete(NULL);
}

uint32_t previous_time_s = 0;
uint32_t current_time_s = 0;
void loop()
{
    current_time_s = millis();
    if (current_time_s - previous_time_s >= 1000)
    {
        previous_time_s = current_time_s;
        Serial.println("FPS: " + String(frames));
        frames = 0;
    }
    controller[0] = controller_input_buffer;
    if (RENDER_ENABLED)
    {
        tft.pushImage(0, 0, 256, 240, pixels);

        // input
        gamepad.readController();

        controller_input_buffer = 0;

        if (gamepad.buttonIsPressed("UP"))
            controller_input_buffer |= 0x08;
        if (gamepad.buttonIsPressed("DOWN"))
            controller_input_buffer |= 0x04;
        if (gamepad.buttonIsPressed("LEFT"))
            controller_input_buffer |= 0x02;
        if (gamepad.buttonIsPressed("RIGHT"))
            controller_input_buffer |= 0x01;
        if (gamepad.buttonIsPressed("CROSS"))
            controller_input_buffer |= 0x20; 
        if (gamepad.buttonIsPressed("CIRCLE"))
            controller_input_buffer |= 0x10; 
        if (gamepad.buttonIsPressed("SELECT"))
            controller_input_buffer |= 0x40; 
        if (gamepad.buttonIsPressed("START"))
            controller_input_buffer |= 0x80; 
        Serial.println(controller_input_buffer);

        // Serial.println("Frame rendered");
        frames++;
        RENDER_ENABLED = false;
    }
    // current_time_s = millis();
    // if (current_time_s - previous_time_s >= 1000)
    // {
    //     previous_time_s = current_time_s;
    //     Serial.println("FPS: " + String(frames));
    //     frames = 0;
    // }
    // start_cycles_d = xthal_get_ccount();
    // bus_clock_t(); // does one clock systemwide. debug logs disabled
    // end_cycles_d = xthal_get_ccount();
    // uint32_t my_cycles = end_cycles_d - start_cycles_d;
    // float time_ns = (float)my_cycles * (1e9 / (float)CPU_FREQ_MHZ / 1e6);
    // Serial.printf("Clock took %u cycles (~%.2f ns)\n", my_cycles, time_ns);
    // delay(10);

    // uint32_t cycles = end_time- start_time;
    // float time_ns = (float)cycles * (1e9 / (float)CPU_FREQ_MHZ / 1e6);
    // Serial.printf("Clock took %u cycles (~%.2f ns)\n", cycles, time_ns);
    // delay(100);

    // memset(frameBuffer, 0, sizeof(frameBuffer));
    // tft.pushImage(0, 0, 256, 240, frameBuffer); // This is the method to push framebuffer to screen
}

void Core0Loop(void *parameter)
{
    for (;;)
    {

        for (int i = 0; i <= 200000; i++)
        {
            // start_cycles_d = xthal_get_ccount();
            bus_clock_t(); // does one clock systemwide. debug logs disabled
            // end_cycles_d = xthal_get_ccount();
            // uint32_t my_cycles = end_cycles_d - start_cycles_d;
            // float time_ns = (float)my_cycles * (1e9 / (float)CPU_FREQ_MHZ / 1e6);
            // Serial.printf("Clock took %u cycles (~%.2f ns)\n", my_cycles, time_ns);
        }

        // if (RENDER_ENABLED)
        // {
        //     // tft.pushImage(0, 0, 256, 240, pixels);
        //     // Serial.println("Frame rendered");
        //     frames++;
        //     RENDER_ENABLED = false;
        // }
        vTaskDelay(1);
    }
}
