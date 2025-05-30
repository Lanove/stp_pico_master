#ifndef MODBUS_MASTER_H
#define MODBUS_MASTER_H
#include <stdio.h>

#include "hardware/gpio.h"
#include "hardware/uart.h"
#include "pico/stdlib.h"

#define MAX_RESP_SIZE 256U

typedef enum {
  READ_COILS               = 0x01,
  READ_DISCRETE_INPUTS     = 0x02,
  READ_HOLDING_REGISTERS   = 0x03,
  READ_INPUT_REGISTERS     = 0x04,
  WRITE_SINGLE_COIL        = 0x05,
  WRITE_SINGLE_REGISTER    = 0x06,
  WRITE_MULTIPLE_COILS     = 0x0F,
  WRITE_MULTIPLE_REGISTERS = 0x10
} modbus_function_code_t;

class ModbusMaster {
 public:
  ModbusMaster(uint8_t de_re_pin, uint8_t rx_pin, uint8_t tx_pin, uart_inst_t *uart_id, uint baud_rate, uint data_bits, uint stop_bits,
               uart_parity_t parity) {
    this->de_re_pin = de_re_pin;
    this->rx_pin    = rx_pin;
    this->tx_pin    = tx_pin;
    this->uart_id   = uart_id;
    this->baud_rate = baud_rate;
    this->data_bits = data_bits;
    this->stop_bits = stop_bits;
    this->parity    = parity;
  }
  void init() {
    uart_init(uart_id, baud_rate);

    gpio_init(de_re_pin);
    gpio_set_dir(de_re_pin, GPIO_OUT);
    gpio_put(de_re_pin, 0);

    gpio_set_function(rx_pin, GPIO_FUNC_UART);
    gpio_set_function(tx_pin, GPIO_FUNC_UART);

    uart_set_hw_flow(uart_id, false, false);
    uart_set_format(uart_id, data_bits, stop_bits, parity);
    uart_set_fifo_enabled(uart_id, false);
  }

  void change_stop_bits(uint stop_bits) {
    this->stop_bits = stop_bits;
    uart_set_format(uart_id, data_bits, stop_bits, parity);
  }

  void
  send_message(uint8_t slave_addr, modbus_function_code_t function, uint16_t reg_addr, uint16_t reg_count, uint pre_tx_delay, uint post_tx_delay);

  bool     receive_response(uint8_t *resp, size_t len, uint32_t timeout_ms, size_t max_buffer_size = MAX_RESP_SIZE);
  uint16_t modbus_crc(uint8_t *buf, int len);
  bool     validate_crc(uint8_t *buf, int len, uint16_t crc);
  uint8_t* get_response_buffer() { return resp_buf; }

 private:
  uint8_t       frame[256];
  uint8_t       resp_buf[MAX_RESP_SIZE];
  uint8_t       de_re_pin;
  uint8_t       rx_pin;
  uint8_t       tx_pin;
  uart_inst_t  *uart_id;
  uint          baud_rate;
  uint          data_bits;
  uint          stop_bits;
  uart_parity_t parity;
};

#endif