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
It Took 100 ESP Cycles/CPU Cycle on average! 
It now takes 80 ESP Cycles/CPU Cycle on average, after changing the CPU core to be more efficient - and less readable. 

It takes around ~566952 ESP cycles / scanline rendered! => More than half the CPU time is taken by PPU rendering, which is ok.
if i get the CPU emulation to take only the other half of the CPU time, then this can run in 60FPS;
*/

// Rendering scanline then running the cpu works as most interaction between CPU and PPU happens during vblank, when the PPU is doing nothing
// This may cause small glitches as CPU can stop mid execution of an instruction. Remember, the CPU is not emulated
// CYCLE-perfect, in the sense that when it fetches an instruction it calculates how long it would take, wait for that ammount ofcycles and then
// run the instruction.
uint64_t cpu_cycles_avg = 0;
uint32_t cpu_cycles_cnt = 0;
uint32_t scanline_avg = 0;
inline void bus_clock_t()
{
    // start_cycles_d = xthal_get_ccount();
    ppu_render_scanline();
    // end_cycles_d = xthal_get_ccount();
    // scanline_avg += end_cycles_d - start_cycles_d;
    // it should go from (0, 341/3) but since 341/3 = 113.66666, we round to 113. Rounding to 114 causes a small glitch on the
    // scanline when sprite0 hit happens ()
    //start_cycles_d = xthal_get_ccount();
    for (int i = 0; i < 113; i++)
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
    
    // end_cycles_d = xthal_get_ccount();
    // uint32_t my_cycles = end_cycles_d - start_cycles_d;
    // cpu_cycles_cnt += 113;
    // cpu_cycles_avg += my_cycles;
    // Serial.printf("On average %u ESP cycles/CPU Cycle\n", my_cycles/113);
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
    if (!cartridge_read_file("/Pacman.nes"))
        exit(-1);

    Serial.println("READ FILE SUCCESFULLY\n");

    // This initializes the eulator!
    screen_init();
    bus_init();
    cpu_init();
    cpu_reset();
    ppu_init();
    uint32_t frames = 0;

    xTaskCreatePinnedToCore(Core0Loop, "Core0Loop", 10000, NULL, configMAX_PRIORITIES - 1, &Core0Task, 0);

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

        if(gamepad.buttonIsPressed("TRIANGLE"))
            cpu_debug_print = !cpu_debug_print;

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

        for (int i = 0; i <= 500; i++)
        {
            //            start_cycles_d = xthal_get_ccount();
            bus_clock_t();
            // end_cycles_d = xthal_get_ccount();
            // uint32_t my_cycles = end_cycles_d - start_cycles_d;
            // Serial.printf("On average %u ESP cycles/NES Scanline\n", my_cycles);
        }
        
        //Serial.printf("On average %u ESP cycles/CPU Cycle\n", cpu_cycles_avg / cpu_cycles_cnt);
        // Serial.printf("On average %u ESP cycles/Scanline\n", scanline_avg / 5000);
        vTaskDelay(1); // keep the watchdog happy!
    }
}
