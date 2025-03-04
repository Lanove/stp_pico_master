#ifndef PZEM017_H
#define PZEM017_H

#include "pico/stdlib.h"
#include <stdio.h>
#include "modbus_master.h"

class PZEM017 {
public:
  typedef enum {
    Voltage_Value = 0x00,
    Current_Value = 0x01,
    Power_Value_Low = 0x02,
    Power_Value_High = 0x03,
    Energy_Value_Low = 0x04,
    Energy_Value_High = 0x05,
    High_Voltage_Alarm = 0x06,
    Low_Voltage_Alarm = 0x07,
    Read_Parameter_Exception = 0x84,
    Write_Parameter_Exception = 0x86,
    Reset_Energy_Exception = 0xC2,
    Calibration_Exception = 0xC1,
  } input_register_t;

  typedef enum {
    Calibration = 0x41,
    Reset_Energy = 0x42,
  } modbus_function_code_extension_t;

  typedef enum {
    High_Voltage_Alarm_Parameter = 0x00,
    Low_Voltage_Alarm_Parameter = 0x01,
    Modbus_RTU_Address_Parameter = 0x02,
    Current_Range_Parameter = 0x03,
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
    bool high_voltage_alarm;
    bool low_voltage_alarm;
  };

public:
  PZEM017(ModbusMaster &mbm, uint8_t address);

  status_t request_all(measurement_t &output);

  /**
   * @brief Set the high voltage alarm parameter
   * @param value The high voltage alarm parameter, default is 300V, range is 5
   * ~ 350V
   * @return status_t
   */
  status_t set_high_voltage_alarm(float value);

  /*
   * @brief Set the low voltage alarm parameter
   * @param value The low voltage alarm parameter, default is 7V, range is 1 ~
   * 350V
   * @return status_t
   */
  status_t set_low_voltage_alarm(float value);

  /**
   * @brief Set the Modbus RTU address
   * @param value The Modbus RTU address, default is 1, range is 1 ~ 247
   * @return status_t
   */
  status_t set_modbus_rtu_address(uint16_t value);

  /**
   * @brief Set the current range parameter
   * @param value The current range parameter, default is 0x0000 (100A), range
   * is 0x0000 ~ 0x0003 (100A, 50A, 200A, 300A)
   * @return status_t
   */
  status_t set_current_range(uint16_t value);

  status_t reset_energy();

  const char *error_to_string(status_t error);

private:
  ModbusMaster *mbm;
  uint8_t address;
  uint8_t *response_buf;

  status_t validate_response(uint response_len,
                             modbus_function_code_t function);
};

#endif