#ifndef ESP32_H
#define ESP32_H

#include <stdio.h>

#include "modbus_master.h"
#include "pico/stdlib.h"

typedef enum { Source_Off, Source_AC, Source_DC } Sensed_Source;


class ESP32 {
 public:
  typedef enum {
    Temperature_Value = 0x0000,
    Supply_Status     = 0x0001,
    SSID_Request      = 0x0002,
    Scan_Request      = 0x0003,
  } input_register_t;

  typedef enum {
    Relay_State_Low  = 0x0000,
    Relay_State_High = 0x0001,
  } parameter_register_t;

  typedef enum {
    No_Error         = 0,
    Illegal_Function = 1,
    Illegal_Address  = 2,
    Illegal_Data     = 3,
    Slave_Error      = 4,
    Timeout          = 5,
    Invalid_Response = 6,
    Invalid_CRC      = 7,
  } status_t;

  struct Registers {
    float    temperature    = 0.0;
    uint16_t relay_state[2] = {0, 0};
  };

 public:
  ESP32(ModbusMaster &mbm, uint8_t address);

  Registers &get_reg() { return reg; }

  status_t request_temperature(float &output);
  status_t request_sensed_source(Sensed_Source &output);

  status_t set_relay_state(uint16_t state, uint8_t index);

  const char *error_to_string(status_t error);

 private:
  ModbusMaster *mbm;
  uint8_t       address;
  uint8_t       *response_buf;

  Registers reg;

  status_t validate_response(uint response_len, modbus_function_code_t function);
};

#endif