#ifndef LV_APP_HPP
#define LV_APP_HPP

#include <stdio.h>
#include <stdlib.h>

#include <algorithm>
#include <functional>

#include "esp32.h"
#include "lv_colors.h"
#include "lvgl.h"
#include "src/misc/lv_color.h"
#include "stdint.h"
#include "stdio.h"
#include "string"
#include "string.h"
#include "vector"

const float resistance_map[] = {9e6, 11.1, 5.578};

LV_IMG_DECLARE(STP_SPLASH);
#define DISPLAY_SIZE_Y 320

#define SPLASH_ANIM 0

struct Keyboards {
  lv_obj_t *numeric;
  lv_obj_t *password;
};

struct Highlightable_Container {
  lv_obj_t *label;
  lv_obj_t *label2;
  lv_obj_t *container;
};

struct Highlightable_Containers {
  Highlightable_Container setpoint;
  Highlightable_Container source_ac;
  Highlightable_Container source_dc;
  Highlightable_Container source_off;
  Highlightable_Container cutoff_v;
  Highlightable_Container cutoff_e;
  Highlightable_Container timer;
  Highlightable_Container start_stop;
};

struct Big_Labels {
  lv_obj_t *v_label;
  lv_obj_t *a_label;
  lv_obj_t *w_label;
  lv_obj_t *wh_label;
};

struct Top_Grid_Labels {
  lv_obj_t *temp_label;
  lv_obj_t *timer_label;
  lv_obj_t *wifi_label;
};

struct Bottom_Grid_Buttons {
  lv_obj_t *setpoint;
  lv_obj_t *timer;
  lv_obj_t *cutoff_v;
  lv_obj_t *cutoff_e;
  lv_obj_t *settings;
};

struct Big_Labels_Value {
  float v;
  float a;
  float w;
  float wh;
};

struct Setting_Labels_Value {
  float   setpoint;
  int32_t timer;
  float   cutoff_v;
  float   cutoff_e;
};

struct Status_Labels_Value {
  bool     wifi;
  bool     started;
  float    temp;
  uint32_t time_running;
};

typedef enum {
  Setpoint,
  CutOff_V,
  CutOff_E,
  Timer,
} Setting_Highlighted_Container;

typedef enum {
  MODAL_CONFIRM_EVENT,
  PROPAGATE_CUTOFF_E,
  PROPAGATE_CUTOFF_V,
  PROPAGATE_SETPOINT,
  PROPAGATE_TIMER,
  SCAN_WIFI,
  CONNECT_WIFI,
  PROPAGATE_OVERLAY
} EventType;

struct WidgetParameterData {
  lv_obj_t *issuer = NULL;
  void     *param  = NULL;
};

struct ModalConfirmData {
  WidgetParameterData  *data       = nullptr;
  std::function<void()> confirm_cb = nullptr;
};

// Custom data structure to pass through events
struct EventData {
  WidgetParameterData data;
  lv_obj_t           *overlay;
  lv_obj_t           *textarea;
  EventType           event_type;
};

class LVGL_App {
 private:
  typedef enum { FORMAT_DECIMAL, FORMAT_CLOCK, FORMAT_PASSWORD } NumberFormatType;

 public:
  void app_entry();
  void app_update(const Big_Labels_Value &big_labels_value, const Setting_Labels_Value &setting_labels_value,
                  const Status_Labels_Value &status_labels_value);

  void set_setting_highlight(Setting_Highlighted_Container container, bool highlight);
  void set_source_highlight(Sensed_Source container, bool highlight);
  void set_start_stop_highlight(bool started);

  Setting_Highlighted_Container get_highlighted_setting() { return setting_highlight; }

  void attach_internal_changes_cb(std::function<void(EventData *)> internal_changes_cb) { this->internal_changes_cb = internal_changes_cb; }
  void attach_wifi_cb(std::function<void(EventData *)> wifi_cb) { this->wifi_cb = wifi_cb; }

  lv_obj_t *modal_create_alert(const char *message, const char *headerText = "Informasi!", const lv_font_t *headerFont = &lv_font_montserrat_20,
                               const lv_font_t *messageFont = &lv_font_montserrat_14, lv_color_t headerTextColor = bs_white,
                               lv_color_t textColor = bs_white, lv_color_t headerColor = lv_palette_main(LV_PALETTE_BLUE),
                               const char *buttonText = "Ok", lv_coord_t xSize = lv_pct(70), lv_coord_t ySize = lv_pct(70));

  lv_obj_t *modal_create_setting(const lv_font_t *headerFont = &lv_font_montserrat_20, const lv_font_t *messageFont = &lv_font_montserrat_14,
                                 lv_color_t headerTextColor = bs_white, lv_color_t textColor = bs_white, lv_color_t headerColor = bs_warning,
                                 const char *buttonText = "Ok", lv_coord_t xSize = lv_pct(90), lv_coord_t ySize = lv_pct(90));

  void set_wifi_list(std::vector<std::string> wifi_list) { this->wifi_list = wifi_list; }
  void set_connected_wifi(std::string connected_wifi) { this->connected_wifi = connected_wifi; }

