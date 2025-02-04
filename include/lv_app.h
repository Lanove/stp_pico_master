#ifndef LV_APP_HPP
#define LV_APP_HPP

#include <functional>

#include "lvgl.h"
#include "src/misc/lv_color.h"
#include "stdint.h"
#include "stdio.h"

LV_IMG_DECLARE(STP_SPLASH);
#define DISPLAY_SIZE_Y 320

#define SPLASH_ANIM 0

struct Highlightable_Container {
  lv_obj_t *label;
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
  float    setpoint;
  uint32_t timer;
  float    cutoff_v;
  float    cutoff_e;
};

struct Status_Labels_Value {
  bool     wifi;
  bool     started;
  float    temp;
  uint32_t time_running;
};

typedef enum {
  Setpoint,
  Timer,
  CutOff_V,
  CutOff_E,
} Setting_Highlighted_Container;

typedef enum {
  Source_AC,
  Source_DC,
  Source_Off,
} Source_Highlighted_Container;

class LVGL_App {
 private:
 public:
  void app_entry();
  void app_update(const Big_Labels_Value &big_labels_value, const Setting_Labels_Value &setting_labels_value,
                  const Status_Labels_Value &status_labels_value);

 private:
  const char *bottom_home_btns[5] = {"Setpoint", "Timer", "Cut-off V", "Cut-off E", "Settings"};
  lv_obj_t   *scr_home;

  Setting_Highlighted_Container setting_highlight;
  Source_Highlighted_Container  source_highlight;

  Highlightable_Containers highlightable_containers;
  Big_Labels               big_labels;
  Top_Grid_Labels          top_grid_labels;
  Bottom_Grid_Buttons      bottom_grid_buttons;

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
  void button_event_handler(lv_event_t *e);

  void      set_setting_highlight(Setting_Highlighted_Container container, bool highlight);
  void      set_source_highlight(Source_Highlighted_Container container, bool highlight);
  void      clear_setting_highlight();
  void      clear_source_highlight();
  void      set_highlight_container(lv_obj_t *container, bool highlight);
  void      splash_screen(uint32_t delay);
  void      settings_screen(uint32_t delay);
  lv_obj_t *create_row_container(lv_obj_t *parent, int flex_grow, std::function<void(lv_obj_t *)> create_child_cb);
  void      home_screen(uint32_t delay);
};

#endif