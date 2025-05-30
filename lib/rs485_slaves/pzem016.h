#ifndef PZEM016_H
#define PZEM016_H

#include "pico/stdlib.h"
#include <stdio.h>
#include "modbus_master.h"

class PZEM016 {
public:
  typedef enum {
    Voltage_Value = 0x0000,
    Current_Value = 0x0001,
    Power_Value_Low = 0x0003,
    Power_Value_High = 0x0004,
    Energy_Value_Low = 0x0005,
    Energy_Value_High = 0x0006,
    Frequency_Value = 0x0007,
    Power_Factor_Value = 0x0008,
    Alarm_Status = 0x0009,
    Read_Parameter_Exception = 0x84,
    Write_Parameter_Exception = 0x86,
    Reset_Energy_Exception = 0xC2,
  } input_register_t;

  typedef enum {
    Reset_Energy = 0x42,
  } modbus_function_code_extension_t;

  typedef enum {
    Modbus_RTU_Address_Parameter = 0x0002,
  } parameter_register_t;

  typedef enum {
    No_Error = 0,
    Illegal_Function = 1,
    Illegal_Address = 2,
    Illegal_Data = 3,
    Slave_Error = 4,
    Timeout = 5,
    Invalid_Response = 6,
    Invalid_CRC = 7,
  } status_t;

  struct measurement_t {
    float voltage;
    float current;
    float power;
    float energy;
    float frequency;
    float power_factor;
    bool alarm_status;
  };

public:
  PZEM016(ModbusMaster &mbm, uint8_t address);

  status_t request_all(measurement_t &output);

  /**
   * @brief Set the Modbus RTU address
   * @param value The Modbus RTU address, default is 1, range is 1 ~ 247
   * @return status_t
   */
  status_t set_modbus_rtu_address(uint16_t value);

  status_t reset_energy();

  const char *error_to_string(status_t error);

private:
  ModbusMaster *mbm;
  uint8_t address;
  uint8_t *response_buf;

  status_t validate_response(uint response_len,
                             modbus_function_code_t function, uint8_t addr);
};

#endif