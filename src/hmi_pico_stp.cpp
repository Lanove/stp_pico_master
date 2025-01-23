#include "hardware/clocks.h"
#include "hardware/pll.h"
#include "hardware/structs/clocks.h"
#include "hardware/structs/pll.h"
#include "hardware/structs/rosc.h"
#include "ili9486_drivers.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <stdlib.h>

uint8_t tft_tx = 11;
uint8_t tft_rx = 8;
uint8_t tft_sck = 10;

uint8_t tft_cs = 13;
uint8_t tft_dc = 12;
uint8_t tft_reset = 9;
spi_inst_t *tft_spi = spi1;

ili9486_drivers tft(tft_spi, tft_cs, tft_dc, tft_reset, tft_tx, tft_rx, tft_sck,
                    125 * 1000 * 1000);

int main() {
  stdio_init_all();
  sleep_ms(2000);
  if (!set_sys_clock_khz(250 * 1000, false))
    printf("set system clock to %dMHz failed\n", 250);
  else
    printf("system clock is now %dMHz\n", 250);
  clock_configure(clk_peri,
                  0, // No glitchless mux
                  CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLK_SYS,
                  250 * 1000 * 1000,  // 250 MHz
                  250 * 1000 * 1000); // 250 MHz
  tft.init();
  uint32_t clk_freq = clock_get_hz(clk_peri);
  printf("SPI1 source clock frequency: %u Hz\n", clk_freq);
  clk_freq = spi_get_baudrate(tft_spi);
  printf("SPI1 clock frequency: %u Hz\n", clk_freq);
  //   tft.fillScreen(tft.create565Color(255, 0, 0));
  int r = 0;
  int g = 0;
  int b = 0;
  while (true) {
    // checkID();
    // printf("It's MyGO!!!!!\n");
    absolute_time_t start_time = get_absolute_time();
    tft.fillScreen(tft.create888Color(r, g, b));
    absolute_time_t end_time = get_absolute_time();
    int64_t time_taken = absolute_time_diff_us(start_time, end_time);
    printf("Time taken for fillScreen: %lld us\n", time_taken);
    tft.fillScreen(tft.create888Color(r, g, b));
    r += rand() % 10;
    g += rand() % 25;
    b += rand() % 50;
    r %= 255;
    g %= 255;
    b %= 255;
    sleep_ms(100);
  }
}
