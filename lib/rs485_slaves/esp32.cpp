#include "esp32.h"

ESP32::ESP32(ModbusMaster &mbm, uint8_t address) {
  this->mbm     = &mbm;
  this->address = address;
}

const char *ESP32::error_to_string(ESP32::status_t error) {
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

ESP32::status_t ESP32::set_relay_state(uint16_t state, uint8_t index) {
  mbm->send_message(address, WRITE_SINGLE_REGISTER, Relay_State_Low + index, state, 5, 3);
  if (!mbm->receive_response(response_buf, 8, 1000))
    return Timeout;
  ESP32::status_t status = validate_response(8, WRITE_SINGLE_REGISTER);
  if (status == No_Error)
    reg.relay_state[index] = state;
  return status;
}

ESP32::status_t ESP32::request_temperature(float &output) {
  mbm->send_message(address, READ_HOLDING_REGISTERS, Temperature_Value, 1, 5, 3);
  if (!mbm->receive_response(response_buf, 7, 1000))
    return Timeout;
  ESP32::status_t status = validate_response(7, READ_HOLDING_REGISTERS);
  if (status == No_Error) {
    uint16_t raw = (response_buf[3] << 8) | (response_buf[4]);
    output       = raw * 0.25;
  }
  return status;
}

ESP32::status_t ESP32::validate_response(uint response_len, modbus_function_code_t function) {
  uint16_t crc = response_buf[response_len - 2] | (response_buf[response_len - 1] << 8);
  if (!mbm->validate_crc(response_buf, response_len - 2, crc))
    return Invalid_CRC;
  if (response_buf[0] != address)
    return Invalid_Response;
  if (response_buf[1] != function)
    return Invalid_Response;
  return No_Error;
}