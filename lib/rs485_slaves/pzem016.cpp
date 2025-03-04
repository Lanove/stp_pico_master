#include "pzem016.h"

PZEM016::PZEM016(ModbusMaster &mbm, uint8_t address) {
  this->mbm          = &mbm;
  this->address      = address;
  this->response_buf = mbm.get_response_buffer();
}

PZEM016::status_t PZEM016::request_all(measurement_t &output) {
  mbm->send_message(address, READ_INPUT_REGISTERS, Voltage_Value, 10, 5, 5);
  if (!mbm->receive_response(response_buf, 25, 1000))
    return Timeout;
  PZEM016::status_t status = validate_response(25, READ_INPUT_REGISTERS, address);
  if (status != No_Error)
    return status;

  // Parse values
  uint16_t voltage      = (response_buf[3] << 8) | response_buf[4];
  uint16_t current      = (response_buf[5] << 8) | response_buf[6];
  uint16_t power_low    = (response_buf[9] << 8) | response_buf[10];
  uint16_t power_high   = (response_buf[11] << 8) | response_buf[12];
  uint32_t power        = ((uint32_t) power_high << 16) | power_low;
  uint16_t energy_low   = (response_buf[13] << 8) | response_buf[14];
  uint16_t energy_high  = (response_buf[15] << 8) | response_buf[16];
  uint32_t energy       = ((uint32_t) energy_high << 16) | energy_low;
  uint16_t frequency    = (response_buf[17] << 8) | response_buf[18];
  uint16_t power_factor = (response_buf[19] << 8) | response_buf[20];
  uint16_t alarm_status = (response_buf[21] << 8) | response_buf[22];

  output.voltage      = voltage / 10.0f;
  output.current      = current / 1000.0f;
  output.power        = power / 10.0f;
  output.energy       = energy / 1000.0f;  // Convert Wh to kWh
  output.frequency    = frequency / 10.0f;
  output.power_factor = power_factor / 100.0f;
  output.alarm_status = (alarm_status != 0);

  return No_Error;
}

PZEM016::status_t PZEM016::set_modbus_rtu_address(uint16_t value) {
  mbm->send_message(address, WRITE_SINGLE_REGISTER, Modbus_RTU_Address_Parameter, value, 5, 5);
  if (!mbm->receive_response(response_buf, 8, 1000))
    return Timeout;
  return validate_response(8, WRITE_SINGLE_REGISTER, address);
}

PZEM016::status_t PZEM016::reset_energy() {
  mbm->send_message(address, (modbus_function_code_t) Reset_Energy, 0, 0, 5, 5);
  if (!mbm->receive_response(response_buf, 4, 1000))
    return Timeout;
  return validate_response(4, (modbus_function_code_t) Reset_Energy, address);
}

const char *PZEM016::error_to_string(PZEM016::status_t error) {
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

PZEM016::status_t PZEM016::validate_response(uint response_len, modbus_function_code_t function, uint8_t addr) {
  uint16_t crc = response_buf[response_len - 2] | (response_buf[response_len - 1] << 8);
  if (!mbm->validate_crc(response_buf, response_len - 2, crc))
    return Invalid_CRC;
  if (response_buf[0] != addr)
    return Invalid_Response;
  if (response_buf[1] != function) {
    if (response_buf[1] == Read_Parameter_Exception || response_buf[1] == Write_Parameter_Exception || response_buf[1] == Reset_Energy_Exception)
      return (status_t) response_buf[2];
    return Invalid_Response;
  }
  return No_Error;
}