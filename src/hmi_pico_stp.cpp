#include "ClickEncoder.h"
#include "hardware/clocks.h"
#include "hardware/pll.h"
#include "hardware/structs/clocks.h"
#include "hardware/structs/pll.h"
#include "hardware/structs/rosc.h"
#include "ili9486_drivers.h"
#include "lv_app.h"
#include "lv_drivers.h"
#include "modbus_master.h"
#include "pico/stdlib.h"
#include "pico/time.h"
#include "xpt2046.h"
#include <stdio.h>
#include <stdlib.h>

uint8_t encoder_A = 26;
uint8_t encoder_B = 27;
uint8_t encoder_button = 28;

uint8_t modbus_de_re = 18;
uint8_t modbus_rx = 17;
uint8_t modbus_tx = 16;
uart_inst_t *modbus_uart = uart0;
uint32_t modbus_baudrate = 9600;
uint32_t modbus_data_bits = 8;
uint32_t modbus_stop_bits = 2;
uart_parity_t modbus_parity = UART_PARITY_NONE;

static repeating_timer encoder_service_timer;
ClickEncoder encoder = ClickEncoder(encoder_A, encoder_B, encoder_button);
ModbusMaster mbm = ModbusMaster(modbus_de_re, modbus_rx, modbus_tx, modbus_uart,
                                modbus_baudrate, modbus_data_bits,
                                modbus_stop_bits, modbus_parity);

constexpr int processor_mhz = 250;

typedef enum {
  Voltage_Value = 0x00,
  Current_Value = 0x01,
  Power_Value_Low = 0x02,
  Power_Value_High = 0x03,
  Energy_Value_Low = 0x04,
  Energy_Value_High = 0x05,
  High_Voltage_Alarm = 0x06,
  Low_Voltage_Alarm = 0x07,
} PZEM017_Measurement_Table;

struct PZEM017_Measurement {
  const uint8_t address = 0x01;

  uint8_t response_buf[21];

  float voltage;
  float current;
  float power;
  float energy;
  uint16_t high_voltage_alarm;
  uint16_t low_voltage_alarm;
};

PZEM017_Measurement pzem017;

uint get_uart_baudrate(uart_inst_t *uart) {
  // Get the UART hardware instance
  uart_hw_t *uart_reg = uart_get_hw(uart);

  // Read the IBRD (Integer Baud Rate Divisor)
  uint ibrd = uart_reg->ibrd;

  // Read the FBRD (Fractional Baud Rate Divisor)
  uint fbrd = uart_reg->fbrd;

  // Get the system clock frequency
  uint sys_clk = clock_get_hz(clk_peri); // Peripheral clock

  // Calculate the baud rate
  uint baud_rate = sys_clk / (16 * (ibrd + (fbrd / 64.0)));

  return baud_rate;
}

int main() {
  stdio_init_all();

  sleep_ms(2000);

  if (!set_sys_clock_khz(processor_mhz * 1000, false))
    printf("set system clock to %dMHz failed\n", processor_mhz);
  else
    printf("system clock is now %dMHz\n", processor_mhz);
  clock_configure(clk_peri, 0, CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLK_SYS,
                  processor_mhz * 1000 * 1000, processor_mhz * 1000 * 1000);

  encoder.init();
  mbm.init();
  lvgl_display_init();

  uint32_t clk_freq = clock_get_hz(clk_peri);
  printf("SPI1 source clock frequency: %u Hz\n", clk_freq);
  clk_freq = spi_get_baudrate(spi1);
  printf("SPI1 clock frequency: %u Hz\n", clk_freq);
  clk_freq = get_uart_baudrate(modbus_uart);
  printf("UART0 baudrate: %u bps\n", clk_freq);

  add_repeating_timer_us(
      100,
      [](struct repeating_timer *t) -> bool {
        encoder.service();
        return true;
      },
      NULL, &encoder_service_timer);

  // lvgl_app::app_entry();
  int encoder_value = 0;
  int last_encoder_value = 0;
  int undivided_encoder_value = 0;

  gpio_init(15);
  gpio_set_dir(15, GPIO_OUT);

  while (true) {

    undivided_encoder_value += encoder.getValue();
    encoder_value = undivided_encoder_value / 4;
    ClickEncoder::Button b = encoder.getButton();
    if (encoder_value != last_encoder_value) {
      // printf("Encoder value: %d\n", encoder_value);
    }
    last_encoder_value = encoder_value;

    mbm.send_request(pzem017.address, READ_INPUT_REGISTERS, Voltage_Value, 8);
    if (mbm.receive_response(pzem017.response_buf, 21, 1000)) {
      if (pzem017.response_buf[0] == pzem017.address &&
          pzem017.response_buf[1] == READ_INPUT_REGISTERS) {
        // Parse values
        uint16_t voltage =
            (pzem017.response_buf[3] << 8) | pzem017.response_buf[4];
        uint16_t current =
            (pzem017.response_buf[5] << 8) | pzem017.response_buf[6];
        uint16_t power_lsb =
            (pzem017.response_buf[7] << 8) | pzem017.response_buf[8];
        uint16_t power_msb =
            (pzem017.response_buf[9] << 8) | pzem017.response_buf[10];
        uint32_t power_raw = ((uint32_t)power_msb << 16) | (uint16_t)power_lsb;
        uint16_t energy_lsb =
            (pzem017.response_buf[11] << 8) | pzem017.response_buf[12];
        uint16_t energy_msb =
            (pzem017.response_buf[13] << 8) | pzem017.response_buf[14];
        uint32_t energy_raw = ((uint32_t)energy_msb << 16) | (uint16_t)energy_lsb;
        uint16_t high_voltage_alarm =
            (pzem017.response_buf[15] << 8) | pzem017.response_buf[16];
        uint16_t low_voltage_alarm =
            (pzem017.response_buf[17] << 8) | pzem017.response_buf[18];

        pzem017.voltage = voltage / 100.0;
        pzem017.current = current / 100.0;
        pzem017.power = power_raw / 10.0;
        pzem017.energy = energy_raw / 1.0;
        pzem017.high_voltage_alarm = high_voltage_alarm;
        pzem017.low_voltage_alarm = low_voltage_alarm;

        printf("Voltage: %.2f V\n", pzem017.voltage);
        printf("Current: %.2f A\n", pzem017.current);
        printf("Power: %.2f W\n", pzem017.power);
        printf("Energy: %.2f Wh\n", pzem017.energy);
        printf("High Voltage Alarm: %d V\n", pzem017.high_voltage_alarm);
        printf("Low Voltage Alarm: %d V\n", pzem017.low_voltage_alarm);
      }
    } else {
      printf("No response\n");
    }
    sleep_ms(1000);

    // lv_timer_handler();
    // sleep_ms(5);
  }
}