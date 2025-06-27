#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <string>
#include <vector>

#include "click_encoder.h"
#include "esp32.h"
#include "hardware/clocks.h"
#include "hardware/pll.h"
#include "hardware/structs/clocks.h"
#include "hardware/structs/pll.h"
#include "hardware/structs/rosc.h"
#include "hardware/vreg.h"
#include "ili9486_drivers.h"
#include "lv_app.h"
#include "lv_drivers.h"
#include "math.h"
#include "modbus_master.h"
#include "pico/cyw43_arch.h"
#include "pico/multicore.h"
#include "pico/stdlib.h"
#include "pico/sync.h"
#include "pico/time.h"
#include "plc_utility.hpp"
#include "pzem017.h"
#include "xpt2046.h"

// lwIP includes for HTTP client
#include "lwip/tcp.h"
#include "lwip/dns.h"
#include "lwip/err.h"
#include "lwip/pbuf.h"

struct MachineState {
  bool          relay_state[2];
  bool          started;
  Sensed_Source sensed_source    = Source_DC;
  bool          polarity_flipped = false;
};

static repeating_timer one_sec_timer;

constexpr int processor_mhz = 250;

bool reset_pzem;

uint8_t                pin_buzzer = 15;
uint8_t                pin_start  = 21;
uint8_t                pin_stop   = 22;
uint8_t                pin_dc     = 20;
uint8_t                pin_ac     = 19;
uint8_t                pin_relay0 = 2;
uint8_t                pin_relay1 = 4;
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

PZEM017                pzem017 = PZEM017(mbm, 0xF8);
PZEM017::measurement_t pzem017_measurement;

// Following variabbles are shared between the two cores
Big_Labels_Value     shared_big_labels_value;
Setting_Labels_Value shared_setting_labels_value;
Status_Labels_Value  shared_status_labels_value;
mutex_t              shared_data_mutex;

std::vector<std::string> wifi_list;
std::string              connected_wifi;

// WiFi scanning state
static bool            wifi_scan_in_progress = false;
static absolute_time_t wifi_scan_timeout;

// HTTP client state
struct http_client_state {
  struct tcp_pcb *tcp_pcb;
  ip_addr_t       server_ip;
  bool            connected;
  bool            request_sent;
  bool            response_complete;
  char           *response_buffer;
  size_t          response_length;
  size_t          response_capacity;
};

static struct http_client_state http_state = {0};
static bool                     http_request_in_progress = false;
static absolute_time_t          last_http_request_time;
static const uint32_t           HTTP_REQUEST_INTERVAL_MS = 3000;  // Send request every 3 seconds

LVGL_App             app;
Big_Labels_Value     big_labels_value;
Setting_Labels_Value setting_labels_value;
Status_Labels_Value  status_labels_value;

MachineState machine_state;

void        wifi_cb_dummy(EventData *ed);
void        core1_entry();
void        core0_entry();
void        changes_cb(EventData *data);
bool        encoder_service(struct repeating_timer *t);
bool        input_service(struct repeating_timer *t);
bool        one_sec_service(struct repeating_timer *t);
const char *wifi_error_to_string_id(int error_code);

// WiFi helper functions
bool is_wifi_connected();
void start_wifi_scan();
void check_wifi_scan_completion();

// HTTP client functions
void http_client_init();
void http_client_close();
err_t http_client_connected(void *arg, struct tcp_pcb *tpcb, err_t err);
err_t http_client_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
err_t http_client_sent(void *arg, struct tcp_pcb *tpcb, u16_t len);
void http_client_err(void *arg, err_t err);
void http_send_request();
void http_client_process();

template <typename T>
void apply_min_max(T &value, T min, T max) {
  if (value < min)
    value = min;
  if (value > max)
    value = max;
}

static int scan_result(void *env, const cyw43_ev_scan_result_t *result) {
  if (result) {
    printf("ssid: %-32s rssi: %4d chan: %3d mac: %02x:%02x:%02x:%02x:%02x:%02x sec: %u\n", result->ssid, result->rssi, result->channel,
           result->bssid[0], result->bssid[1], result->bssid[2], result->bssid[3], result->bssid[4], result->bssid[5], result->auth_mode);

    // Add the SSID to wifi_list if it's not empty and not already in the list
    if (strlen((char *) result->ssid) > 0) {
      std::string ssid_str((char *) result->ssid);
      // Check if this SSID is already in the list
      auto it = std::find(wifi_list.begin(), wifi_list.end(), ssid_str);
      if (it == wifi_list.end()) {
        wifi_list.push_back(ssid_str);
      }
    }
  }
  return 0;
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

  if (cyw43_arch_init()) {
    printf("failed to initialise\n");
    return 1;
  }

  cyw43_arch_enable_sta_mode();

  multicore_launch_core1(core1_entry);

  core0_entry();
}

