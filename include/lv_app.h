#ifndef LV_APP_HPP
#define LV_APP_HPP

#include "lvgl.h"
#include "src/misc/lv_color.h"
#include "stdint.h"

LV_IMG_DECLARE(STP_SPLASH);
#define DISPLAY_SIZE_Y 320

#define SPLASH_ANIM 1

namespace lvgl_app {

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

struct Highlightable_Container
{
    lv_obj_t * label;
    lv_obj_t * container;
};

struct Highlightable_Containers
{
    Highlightable_Container setpoint;
    Highlightable_Container source_ac;
    Highlightable_Container source_dc;
    Highlightable_Container source_off;
    Highlightable_Container cutoff_v;
    Highlightable_Container cutoff_e;
    Highlightable_Container timer;
    Highlightable_Container start_stop;
};

struct Big_Labels
{
    lv_obj_t * v_label;
    lv_obj_t * a_label;
    lv_obj_t * w_label;
    lv_obj_t * wh_label;
};

struct Top_Grid_Labels
{
    lv_obj_t * temp_label;
    lv_obj_t * timer_label;
    lv_obj_t * wifi_label;
};

extern struct Highlightable_Containers highlightable_containers;
extern struct Big_Labels big_labels;
extern struct Top_Grid_Labels top_grid_labels;
extern const char * bottom_home_btns[];
extern lv_obj_t * scr_home;

void splash_screen(uint32_t delay);
void settings_screen(uint32_t delay);
void app_entry();
void splash_screen(uint32_t delay);
lv_obj_t * create_row_container(lv_obj_t * parent, int flex_grow, void (*create_child_cb)(lv_obj_t *));
void home_screen(uint32_t delay);

} // namespace lvgl_app
#endif