#include <stdio.h>
#include <stdlib.h>

#include <vector>

#include "click_encoder.h"
#include "esp32.h"
#include "hardware/clocks.h"
#include "hardware/pll.h"
#include "hardware/structs/clocks.h"
#include "hardware/structs/pll.h"
#include "hardware/structs/rosc.h"
#include "ili9486_drivers.h"
#include "lv_app.h"
#include "lv_drivers.h"
#include "math.h"
#include "modbus_master.h"
#include "pico/multicore.h"
#include "pico/stdlib.h"
#include "pico/sync.h"
#include "pico/time.h"
#include "plc_utility.hpp"
#include "pzem017.h"
#include "xpt2046.h"

struct MachineState {
  uint16_t      relay_state[2];
  bool          started;
  Sensed_Source sensed_source;
  bool          polarity_flipped;
};
static repeating_timer one_sec_timer;

constexpr int processor_mhz = 250;

uint8_t                pin_buzzer = 15;
uint8_t                pin_start  = 21;
uint8_t                pin_stop   = 22;
uint8_t                pin_dc     = 20;
uint8_t                pin_ac     = 19;
Differential_Up        stop;
Differential_Down      start;
static repeating_timer io_service_timer;
Sensed_Source          ac_dc_off;

uint8_t                encoder_A      = 26;
uint8_t                encoder_B      = 27;
uint8_t                encoder_button = 28;
static repeating_timer encoder_service_timer;
ClickEncoder           encoder = ClickEncoder(encoder_A, encoder_B, encoder_button);

uint8_t       modbus_de_re     = 18;
uint8_t       modbus_rx        = 17;
uint8_t       modbus_tx        = 16;
uart_inst_t  *modbus_uart      = uart0;
uint32_t      modbus_baudrate  = 9600;
uint32_t      modbus_data_bits = 8;
uint32_t      modbus_stop_bits = 2;
uart_parity_t modbus_parity    = UART_PARITY_NONE;
ModbusMaster  mbm = ModbusMaster(modbus_de_re, modbus_rx, modbus_tx, modbus_uart, modbus_baudrate, modbus_data_bits, modbus_stop_bits, modbus_parity);

PZEM017                pzem017 = PZEM017(mbm, 0x02);
ESP32                  esp32   = ESP32(mbm, 0x03);
PZEM017::measurement_t pzem017_measurement;

// Following variabbles are shared between the two cores
Big_Labels_Value     shared_big_labels_value;
Setting_Labels_Value shared_setting_labels_value;
Status_Labels_Value  shared_status_labels_value;
mutex_t              shared_data_mutex;

std::vector<std::string> wifi_list;
std::string              connected_wifi;

LVGL_App             app;
Big_Labels_Value     big_labels_value;
Setting_Labels_Value setting_labels_value;
Status_Labels_Value  status_labels_value;

MachineState machine_state;

void wifi_cb_dummy(EventData *ed);
void core1_entry();
void core0_entry();
void changes_cb(EventData *data);
bool encoder_service(struct repeating_timer *t);
bool input_service(struct repeating_timer *t);
bool one_sec_service(struct repeating_timer *t);

template <typename T>
void apply_min_max(T &value, T min, T max) {
  if (value < min)
    value = min;
  if (value > max)
    value = max;
}

int main() {
  stdio_init_all();
  sleep_ms(2000);

  if (!set_sys_clock_khz(processor_mhz * 1000, false))
    printf("set system clock to %dMHz failed\n", processor_mhz);
  else
    printf("system clock is now %dMHz\n", processor_mhz);
  clock_configure(clk_peri, 0, CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLK_SYS, processor_mhz * 1000 * 1000, processor_mhz * 1000 * 1000);

  mutex_init(&shared_data_mutex);

  multicore_launch_core1(core1_entry);

  core0_entry();
}

uint16_t test_relay_state[2] = {0, 0};

