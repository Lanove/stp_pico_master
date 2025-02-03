#include "modbus_master.h"
void ModbusMaster::send_request(uint8_t slave_addr, ModbusFunctionCode function, uint16_t reg_addr, uint16_t reg_count) {
    uint8_t frame[8];
    
    frame[0] = slave_addr;
    frame[1] = function;
    frame[2] = reg_addr >> 8;    // High byte
    frame[3] = reg_addr & 0xFF;  // Low byte
    frame[4] = reg_count >> 8;   // High byte
    frame[5] = reg_count & 0xFF; // Low byte
    
    // Calculate CRC
    uint16_t crc = modbus_crc(frame, 6);
    frame[6] = crc & 0xFF;
    frame[7] = crc >> 8;
    
    // Enable transmit mode
    gpio_put(de_re_pin, 1);
    uart_write_blocking(uart_id, frame, sizeof(frame));
    // Wait for transmission to complete
    sleep_ms(2);
    // Disable transmit mode
    gpio_put(de_re_pin, 0);
}

bool ModbusMaster::receive_response(uint8_t *resp, uint8_t len, uint32_t timeout_ms) {
    uint32_t start_time = to_ms_since_boot(get_absolute_time());
    uint8_t index = 0;
    
    while((to_ms_since_boot(get_absolute_time()) - start_time) < timeout_ms) {
        if(uart_is_readable(uart_id)) {
            uart_read_blocking(uart_id, &resp[index], 1);
            if(++index >= len) break;
        }
    }

    // Verify CRC
    if(index >= 5) {  // Minimum valid response length
        uint16_t crc = modbus_crc(resp, index - 2);
        if((resp[index-2] == (crc & 0xFF)) && (resp[index-1] == (crc >> 8))) {
            return true;
        }
    }
    return false;
}

// Modbus CRC calculation
uint16_t ModbusMaster::modbus_crc(uint8_t *buf, int len) {
    uint16_t crc = 0xFFFF;
    
    for(int pos = 0; pos < len; pos++) {
        crc ^= (uint16_t)buf[pos];
        
        for(int i = 8; i != 0; i--) {
            if((crc & 0x0001) != 0) {
                crc >>= 1;
                crc ^= 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}