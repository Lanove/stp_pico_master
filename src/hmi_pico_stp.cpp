#include <stdio.h>
#include <stdlib.h>

#include "click_encoder.h"
#include "hardware/clocks.h"
#include "hardware/pll.h"
#include "hardware/structs/clocks.h"
#include "hardware/structs/pll.h"
#include "hardware/structs/rosc.h"
#include "ili9486_drivers.h"
#include "lv_app.h"
#include "lv_drivers.h"
#include "modbus_master.h"
#include "pico/multicore.h"
#include "pico/stdlib.h"
#include "pico/sync.h"
#include "pico/time.h"
#include "pzem017.h"
#include "xpt2046.h"

constexpr int processor_mhz = 250;

uint8_t                encoder_A      = 26;
uint8_t                encoder_B      = 27;
uint8_t                encoder_button = 28;
static repeating_timer encoder_service_timer;
ClickEncoder           encoder = ClickEncoder(encoder_A, encoder_B, encoder_button);

uint8_t       modbus_de_re     = 18;
uint8_t       modbus_rx        = 17;
uint8_t       modbus_tx        = 16;
uart_inst_t  *modbus_uart      = uart0;
uint32_t      modbus_baudrate  = 9600;
uint32_t      modbus_data_bits = 8;
uint32_t      modbus_stop_bits = 2;
uart_parity_t modbus_parity    = UART_PARITY_NONE;
ModbusMaster  mbm = ModbusMaster(modbus_de_re, modbus_rx, modbus_tx, modbus_uart, modbus_baudrate, modbus_data_bits, modbus_stop_bits, modbus_parity);

PZEM017                pzem017 = PZEM017(mbm, 0x02);
PZEM017::measurement_t pzem017_measurement;

// Following variabbles are shared between the two cores
Big_Labels_Value     shared_big_labels_value;
Setting_Labels_Value shared_setting_labels_value;
Status_Labels_Value  shared_status_labels_value;
mutex_t                        shared_data_mutex;

void core1_entry();
void core0_entry();

int main() {
  stdio_init_all();

  sleep_ms(2000);

  if (!set_sys_clock_khz(processor_mhz * 1000, false))
    printf("set system clock to %dMHz failed\n", processor_mhz);
  else
    printf("system clock is now %dMHz\n", processor_mhz);
  clock_configure(clk_peri, 0, CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLK_SYS, processor_mhz * 1000 * 1000, processor_mhz * 1000 * 1000);

  mutex_init(&shared_data_mutex);

  multicore_launch_core1(core1_entry);

  core0_entry();
}

void core0_entry() {
  mbm.init();

  gpio_init(15);
  gpio_set_dir(15, GPIO_OUT);
  PZEM017::status_t status;
  while (true) {
    status = pzem017.request_all(pzem017_measurement);
    if (status != PZEM017::No_Error) {
      printf("Error: %s\n", pzem017.error_to_string(status));
    }

    mutex_enter_blocking(&shared_data_mutex);
    shared_big_labels_value.v  = pzem017_measurement.voltage;
    shared_big_labels_value.a  = pzem017_measurement.current;
    shared_big_labels_value.w  = pzem017_measurement.power;
    shared_big_labels_value.wh = pzem017_measurement.energy;
    mutex_exit(&shared_data_mutex);

    sleep_ms(1000);
  }
  return;
}

void core1_entry() {
  LVGL_App            app;
  Big_Labels_Value     big_labels_value;
  Setting_Labels_Value setting_labels_value;
  Status_Labels_Value  status_labels_value;

  lvgl_display_init();
  app.app_entry();

  encoder.init();
  add_repeating_timer_us(
      100,
      [](struct repeating_timer *t) -> bool {
        encoder.service();
        return true;
      },
      NULL, &encoder_service_timer);

  int encoder_value           = 0;
  int last_encoder_value      = 0;
  int undivided_encoder_value = 0;

  while (true) {
    mutex_enter_blocking(&shared_data_mutex);
    big_labels_value     = shared_big_labels_value;
    setting_labels_value = shared_setting_labels_value;
    status_labels_value  = shared_status_labels_value;
    mutex_exit(&shared_data_mutex);

    app.app_update(big_labels_value, setting_labels_value, status_labels_value);

    undivided_encoder_value += encoder.get_value();
    encoder_value          = undivided_encoder_value / 4;
    ClickEncoder::Button b = encoder.get_button();
    if (encoder_value != last_encoder_value) {
      printf("Encoder value: %d\n", encoder_value);
    }

    last_encoder_value = encoder_value;

    lv_timer_handler();
    sleep_ms(5);
  }
  return;
}