#include "modbus_master.h"
void ModbusMaster::send_message(uint8_t slave_addr, modbus_function_code_t function, uint16_t reg_addr, uint16_t reg_count, uint pre_tx_delay,
                                uint post_tx_delay) {
  while (uart_is_readable(uart_id)) {
    int c = uart_getc(uart_id);
  }

  int frame_size = 8;
  if (reg_count == 0xFFFF) {
    frame_size = 4;
    frame[0]   = slave_addr;
    frame[1]   = function;
    // Calculate CRC
    uint16_t crc = modbus_crc(frame, 2);
    frame[2]     = crc & 0xFF;
    frame[3]     = crc >> 8;
  } else if (reg_count == 0xF170) {
    frame_size = 6;
    frame[0]   = slave_addr;
    frame[1]   = function;
    frame[2]   = 0x37;
    frame[3]   = 0x21;
    // Calculate CRC
    uint16_t crc = modbus_crc(frame, 4);
    frame[4]     = crc & 0xFF;
    frame[5]     = crc >> 8;

  } else {
    frame_size = 8;
    frame[0]   = slave_addr;
    frame[1]   = function;
    frame[2]   = reg_addr >> 8;     // High byte
    frame[3]   = reg_addr & 0xFF;   // Low byte
    frame[4]   = reg_count >> 8;    // High byte
    frame[5]   = reg_count & 0xFF;  // Low byte

    // Calculate CRC
    uint16_t crc = modbus_crc(frame, 6);
    frame[6]     = crc & 0xFF;
    frame[7]     = crc >> 8;
  }

  // Enable transmit mode
  gpio_put(de_re_pin, 1);
  sleep_ms(pre_tx_delay);

  uart_write_blocking(uart_id, frame, frame_size);
  // Wait for transmission to complete
  sleep_ms(post_tx_delay);
  // Disable transmit mode
  gpio_put(de_re_pin, 0);
}
bool ModbusMaster::receive_response(uint8_t *resp, size_t len, uint32_t timeout_ms, size_t max_buffer_size) {
  if (len < 2 || len > max_buffer_size) {
    return false;
  }

  uint32_t start_time = to_ms_since_boot(get_absolute_time());
  size_t   index      = 0;

  while ((to_ms_since_boot(get_absolute_time()) - start_time) < timeout_ms) {
    if (uart_is_readable(uart_id)) {
      if (index >= max_buffer_size)
        return false;

      // Replace blocking read with single-byte non-blocking read
      int c = uart_getc(uart_id);
      if (c != PICO_ERROR_TIMEOUT) {
        resp[index] = (uint8_t) c;
        if (++index >= len)
          break;
      }
    }
    sleep_us(100);
  }

  if (index == len) {
    uint16_t crc = modbus_crc(resp, len - 2);
    if (resp[len - 2] == (crc & 0xFF) && resp[len - 1] == (crc >> 8)) {
      return true;
    }
  }
  return false;
}

// Modbus CRC calculation
uint16_t ModbusMaster::modbus_crc(uint8_t *buf, int len) {
  uint16_t crc = 0xFFFF;

  for (int pos = 0; pos < len; pos++) {
    crc ^= (uint16_t) buf[pos];

    for (int i = 8; i != 0; i--) {
      if ((crc & 0x0001) != 0) {
        crc >>= 1;
        crc ^= 0xA001;
      } else {
        crc >>= 1;
      }
    }
  }
  return crc;
}

bool ModbusMaster::validate_crc(uint8_t *buf, int len, uint16_t crc) {
  uint16_t calculated_crc = modbus_crc(buf, len);
  return (calculated_crc == crc);
}