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
void draw_menu();
void load_selected_game();
volatile bool profiling;

// MENU SYSTEM
enum AppState
{
    STATE_MENU,
    STATE_GAME,
    STATE_LOADING
};

volatile AppState currentState = STATE_MENU;

struct GameEntry
{
    const char *name;
    const char *path;
};

const int GAME_COUNT = 5;
GameEntry games[GAME_COUNT] = {
    {"Pac-Man", "/Pacman.nes"},
    {"Super Mario Bros", "/smb.bin"},
    {"DonkeyKong", "/donkeykong.nes"},
    {"Balloon_Fight", "/Balloon_fight.nes"},
    {"Tetris", "/tetris_cnrom.nes"}};

int selectedGameIdx = 0;
bool menuRedrawNeeded = true;
uint32_t lastInputTime = 0;

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

    // WiFi and Bt cleanup for better FPS
    btStop();
    WiFi.mode(WIFI_OFF);

    // Hardware Initialization
    gamepad.begin();
    delay(500);
    tft.init();
    tft.setRotation(0);
    tft.fillScreen(TFT_BLACK);
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

    currentState = STATE_MENU;
    menuRedrawNeeded = true;
}

uint32_t previous_time_s = 0;
uint32_t current_time_s = 0;

void loop()
{
    if (currentState == STATE_MENU)
    {
        if (menuRedrawNeeded)
        {
            draw_menu();
            menuRedrawNeeded = false;
        }

        if (millis() - lastInputTime > 150)
        {
            if (gamepad.buttonIsPressed(DOWN))
            {
                selectedGameIdx = (selectedGameIdx + 1) % GAME_COUNT;
                menuRedrawNeeded = true;
                lastInputTime = millis();
            }
        }
        if (gamepad.buttonIsPressed(CROSS))
        {
            load_selected_game();
            lastInputTime = millis();
        }
    }
    else if (currentState == STATE_GAME)
    {
        current_time_s = millis();
        if (current_time_s - previous_time_s >= 1000)
        {
            previous_time_s = current_time_s;
            Serial.println("FPS: " + String(frames));
            frames = 0;
        }
        controller[0] = controller_input_buffer;

        if (gamepad.buttonIsPressed(L2))
        {
            currentState = STATE_MENU;
            tft.fillScreen(TFT_BLACK);
            menuRedrawNeeded = true;
            delay(500);
            return;
        }

        if (RENDER_ENABLED)
        {
            tft.pushImage(0, 0, 256, 240, pixels);

            controller_input_buffer = get_controller_input(&gamepad);
            if (gamepad.buttonIsPressed(CIRCLE))
            {
                profiling = !profiling;
            }
            frames++;
            RENDER_ENABLED = false;
        }
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
    for (;;)
    {
        // CRITICAL FIX: Only run the emulator if we are in STATE_GAME
        if (currentState == STATE_GAME) 
        {
            for (int i = 0; i <= 500; i++)
            {
                bus_clock_t();
            }

            if (profiling)
            {
                Serial.printf("Avg ESP cycles/CPU: %u\n", cpu_cycles_avg / cpu_cycles_cnt);
                scanline_avg = 0;
            }
        } 
        else 
        {
            // If in MENU or LOADING, do nothing and wait. 
            // This prevents the CPU from reading memory while we are loading a file.
            vTaskDelay(10); 
        }
       
        vTaskDelay(1); // Keep the watchdog happy
    }
}
void load_selected_game()
{

    currentState = STATE_LOADING;
    memory_init();
    
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(10, 110);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(2);
    tft.printf("Loading %s...", games[selectedGameIdx].name);

    if (!cartridge_read_file(games[selectedGameIdx].path)) {
        tft.setCursor(10, 140);
        tft.setTextColor(TFT_RED);
        tft.print("Load Failed!");
        Serial.println("Failed to read file!");
        delay(2000);
        currentState = STATE_MENU;
        menuRedrawNeeded = true;
        return;
    }

    
    screen_init();
    bus_init();
    cpu_init();
    cpu_reset();
    ppu_init();

    Serial.println("Game loaded. Starting Emulation.");
    tft.fillScreen(TFT_BLACK);
    currentState = STATE_GAME;
}

void draw_menu() {
    tft.fillScreen(TFT_BLUE); // Retro blue background
    
    tft.setTextSize(2);
    tft.setTextColor(TFT_YELLOW, TFT_BLUE);
    tft.setCursor(50, 20);
    tft.print("NES EMULATOR");
    
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_BLUE);
    tft.setCursor(20, 220);
    tft.print("D-PAD: Move | X: Select | L2: Menu");

    int startY = 80;
    int gap = 30;

    for (int i = 0; i < GAME_COUNT; i++) {
        if (i == selectedGameIdx) {
            tft.setTextColor(TFT_BLACK, TFT_WHITE); // Highlighted
            tft.setTextSize(2);
            tft.setCursor(20, startY + (i * gap));
            tft.printf("> %s  ", games[i].name);
        } else {
            tft.setTextColor(TFT_WHITE, TFT_BLUE); // Normal
            tft.setTextSize(2);
            tft.setCursor(20, startY + (i * gap));
            tft.printf("  %s  ", games[i].name);
        }
    }
}