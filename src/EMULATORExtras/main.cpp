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
// An amazing resource for hooking up PS2 controllers to other devices
// http://www.billporter.info/2010/06/05/playstation-2-controller-arduino-library-v1-0/
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
void ps2Task(void *pvParameters);

volatile bool profiling;
/*
It took 80 ESP Cycles/CPU Cycle on average, after changing the CPU core to be more efficient - and less readable.
It now takes 69-70 ESP cycles/CPU cycle but DMA is done separately and not meassured here.
It seems that the CPU is no longer the bottleneck.


It takes around ~47695 ESP cycles / scanline rendered! => More than half the (ESP32S3's) CPU time is taken by PPU rendering, which is ok.
if i get the CPU emulation to take only the other half of the CPU time, then this can run in 60FPS;
*/

// Rendering scanline then running the cpu works as most interaction between CPU and PPU happens during vblank, when the PPU is doing nothing
// This may cause small glitches as CPU can stop mid execution of an instruction. Remember, the emulator is NOT
// CYCLE-perfect and it doesn't aim to be on such a platform!

uint64_t cpu_cycles_avg = 0;
uint32_t cpu_cycles_cnt = 0;
uint32_t scanline_avg = 0;

bool leap_cycle = false;
inline void bus_clock_t()
{
    start_cycles_d = xthal_get_ccount();
    ppu_render_scanline();
    end_cycles_d = xthal_get_ccount();
    scanline_avg += end_cycles_d - start_cycles_d;

    // It takes 341 PPU cycles to render a scanline. PPU runs three times faster than the CPU.
    // it should go from (0, 341/3) but since 341/3 = 113.66666, we round to 113. Rounding to 114 causes a small glitch on the
    // To fix this, we do a leap cycle every 226 cycles
    // Kinda like the calendar does :)))
    start_cycles_d = xthal_get_ccount();
    for (int i = 0; i < 113 + leap_cycle; i++)
    {
        cpu_clock();
        leap_cycle != leap_cycle;
    }
    end_cycles_d = xthal_get_ccount();
    uint32_t my_cycles = end_cycles_d - start_cycles_d;
    cpu_cycles_cnt += 113;
    cpu_cycles_avg += my_cycles;
}

void setup()
{
    Serial.begin(115200);
    if (!LittleFS.begin(true))
    {
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

    // Reads GAME from Cartridge and loads it into RAM
    if (!cartridge_read_file("/smb.bin"))
        exit(-1);

    Serial.println("READ FILE SUCCESFULLY\n");

    // This initializes the emulator (obviously!)
    screen_init();
    bus_init();
    cpu_init();
    cpu_reset();
    ppu_init();

    uint32_t frames = 0;

    xTaskCreatePinnedToCore(Core0Loop, "Core0Loop", 10000, NULL, configMAX_PRIORITIES - 1, &Core0Task, 0);
    xTaskCreatePinnedToCore(ps2Task, "PS2_Task", 4096, &gamepad, 1, NULL, 1);
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

        controller_input_buffer = get_controller_input(&gamepad);

        // if(gamepad.buttonIsPressed("TRIANGLE"))
        //     cpu_debug_print = !cpu_debug_print;

        if (gamepad.buttonIsPressed("CIRCLE"))
        {
            profiling = !profiling;
        }
        frames++;
        RENDER_ENABLED = false;
    }
}

void ps2Task(void *pvParameters)
{
    PS2 *controller = (PS2 *)pvParameters;
    for (;;)
    {
        controller->readController();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void IRAM_ATTR Core0Loop(void *parameter)
{
    // esp_task_wdt_deinit(); // disable task WDT -> This is supposed to
    for (;;)
    {

        for (int i = 0; i <= 500; i++)
        {
            bus_clock_t();
        }

        if (profiling)
        {
            Serial.printf("On average %u ESP cycles/CPU Cycle\n", cpu_cycles_avg / cpu_cycles_cnt);
            Serial.printf("On average %u ESP cycles/Scanline\n", scanline_avg / 500);
            scanline_avg = 0;
        }
       
        vTaskDelay(1); // keep the watchdog happy!
    }
}
