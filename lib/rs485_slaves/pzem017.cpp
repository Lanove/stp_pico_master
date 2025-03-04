#include "pzem017.h"

PZEM017::PZEM017(ModbusMaster &mbm, uint8_t address) {
  this->mbm          = &mbm;
  this->address      = address;
  this->response_buf = mbm.get_response_buffer();
}

PZEM017::status_t PZEM017::request_all(measurement_t &output) {
  mbm->send_message(address, READ_INPUT_REGISTERS, Voltage_Value, 8, 0, 2);
  if (!mbm->receive_response(response_buf, 21, 1000))
    return Timeout;
  PZEM017::status_t status = validate_response(21, READ_INPUT_REGISTERS, address);
  if (status != No_Error)
    return status;

  // Parse values
  uint16_t voltage = (response_buf[3] << 8) | response_buf[4];
  uint16_t current = (response_buf[5] << 8) | response_buf[6];
  uint16_t power_lsb          = (response_buf[7] << 8) | response_buf[8];
  uint16_t power_msb          = (response_buf[9] << 8) | response_buf[10];
  uint32_t power_raw          = ((uint32_t) power_msb << 16) | (uint16_t) power_lsb;
  uint16_t energy_lsb         = (response_buf[11] << 8) | response_buf[12];
  uint16_t energy_msb         = (response_buf[13] << 8) | response_buf[14];
  uint32_t energy_raw         = ((uint32_t) energy_msb << 16) | (uint16_t) energy_lsb;
  uint16_t high_voltage_alarm = (response_buf[15] << 8) | response_buf[16];
  uint16_t low_voltage_alarm  = (response_buf[17] << 8) | response_buf[18];

  output.voltage            = voltage / 100.0;
  output.current            = current / 100.0;
  output.power              = power_raw / 10.0;
  output.energy             = energy_raw / 1.0;
  output.high_voltage_alarm = high_voltage_alarm == 0xFFFF;
  output.low_voltage_alarm  = low_voltage_alarm == 0xFFFF;

  return No_Error;
}

/**
 * @brief Set the high voltage alarm parameter
 * @param value The high voltage alarm parameter, default is 300V, range is 5
 * ~ 350V
 * @return PZEM017::status_t
 */
PZEM017::status_t PZEM017::set_high_voltage_alarm(float value) {
  uint16_t value_uint16 = value * 100.;
  mbm->send_message(address, WRITE_SINGLE_REGISTER, High_Voltage_Alarm_Parameter, value_uint16, 5, 5);
  if (!mbm->receive_response(response_buf, 8, 1000))
    return Timeout;
  PZEM017::status_t status = validate_response(8, WRITE_SINGLE_REGISTER, address);
  return status;
}

/*
 * @brief Set the low voltage alarm parameter
 * @param value The low voltage alarm parameter, default is 7V, range is 1 ~
 * 350V
 * @return PZEM017::status_t
 */
PZEM017::status_t PZEM017::set_low_voltage_alarm(float value) {
  uint16_t value_uint16 = value * 100.;
  mbm->send_message(address, WRITE_SINGLE_REGISTER, Low_Voltage_Alarm_Parameter, value_uint16, 5, 5);
  if (!mbm->receive_response(response_buf, 8, 1000))
    return Timeout;
  PZEM017::status_t status = validate_response(8, WRITE_SINGLE_REGISTER, address);
  return status;
}

/**
 * @brief Set the Modbus RTU address
 * @param value The Modbus RTU address, default is 1, range is 1 ~ 247
 * @return PZEM017::status_t
 */
PZEM017::status_t PZEM017::set_modbus_rtu_address(uint16_t value) {
  mbm->send_message(address, WRITE_SINGLE_REGISTER, Modbus_RTU_Address_Parameter, value, 5, 5);
  if (!mbm->receive_response(response_buf, 8, 1000))
    return Timeout;
  PZEM017::status_t status = validate_response(8, WRITE_SINGLE_REGISTER, address);
  return status;
}

/**
 * @brief Set the current range parameter
 * @param value The current range parameter, default is 0x0000 (100A), range
 * is 0x0000 ~ 0x0003 (100A, 50A, 200A, 300A)
 * @return PZEM017::status_t
 */
PZEM017::status_t PZEM017::set_current_range(uint16_t value) {
  mbm->send_message(address, WRITE_SINGLE_REGISTER, Current_Range_Parameter, value, 5, 5);
  if (!mbm->receive_response(response_buf, 8, 1000))
    return Timeout;
  PZEM017::status_t status = validate_response(8, WRITE_SINGLE_REGISTER, address);
  return status;
}

PZEM017::status_t PZEM017::reset_energy() {
  mbm->send_message(address, (modbus_function_code_t) (Reset_Energy), 0, 0, 5, 5);
  if (!mbm->receive_response(response_buf, 4, 1000))
    return Timeout;
  PZEM017::status_t status = validate_response(4, (modbus_function_code_t) Reset_Energy, address);
  return status;
}

PZEM017::status_t PZEM017::calibrate() {
  mbm->send_message(0xF8, (modbus_function_code_t) (Calibration), 0, 0xF170, 5, 5);
  if (!mbm->receive_response(response_buf, 6, 5000))
    return Timeout;
  PZEM017::status_t status = validate_response(6, (modbus_function_code_t) Calibration, 0xF8);
  return status;
}

const char *PZEM017::error_to_string(PZEM017::status_t error) {
  switch (error) {
  case No_Error:
    return "No error";
  case Timeout:
    return "Timeout";
  case Illegal_Function:
    return "Illegal function";
  case Illegal_Address:
    return "Illegal address";
  case Illegal_Data:
    return "Illegal data";
  case Slave_Error:
    return "Slave error";
  case Invalid_Response:
    return "Invalid response";
  case Invalid_CRC:
    return "Invalid CRC";
  default:
    return "Unknown error";
  }
}

PZEM017::status_t PZEM017::validate_response(uint response_len, modbus_function_code_t function, uint8_t addr) {
  uint16_t crc = response_buf[response_len - 2] | (response_buf[response_len - 1] << 8);
  if (!mbm->validate_crc(response_buf, response_len - 2, crc))
    return Invalid_CRC;
  if (response_buf[0] != addr)
    return Invalid_Response;
  if (response_buf[1] != function)
    return Invalid_Response;
  if (response_buf[1] == Read_Parameter_Exception || response_buf[1] == Write_Parameter_Exception || response_buf[1] == Reset_Energy_Exception ||
      response_buf[1] == Calibration_Exception)
    return (PZEM017::status_t) response_buf[2];
  return No_Error;
}