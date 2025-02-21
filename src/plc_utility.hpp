#ifndef PLC_UTILITY_HPP
#define PLC_UTILITY_HPP

#include <cstdint>
#include <cstdlib>
#include <cstring>

#include "pico/stdlib.h"
#include "pico/time.h"

// Flip-flop types using differential signals

class Differential_Down {
  bool clk = false;
  bool q   = false;

 public:
  void CLK(bool val) {
    q   = clk && !val;
    clk = val;
  }
  bool Q() { return q; }
};

class Differential_Up {
  bool clk = true;
  bool q   = true;

 public:
  void CLK(bool val) {
    q   = !clk && val;
    clk = val;
  }
  bool Q() { return q; }
};

// Stopwatch class using Pico's time functions.
// Time is measured in microseconds and converted to seconds (double).
class Stopwatch {
  enum State { RESET, RUNNING, STOPPED };
  uint64_t startTime = 0, stopTime = 0;
  State    state = RESET;

 public:
  Stopwatch() { reset(); }
  bool isRunning() { return state == RUNNING; }
  void start() {
    uint64_t t = time_us_64();
    if (state == RESET || state == STOPPED) {
      state = RUNNING;
      // When restarting after a stop, adjust the start time to discount the pause.
      startTime += (t - stopTime);
      stopTime = t;
    }
  }
  void stop() {
    if (state == RUNNING) {
      state    = STOPPED;
      stopTime = time_us_64();
    }
  }
  void reset() {
    state     = RESET;
    startTime = 0;
    stopTime  = 0;
  }
  // Returns elapsed time in seconds
  double elapsed() {
    if (state == RUNNING)
      return (time_us_64() - startTime) / 1e6;
    return (stopTime - startTime) / 1e6;
  }
};

// Pulse Contact: toggles its internal state every duration/2 seconds.
class PulseContact {
  uint64_t lastTime = 0;
  double   duration = 0;  // seconds
  bool     state    = false;

 public:
  /**
     * @brief Construct a new Pulse Contact object.
     * @param _duration Duration of the full pulse period in seconds.
     */
  PulseContact(double _duration) : duration(_duration) { lastTime = time_us_64(); }
  void service() {
    uint64_t currentTime = time_us_64();
    double   elapsed     = (currentTime - lastTime) / 1e6;
    if (elapsed >= duration / 2.0) {
      state    = !state;
      lastTime = currentTime;
    }
  }
  void setDuration(double _duration) { duration = _duration; }
  void resetCounter() { lastTime = time_us_64(); }
  bool Q() { return state; }
};

// Shift register (SFT) – note: behavior is similar to the original code.
// Uses dynamic allocation; make sure to free the memory (or better, refactor to use std::vector).
class SFT {
  uint16_t* _data;
  bool      clk  = true;
  bool      rclk = true;
  uint8_t   _len;

 public:
  SFT(uint8_t len) : _len(len) {
    if (_len >= 10)
      _len = 10;
    _data = (uint16_t*) malloc(sizeof(uint16_t) * _len);
    memset(_data, 0, sizeof(uint16_t) * _len);
  }
  ~SFT() {
    if (_data)
      free(_data);
  }
  bool IN(bool din, bool clock, bool reset) {
    bool q = (!clk && clock);
    if (q) {  // Differential Up clock
      bool ro[_len];
      for (uint8_t i = 0; i < _len; i++) {
        ro[i] = Q_bit((16 * (i + 1)) - 1);
        _data[i] <<= 1;
      }
      _data[0] |= din;
      if (_len > 1) {
        for (uint8_t i = 1; i < _len; i++) _data[i] |= ro[i - 1];
      }
    }
    if (!rclk && reset) {  // Differential Up Reset
      memset(_data, 0, sizeof(uint16_t) * _len);
      q = true;
    }
    clk  = clock;
    rclk = reset;
    return q;
  }
  bool     Q_bit(uint8_t pos) { return (_data[pos / 16] & (0x01 << (pos % 16))) >> (pos % 16); }
  uint16_t data(uint8_t pos) { return _data[pos]; }
  uint8_t  len() { return _len; }
};

// On-delay timer (TON): Q becomes true if DIN is held true for at least PT seconds.
class TON {
  double    pt;  // Preset time in seconds
  bool      input = false;
  Stopwatch sw;

 public:
  TON(double _pt) : pt(_pt) {}
  void setPT(double _pt) {
    pt = _pt;
    sw.reset();
  }
  double getPT() { return pt; }
  // The timer counts while DIN is true; it resets when DIN goes false or when reset is true.
  void IN(bool din, bool reset = false) {
    if (din && !input)
      sw.start();
    if (!din && input)
      sw.reset();
    input = din;
    if (reset)
      sw.reset();
  }
  // Q is true if the elapsed time exceeds the preset time.
  bool Q() { return sw.elapsed() > pt; }
  // ET returns the elapsed time (capped at PT)
  double ET() { return (Q()) ? pt : sw.elapsed(); }
};

// Off-delay timer (TOFF): Q is initially on when DIN rises and stays on for PT seconds after DIN goes false.
class TOFF {
  Stopwatch sw;
  bool      input = false;
  bool      q     = false;
  double    et    = 0.0;
  double    pt;  // Preset time in seconds

  void CheckElapsedTime() {
    et = sw.elapsed();
    if (et > pt) {
      q  = false;
      et = pt;
    }
  }

 public:
  TOFF(double _pt) : pt(_pt) {}
  void   setPT(double _pt) { pt = _pt; }
  double getPT() { return pt; }
  // When DIN falls, the timer starts counting; if DIN rises, the timer resets.
  void IN(bool din, bool reset = false) {
    if (!input && din) {
      sw.reset();
      q = true;
    }
    if (input && !din)
      sw.start();
    input = din;
    if (reset)
      sw.reset();
  }
  bool Q() {
    CheckElapsedTime();
    return q;
  }
  double ET() {
    CheckElapsedTime();
    return et;
  }
};

// Pulsed timer (TP): Q turns on at a rising edge of DIN and remains on for PT seconds.
class TP {
  Stopwatch sw;
  bool      input = false;
  double    et    = 0.0;
  double    pt;  // Preset time in seconds

  void CheckElapsedTime() {
    if (sw.isRunning())
      et = sw.elapsed();
    if (et > pt) {
      sw.reset();
      et = pt;
    }
  }

 public:
  TP(double _pt) : pt(_pt) {}
  double getPT() { return pt; }
  void   setPT(double _pt) { pt = _pt; }
  // A rising edge on DIN starts the timer. If reset is true, the timer is reset.
  void IN(bool din, bool reset = false) {
    if (reset)
      sw.reset();
    if (sw.isRunning())
      return;
    if (!input && din && !sw.isRunning())
      sw.start();
    if (input && !din && !sw.isRunning())
      et = 0.0;
    input = din;
    if (reset)
      sw.reset();
  }
  // Q is true while the timer is counting.
  bool Q() {
    CheckElapsedTime();
    return sw.isRunning();
  }
  // ET returns the elapsed time (capped at PT).
  double ET() {
    CheckElapsedTime();
    return et;
  }
};

#endif  // PLC_UTILITY_HPP
