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
#include "xpt2046.h"
#include <stdio.h>
#include <stdlib.h>

constexpr int processor_mhz = 125;

int main() {
  stdio_init_all();

  sleep_ms(2000);

  if (!set_sys_clock_khz(processor_mhz * 1000, false))
    printf("set system clock to %dMHz failed\n", processor_mhz);
  else
    printf("system clock is now %dMHz\n", processor_mhz);
  clock_configure(clk_peri, 0, CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLK_SYS,
                  processor_mhz * 1000 * 1000, processor_mhz * 1000 * 1000);

  uint32_t clk_freq = clock_get_hz(clk_peri);
  printf("SPI1 source clock frequency: %u Hz\n", clk_freq);
  clk_freq = spi_get_baudrate(spi1);
  printf("SPI1 clock frequency: %u Hz\n", clk_freq);

  lvgl_display_init();
  lvgl_app::app_entry();
  while (true) {
    lv_timer_handler();
    sleep_ms(5);
  }
}
