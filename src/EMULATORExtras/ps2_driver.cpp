#include "ps2_driver.h"
#include <Arduino.h>

PS2::PS2(int datPin, int cmdPin, int attPin, int clkPin, int delayUS) : PS2_DAT(datPin), PS2_CMD(cmdPin), PS2_ATT(attPin), PS2_CLK(clkPin), PS2_DELAY(delayUS),
                                                                        buttonsLow(0xFF), buttonsHigh(0xFF) {}

void PS2::begin()
{
    pinMode(PS2_DAT, INPUT_PULLUP); // MISO
    pinMode(PS2_CMD, OUTPUT);       // MOSI
    pinMode(PS2_ATT, OUTPUT);       // CS active on 0
    pinMode(PS2_CLK, OUTPUT);       // SCLK

    digitalWrite(PS2_ATT, HIGH);
    digitalWrite(PS2_CMD, HIGH);
    digitalWrite(PS2_CLK, HIGH);
}

byte PS2::shiftInOut(byte dataOut)
{
    byte dataIn = 0;
    for (byte i = 0; i < 8; i++)
    {
        digitalWrite(PS2_CMD, (dataOut & (1 << i)) ? HIGH : LOW);
        digitalWrite(PS2_CLK, LOW);

        // Yes, this delay is blocking, but since it runs on a separate thread it should not be an issue.
        delayMicroseconds(PS2_DELAY);

        if (digitalRead(PS2_DAT))
            dataIn |= (1 << i);

        digitalWrite(PS2_CLK, HIGH);
        delayMicroseconds(PS2_DELAY);
    }
    return dataIn;
}
bool PS2::buttonIsPressed(buttons_t btn)
{
    switch (btn)
    {
    // Low Byte buttons
    case SELECT:
        return !(buttonsLow & 0x01);
    case L3:
        return !(buttonsLow & 0x02);
    case R3:
        return !(buttonsLow & 0x04);
    case START:
        return !(buttonsLow & 0x08);
    case UP:
        return !(buttonsLow & 0x10);
    case RIGHT:
        return !(buttonsLow & 0x20);
    case DOWN:
        return !(buttonsLow & 0x40);
    case LEFT:
        return !(buttonsLow & 0x80);

    // High Byte buttons
    case L2:
        return !(buttonsHigh & 0x01);
    case R2:
        return !(buttonsHigh & 0x02);
    case L1:
        return !(buttonsHigh & 0x04);
    case R1:
        return !(buttonsHigh & 0x08);
    case TRIANGLE:
        return !(buttonsHigh & 0x10);
    case CIRCLE:
        return !(buttonsHigh & 0x20);
    case CROSS:
        return !(buttonsHigh & 0x40);
    case SQUARE:
        return !(buttonsHigh & 0x80);

    default:
        return false;
    }
}

void PS2::readController()
{
    digitalWrite(PS2_ATT, LOW);

    shiftInOut(0x01);
    shiftInOut(0x42);
    shiftInOut(0x00);
    buttonsLow = shiftInOut(0x00);
    buttonsHigh = shiftInOut(0x00);

    digitalWrite(PS2_ATT, HIGH);
}

byte get_controller_input(PS2 *gamepad)
{
    byte controller_input_buffer;
    if (gamepad->buttonIsPressed(UP))
        controller_input_buffer |= 0x08;

    if (gamepad->buttonIsPressed(DOWN))
        controller_input_buffer |= 0x04;

    if (gamepad->buttonIsPressed(LEFT))
        controller_input_buffer |= 0x02;

    if (gamepad->buttonIsPressed(RIGHT))
        controller_input_buffer |= 0x01;

    if (gamepad->buttonIsPressed(START))
        controller_input_buffer |= 0x10;

    if (gamepad->buttonIsPressed(SELECT))
        controller_input_buffer |= 0x20;

    if (gamepad->buttonIsPressed(SQUARE))
        controller_input_buffer |= 0x40;

    if (gamepad->buttonIsPressed(CROSS))
        controller_input_buffer |= 0x80;

    return controller_input_buffer;
}