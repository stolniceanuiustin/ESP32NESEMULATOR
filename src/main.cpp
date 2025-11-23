#include <Arduino.h>
#include <TFT_eSPI.h>
#include <esp_task_wdt.h>
#include "ps2_driver.h"
#include <esp_int_wdt.h>
#include <esp_task_wdt.h>
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
uint32_t start_cycles_d = 0;
uint32_t end_cycles_d = 0;

Config configClass = Config("/smb.bin");
uint32_t frames = 0;
void Core0Loop(void *parameter);

uint8_t div3 = 0;
bool dma_phase = false;

/*
Takes on average ~400 cycles / bus clock
With only ppu it takes ~250 (some of it is function overhead) (before sprite rendering happens!)
With sprite rendering it takes somewhere near 340 cycles
So this is the thing to optimize!!
CPU is pretty efficient
With only cpu it takes  60 cycles (so cpu is almost as optimized as it can be?)
*/

//Rendering scanline then running the cpu works as most interaction between CPU and PPU happens during vblank, when the PPU is doing nothing
inline void bus_clock_t()
{
    ppu_render_scanline(); 
    for (int i = 0; i < 114; i++)
    {
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
    uint32_t frames = 0;

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
        // for(int i=0; i<256*240; i++)
        // {
        //     pixels[i] = TFT_BLUE;
        // }
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

        if (gamepad.buttonIsPressed("START")) // SDLK_s  = 0x10
            controller_input_buffer |= 0x10;

        if (gamepad.buttonIsPressed("SELECT")) // SDLK_a  = 0x20
            controller_input_buffer |= 0x20;

        if (gamepad.buttonIsPressed("SQUARE")) // SDLK_z  = 0x40
            controller_input_buffer |= 0x40;

        if (gamepad.buttonIsPressed("CROSS")) // SDLK_x  = 0x80
            controller_input_buffer |= 0x80;

        //=====IF CONTROLLER NOT WORKING ,UNCOMMENT THIS AND SEE INPUT_BUFFER IN REAL TIME!
        // Serial.println(controller_input_buffer);
        // Serial.println("Frame rendered");
        frames++;
        RENDER_ENABLED = false;
    }
}

void IRAM_ATTR Core0Loop(void *parameter)
{
    // esp_task_wdt_deinit(); // disable task WDT
    for (;;)
    {
        start_cycles_d = xthal_get_ccount();
        for (int i = 0; i <= 10000; i++)
        {
            bus_clock_t();
        }
        end_cycles_d = xthal_get_ccount();
        uint32_t my_cycles = end_cycles_d - start_cycles_d;
        Serial.printf("On average %u ESP cycles/NES cycle\n", my_cycles / 100000);

        vTaskDelay(1); // keep the watchdog happy!
    }
}