  lv_obj_t *modal_create_confirm(WidgetParameterData *widgetParameterData, std::function<void()> confirm_cb, const char *message,
                                 const char *headerText = "Informasi!", lv_color_t headerColor = lv_palette_main(LV_PALETTE_BLUE),
                                 const lv_font_t *headerFont = &lv_font_montserrat_20, const lv_font_t *messageFont = &lv_font_montserrat_14,
                                 lv_color_t headerTextColor = bs_white, lv_color_t textColor = bs_white, const char *confirmButtonText = "Ok",
                                 const char *cancelButtonText = "Batal", lv_coord_t xSize = lv_pct(70), lv_coord_t ySize = lv_pct(70));

  void set_wifi_status(bool connected);

  Bottom_Grid_Buttons &get_bottom_grid_buttons() { return bottom_grid_buttons; }

 private:
  const char              *bottom_home_btns[5] = {"Setpoint", "Cut-off V", "Cut-off E", "Timer", "Wi-Fi"};
  lv_obj_t                *scr_home;
  lv_obj_t                *kb_numeric;
  lv_obj_t                *kb_password;
  lv_obj_t                *wifi_list_obj;
  std::vector<std::string> wifi_list;
  std::string              connected_wifi;
  bool                     is_wifi_connected = false;

  Keyboards                     keyboards;
  Setting_Highlighted_Container setting_highlight;
  Sensed_Source                 source_highlight;

  Highlightable_Containers         highlightable_containers;
  Big_Labels                       big_labels;
  Top_Grid_Labels                  top_grid_labels;
  Bottom_Grid_Buttons              bottom_grid_buttons;
  std::function<void(EventData *)> internal_changes_cb = nullptr;
  std::function<void(EventData *)> wifi_cb             = nullptr;

  static constexpr uint32_t anim_time          = 500;
  static constexpr uint32_t anim_translation_y = 150;
#if SPLASH_ANIM == 1
  static constexpr uint32_t splash_fadein_dur  = 1000;
  static constexpr uint32_t splash_stay_dur    = 2000;
  static constexpr uint32_t splash_fadeout_dur = 1000;
#else
  static constexpr uint32_t splash_fadein_dur  = 0;
  static constexpr uint32_t splash_stay_dur    = 0;
  static constexpr uint32_t splash_fadeout_dur = 0;
#endif
  void        button_event_handler(lv_event_t *e);
  void        ta_event_setting_handler(lv_event_t *e, EventData *tb);
  void        kb_create_handler(lv_event_t *e);
  static void kb_custom_event_cb_static(lv_event_t *e);
  void        kb_custom_event_cb(lv_event_t *e, lv_obj_t *kb, lv_obj_t *ta);
  void        hide_kb_event_cb(lv_event_t *e, lv_obj_t *cont, Keyboards *kbs);
  static void hide_kb_event_cb_static(lv_event_t *e);
  void        ta_event_cb(lv_event_t *e, lv_obj_t *ta, lv_obj_t *kibod);
  static void ta_event_cb_static(lv_event_t *e);
  void        clear_setting_highlight();
  void        clear_source_highlight();
  void        set_highlight_container(lv_obj_t *container, bool highlight);
  void        splash_screen(uint32_t delay);
  void        settings_screen(uint32_t delay);
  lv_obj_t   *create_row_container(lv_obj_t *parent, int flex_grow, std::function<void(lv_obj_t *)> create_child_cb);
  void        home_screen(uint32_t delay);

  static lv_obj_t *lvc_create_loading(lv_obj_t *parent, const char *message);
  static lv_obj_t *lvc_create_overlay(lv_obj_t *parent);
  static void
  lvc_label_init(lv_obj_t *label, const lv_font_t *font = &lv_font_montserrat_14, lv_align_t align = LV_ALIGN_DEFAULT, lv_coord_t offsetX = 0,
                 lv_coord_t offsetY = 0, lv_color_t textColor = bs_dark, lv_text_align_t alignText = LV_TEXT_ALIGN_CENTER,
                 lv_label_long_mode_t longMode = LV_LABEL_LONG_WRAP, lv_coord_t textWidth = 0);
  static lv_obj_t *
  lvc_btn_init(lv_obj_t *btn, const char *labelText, lv_align_t align = LV_ALIGN_DEFAULT, lv_coord_t offsetX = 0, lv_coord_t offsetY = 0,
               const lv_font_t *font = &lv_font_montserrat_14, lv_color_t bgColor = lv_palette_main(LV_PALETTE_BLUE), lv_color_t textColor = bs_white,
               lv_text_align_t alignText = LV_TEXT_ALIGN_CENTER, lv_label_long_mode_t longMode = LV_LABEL_LONG_WRAP, lv_coord_t labelWidth = 0,
               lv_coord_t btnSizeX = 0, lv_coord_t btnSizeY = 0);

  lv_obj_t *modal_create_textarea_input(WidgetParameterData *data, const char *initialText, const char *headerText, NumberFormatType format,
                                        const lv_font_t *headerFont = &lv_font_montserrat_20, const lv_font_t *textboxFont = &lv_font_montserrat_16,
                                        lv_color_t headerTextColor = bs_white, lv_color_t textColor = bs_dark,
                                        lv_color_t headerColor = lv_palette_main(LV_PALETTE_BLUE), const char *confirmButtonText = "Ok",
                                        const char *cancelButtonText = "Batal", lv_coord_t xSize = lv_pct(100), lv_coord_t ySize = lv_pct(100));
};

#endif