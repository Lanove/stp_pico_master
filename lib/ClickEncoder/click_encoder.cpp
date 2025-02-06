#include "click_encoder.h"

const int8_t ClickEncoder::encoder_table[16] = {0, 0, -1, 0, 0, 0,  0, 1,
                                               1, 0, 0,  0, 0, -1, 0, 0};

ClickEncoder::ClickEncoder(uint8_t A, uint8_t B, uint8_t BTN,
                           uint8_t stepsPerNotch, bool active)
    : pin_a(A), pin_b(B), pin_btn(BTN), pins_active(active), delta(0), last(0),
      steps(stepsPerNotch), acceleration(0), acceleration_enabled(false),
      button(Open) {}

void ClickEncoder::init() {

  if (pin_a != 0xFF) {
    gpio_init(pin_a);
    gpio_set_dir(pin_a, GPIO_IN);
    gpio_pull_up(pin_a);
  }

  if (pin_b != 0xFF) {
    gpio_init(pin_b);
    gpio_set_dir(pin_b, GPIO_IN);
    gpio_pull_up(pin_b);
  }

  if (pin_btn != 0xFF) {
    gpio_init(pin_btn);
    gpio_set_dir(pin_btn, GPIO_IN);
    gpio_pull_up(pin_btn);
  }

  // Initial state
  last = (gpio_get(pin_a) == pins_active) ? 3 : 0;
  last |= (gpio_get(pin_b) == pins_active) ? 1 : 0;
}

void ClickEncoder::service() {
  // Encoder handling
  bool moved = false;

  if (pin_a >= 0 && pin_b >= 0) {
    if (acceleration_enabled) { // decelerate every tick
      acceleration -= accel_dec;
      if(acceleration < 0)
        acceleration = 0;
    }
    int8_t curr = 0;

    // Read current state of pins
    if (gpio_get(pin_a) == pins_active) {
      curr |= 2; // Set bit 1 (A pin)
    }
    if (gpio_get(pin_b) == pins_active) {
      curr |= 1; // Set bit 0 (B pin)
    }

    // State transition table for quadrature decoding
    // Each mechanical step corresponds to a full cycle of 4 states
    // The table maps (last << 2 | curr) to delta change
    static const int8_t transitionTable[16] = {
        0,  -1, 1,  0,  // Previous state 0b00
        1,  0,  0,  -1, // Previous state 0b01
        -1, 0,  0,  1,  // Previous state 0b10
        0,  1,  -1, 0   // Previous state 0b11
    };

    // Calculate the transition index
    int8_t transition = transitionTable[(last << 2) | curr];

    // If a valid transition occurred, update delta and state
    if (transition != 0) {
      delta += transition; // Add ±1 per mechanical step
      last = curr;         // Update the last state
      moved = true;        // Flag that movement occurred

      // Handle acceleration if enabled
      if (acceleration_enabled && acceleration <= (accel_top - accel_inc)) {
        acceleration += accel_inc;
      }
    }
  }

  // Button handling
  if (pin_btn != 0xFF) {
    absolute_time_t now = get_absolute_time();
    uint64_t timeDiff = absolute_time_diff_us(last_button_check, now) / 1000;

    if (timeDiff >= debounce_time) {
      last_button_check = now;
      bool btnState = gpio_get(pin_btn) == pins_active;

      if (btnState) {
        keydown_ticks++;
        if (keydown_ticks > (button_hold_time / 10) && button_held_enabled) {
          button = Held;
        }
      } else {
        if (keydown_ticks > 1) {
          if (button == Held) {
            button = Released;
            doubleclick_ticks = 0;
          } else {
            if (doubleclick_ticks > 0 &&
                doubleclick_ticks < (button_double_click_time / 10)) {
              button = DoubleClicked;
              doubleclick_ticks = 0;
            } else {
              doubleclick_ticks = button_double_click_time / 10;
            }
          }
        }
        keydown_ticks = 0;
      }

      if (doubleclick_ticks > 0) {
        doubleclick_ticks--;
        if (doubleclick_ticks == 0) {
          button = Clicked;
        }
      }
    }
  }

  // Decay acceleration
  if (acceleration > 5)
    acceleration -= 5;
  else
    acceleration = 0;
}

int16_t ClickEncoder::get_value() {
  int16_t val = delta;
  delta = 0;

  // Apply acceleration
  if (acceleration_enabled) {
    val *= 1 + (acceleration >> 4);
  }

  return val;
}

ClickEncoder::Button ClickEncoder::get_button() {
  Button ret = button;
  if (button != Held && ret != Open) {
    button = Open;
  }
  return ret;
}