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
  PZEM017(ModbusMaster &mbm, uint8_t address) {
    this->mbm = &mbm;
    this->address = address;
  }

  status_t request_all(measurement_t &output) {
    mbm->send_message(address, READ_INPUT_REGISTERS, Voltage_Value, 8, 0, 2);
    if (!mbm->receive_response(response_buf, 21, 1000))
      return Timeout;
    status_t status = validate_response(21, READ_INPUT_REGISTERS);
    if (status != No_Error)
      return status;

    // Parse values
    uint16_t voltage = (response_buf[3] << 8) | response_buf[4];
    uint16_t current = (response_buf[5] << 8) | response_buf[6];
    uint16_t power_lsb = (response_buf[7] << 8) | response_buf[8];
    uint16_t power_msb = (response_buf[9] << 8) | response_buf[10];
    uint32_t power_raw = ((uint32_t)power_msb << 16) | (uint16_t)power_lsb;
    uint16_t energy_lsb = (response_buf[11] << 8) | response_buf[12];
    uint16_t energy_msb = (response_buf[13] << 8) | response_buf[14];
    uint32_t energy_raw = ((uint32_t)energy_msb << 16) | (uint16_t)energy_lsb;
    uint16_t high_voltage_alarm = (response_buf[15] << 8) | response_buf[16];
    uint16_t low_voltage_alarm = (response_buf[17] << 8) | response_buf[18];

    output.voltage = voltage / 100.0;
    output.current = current / 100.0;
    output.power = power_raw / 10.0;
    output.energy = energy_raw / 1.0;
    output.high_voltage_alarm = high_voltage_alarm == 0xFFFF;
    output.low_voltage_alarm = low_voltage_alarm == 0xFFFF;

    return No_Error;
  }

  /**
   * @brief Set the high voltage alarm parameter
   * @param value The high voltage alarm parameter, default is 300V, range is 5
   * ~ 350V
   * @return status_t
   */
  status_t set_high_voltage_alarm(float value) {
    uint16_t value_uint16 = value * 100.;
    mbm->send_message(address, WRITE_SINGLE_REGISTER,
                      High_Voltage_Alarm_Parameter, value_uint16, 5, 5);
    if (!mbm->receive_response(response_buf, 8, 1000))
      return Timeout;
    status_t status = validate_response(8, WRITE_SINGLE_REGISTER);
    return status;
  }

  /*
   * @brief Set the low voltage alarm parameter
   * @param value The low voltage alarm parameter, default is 7V, range is 1 ~
   * 350V
   * @return status_t
   */
  status_t set_low_voltage_alarm(float value) {
    uint16_t value_uint16 = value * 100.;
    mbm->send_message(address, WRITE_SINGLE_REGISTER,
                      Low_Voltage_Alarm_Parameter, value_uint16, 5, 5);
    if (!mbm->receive_response(response_buf, 8, 1000))
      return Timeout;
    status_t status = validate_response(8, WRITE_SINGLE_REGISTER);
    return status;
  }

  /**
   * @brief Set the Modbus RTU address
   * @param value The Modbus RTU address, default is 1, range is 1 ~ 247
   * @return status_t
   */
  status_t set_modbus_rtu_address(uint16_t value) {
    mbm->send_message(address, WRITE_SINGLE_REGISTER,
                      Modbus_RTU_Address_Parameter, value, 5, 5);
    if (!mbm->receive_response(response_buf, 8, 1000))
      return Timeout;
    status_t status = validate_response(8, WRITE_SINGLE_REGISTER);
    return status;
  }

  /**
   * @brief Set the current range parameter
   * @param value The current range parameter, default is 0x0000 (100A), range
   * is 0x0000 ~ 0x0003 (100A, 50A, 200A, 300A)
   * @return status_t
   */
  status_t set_current_range(uint16_t value) {
    mbm->send_message(address, WRITE_SINGLE_REGISTER, Current_Range_Parameter,
                      value, 5, 5);
    if (!mbm->receive_response(response_buf, 8, 1000))
      return Timeout;
    status_t status = validate_response(8, WRITE_SINGLE_REGISTER);
    return status;
  }

  status_t reset_energy() {
    mbm->send_message(address, (modbus_function_code_t)(Reset_Energy), 0,
                      0, 5, 5);
    if (!mbm->receive_response(response_buf, 8, 1000))
      return Timeout;
    status_t status =
        validate_response(8, (modbus_function_code_t)Reset_Energy);
    return status;
  }

  const char *error_to_string(status_t error) {
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

private:
  ModbusMaster *mbm;
  uint8_t address;
  uint8_t response_buf[32];

  status_t validate_response(uint response_len,
                             modbus_function_code_t function) {
    uint16_t crc =
        response_buf[response_len - 2] | (response_buf[response_len - 1] << 8);
    if (!mbm->validate_crc(response_buf, response_len - 2, crc))
      return Invalid_CRC;
    if (response_buf[0] != address)
      return Invalid_Response;
    if (response_buf[1] != function)
      return Invalid_Response;
    if (response_buf[1] == Read_Parameter_Exception ||
        response_buf[1] == Write_Parameter_Exception ||
        response_buf[1] == Reset_Energy_Exception ||
        response_buf[1] == Calibration_Exception)
      return (status_t)response_buf[2];
    return No_Error;
  }
};

PZEM017 pzem017 = PZEM017(mbm, 0x01);
PZEM017::measurement_t pzem017_measurement;

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
  PZEM017::status_t status;
  while (true) {

    undivided_encoder_value += encoder.getValue();
    encoder_value = undivided_encoder_value / 4;
    ClickEncoder::Button b = encoder.getButton();
    if (encoder_value != last_encoder_value) {
      printf("Encoder value: %d\n", encoder_value);
    }
    if (b == ClickEncoder::DoubleClicked) {
      status = pzem017.set_high_voltage_alarm(300);
      if (status == PZEM017::No_Error) {
        printf("High voltage alarm set\n");
      } else {
        printf("Error: %s\n", pzem017.error_to_string(status));
      }
    } else if (b == ClickEncoder::Clicked) {
      status = pzem017.set_low_voltage_alarm(10);
      if (status == PZEM017::No_Error) {
        printf("Low voltage alarm set\n");
      } else {
        printf("Error: %s\n", pzem017.error_to_string(status));
      }
    }else if( b== ClickEncoder::Released){
      status = pzem017.reset_energy();
      if (status == PZEM017::No_Error) {
        printf("Energy reset\n");
      } else {
        printf("Error: %s\n", pzem017.error_to_string(status));
      }
    }
    last_encoder_value = encoder_value;
    status = pzem017.request_all(pzem017_measurement);
    if (status == PZEM017::No_Error) {
      printf("BVoltage: %.2f V\n", pzem017_measurement.voltage);
      // printf("BCurrent: %.2f A\n", pzem017_measurement.current);
      // printf("BPower: %.2f W\n", pzem017_measurement.power);
      printf("BEnergy: %.2f Wh\n", pzem017_measurement.energy);
      // printf("BHigh Voltage Alarm: %d\n",
      //        pzem017_measurement.high_voltage_alarm);
      // printf("ALow Voltage Alarm: %d\n",
      // pzem017_measurement.low_voltage_alarm);
    } else {
      // printf("Error: %s\n", pzem017.error_to_string(status));
    }
    sleep_ms(1000);

    // lv_timer_handler();
    // sleep_ms(5);
  }
}