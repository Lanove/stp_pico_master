#ifndef _LV_DRIVERS_H
#define _LV_DRIVERS_H
// #include "globals.h"
#include "lvgl.h"
#include "ili9486_drivers.h"
#include "hardware/clocks.h"
#include "xpt2046.h"

extern void init_display();
void lvgl_display_init(ili9486_drivers& driver, XPT2046 &touch);
#endif