void core0_entry() {
  mbm.init();

  gpio_init(pin_buzzer);
  gpio_init(pin_start);
  gpio_init(pin_stop);
  gpio_init(pin_ac);
  gpio_init(pin_dc);
  gpio_init(pin_relay0);
  gpio_init(pin_relay1);

  gpio_set_dir(pin_buzzer, GPIO_OUT);
  gpio_set_dir(pin_relay0, GPIO_OUT);
  gpio_set_dir(pin_relay1, GPIO_OUT);
  gpio_set_dir(pin_start, GPIO_IN);
  gpio_set_dir(pin_stop, GPIO_IN);
  gpio_set_dir(pin_ac, GPIO_IN);
  gpio_set_dir(pin_dc, GPIO_IN);
  PZEM017::status_t pzem017_status;
  ESP32::status_t   esp_status;
  float             temperature;

  PulseContact    sample_pulse(0.25);
  Differential_Up sample_up;

  while (true) {
    int setpoint_idx = static_cast<int>(shared_setting_labels_value.setpoint / 5);
    sample_pulse.service();
    sample_up.CLK(sample_pulse.Q());

    if (machine_state.started) {
      int resistance_idx = (int) setting_labels_value.setpoint / 50.;
      if (resistance_idx > 0)
        gpio_put(pin_relay0, 0);
      if (resistance_idx > 1)
        gpio_put(pin_relay1, 0);
    } else {
      gpio_put(pin_relay0, 1);
      gpio_put(pin_relay1, 1);
    }

    if (sample_up.Q()) {
      mbm.change_stop_bits(2);
      if (reset_pzem) {
        pzem017_status = pzem017.reset_energy();
        reset_pzem     = false;
        if (pzem017_status != PZEM017::No_Error)
          printf("Reset Energy PZEM017 Error: %s\n", pzem017.error_to_string(pzem017_status));
        shared_big_labels_value.wh = 0;
      }

      pzem017_status = pzem017.request_all(pzem017_measurement);
      if (pzem017_status != PZEM017::No_Error) {
        printf("PZEM017 Error: %s\n", pzem017.error_to_string(pzem017_status));
      } else {
        shared_big_labels_value.v  = pzem017_measurement.voltage;
        shared_big_labels_value.a  = pzem017_measurement.current;
        shared_big_labels_value.w  = pzem017_measurement.power;
        shared_big_labels_value.wh = pzem017_measurement.energy;

        // printf("DC Voltage: %f\n", shared_big_labels_value.v);
        // printf("DC Current: %f\n", shared_big_labels_value.a);
        // printf("DC Power: %f\n", shared_big_labels_value.w);
        // printf("DC Energy: %f\n", shared_big_labels_value.wh);
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

  // Check initial WiFi connection status
  if (is_wifi_connected()) {
    connected_wifi = "Connected";  // We could improve this by getting actual SSID
  } else {
    connected_wifi = "NaN";
  }

  // Initialize with empty WiFi list (will be populated when scan is performed)
  wifi_list.clear();

  encoder.set_enable_acceleration(true);
  app.attach_internal_changes_cb(changes_cb);

  app.set_connected_wifi(connected_wifi);
  app.set_wifi_list(wifi_list);
  app.attach_wifi_cb(wifi_cb_dummy);
  app.set_wifi_status(is_wifi_connected());

  // Initialize HTTP client
  http_client_init();
  last_http_request_time = get_absolute_time();

  add_repeating_timer_ms(10, input_service, NULL, &io_service_timer);
  add_repeating_timer_us(100, encoder_service, NULL, &encoder_service_timer);
  add_repeating_timer_ms(1000, one_sec_service, NULL, &one_sec_timer);

  while (true) {
    // Check WiFi scan completion
    check_wifi_scan_completion();

    // Process HTTP client when WiFi is connected
    http_client_process();

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
      machine_state.started              = false;
      app.modal_create_alert("Timer telah berakhir, menghentikan load bank");
    }
    if (shared_setting_labels_value.cutoff_e != 0 && shared_big_labels_value.wh >= shared_setting_labels_value.cutoff_e) {
      shared_status_labels_value.started = false;
      machine_state.started              = false;
      app.modal_create_alert("Energi telah mencapai batas, menghentikan load bank");
    }
    if (shared_setting_labels_value.cutoff_v != 0) {
      // Check for falling edge: previous voltage > cutoff && current voltage <= cutoff
      if (last_voltage > shared_setting_labels_value.cutoff_v && shared_big_labels_value.v <= shared_setting_labels_value.cutoff_v) {
        shared_status_labels_value.started = false;
        machine_state.started              = false;
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
  reset_pzem                              = true;
}

bool input_service(struct repeating_timer *t) {
  static Sensed_Source last_ac_dc_off = Source_Off;
  // mutex_enter_blocking(&shared_data_mutex);
  start.CLK(gpio_get(pin_start));
  stop.CLK(gpio_get(pin_stop));
  ac_dc_off = (Sensed_Source) ((gpio_get(pin_ac) << 1) | gpio_get(pin_dc));

  if (start.Q() && !modal_active) {
    if (machine_state.polarity_flipped) {
      app.modal_create_confirm(nullptr, start_cb, "Apakah anda yakin ingin memulai load bank?", "Polaritas Terbalik", bs_warning);
    } else if (ac_dc_off == Source_Off) {
      // app.modal_create_alert("Sumber daya belum dipilih, silahkan pilih sumber daya terlebih dahulu", "Peringatan!");
      app.modal_create_confirm(nullptr, start_cb, "Sumber daya belum dipilih\nApakah anda yakin ingin memulai load bank?",
                               "Sumber daya belum dipilih", bs_warning);
    } else if (ac_dc_off != machine_state.sensed_source) {
      // app.modal_create_alert("Sumber daya yang dipilih tidak sesuai dengan sumber daya yang terdeteksi");
      app.modal_create_confirm(nullptr, start_cb,
                               "Sumber daya yang dipilih tidak sesuai dengan sumber daya yang terdeteksi\nApakah anda yakin ingin memulai load bank?",
                               "Sumber daya tidak sesuai", bs_warning);
    } else if (shared_big_labels_value.v == 0) {
      app.modal_create_alert("Tegangan belum terdeteksi, silahkan cek koneksi tegangan");
    } else if (shared_big_labels_value.v >= 110) {
      app.modal_create_alert("Tegangan terdeteksi terlalu tinggi, tidak bisa memulai load bank");
    } else {
      start_cb();
    }
  }

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

  undivided_encoder_value += encoder.get_value();
  encoder_value                                     = undivided_encoder_value / 4;
  ClickEncoder::Button b                            = encoder.get_button();
  encoder_delta                                     = encoder_value - last_encoder_value;
  Setting_Highlighted_Container highlighted_setting = app.get_highlighted_setting();
  switch (highlighted_setting) {
  case Setpoint:
    encoder.set_enable_acceleration(false);
    shared_setting_labels_value.setpoint += encoder_delta * 50.0;
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

lv_obj_t *wifi_scan_overlay;
void      wifi_cb_dummy(EventData *ed) {
  printf("WiFi callback\n");
  EventType event_type = ed->event_type;
  switch (event_type) {
  case PROPAGATE_OVERLAY:
    if (ed->data.issuer) {
      wifi_scan_overlay = ed->data.issuer;
    }
    break;
  case SCAN_WIFI:
    printf("Starting WiFi scan...\n");
    start_wifi_scan();
    break;
  case CONNECT_WIFI:
    WidgetParameterData *data2 = nullptr;
    data2                      = (WidgetParameterData *) ed->data.param;
    char       *ssid           = (char *) data2->param;
    const char *pwd            = lv_textarea_get_text(ed->textarea);
    printf("Connecting to WiFi: %s\n", ssid);

    // Attempt to connect to WiFi
    bool is_connected = is_wifi_connected();
    if (is_wifi_connected()) {
      cyw43_arch_disable_sta_mode();
      sleep_ms(500);  // Give some time for the interface to reset
      cyw43_arch_enable_sta_mode();
      sleep_ms(500);  // Give some time for the interface to stabilize
    }

    int result = cyw43_arch_wifi_connect_timeout_ms(ssid, pwd, CYW43_AUTH_WPA2_AES_PSK, 10000);
    app.set_wifi_status(is_wifi_connected());
    if (result == 0) {
      connected_wifi = std::string(ssid);

      app.set_connected_wifi(connected_wifi);

      printf("Successfully connected to %s\n", ssid);
      
      // Reset HTTP client state when WiFi connection changes
      if (http_request_in_progress) {
        http_client_close();
      }
      last_http_request_time = get_absolute_time();
    } else {
      printf("Failed to connect to %s (error: %d)\n", ssid, result);
      
      // Close HTTP client if WiFi connection failed
      if (http_request_in_progress) {
        http_client_close();
      }
    }

    if (wifi_scan_overlay) {
      lv_obj_clean(wifi_scan_overlay);
      lv_obj_del(wifi_scan_overlay);
      wifi_scan_overlay        = nullptr;
      lv_obj_t *setting_button = app.get_bottom_grid_buttons().settings;
      lv_obj_send_event(setting_button, LV_EVENT_CLICKED, nullptr);
    }

    std::string status_msg = "Menghubungkan ke ";
    status_msg += ssid;
    status_msg += "\nStatus: ";
    status_msg += wifi_error_to_string_id(result);
    app.modal_create_alert(status_msg.c_str());
    break;
  }
}

// Helper function to check if WiFi is connected
bool is_wifi_connected() {
  return cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA) == CYW43_LINK_UP;
}

// Helper function to start WiFi scan
void start_wifi_scan() {
  if (wifi_scan_in_progress) {
    printf("WiFi scan already in progress\n");
    return;
  }

  // Clear the existing WiFi list
  wifi_list.clear();

  // Start the WiFi scan
  cyw43_wifi_scan_options_t scan_options = {0};
  int                       result       = cyw43_wifi_scan(&cyw43_state, &scan_options, NULL, scan_result);

  if (result == 0) {
    wifi_scan_in_progress = true;
    wifi_scan_timeout     = make_timeout_time_ms(10000);  // 10 second timeout
    printf("WiFi scan started successfully\n");
  } else {
    printf("Failed to start WiFi scan (error: %d)\n", result);
  }
}

// Helper function to check WiFi scan completion
void check_wifi_scan_completion() {
  if (!wifi_scan_in_progress) {
    return;
  }

  // Check if scan has completed or timed out
  if (!cyw43_wifi_scan_active(&cyw43_state) || absolute_time_diff_us(get_absolute_time(), wifi_scan_timeout) <= 0) {
    wifi_scan_in_progress = false;
    printf("WiFi scan completed. Found %zu networks\n", wifi_list.size());

    // Check if we're connected to any of the found networks
    if (is_wifi_connected()) {
      // Get current connected network info if possible
      // For now, we'll keep the connected_wifi as is if it's already set
      if (connected_wifi == "NaN" || connected_wifi.empty()) {
        connected_wifi = "Connected";  // Fallback if we can't get SSID
      }
    } else {
      if (connected_wifi != "NaN") {
        connected_wifi = "NaN";
      }
    }

    // Update the app with the new WiFi list and connection status
    app.set_wifi_list(wifi_list);
    app.set_connected_wifi(connected_wifi);
    app.set_wifi_status(is_wifi_connected());

    if (wifi_scan_overlay) {
      lv_obj_clean(wifi_scan_overlay);
      lv_obj_del(wifi_scan_overlay);
      wifi_scan_overlay        = nullptr;
      lv_obj_t *setting_button = app.get_bottom_grid_buttons().settings;
      lv_obj_send_event(setting_button, LV_EVENT_CLICKED, nullptr);
    }
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
    apply_min_max<double>(data, 0.0, 101.0);
    if (data < 25.0)
      data = 0.0;
    else if (data < 75.0)
      data = 50.0;
    else
      data = 100.0;
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

const char *wifi_error_to_string_id(int error_code) {
  switch (error_code) {
  case 0:
    return "Berhasil";
  case -1:
    return "Kesalahan umum";
  case -2:
    return "Timeout - koneksi gagal dalam waktu yang ditentukan";
  case -3:
    return "Password salah atau tidak valid";
  case -4:
    return "Jaringan WiFi tidak ditemukan";
  case -5:
    return "Kesalahan autentikasi";
  case -6:
    return "Kesalahan protokol WiFi";
  case -7:
    return "WiFi adapter tidak siap";
  case -8:
    return "Koneksi ditolak oleh access point";
  case -9:
    return "Sumber daya tidak mencukupi";
  case -10:
    return "Operasi tidak didukung";
  case -11:
    return "Parameter tidak valid";
  case -12:
    return "WiFi belum diinisialisasi";
  case -13:
    return "Operasi dibatalkan";
  case -14:
    return "Buffer terlalu kecil";
  case -15:
    return "Kesalahan hardware";
  default:
    return "Kesalahan tidak dikenal";
  }
}

// HTTP Client Implementation
void http_client_init() {
  memset(&http_state, 0, sizeof(http_state));
  http_state.response_capacity = 1024;  // Initial capacity
  http_state.response_buffer = (char*)malloc(http_state.response_capacity);
  if (!http_state.response_buffer) {
    printf("Failed to allocate HTTP response buffer\n");
    return;
  }
  
  // Set server IP address (192.168.1.22)
  IP4_ADDR(&http_state.server_ip, 192, 168, 1, 22);
  
  printf("HTTP client initialized\n");
}

void http_client_close() {
  if (http_state.tcp_pcb) {
    tcp_arg(http_state.tcp_pcb, NULL);
    tcp_sent(http_state.tcp_pcb, NULL);
    tcp_recv(http_state.tcp_pcb, NULL);
    tcp_err(http_state.tcp_pcb, NULL);
    tcp_close(http_state.tcp_pcb);
    http_state.tcp_pcb = NULL;
  }
  
  if (http_state.response_buffer) {
    free(http_state.response_buffer);
    http_state.response_buffer = NULL;
  }
  
  http_state.connected = false;
  http_state.request_sent = false;
  http_state.response_complete = false;
  http_state.response_length = 0;
  http_request_in_progress = false;
  
  printf("HTTP client closed\n");
}

err_t http_client_connected(void *arg, struct tcp_pcb *tpcb, err_t err) {
  if (err != ERR_OK) {
    printf("HTTP connection failed: %d\n", err);
    http_client_close();
    return err;
  }
  
  printf("HTTP connected to server\n");
  http_state.connected = true;
  
  // Get current sensor readings
  float voltage = shared_big_labels_value.v;
  float current = shared_big_labels_value.a;
  float power = shared_big_labels_value.w;
  float energy = shared_big_labels_value.wh;
  bool is_started = machine_state.started; // Use machine_state.started instead of shared_status_labels_value.started
  
  // Create simplified JSON payload - let server handle timestamp and defaults
  char json_payload[256];
  snprintf(json_payload, sizeof(json_payload),
    "{"
    "\"voltage\":%.2f,"
    "\"current\":%.2f,"
    "\"power\":%.2f,"
    "\"energy\":%.2f,"
    "\"source\":\"DC\","
    "\"temperature\":30,"
    "\"is_started\":%s"
    "}",
    voltage, current, power, energy,
    is_started ? "true" : "false"
  );
  
  // Create HTTP POST request
  char request[512];
  snprintf(request, sizeof(request),
    "POST /api/readings HTTP/1.1\r\n"
    "Host: 192.168.1.22:5000\r\n"
    "User-Agent: PicoW-HMI/1.0\r\n"
    "Content-Type: application/json\r\n"
    "Content-Length: %d\r\n"
    "Connection: close\r\n"
    "\r\n"
    "%s",
    (int)strlen(json_payload), json_payload
  );
  
  printf("Sending sensor data: %s\n", json_payload);
  
  err_t write_err = tcp_write(tpcb, request, strlen(request), TCP_WRITE_FLAG_COPY);
  if (write_err == ERR_OK) {
    tcp_output(tpcb);
    http_state.request_sent = true;
    printf("HTTP POST request sent\n");
  } else {
    printf("Failed to send HTTP request: %d\n", write_err);
    http_client_close();
    return write_err;
  }
  
  return ERR_OK;
}

err_t http_client_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
  if (err != ERR_OK) {
    printf("HTTP receive error: %d\n", err);
    if (p) pbuf_free(p);
    http_client_close();
    return err;
  }
  
  if (p == NULL) {
    // Connection closed by server
    printf("HTTP response complete (connection closed)\n");
    http_state.response_complete = true;
    
    // Process the response (you can add your response processing here)
    if (http_state.response_length > 0) {
      printf("HTTP Response received (%zu bytes):\n", http_state.response_length);
      // Add null terminator for safe string operations
      if (http_state.response_length < http_state.response_capacity) {
        http_state.response_buffer[http_state.response_length] = '\0';
        
        // Find the start of the HTTP body (after headers)
        char *body_start = strstr(http_state.response_buffer, "\r\n\r\n");
        if (body_start) {
          body_start += 4; // Skip the "\r\n\r\n"
          printf("Response headers + body:\n%s\n", http_state.response_buffer);
          printf("Response body only: %s\n", body_start);
        } else {
          printf("Response content (full): %s\n", http_state.response_buffer);
        }
      } else {
        printf("Response too large for buffer, truncated at %zu bytes\n", http_state.response_capacity);
      }
    }
    
    http_client_close();
    return ERR_OK;
  }
  
  // Receive data
  size_t data_len = p->tot_len;
  
  // Ensure we have enough buffer space
  if (http_state.response_length + data_len >= http_state.response_capacity) {
    size_t new_capacity = http_state.response_capacity * 2;
    while (new_capacity <= http_state.response_length + data_len) {
      new_capacity *= 2;
    }
    
    char *new_buffer = (char*)realloc(http_state.response_buffer, new_capacity);
    if (new_buffer) {
      http_state.response_buffer = new_buffer;
      http_state.response_capacity = new_capacity;
    } else {
      printf("Failed to expand HTTP response buffer\n");
      pbuf_free(p);
      http_client_close();
      return ERR_MEM;
    }
  }
  
  // Copy data to buffer
  pbuf_copy_partial(p, http_state.response_buffer + http_state.response_length, data_len, 0);
  http_state.response_length += data_len;
  
  // Acknowledge received data
  tcp_recved(tpcb, p->tot_len);
  pbuf_free(p);
  
  printf("HTTP data received: %u bytes (total: %zu)\n", data_len, http_state.response_length);
  
  return ERR_OK;
}

err_t http_client_sent(void *arg, struct tcp_pcb *tpcb, u16_t len) {
  printf("HTTP data sent: %u bytes\n", len);
  return ERR_OK;
}

void http_client_err(void *arg, err_t err) {
  printf("HTTP client error: %d\n", err);
  http_client_close();
}

void http_send_request() {
  if (http_request_in_progress) {
    printf("HTTP request already in progress\n");
    return;
  }
  
  if (!is_wifi_connected()) {
    printf("Cannot send HTTP request: WiFi not connected\n");
    return;
  }
  
  printf("Starting HTTP request to 192.168.1.22:5000\n");
  
  // Create new TCP PCB
  http_state.tcp_pcb = tcp_new();
  if (!http_state.tcp_pcb) {
    printf("Failed to create TCP PCB for HTTP request\n");
    return;
  }
  
  // Reset response state
  http_state.response_length = 0;
  http_state.connected = false;
  http_state.request_sent = false;
  http_state.response_complete = false;
  http_request_in_progress = true;
  
  // Set up TCP callbacks
  tcp_arg(http_state.tcp_pcb, &http_state);
  tcp_sent(http_state.tcp_pcb, http_client_sent);
  tcp_recv(http_state.tcp_pcb, http_client_recv);
  tcp_err(http_state.tcp_pcb, http_client_err);
  
  // Connect to server (port 5000)
  err_t err = tcp_connect(http_state.tcp_pcb, &http_state.server_ip, 5000, http_client_connected);
  if (err != ERR_OK) {
    printf("Failed to start HTTP connection: %d\n", err);
    http_client_close();
  }
}

void http_client_process() {
  if (!is_wifi_connected()) {
    // If WiFi is disconnected, clean up any ongoing HTTP operations
    if (http_request_in_progress) {
      printf("WiFi disconnected, closing HTTP client\n");
      http_client_close();
    }
    return;
  }
  
  // Check if it's time to send a new request
  if (!http_request_in_progress) {
    absolute_time_t current_time = get_absolute_time();
    if (absolute_time_diff_us(last_http_request_time, current_time) >= (HTTP_REQUEST_INTERVAL_MS * 1000)) {
      http_send_request();
      last_http_request_time = current_time;
    }
  }
}