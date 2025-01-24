#ifndef XPT2046_H
#define XPT2046_H

#include "hardware/spi.h"
#include "ili9486_drivers.h"
#include "pico/stdlib.h"

class XPT2046 {
public:
  XPT2046(spi_inst_t *spi, uint mosi, uint miso, uint sck, uint cs,
          uint irq = 0);

  void begin(Rotations rotation);
  bool isTouched();
  bool getTouch(uint16_t &x, uint16_t &y);
  void setCalibration(uint16_t xmin, uint16_t xmax, uint16_t ymin,
                      uint16_t ymax);
  void getRaw(uint16_t &x, uint16_t &y);
  void setRotation(Rotations rotation);

  // Interrupt handling
  void enableInterrupt();
  void disableInterrupt();
  bool getInterrupt() { return gpio_get(irq_pin); }
  void map(uint16_t x, uint16_t y, uint16_t &x_mapped, uint16_t &y_mapped);
  void getMappedRaw(uint16_t &x, uint16_t &y);

private:
  spi_inst_t *spi;
  uint cs_pin;
  uint mosi_pin, miso_pin, sck_pin;
  uint irq_pin;
  Rotations rotation;
  uint16_t width;
  uint16_t height;

  // Calibration values
  uint16_t x_min = 220, x_max = 3900;
  uint16_t y_min = 180, y_max = 3850;

  uint16_t readData(uint8_t command);
  void spiWrite(uint8_t data);
  uint16_t spiRead();
  void startTransaction();
  void endTransaction();
};

#endif