#pragma once

#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"

#define BTN_DOUBLECLICKTIME  400
#define BTN_HOLDTIME        1000

// Encoder types
#define ENC_NORMAL        (1 << 1)
#define ENC_FLAKY         (1 << 2)

#ifndef ENC_DECODER
#  define ENC_DECODER     ENC_NORMAL
#endif

class ClickEncoder {
public:
    enum Button {
        Open = 0,
        Closed,
        Pressed,
        Held,
        Released,
        Clicked,
        DoubleClicked
    };

    ClickEncoder(uint8_t pinA, uint8_t pinB, uint8_t pinBTN = 0xFF, 
                uint8_t stepsPerNotch = 1, bool active = 0);
    
    void init();
    void service(void);
    int16_t getValue(void);

    // Button handling
    Button getButton(void);
    void setDoubleClickTime(uint16_t ms) { buttonDoubleClickTime = ms; }
    void setHoldTime(uint16_t ms) { buttonHoldTime = ms; }
    void setDoubleClickEnabled(bool d) { doubleClickEnabled = d; }
    void setButtonHeldEnabled(bool d) { buttonHeldEnabled = d; }

private:
    uint8_t pinA, pinB, pinBTN;
    bool pinsActive;
    volatile int16_t delta;
    volatile int16_t last;
    volatile uint8_t steps;
    volatile uint16_t acceleration;
    bool accelerationEnabled;

    // Button state
    volatile Button button;
    bool doubleClickEnabled = true;
    bool buttonHeldEnabled = true;
    uint16_t buttonHoldTime = BTN_HOLDTIME;
    uint16_t buttonDoubleClickTime = BTN_DOUBLECLICKTIME;
    absolute_time_t lastButtonCheck = nil_time;
    uint32_t keyDownTicks = 0;
    uint32_t doubleClickTicks = 0;

    bool getPinState();
    static const int8_t encoderTable[16];
};