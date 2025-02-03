#include "lv_app.h"

namespace lvgl_app {

const char *bottom_home_btns[] = {"Setpoint", "Timer", "Cut-off V", "Cut-off E",
                                  "Settings"};
lv_obj_t *scr_home;

struct Highlightable_Containers highlightable_containers;
struct Big_Labels big_labels;
struct Top_Grid_Labels top_grid_labels;
struct Bottom_Grid_Buttons bottom_grid_buttons;

void app_entry() {
  splash_screen(0);

  lv_theme_t *th = lv_theme_default_init(
      NULL, /* Use DPI, size, etc. from this display */
      lv_palette_main(LV_PALETTE_BLUE), /* Primary and secondary palette */
      lv_palette_main(LV_PALETTE_CYAN),
      true, /* Dark theme?  False = light theme. */
      &lv_font_montserrat_12 /* Small, normal, large fonts */);

  lv_display_set_theme(NULL, th); /* Assign theme to display */

  // Open home screen after logo showcase is done
  home_screen(splash_fadein_dur + splash_stay_dur + splash_fadeout_dur);
}

void splash_screen(uint32_t delay) {
  lv_obj_t *scr = lv_obj_create(NULL);
  lv_scr_load(scr);

  lv_obj_t *hb_logo = lv_img_create(scr);
  lv_img_set_src(hb_logo, &STP_SPLASH);
  lv_obj_align(hb_logo, LV_ALIGN_CENTER, 0, 0);

  // For some reason fade-in and fade-out using built-in function didn't work
  // so use regular animation for fade-in and use built-in function for fade-out
  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, hb_logo);
  lv_anim_set_time(&a, splash_fadein_dur);
  lv_anim_set_exec_cb(&a, [](void *obj, int32_t v) {
    lv_obj_set_style_opa((lv_obj_t *)obj, v, 0);
    if (v == 255)
      lv_obj_fade_out((lv_obj_t *)obj, splash_fadeout_dur, splash_stay_dur);
  });
  lv_anim_set_values(&a, 0, 255);
  lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
  lv_anim_start(&a);
}

