#include "ClickEncoder.h"

#define ENC_ACCEL_TOP 16000 // max. acceleration: *12 (val >> 8)
#define ENC_ACCEL_INC 200
#define ENC_ACCEL_DEC 5

const int8_t ClickEncoder::encoderTable[16] = {0, 0, -1, 0, 0, 0,  0, 1,
                                               1, 0, 0,  0, 0, -1, 0, 0};

ClickEncoder::ClickEncoder(uint8_t A, uint8_t B, uint8_t BTN,
                           uint8_t stepsPerNotch, bool active)
    : pinA(A), pinB(B), pinBTN(BTN), pinsActive(active), delta(0), last(0),
      steps(stepsPerNotch), acceleration(0), accelerationEnabled(false),
      button(Open) {}

void ClickEncoder::init() {

  if (pinA != 0xFF) {
    gpio_init(pinA);
    gpio_set_dir(pinA, GPIO_IN);
    gpio_pull_up(pinA);
  }

  if (pinB != 0xFF) {
    gpio_init(pinB);
    gpio_set_dir(pinB, GPIO_IN);
    gpio_pull_up(pinB);
  }

  if (pinBTN != 0xFF) {
    gpio_init(pinBTN);
    gpio_set_dir(pinBTN, GPIO_IN);
    gpio_pull_up(pinBTN);
  }

  // Initial state
  last = (gpio_get(pinA) == pinsActive) ? 3 : 0;
  last |= (gpio_get(pinB) == pinsActive) ? 1 : 0;
}

void ClickEncoder::service() {
  // Encoder handling
  bool moved = false;

  if (pinA >= 0 && pinB >= 0) {
    if (accelerationEnabled) { // decelerate every tick
      acceleration -= ENC_ACCEL_DEC;
      if (acceleration & 0x8000) { // handle overflow of MSB is set
        acceleration = 0;
      }
    }
    int8_t curr = 0;

    // Read current state of pins
    if (gpio_get(pinA) == pinsActive) {
      curr |= 2; // Set bit 1 (A pin)
    }
    if (gpio_get(pinB) == pinsActive) {
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
      if (accelerationEnabled && acceleration <= (16000 - 200)) {
        acceleration += 200;
      }
    }
  }

  // Button handling
  if (pinBTN != 0xFF) {
    absolute_time_t now = get_absolute_time();
    uint64_t timeDiff = absolute_time_diff_us(lastButtonCheck, now) / 1000;

    if (timeDiff >= 10) {
      lastButtonCheck = now;
      bool btnState = gpio_get(pinBTN) == pinsActive;

      if (btnState) {
        keyDownTicks++;
        if (keyDownTicks > (buttonHoldTime / 10) && buttonHeldEnabled) {
          button = Held;
        }
      } else {
        if (keyDownTicks > 1) {
          if (button == Held) {
            button = Released;
            doubleClickTicks = 0;
          } else {
            if (doubleClickTicks > 0 &&
                doubleClickTicks < (buttonDoubleClickTime / 10)) {
              button = DoubleClicked;
              doubleClickTicks = 0;
            } else {
              doubleClickTicks = buttonDoubleClickTime / 10;
            }
          }
        }
        keyDownTicks = 0;
      }

      if (doubleClickTicks > 0) {
        doubleClickTicks--;
        if (doubleClickTicks == 0) {
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

int16_t ClickEncoder::getValue() {
  int16_t val = delta;
  delta = 0;

  // Apply acceleration
  if (accelerationEnabled) {
    val *= 1 + (acceleration >> 8);
  }

  return val;
}

ClickEncoder::Button ClickEncoder::getButton() {
  Button ret = button;
  if (button != Held && ret != Open) {
    button = Open;
  }
  return ret;
}