void core0_entry() {
  mbm.init();

  gpio_init(pin_buzzer);
  gpio_init(pin_start);
  gpio_init(pin_stop);
  gpio_init(pin_ac);
  gpio_init(pin_dc);

  gpio_set_dir(pin_buzzer, GPIO_OUT);
  gpio_set_dir(pin_start, GPIO_IN);
  gpio_set_dir(pin_stop, GPIO_IN);
  gpio_set_dir(pin_ac, GPIO_IN);
  gpio_set_dir(pin_dc, GPIO_IN);
  PZEM017::status_t status;
  ESP32::status_t   esp_status;
  float             temperature;

  PulseContact    sample_pulse(1.0);
  Differential_Up sample_up;

  while (true) {
    sample_pulse.service();

    sample_up.CLK(sample_pulse.Q());

    if (sample_up.Q()) {
      if (machine_state.sensed_source == Source_DC) {
        status = pzem017.request_all(pzem017_measurement);
        if (status != PZEM017::No_Error) {
          printf("PZEM017 Error: %s\n", pzem017.error_to_string(status));
        } else {
          // pzem017.calibrate();
          // sleep_ms(5000);
          printf("Voltage: %f\n", pzem017_measurement.voltage);
          printf("Current: %f\n", pzem017_measurement.current);
          printf("Power: %f\n", pzem017_measurement.power);
          printf("Energy: %f\n", pzem017_measurement.energy);

          shared_big_labels_value.v  = pzem017_measurement.voltage;
          shared_big_labels_value.a  = pzem017_measurement.current;
          shared_big_labels_value.w  = pzem017_measurement.power;
          shared_big_labels_value.wh = pzem017_measurement.energy;
        }
      } else if (machine_state.sensed_source == Source_AC) {
      } else {
        shared_big_labels_value.v  = 0;
        shared_big_labels_value.a  = 0;
        shared_big_labels_value.w  = 0;
        shared_big_labels_value.wh = 0;
      }

      esp_status = esp32.set_relay_state(test_relay_state[0], 0);
      if (esp_status != ESP32::No_Error) {
        printf("ESP32 Error: %s\n", esp32.error_to_string(esp_status));
      } else {
        esp_status = esp32.set_relay_state(test_relay_state[1], 1);
        esp_status = esp32.request_temperature(temperature);
        if (esp_status == ESP32::No_Error) {
          printf("Temperature: %f\n", temperature);
        }
        esp_status = esp32.request_sensed_source((Sensed_Source &) machine_state.sensed_source);
        if (esp_status == ESP32::No_Error) {
          printf("Sensed Source: %d\n", machine_state.sensed_source);
        }
      }

      // mutex_enter_blocking(&shared_data_mutex);
      shared_status_labels_value.temp = temperature;
      // mutex_exit(&shared_data_mutex);
    }
  }
  return;
}

void core1_entry() {
  lvgl_display_init();
  app.app_entry();

  encoder.init();

  connected_wifi = "NaN";
  wifi_list.push_back("SSID1");
  wifi_list.push_back("SSID2");

  encoder.set_enable_acceleration(true);
  app.attach_internal_changes_cb(changes_cb);
  app.set_connected_wifi(connected_wifi);
  app.attach_wifi_cb(wifi_cb_dummy);
  app.set_wifi_list(wifi_list);

  add_repeating_timer_ms(10, input_service, NULL, &io_service_timer);
  add_repeating_timer_us(100, encoder_service, NULL, &encoder_service_timer);
  add_repeating_timer_ms(1000, one_sec_service, NULL, &one_sec_timer);

  while (true) {
    // mutex_enter_blocking(&shared_data_mutex);
    big_labels_value     = shared_big_labels_value;
    setting_labels_value = shared_setting_labels_value;
    status_labels_value  = shared_status_labels_value;
    // mutex_exit(&shared_data_mutex);

    app.app_update(big_labels_value, setting_labels_value, status_labels_value);

    lv_timer_handler();
    sleep_ms(5);
  }

  return;
}

