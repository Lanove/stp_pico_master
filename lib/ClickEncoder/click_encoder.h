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
    int16_t get_value(void);

    // Button handling
    Button get_button(void);
    void set_double_click_time(uint16_t ms) { button_double_click_time = ms; }
    void set_hold_time(uint16_t ms) { button_hold_time = ms; }
    void set_double_click_enabled(bool d) { double_click_enabled = d; }
    void set_button_held_enabled(bool d) { button_held_enabled = d; }
    void set_enable_acceleration(bool a) { acceleration_enabled = a; }
    void set_acceleration_properties(int inc, int dec, int top) {
        accel_inc = inc;
        accel_dec = dec;
        accel_top = top;
    }

private:
    uint8_t pin_a, pin_b, pin_btn;
    bool pins_active;
    int32_t delta;
    int32_t last;
    uint16_t steps;
    int32_t acceleration;
    bool acceleration_enabled;

    // Button state
    volatile Button button;
    bool double_click_enabled = true;
    bool button_held_enabled = true;
    uint32_t button_hold_time = BTN_HOLDTIME;
    uint32_t button_double_click_time = BTN_DOUBLECLICKTIME;
    absolute_time_t last_button_check = nil_time;
    uint32_t keydown_ticks = 0;
    uint32_t doubleclick_ticks = 0;

    int accel_inc = 200;
    int accel_dec = 5;
    int accel_top = 16000;

    bool get_pin_state();
    static const int8_t encoder_table[16];
};