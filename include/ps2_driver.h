#ifndef PS2_CONTROLLER_HEADERGUARD
#define PS2_CONTROLLER_HEADERGUARD


/*
This file only includes input handling for buttons - no need for analog stuff
This is simulated SPI and that is not an issue as we have a lot of headroom on Core1
*/
#include <Arduino.h>
class PS2 {
    public:
        PS2(int datPin, int cmdPin, int attPin, int clkPin, int delayUS = 50);
        void begin();
        void readController();
        bool buttonIsPressed(String btn);
        
    public:
        int PS2_DAT, PS2_CMD, PS2_ATT, PS2_CLK;
        int PS2_DELAY;
        
        byte buttonsLow;
        byte buttonsHigh;

        byte shiftInOut(byte dataOut);
};

byte get_controller_input(PS2* gamepad);
#endif