bool one_sec_service(struct repeating_timer *t) {
  static float last_voltage = shared_big_labels_value.v;  // store the previous voltage value
  // mutex_enter_blocking(&shared_data_mutex);
  if (shared_status_labels_value.started) {
    shared_status_labels_value.time_running++;
    if (shared_status_labels_value.time_running >= shared_setting_labels_value.timer && shared_setting_labels_value.timer != 0) {
      shared_status_labels_value.started = false;
      app.modal_create_alert("Timer telah berakhir, menghentikan load bank");
    }
    if (shared_setting_labels_value.cutoff_e != 0 && shared_big_labels_value.wh >= shared_setting_labels_value.cutoff_e) {
      shared_status_labels_value.started = false;
      app.modal_create_alert("Energi telah mencapai batas, menghentikan load bank");
    }
    if (shared_setting_labels_value.cutoff_v != 0) {
      // Check for falling edge: previous voltage > cutoff && current voltage <= cutoff
      if (last_voltage > shared_setting_labels_value.cutoff_v && shared_big_labels_value.v <= shared_setting_labels_value.cutoff_v) {
        shared_status_labels_value.started = false;
        app.modal_create_alert("Tegangan telah mencapai batas bawah, menghentikan load bank");
      }
    }
    // Update the previous voltage value
    last_voltage = shared_big_labels_value.v;
  }
  // mutex_exit(&shared_data_mutex);
  return true;
}

static bool modal_active = false;
void        start_cb() {
  machine_state.started                   = true;
  shared_status_labels_value.started      = true;
  shared_status_labels_value.time_running = 0;
}

uint8_t shift_counter = 0;

bool input_service(struct repeating_timer *t) {
  static Sensed_Source last_ac_dc_off = Source_Off;
  // mutex_enter_blocking(&shared_data_mutex);
  start.CLK(gpio_get(pin_start));
  stop.CLK(gpio_get(pin_stop));
  ac_dc_off = (Sensed_Source) ((gpio_get(pin_ac) << 1) | gpio_get(pin_dc));
  if (start.Q()) {
    if (shift_counter < 12)
      test_relay_state[0] |= (1 << shift_counter);
    else
      test_relay_state[1] |= (1 << (shift_counter - 12));

    for (int i = 0; i < 12; i++) {
      printf("%d", (test_relay_state[0] >> i) & 1);
    }
    printf(" ");
    for (int i = 0; i < 8; i++) {
      printf("%d", (test_relay_state[1] >> i) & 1);
    }
    printf("\n");
    shift_counter++;
    if (shift_counter > 20) {
      shift_counter       = 0;
      test_relay_state[0] = 0;
      test_relay_state[1] = 0;
    }
  }
  // if (start.Q() && !modal_active) {
  //   if (machine_state.polarity_flipped) {
  //     app.modal_create_confirm(nullptr, start_cb, "Apakah anda yakin ingin memulai load bank?", "Polaritas Terbalik", bs_warning);
  //   } else if (ac_dc_off == Source_Off) {
  //     // app.modal_create_alert("Sumber daya belum dipilih, silahkan pilih sumber daya terlebih dahulu", "Peringatan!");
  //     app.modal_create_confirm(nullptr, start_cb, "Sumber daya belum dipilih\nApakah anda yakin ingin memulai load bank?", "Sumber daya belum dipilih", bs_warning);
  //   } else if (ac_dc_off != machine_state.sensed_source) {
  //     // app.modal_create_alert("Sumber daya yang dipilih tidak sesuai dengan sumber daya yang terdeteksi");
  //     app.modal_create_confirm(nullptr, start_cb, "Sumber daya yang dipilih tidak sesuai dengan sumber daya yang terdeteksi\nApakah anda yakin ingin memulai load bank?", "Sumber daya tidak sesuai", bs_warning);
  //   } else if (shared_big_labels_value.v == 0) {
  //     app.modal_create_alert("Tegangan belum terdeteksi, silahkan cek koneksi tegangan");
  //   } else {
  //     start_cb();
  //   }
  // }

  if (stop.Q()) {
    machine_state.started              = false;
    shared_status_labels_value.started = false;
  }
  if (ac_dc_off != last_ac_dc_off) {
    app.set_source_highlight(ac_dc_off, true);
  }
  last_ac_dc_off = ac_dc_off;
  // mutex_exit(&shared_data_mutex);
  return true;
}

