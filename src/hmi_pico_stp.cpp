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
#include "ClickEncoder.h"

static repeating_timer encoder_service_timer;
ClickEncoder encoder = ClickEncoder(26, 27, 28);

constexpr int processor_mhz = 250;

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

  encoder.init();
  // lvgl_display_init();


  add_repeating_timer_us(
      100,
      [](struct repeating_timer *t) -> bool {
        encoder.service();
        return true;
      },
      NULL, &encoder_service_timer);

  // lvgl_app::app_entry();
  int encoder_value  = 0;
  int last_encoder_value = 0;
  int undivided_encoder_value = 0;
  while (true) {
    undivided_encoder_value += encoder.getValue();
    encoder_value = undivided_encoder_value / 4;
    ClickEncoder::Button b = encoder.getButton();
    switch(b){
      case ClickEncoder::Button::Clicked:
        printf("Button clicked\n");
        break;
      case ClickEncoder::Button::DoubleClicked:
        printf("Button double clicked\n");
        break;
      case ClickEncoder::Button::Held:
        printf("Button held\n");
        break;
      case ClickEncoder::Button::Pressed:
        printf("Button pressed\n");
        break;
      case ClickEncoder::Button::Released:
        printf("Button released\n");
        break;
    }
    if (encoder_value != last_encoder_value) {
      printf("Encoder value: %d\n", encoder_value);
    }
    last_encoder_value = encoder_value;
    // lv_timer_handler();
    // sleep_ms(5);
  }
}
