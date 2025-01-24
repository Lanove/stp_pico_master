#include "xpt2046.h"
#include "hardware/gpio.h"

XPT2046::XPT2046(spi_inst_t *spi, uint mosi, uint miso, uint sck, uint cs,
                 uint irq)
    : spi(spi), mosi_pin(mosi), miso_pin(miso), sck_pin(sck), cs_pin(cs_pin),
      irq_pin(irq) {}

void XPT2046::begin(Rotations rotation) {
  setRotation(rotation);
  // Initialize CS pin
  gpio_init(cs_pin);
  gpio_set_dir(cs_pin, GPIO_OUT);
  gpio_put(cs_pin, 1);

  gpio_set_function(mosi_pin, GPIO_FUNC_SPI);
  gpio_set_function(sck_pin, GPIO_FUNC_SPI);
  gpio_set_function(miso_pin, GPIO_FUNC_SPI);

  // Initialize IRQ pin if used
  if (irq_pin) {
    gpio_init(irq_pin);
    gpio_set_dir(irq_pin, GPIO_IN);
    gpio_pull_up(irq_pin);
  }

  // Initialize SPI
  spi_init(spi, 1000000); // 1MHz clock
  spi_set_format(spi, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
}

bool XPT2046::isTouched() {
  if (irq_pin) {
    return !gpio_get(irq_pin);
  }
  uint16_t x, y;
  return getTouch(x, y);
}

bool XPT2046::getTouch(uint16_t &x, uint16_t &y) {
  uint16_t raw_x, raw_y;
  getMappedRaw(raw_x, raw_y);
  switch (rotation) {
  case PORTRAIT:
    x = raw_y;
    y = raw_x;
    break;
  case INVERTED_PORTRAIT:
    x = width - raw_x;
    y = height - raw_y;
    break;
  case LANDSCAPE:
    x = width - raw_x;
    y = height - raw_y;
    break;
  case INVERTED_LANDSCAPE:
    x = raw_x;
    y = raw_y;
    break;
  }
  return true;
}

uint16_t XPT2046::readData(uint8_t command) {
  startTransaction();

  // Send command
  spiWrite(command);

  // Read 16-bit response (12-bit data + 4 dummy bits)
  uint16_t result = spiRead() << 5;
  result |= spiRead() >> 3;

  endTransaction();
  return result;
}

void XPT2046::startTransaction() {
  gpio_put(cs_pin, 0);
  sleep_us(1);
}

void XPT2046::endTransaction() {
  gpio_put(cs_pin, 1);
  sleep_us(1);
}

void XPT2046::spiWrite(uint8_t data) { spi_write_blocking(spi, &data, 1); }

uint16_t XPT2046::spiRead() {
  uint8_t data;
  spi_read_blocking(spi, 0x00, &data, 1);
  return data;
}

void XPT2046::setCalibration(uint16_t xmin, uint16_t xmax, uint16_t ymin,
                             uint16_t ymax) {
  x_min = xmin;
  x_max = xmax;
  y_min = ymin;
  y_max = ymax;
}

void XPT2046::map(uint16_t x, uint16_t y, uint16_t &x_mapped,
                  uint16_t &y_mapped) {
  // Handle underflow (x < x_min or y < y_min)
  if (x < x_min)
    x = x_min;
  if (y < y_min)
    y = y_min;

  // Handle overflow (x > x_max or y > y_max)
  if (x > x_max)
    x = x_max;
  if (y > y_max)
    y = y_max;

  // Map the coordinates to display dimensions
  x_mapped = static_cast<uint16_t>(static_cast<uint32_t>(x - x_min) *
                                   width / (x_max - x_min));
  y_mapped = static_cast<uint16_t>(static_cast<uint32_t>(y - y_min) *
                                   height / (y_max - y_min));

  // Clamp the mapped values to ensure they stay within display bounds
  if (x_mapped >= width)
    x_mapped = width - 1;
  if (y_mapped >= height)
    y_mapped = height - 1;
}

void XPT2046::getRaw(uint16_t &x, uint16_t &y) {
  x = readData(0x90);
  y = readData(0xD0);
}

void XPT2046::getMappedRaw(uint16_t &x, uint16_t &y) {
  uint16_t raw_x, raw_y;
  getRaw(raw_x, raw_y);
  map(raw_x, raw_y, x, y);
}

void XPT2046::setRotation(Rotations rotation) {
  this->rotation = rotation;
  bool swap_dims = false;
  switch (this->rotation) {
  case PORTRAIT:
    break;
  case LANDSCAPE:
    swap_dims = true;
    break;
  case INVERTED_PORTRAIT:
    break;
  case INVERTED_LANDSCAPE:
    swap_dims = true;
    break;
  }
  width = swap_dims ? panel_height : panel_width;
  height = swap_dims ? panel_width : panel_height;
}