bool encoder_service(struct repeating_timer *t) {
  static int encoder_value           = 0;
  static int last_encoder_value      = 0;
  static int undivided_encoder_value = 0;
  static int encoder_delta           = 0;

  // mutex_enter_blocking(&shared_data_mutex);
  undivided_encoder_value += encoder.get_value();
  encoder_value                                     = undivided_encoder_value / 4;
  ClickEncoder::Button b                            = encoder.get_button();
  encoder_delta                                     = encoder_value - last_encoder_value;
  Setting_Highlighted_Container highlighted_setting = app.get_highlighted_setting();
  switch (highlighted_setting) {
  case Setpoint:
    encoder.set_enable_acceleration(false);
    shared_setting_labels_value.setpoint += encoder_delta * 5.0;
    apply_min_max<float>(shared_setting_labels_value.setpoint, 0.0, 100.0);
    break;
  case Timer:
    encoder.set_enable_acceleration(true);
    encoder.set_acceleration_properties(300, 5, 64000);
    shared_setting_labels_value.timer += encoder_delta;
    apply_min_max<int32_t>(shared_setting_labels_value.timer, 0, 1000000);
    break;
  case CutOff_V:
    encoder.set_enable_acceleration(true);
    encoder.set_acceleration_properties(200, 10, 16000);
    shared_setting_labels_value.cutoff_v += encoder_delta * 0.1;
    apply_min_max<float>(shared_setting_labels_value.cutoff_v, 0.0, 300.0);
    break;
  case CutOff_E:
    encoder.set_enable_acceleration(true);
    encoder.set_acceleration_properties(400, 10, 32000);
    shared_setting_labels_value.cutoff_e += encoder_delta * 1.0;
    apply_min_max<float>(shared_setting_labels_value.cutoff_e, 0.0, 1000000.0);
    break;
  }
  if (b == ClickEncoder::Clicked) {
    highlighted_setting = static_cast<Setting_Highlighted_Container>((highlighted_setting + 1) % 4);
    app.set_setting_highlight(highlighted_setting, true);
  }
  last_encoder_value = encoder_value;
  // mutex_exit(&shared_data_mutex);
  encoder.service();
  return true;
}

void wifi_cb_dummy(EventData *ed) {
  printf("WiFi callback\n");
  EventType event_type = ed->event_type;
  lv_obj_t *overlay;
  switch (event_type) {
  case SCAN_WIFI:
    break;
  case CONNECT_WIFI:
    WidgetParameterData *data2 = nullptr;
    data2                      = (WidgetParameterData *) ed->data.param;
    char       *ssid           = (char *) data2->param;
    const char *pwd            = lv_textarea_get_text(ed->textarea);
    break;
  }
}

void changes_cb(EventData *ed) {
  const char *txt = lv_textarea_get_text(ed->textarea);
  double      data;
  // mutex_enter_blocking(&shared_data_mutex);
  switch (ed->event_type) {
  case PROPAGATE_CUTOFF_E:
    data = atof(txt);
    apply_min_max<double>(data, 0.0, 1000000.0);
    shared_setting_labels_value.cutoff_e = data;
    break;
  case PROPAGATE_CUTOFF_V:
    data = atof(txt);
    apply_min_max<double>(data, 0.0, 300.0);
    shared_setting_labels_value.cutoff_v = data;
    break;
  case PROPAGATE_SETPOINT:
    data = atof(txt);
    apply_min_max<double>(data, 0.0, 100.0);
    data                                 = round(data / 5.0) * 5.0;
    shared_setting_labels_value.setpoint = data;
    break;
  case PROPAGATE_TIMER:
    int hour = 0, minute = 0, second = 0;
    sscanf(txt, "%d:%d:%d", &hour, &minute, &second);
    apply_min_max<int>(hour, 0, 99);
    apply_min_max<int>(minute, 0, 59);
    apply_min_max<int>(second, 0, 59);
    shared_setting_labels_value.timer = hour * 3600 + minute * 60 + second;
    break;
  }
  // mutex_exit(&shared_data_mutex);
}