#ifndef PS2_CONTROLLER_HEADERGUARD
#define PS2_CONTROLLER_HEADERGUARD
#include <Arduino.h>

/*
This file only includes input handling for buttons - no need for analog stuff
This is simulated SPI and that is not an issue as we have a lot of headroom on Core1
*/


enum buttons_t{
    SELECT,
    L3,
    R3,
    START,
    UP,
    RIGHT,
    DOWN,
    LEFT,
    L2,
    R2, 
    L1,
    R1,
    TRIANGLE,
    CIRCLE,
    CROSS,
    SQUARE
};


class PS2 {
    public:
        PS2(int datPin, int cmdPin, int attPin, int clkPin, int delayUS = 50);
        void begin();
        void readController();
        bool buttonIsPressed(buttons_t btn);
        
    public:
        int PS2_DAT, PS2_CMD, PS2_ATT, PS2_CLK;
        int PS2_DELAY;
        
        byte buttonsLow;
        byte buttonsHigh;

        byte shiftInOut(byte dataOut);
};

byte get_controller_input(PS2* gamepad);
#endif