lv_obj_t *create_row_container(lv_obj_t *parent, int flex_grow,
                               void (*create_child_cb)(lv_obj_t *)) {
  lv_obj_t *row_container = lv_obj_create(parent);

  lv_obj_set_style_pad_all(row_container, 0, LV_PART_MAIN);
  lv_obj_set_size(row_container, LV_PCT(100), LV_SIZE_CONTENT);
  lv_obj_set_flex_flow(row_container, LV_FLEX_FLOW_ROW);
  lv_obj_set_style_pad_ver(row_container, 5, LV_PART_MAIN);
  lv_obj_set_flex_align(row_container, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_border_width(row_container, 0, LV_PART_MAIN);
  lv_obj_set_style_margin_all(row_container, 0, LV_PART_MAIN);
  lv_obj_set_flex_grow(row_container, flex_grow);

  if (create_child_cb) {
    create_child_cb(row_container); // Call the function to create children
  }

  return row_container;
}

void home_screen(uint32_t delay) {
  static int top_grid_height = 30;
  static int bottom_grid_height = 50;
  static int center_grid_height =
      DISPLAY_SIZE_Y - top_grid_height - bottom_grid_height;

  scr_home = lv_obj_create(NULL);

  lv_scr_load_anim(scr_home, LV_SCR_LOAD_ANIM_NONE, 0, delay, true);
  lv_obj_set_scrollbar_mode(scr_home, LV_SCROLLBAR_MODE_OFF);
  lv_obj_clear_flag(scr_home, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_set_style_pad_column(scr_home, 0, 0);
  lv_obj_align(scr_home, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_t *top_grid = lv_obj_create(scr_home);
  lv_obj_set_size(top_grid, lv_pct(100), top_grid_height);
  lv_obj_set_style_bg_color(top_grid, lv_palette_darken(LV_PALETTE_BLUE, 4), 0);
  lv_obj_set_style_radius(top_grid, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(top_grid, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_hor(top_grid, 10, LV_PART_MAIN);
  lv_obj_set_style_border_width(top_grid, 0, LV_PART_MAIN);

  // Temperature label
  lv_obj_t *temp_label = lv_label_create(top_grid);
  lv_label_set_text(temp_label, "125°C");
  lv_obj_set_style_text_font(temp_label, &lv_font_montserrat_20, 0);
  lv_obj_set_align(temp_label, LV_ALIGN_LEFT_MID);
  lv_obj_set_pos(temp_label, 0, 0);
  top_grid_labels.temp_label = temp_label;

  // Timer label
  lv_obj_t *timer_label = lv_label_create(top_grid);
  lv_label_set_text(timer_label, "00:00:00");
  lv_obj_set_style_text_font(timer_label, &lv_font_montserrat_20, 0);
  lv_obj_set_align(timer_label, LV_ALIGN_LEFT_MID);
  lv_obj_set_pos(timer_label, 70, 0);
  top_grid_labels.timer_label = timer_label;

  // Start/Stop container
  lv_obj_t *start_stop_cont =
      create_row_container(top_grid, 1, [](lv_obj_t *row_container) {
        lv_obj_set_style_bg_opa(row_container, LV_OPA_TRANSP, 0);
        lv_obj_t *start_stop_cont = lv_obj_create(row_container);
        lv_obj_t *start_stop_label = lv_label_create(start_stop_cont);
        lv_obj_set_size(start_stop_cont, LV_SIZE_CONTENT, 23);
        lv_obj_set_style_radius(start_stop_cont, 3, LV_PART_MAIN);
        lv_obj_set_style_border_width(start_stop_cont, 0, LV_PART_MAIN);
        lv_obj_set_style_bg_color(start_stop_cont,
                                  lv_palette_main(LV_PALETTE_GREEN), 0);
        lv_label_set_text(start_stop_label, "STARTED");
        lv_obj_clear_flag(start_stop_cont, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_align(start_stop_label, LV_ALIGN_CENTER, 0, -1);
        highlightable_containers.start_stop.container = start_stop_cont;
        highlightable_containers.start_stop.label = start_stop_label;
      });

  lv_obj_set_align(start_stop_cont, LV_ALIGN_RIGHT_MID);
  lv_obj_set_pos(start_stop_cont, -70, 0);

  // WiFi label
  lv_obj_t *wifi_label = lv_label_create(top_grid);
  lv_label_set_text(wifi_label, LV_SYMBOL_WIFI);
  lv_obj_set_style_text_font(wifi_label, &lv_font_montserrat_22, 0);
  lv_obj_set_align(wifi_label, LV_ALIGN_RIGHT_MID);
  lv_obj_set_style_text_color(wifi_label, lv_palette_darken(LV_PALETTE_GREY, 1),
                              0);
  lv_obj_t *wifi_label_cross = lv_label_create(wifi_label);
  lv_label_set_text(wifi_label_cross, LV_SYMBOL_CLOSE);
  lv_obj_set_align(wifi_label_cross, LV_ALIGN_BOTTOM_RIGHT);
  lv_obj_set_style_text_font(wifi_label_cross, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(wifi_label_cross, lv_palette_main(LV_PALETTE_RED),
                              0);
  top_grid_labels.wifi_label = wifi_label;

  lv_obj_t *center_grid = lv_obj_create(scr_home);
  lv_obj_align(center_grid, LV_ALIGN_TOP_MID, 0, top_grid_height);
  lv_obj_set_style_radius(center_grid, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(
      center_grid, 0,
      LV_PART_MAIN); // Ensure no padding in the bottom_button_grid
  lv_obj_set_style_pad_row(center_grid, 0, LV_PART_MAIN); // Remove row padding
  lv_obj_set_style_pad_column(center_grid, 0,
                              LV_PART_MAIN); // Remove column padding
  lv_obj_set_size(center_grid, lv_pct(100), center_grid_height);
  lv_obj_set_style_border_width(center_grid, 0, LV_PART_MAIN);

  lv_obj_t *left_ctr_grid = lv_obj_create(center_grid);
  lv_obj_set_size(left_ctr_grid, lv_pct(50),
                  lv_pct(100)); // Set the size of the grid
  lv_obj_set_style_radius(left_ctr_grid, 0,
                          LV_PART_MAIN); // Set border radius to 0
  lv_obj_set_style_border_width(left_ctr_grid, 0, LV_PART_MAIN);
  // lv_obj_set_style_bg_color(left_ctr_grid, lv_palette_main(LV_PALETTE_GREEN),
  // 0); // Set background color to green
  lv_obj_set_style_pad_all(
      left_ctr_grid, 0,
      LV_PART_MAIN); // Ensure no padding in the bottom_button_grid
  lv_obj_set_style_pad_row(left_ctr_grid, 0,
                           LV_PART_MAIN); // Remove row padding
  lv_obj_set_style_pad_column(left_ctr_grid, 0,
                              LV_PART_MAIN); // Remove column padding
  lv_obj_align(left_ctr_grid, LV_ALIGN_LEFT_MID, 0, 0);

  // Set the flex layout for the left_ctr_grid to arrange children in rows
  lv_obj_set_flex_flow(left_ctr_grid,
                       LV_FLEX_FLOW_COLUMN); // Arrange in a column (vertical)
  lv_obj_set_flex_align(left_ctr_grid, LV_FLEX_ALIGN_START,
                        LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER); // Align children
  lv_obj_clear_flag(left_ctr_grid, LV_OBJ_FLAG_SCROLLABLE);

  create_row_container(left_ctr_grid, 2, [](lv_obj_t *row_container) {
    lv_obj_set_style_pad_hor(row_container, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_right(row_container, 50, LV_PART_MAIN);
    // Create the big label (larger text)
    lv_obj_t *big_label = lv_label_create(row_container);
    lv_label_set_text_fmt(big_label, "123.1", 1);
    lv_obj_set_style_text_font(big_label, &lv_font_montserrat_48,
                               0); // Use a large font for big text
    big_labels.v_label = big_label;

    // Create the small label (smaller text)
    lv_obj_t *small_label = lv_label_create(row_container);
    lv_label_set_text_fmt(small_label, "%s", "V");
    lv_obj_set_style_text_font(small_label, &lv_font_montserrat_16,
                               0); // Use a smaller font for small text
  });

  create_row_container(left_ctr_grid, 2, [](lv_obj_t *row_container) {
    lv_obj_set_style_pad_hor(row_container, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_right(row_container, 50, LV_PART_MAIN);
    // Create the big label (larger text)
    lv_obj_t *big_label = lv_label_create(row_container);
    lv_label_set_text_fmt(big_label, "16.5", 1);
    lv_obj_set_style_text_font(big_label, &lv_font_montserrat_48,
                               0); // Use a large font for big text
    big_labels.a_label = big_label;

    // Create the small label (smaller text)
    lv_obj_t *small_label = lv_label_create(row_container);
    lv_label_set_text_fmt(small_label, "%s", "A");
    lv_obj_set_style_text_font(small_label, &lv_font_montserrat_16,
                               0); // Use a smaller font for small text
  });

  create_row_container(
      left_ctr_grid, 1,
      [](lv_obj_t *row_container) { // Set up the row container with flex layout
        // Create the first label (align left)
        lv_obj_set_style_pad_hor(row_container, 10, LV_PART_MAIN);
        lv_obj_t *big_label_left = lv_label_create(row_container);
        lv_label_set_text(big_label_left, "Setpoint:");
        lv_obj_set_style_text_font(big_label_left, &lv_font_montserrat_16, 0);
        lv_obj_set_flex_grow(big_label_left, 3);

        create_row_container(row_container, 3, [](lv_obj_t *row_container) {
          lv_obj_set_style_bg_color(row_container,
                                    lv_palette_main(LV_PALETTE_BLUE), 0);
          lv_obj_set_style_radius(row_container, 0, LV_PART_MAIN);
          lv_obj_set_size(row_container, LV_PCT(100), LV_PCT(100));
          lv_obj_set_flex_align(row_container, LV_FLEX_ALIGN_CENTER,
                                LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
          // Create the third label (align right)
          lv_obj_t *small_label_right = lv_label_create(row_container);
          lv_label_set_text_fmt(small_label_right, "500W", 5, "%");
          lv_obj_set_style_text_font(small_label_right, &lv_font_montserrat_16,
                                     0);
          highlightable_containers.setpoint.container = row_container;
          highlightable_containers.setpoint.label = small_label_right;
        });

        // Create the third label (align right)
        lv_obj_t *small_label_right = lv_label_create(row_container);
        lv_label_set_text_fmt(small_label_right, "(%d%s)", 5, "%");
        lv_obj_set_style_text_font(small_label_right, &lv_font_montserrat_16,
                                   0);
        lv_obj_set_flex_grow(small_label_right, 2);
      });

  create_row_container(left_ctr_grid, 1, [](lv_obj_t *row_container) {
    lv_obj_set_style_pad_hor(row_container, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_ver(row_container, 5, LV_PART_MAIN);
    lv_obj_t *source_label = lv_label_create(row_container);
    lv_label_set_text(source_label, "Source:");
    lv_obj_set_style_text_font(source_label, &lv_font_montserrat_16, 0);
    lv_obj_set_flex_grow(source_label, 2);

    create_row_container(row_container, 1, [](lv_obj_t *row_container) {
      // lv_obj_set_style_bg_color(row_container,
      // lv_palette_main(LV_PALETTE_BLUE), 0);
      lv_obj_set_style_radius(row_container, 0, LV_PART_MAIN);
      lv_obj_set_size(row_container, LV_PCT(100), LV_PCT(100));
      lv_obj_set_flex_align(row_container, LV_FLEX_ALIGN_CENTER,
                            LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
      // Create the third label (align right)
      lv_obj_t *small_label_right = lv_label_create(row_container);
      lv_label_set_text_fmt(small_label_right, "AC");
      lv_obj_set_style_text_font(small_label_right, &lv_font_montserrat_16, 0);
      highlightable_containers.source_ac.container = row_container;
      highlightable_containers.source_ac.label = small_label_right;
    });
    create_row_container(row_container, 1, [](lv_obj_t *row_container) {
      lv_obj_set_style_bg_color(row_container, lv_palette_main(LV_PALETTE_BLUE),
                                0);
      lv_obj_set_style_radius(row_container, 0, LV_PART_MAIN);
      lv_obj_set_size(row_container, LV_PCT(100), LV_PCT(100));
      lv_obj_set_flex_align(row_container, LV_FLEX_ALIGN_CENTER,
                            LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
      // Create the third label (align right)
      lv_obj_t *small_label_right = lv_label_create(row_container);
      lv_label_set_text_fmt(small_label_right, "DC");
      lv_obj_set_style_text_font(small_label_right, &lv_font_montserrat_16, 0);
      highlightable_containers.source_dc.container = row_container;
      highlightable_containers.source_dc.label = small_label_right;
    });
    create_row_container(row_container, 1, [](lv_obj_t *row_container) {
      // lv_obj_set_style_bg_color(row_container,
      // lv_palette_main(LV_PALETTE_BLUE), 0);
      lv_obj_set_style_radius(row_container, 0, LV_PART_MAIN);
      lv_obj_set_size(row_container, LV_PCT(100), LV_PCT(100));
      lv_obj_set_flex_align(row_container, LV_FLEX_ALIGN_CENTER,
                            LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
      // Create the third label (align right)
      lv_obj_t *small_label_right = lv_label_create(row_container);
      lv_label_set_text_fmt(small_label_right, "OFF");
      lv_obj_set_style_text_font(small_label_right, &lv_font_montserrat_16, 0);
      highlightable_containers.source_off.container = row_container;
      highlightable_containers.source_off.label = small_label_right;
    });
  });

  lv_obj_t *right_ctr_grid = lv_obj_create(center_grid);
  lv_obj_set_size(right_ctr_grid, lv_pct(50),
                  lv_pct(100)); // Set the size of the grid
  lv_obj_set_style_radius(right_ctr_grid, 0,
                          LV_PART_MAIN); // Set border radius to 0
  lv_obj_set_style_border_width(right_ctr_grid, 2, LV_PART_MAIN);
  lv_obj_set_style_border_side(right_ctr_grid, LV_BORDER_SIDE_LEFT, 0);
  lv_obj_set_style_border_color(right_ctr_grid,
                                lv_palette_main(LV_PALETTE_BLUE), 0);
  lv_obj_set_style_pad_all(
      right_ctr_grid, 0,
      LV_PART_MAIN); // Ensure no padding in the bottom_button_grid
  lv_obj_set_style_pad_row(right_ctr_grid, 0,
                           LV_PART_MAIN); // Remove row padding
  lv_obj_set_style_pad_column(right_ctr_grid, 0,
                              LV_PART_MAIN); // Remove column padding
  lv_obj_align(right_ctr_grid, LV_ALIGN_RIGHT_MID, 0, 0);

  // Set the flex layout for the right_ctr_grid to arrange children in rows
  lv_obj_set_flex_flow(right_ctr_grid,
                       LV_FLEX_FLOW_COLUMN); // Arrange in a column (vertical)
  lv_obj_set_flex_align(right_ctr_grid, LV_FLEX_ALIGN_START,
                        LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER); // Align children
  lv_obj_clear_flag(right_ctr_grid, LV_OBJ_FLAG_SCROLLABLE);

  create_row_container(right_ctr_grid, 6, [](lv_obj_t *row_container) {
    lv_obj_set_style_pad_hor(row_container, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_right(row_container, 35, LV_PART_MAIN);
    // Create the big label (larger text)
    lv_obj_t *big_label = lv_label_create(row_container);
    lv_label_set_text_fmt(big_label, "511.2", 1);
    lv_obj_set_style_text_font(big_label, &lv_font_montserrat_48,
                               0); // Use a large font for big text
    big_labels.w_label = big_label;

    // Create the small label (smaller text)
    lv_obj_t *small_label = lv_label_create(row_container);
    lv_label_set_text_fmt(small_label, "%s", "W");
    lv_obj_set_style_text_font(small_label, &lv_font_montserrat_16,
                               0); // Use a smaller font for small text
  });

  create_row_container(right_ctr_grid, 6, [](lv_obj_t *row_container) {
    lv_obj_set_style_pad_hor(row_container, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_right(row_container, 35, LV_PART_MAIN);
    lv_obj_set_style_pad_top(row_container, 10, LV_PART_MAIN);

    // Create the big label (larger text)
    lv_obj_t *big_label = lv_label_create(row_container);
    lv_label_set_text_fmt(big_label, "1651", 1);
    lv_obj_set_style_text_font(big_label, &lv_font_montserrat_48,
                               0); // Use a large font for big text
    big_labels.wh_label = big_label;

    // Create the small label (smaller text)
    lv_obj_t *small_label = lv_label_create(row_container);
    lv_label_set_text_fmt(small_label, "%s", "Wh");
    lv_obj_set_style_text_font(small_label, &lv_font_montserrat_16,
                               0); // Use a smaller font for small text
  });

  create_row_container(right_ctr_grid, 2, [](lv_obj_t *row_container) {
    lv_obj_set_style_pad_hor(row_container, 10, LV_PART_MAIN);

    // Create the small label (smaller text)
    lv_obj_t *cutoff_label = lv_label_create(row_container);
    lv_label_set_text_fmt(cutoff_label, "Cut-Off V:");
    lv_obj_set_style_text_font(cutoff_label, &lv_font_montserrat_16,
                               0); // Use a smaller font for small text
    lv_obj_set_flex_grow(cutoff_label, 3);

    create_row_container(row_container, 2, [](lv_obj_t *row_container) {
      lv_obj_set_style_bg_color(row_container, lv_palette_main(LV_PALETTE_BLUE),
                                0);
      lv_obj_set_style_radius(row_container, 0, LV_PART_MAIN);
      lv_obj_set_size(row_container, LV_PCT(100), LV_PCT(100));
      lv_obj_set_flex_align(row_container, LV_FLEX_ALIGN_CENTER,
                            LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
      lv_obj_clear_flag(row_container, LV_OBJ_FLAG_SCROLLABLE);
      // Create the third label (align right)
      lv_obj_t *small_label_right = lv_label_create(row_container);
      lv_label_set_text_fmt(small_label_right, "70.5");
      lv_obj_set_style_text_font(small_label_right, &lv_font_montserrat_16, 0);
      highlightable_containers.cutoff_v.container = row_container;
      highlightable_containers.cutoff_v.label = small_label_right;
    });

    // Create the third label (align right)
    lv_obj_t *small_label_right = lv_label_create(row_container);
    lv_label_set_text_fmt(small_label_right, "V");
    lv_obj_set_style_text_font(small_label_right, &lv_font_montserrat_16, 0);
    lv_obj_set_flex_grow(small_label_right, 1);
  });

  create_row_container(right_ctr_grid, 2, [](lv_obj_t *row_container) {
    lv_obj_set_style_pad_hor(row_container, 10, LV_PART_MAIN);

    // Create the small label (smaller text)
    lv_obj_t *cutoff_label = lv_label_create(row_container);
    lv_label_set_text_fmt(cutoff_label, "Cut-Off E:");
    lv_obj_set_style_text_font(cutoff_label, &lv_font_montserrat_16,
                               0); // Use a smaller font for small text
    lv_obj_set_flex_grow(cutoff_label, 3);

    create_row_container(row_container, 2, [](lv_obj_t *row_container) {
      lv_obj_set_style_bg_color(row_container, lv_palette_main(LV_PALETTE_BLUE),
                                0);
      lv_obj_set_style_radius(row_container, 0, LV_PART_MAIN);
      lv_obj_set_size(row_container, LV_PCT(100), LV_PCT(100));
      lv_obj_set_flex_align(row_container, LV_FLEX_ALIGN_CENTER,
                            LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
      lv_obj_clear_flag(row_container, LV_OBJ_FLAG_SCROLLABLE);
      // Create the third label (align right)
      lv_obj_t *small_label_right = lv_label_create(row_container);
      lv_label_set_text_fmt(small_label_right, "1250");
      lv_obj_set_style_text_font(small_label_right, &lv_font_montserrat_16, 0);
      highlightable_containers.cutoff_e.container = row_container;
      highlightable_containers.cutoff_e.label = small_label_right;
    });

    // Create the third label (align right)
    lv_obj_t *small_label_right = lv_label_create(row_container);
    lv_label_set_text_fmt(small_label_right, "Wh");
    lv_obj_set_style_text_font(small_label_right, &lv_font_montserrat_16, 0);
    lv_obj_set_flex_grow(small_label_right, 1);
  });

  create_row_container(right_ctr_grid, 2, [](lv_obj_t *row_container) {
    lv_obj_set_style_pad_hor(row_container, 10, LV_PART_MAIN);

    // Create the small label (smaller text)
    lv_obj_t *cutoff_label = lv_label_create(row_container);
    lv_label_set_text_fmt(cutoff_label, "Timer:");
    lv_obj_set_style_text_font(cutoff_label, &lv_font_montserrat_16,
                               0); // Use a smaller font for small text
    lv_obj_set_flex_grow(cutoff_label, 3);

    create_row_container(row_container, 3, [](lv_obj_t *row_container) {
      lv_obj_set_style_bg_color(row_container, lv_palette_main(LV_PALETTE_BLUE),
                                0);
      lv_obj_set_style_radius(row_container, 0, LV_PART_MAIN);
      lv_obj_set_size(row_container, LV_PCT(100), LV_PCT(100));
      lv_obj_set_flex_align(row_container, LV_FLEX_ALIGN_CENTER,
                            LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
      lv_obj_clear_flag(row_container, LV_OBJ_FLAG_SCROLLABLE);
      // Create the third label (align right)
      lv_obj_t *small_label_right = lv_label_create(row_container);
      lv_label_set_text_fmt(small_label_right, "12:50:20");
      lv_obj_set_style_text_font(small_label_right, &lv_font_montserrat_16, 0);
      highlightable_containers.cutoff_e.container = row_container;
      highlightable_containers.cutoff_e.label = small_label_right;
    });
  });

  // Set the flex layout for the right_ctr_grid to arrange children in rows
  lv_obj_set_flex_flow(right_ctr_grid,
                       LV_FLEX_FLOW_COLUMN); // Arrange in a column (vertical)
  lv_obj_set_flex_align(right_ctr_grid, LV_FLEX_ALIGN_START,
                        LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER); // Align children
  lv_obj_clear_flag(right_ctr_grid, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_align(right_ctr_grid, LV_ALIGN_RIGHT_MID, 0, 0);

  lv_obj_t *bottom_button_grid = lv_obj_create(
      scr_home); // Create bottom_button_grid as a child of the screen
  lv_obj_set_size(bottom_button_grid, lv_pct(100),
                  bottom_grid_height); // Set the bottom_button_grid size
  lv_obj_set_style_radius(bottom_button_grid, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(
      bottom_button_grid, 0,
      LV_PART_MAIN); // Ensure no padding in the bottom_button_grid
  lv_obj_set_style_pad_row(bottom_button_grid, 0,
                           LV_PART_MAIN); // Remove row padding
  lv_obj_set_style_pad_column(bottom_button_grid, 0,
                              LV_PART_MAIN); // Remove column padding

  lv_obj_align(bottom_button_grid, LV_ALIGN_BOTTOM_MID, 0,
               0); // Align bottom_button_grid to bottom

  static lv_coord_t bottom_col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1),
                                        LV_GRID_FR(1), LV_GRID_FR(1),
                                        LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
  static lv_coord_t bottom_row_dsc[] = {LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
  lv_obj_set_grid_dsc_array(bottom_button_grid, bottom_col_dsc, bottom_row_dsc);

  for (int i = 0; i < 5; i++) {
    lv_obj_t *btn = lv_btn_create(bottom_button_grid); // Create a button
    lv_obj_set_grid_cell(btn, LV_GRID_ALIGN_STRETCH, i, 1,
                         LV_GRID_ALIGN_STRETCH, 0, 1);

    // Style the button
    lv_obj_set_style_radius(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_margin_all(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_margin_hor(btn, 1, LV_PART_MAIN);
    lv_obj_set_height(btn, 40);

    // Add a label to the button
    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text_fmt(label, "%s", bottom_home_btns[i]);
    lv_obj_center(label); // Center the label on the button
    lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);

    // Add the button to the button array
    if (i == 0) {
      bottom_grid_buttons.setpoint = btn;
    } else if (i == 1) {
      bottom_grid_buttons.timer = btn;
    } else if (i == 2) {
      bottom_grid_buttons.cutoff_v = btn;
    } else if (i == 3) {
      bottom_grid_buttons.cutoff_e = btn;
    } else if (i == 4) {
      bottom_grid_buttons.settings = btn;
    }

    // Add button event handler
    lv_obj_add_event_cb(btn, button_event_handler, LV_EVENT_ALL, NULL);
  }
}

static void button_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = (lv_obj_t*)lv_event_get_target(e);

    if(code == LV_EVENT_CLICKED) {
        if(obj == bottom_grid_buttons.setpoint) {
            printf("Setpoint clicked");
        }
        else if(obj == bottom_grid_buttons.timer) {
            printf("Timer clicked");
        }
        else if(obj == bottom_grid_buttons.cutoff_v) {
            printf("Cut-off V clicked");
        }
        else if(obj == bottom_grid_buttons.cutoff_e) {
            printf("Cut-off E clicked");
        }
        else if(obj == bottom_grid_buttons.settings) {
            printf("Settings clicked");
        }
    }
}

} // namespace lvgl_app