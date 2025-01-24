#include "hardware/clocks.h"
#include "hardware/pll.h"
#include "hardware/structs/clocks.h"
#include "hardware/structs/pll.h"
#include "hardware/structs/rosc.h"
#include "ili9486_drivers.h"
#include "lv_app.h"
#include "lv_drivers.h"
#include "pico/stdlib.h"
#include "pico/time.h"
#include <stdio.h>
#include <stdlib.h>

uint8_t tft_tx = 11;
uint8_t tft_rx = 8;
uint8_t tft_sck = 10;

uint8_t tft_cs = 13;
uint8_t tft_dc = 12;
uint8_t tft_reset = 9;
spi_inst_t *tft_spi = spi1;

constexpr int processor_mhz = 125;

ili9486_drivers tft(tft_spi, tft_cs, tft_dc, tft_reset, tft_tx, tft_rx, tft_sck,
                    62.5 * 1000 * 1000);

static repeating_timer lv_tick_timer;

static uint32_t rp2040_tick_cb() {
  return to_ms_since_boot(get_absolute_time());
}

int main() {
  stdio_init_all();
  sleep_ms(2000);
  if (!set_sys_clock_khz(processor_mhz * 1000, false))
    printf("set system clock to %dMHz failed\n", processor_mhz);
  else
    printf("system clock is now %dMHz\n", processor_mhz);
  clock_configure(clk_peri,
                  0, // No glitchless mux
                  CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLK_SYS,
                  processor_mhz * 1000 * 1000,  // processor_mhz MHz
                  processor_mhz * 1000 * 1000); // processor_mhz MHz
  tft.init();
  uint32_t clk_freq = clock_get_hz(clk_peri);
  printf("SPI1 source clock frequency: %u Hz\n", clk_freq);
  clk_freq = spi_get_baudrate(tft_spi);
  printf("SPI1 clock frequency: %u Hz\n", clk_freq);
  //   tft.fillScreen(tft.create565Color(255, 0, 0));
  int r = 0;
  int g = 0;
  int b = 0;
  add_repeating_timer_ms(
      5,
      [](struct repeating_timer *t) -> bool {
        lv_tick_inc(5);
        return true;
      },
      NULL, &lv_tick_timer);
  tft.setRotation(LANDSCAPE);
  lvgl_display_init(tft);
  lvgl_app::app_entry();
  // lv_tick_set_cb(rp2040_tick_cb);
  while (true) {
    lv_timer_handler();
    sleep_ms(5);
  }
}
