/**
 * @file ili9486_drivers.h
 * @brief ILI9486 SPI driver for RP2040
 */
#ifndef _ILI9486_DRIVERS_H_
#define _ILI9486_DRIVERS_H_

#include "hardware/dma.h"
#include "hardware/spi.h"
#include "hardware/irq.h"
#include "ili9486_commands.h"

#include "pico/stdlib.h"
#include <stdio.h>

// Panel parameters
static constexpr uint16_t panel_width = 320;
static constexpr uint16_t panel_height = 480;

enum Rotations { PORTRAIT, LANDSCAPE, INVERTED_PORTRAIT, INVERTED_LANDSCAPE };

class ILI9486Drivers {
public:
  ILI9486Drivers(spi_inst_t *spi, uint8_t pin_cs, uint8_t pin_dc,
                  uint8_t pin_rst, uint8_t mosi, uint8_t miso, uint8_t sck,
                  uint32_t spi_clock = 10 * 1000 * 1000, bool dma_used = false);
  void init();
  void set_address_window(int32_t x, int32_t y, int32_t w, int32_t h);
  void set_window(int32_t x0, int32_t y0, int32_t x1, int32_t y1);
  uint16_t create_565_color(uint8_t r, uint8_t g, uint8_t b);
  uint32_t create_888_color(uint8_t r, uint8_t g, uint8_t b);
  uint32_t create_666_color(uint8_t r, uint8_t g, uint8_t b);
  void push_colors_dma(uint16_t *colors, uint32_t len);
  void fill_screen(uint32_t color);
  void push_colors(uint32_t *color, uint32_t len);
  void push_colors_dma(uint32_t *colors, uint32_t len);
  void draw_pixel(int16_t x, int16_t y, uint32_t color);
  void dma_init(void (*onComplete_cb)(void));
  bool is_dma_used() { return dma_used; }


  // DMA control functions
  __force_inline bool dma_busy() { return dma_channel_is_busy(dma_tx_channel); };
  __force_inline void dma_wait() {
    dma_channel_wait_for_finish_blocking(dma_tx_channel);
  };
  __force_inline void dma_clear_irq() { dma_hw->ints0 = 1u << dma_tx_channel; }

  // Display properties
  __force_inline uint16_t width() { return _width; }
  __force_inline uint16_t height() { return _height; }
  void set_rotation(Rotations rotation);

  // Transaction control
  void start_transaction();
  void end_transaction();

  void write_command(uint8_t cmd);
  void write_data(uint8_t data);
  void push_block(uint32_t color, uint32_t len);


private:
  spi_inst_t *spi;
  uint8_t pin_cs, pin_dc, pin_rst, pin_mosi, pin_miso, pin_sck;
  uint16_t _width, _height;
  Rotations _rot = PORTRAIT;

  // DMA configuration
  int dma_tx_channel;
  dma_channel_config dma_tx_config;
  bool dma_used = false;
  uint32_t spi_clock;
};

#endif