#include "lv_app.h"

void LVGL_App::app_entry() {
  splash_screen(0);

  lv_theme_t *th = lv_theme_default_init(NULL,                                   /* Use DPI, size, etc. from this display */
                                         lv_palette_main(LV_PALETTE_BLUE),       /* Primary and secondary palette */
                                         lv_palette_main(LV_PALETTE_CYAN), true, /* Dark theme?  False = light theme. */
                                         &lv_font_montserrat_12 /* Small, normal, large fonts */);

  lv_display_set_theme(NULL, th); /* Assign theme to display */

  // Open home screen after logo showcase is done
  home_screen(splash_fadein_dur + splash_stay_dur + splash_fadeout_dur);
}

void LVGL_App::splash_screen(uint32_t delay) {
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
    lv_obj_set_style_opa((lv_obj_t *) obj, v, 0);
    if (v == 255)
      lv_obj_fade_out((lv_obj_t *) obj, LVGL_App::splash_fadeout_dur, LVGL_App::splash_stay_dur);
  });
  lv_anim_set_values(&a, 0, 255);
  lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
  lv_anim_start(&a);
}

lv_obj_t *LVGL_App::create_row_container(lv_obj_t *parent, int flex_grow, std::function<void(lv_obj_t *)> create_child_cb) {
  lv_obj_t *row_container = lv_obj_create(parent);

  lv_obj_set_style_pad_all(row_container, 0, LV_PART_MAIN);
  lv_obj_set_size(row_container, LV_PCT(100), LV_SIZE_CONTENT);
  lv_obj_set_flex_flow(row_container, LV_FLEX_FLOW_ROW);
  lv_obj_set_style_pad_ver(row_container, 5, LV_PART_MAIN);
  lv_obj_set_flex_align(row_container, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_border_width(row_container, 0, LV_PART_MAIN);
  lv_obj_set_style_margin_all(row_container, 0, LV_PART_MAIN);
  lv_obj_set_flex_grow(row_container, flex_grow);

  if (create_child_cb) {
    create_child_cb(row_container);  // Call the function to create children
  }

  return row_container;
}

void LVGL_App::home_screen(uint32_t delay) {
  static int top_grid_height    = 30;
  static int bottom_grid_height = 50;
  static int center_grid_height = DISPLAY_SIZE_Y - top_grid_height - bottom_grid_height;

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
  lv_obj_t *start_stop_cont = create_row_container(top_grid, 1, [this](lv_obj_t *row_container) {
    lv_obj_set_style_bg_opa(row_container, LV_OPA_TRANSP, 0);
    lv_obj_t *start_stop_cont  = lv_obj_create(row_container);
    lv_obj_t *start_stop_label = lv_label_create(start_stop_cont);
    lv_obj_set_size(start_stop_cont, LV_SIZE_CONTENT, 23);
    lv_obj_set_style_radius(start_stop_cont, 3, LV_PART_MAIN);
    lv_obj_set_style_border_width(start_stop_cont, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(start_stop_cont, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_label_set_text(start_stop_label, "STARTED");
    lv_obj_clear_flag(start_stop_cont, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_align(start_stop_label, LV_ALIGN_CENTER, 0, -1);
    highlightable_containers.start_stop.container = start_stop_cont;
    highlightable_containers.start_stop.label     = start_stop_label;
  });

  lv_obj_set_align(start_stop_cont, LV_ALIGN_RIGHT_MID);
  lv_obj_set_pos(start_stop_cont, -70, 0);

  // WiFi label
  lv_obj_t *wifi_label = lv_label_create(top_grid);
  lv_label_set_text(wifi_label, LV_SYMBOL_WIFI);
  lv_obj_set_style_text_font(wifi_label, &lv_font_montserrat_22, 0);
  lv_obj_set_align(wifi_label, LV_ALIGN_RIGHT_MID);
  lv_obj_set_style_text_color(wifi_label, lv_palette_darken(LV_PALETTE_GREY, 1), 0);
  lv_obj_t *wifi_label_cross = lv_label_create(wifi_label);
  lv_label_set_text(wifi_label_cross, LV_SYMBOL_CLOSE);
  lv_obj_set_align(wifi_label_cross, LV_ALIGN_BOTTOM_RIGHT);
  lv_obj_set_style_text_font(wifi_label_cross, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(wifi_label_cross, lv_palette_main(LV_PALETTE_RED), 0);
  top_grid_labels.wifi_label = wifi_label;

  lv_obj_t *center_grid = lv_obj_create(scr_home);
  lv_obj_align(center_grid, LV_ALIGN_TOP_MID, 0, top_grid_height);
  lv_obj_set_style_radius(center_grid, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(center_grid, 0,
                           LV_PART_MAIN);                  // Ensure no padding in the bottom_button_grid
  lv_obj_set_style_pad_row(center_grid, 0, LV_PART_MAIN);  // Remove row padding
  lv_obj_set_style_pad_column(center_grid, 0,
                              LV_PART_MAIN);  // Remove column padding
  lv_obj_set_size(center_grid, lv_pct(100), center_grid_height);
  lv_obj_set_style_border_width(center_grid, 0, LV_PART_MAIN);

  lv_obj_t *left_ctr_grid = lv_obj_create(center_grid);
  lv_obj_set_size(left_ctr_grid, lv_pct(50),
                  lv_pct(100));  // Set the size of the grid
  lv_obj_set_style_radius(left_ctr_grid, 0,
                          LV_PART_MAIN);  // Set border radius to 0
  lv_obj_set_style_border_width(left_ctr_grid, 0, LV_PART_MAIN);
  // lv_obj_set_style_bg_color(left_ctr_grid, lv_palette_main(LV_PALETTE_GREEN),
  // 0); // Set background color to green
  lv_obj_set_style_pad_all(left_ctr_grid, 0,
                           LV_PART_MAIN);  // Ensure no padding in the bottom_button_grid
  lv_obj_set_style_pad_row(left_ctr_grid, 0,
                           LV_PART_MAIN);  // Remove row padding
  lv_obj_set_style_pad_column(left_ctr_grid, 0,
                              LV_PART_MAIN);  // Remove column padding
  lv_obj_align(left_ctr_grid, LV_ALIGN_LEFT_MID, 0, 0);

  // Set the flex layout for the left_ctr_grid to arrange children in rows
  lv_obj_set_flex_flow(left_ctr_grid,
                       LV_FLEX_FLOW_COLUMN);  // Arrange in a column (vertical)
  lv_obj_set_flex_align(left_ctr_grid, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);  // Align children
  lv_obj_clear_flag(left_ctr_grid, LV_OBJ_FLAG_SCROLLABLE);

  create_row_container(left_ctr_grid, 2, [this](lv_obj_t *row_container) {
    lv_obj_set_style_pad_hor(row_container, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_right(row_container, 50, LV_PART_MAIN);
    // Create the big label (larger text)
    lv_obj_t *big_label = lv_label_create(row_container);
    lv_label_set_text_fmt(big_label, "0", 1);
    lv_obj_set_style_text_font(big_label, &lv_font_montserrat_48,
                               0);  // Use a large font for big text
    big_labels.v_label = big_label;

    // Create the small label (smaller text)
    lv_obj_t *small_label = lv_label_create(row_container);
    lv_label_set_text_fmt(small_label, "%s", "V");
    lv_obj_set_style_text_font(small_label, &lv_font_montserrat_16,
                               0);  // Use a smaller font for small text
  });

  create_row_container(left_ctr_grid, 2, [this](lv_obj_t *row_container) {
    lv_obj_set_style_pad_hor(row_container, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_right(row_container, 50, LV_PART_MAIN);
    // Create the big label (larger text)
    lv_obj_t *big_label = lv_label_create(row_container);
    lv_label_set_text_fmt(big_label, "0", 1);
    lv_obj_set_style_text_font(big_label, &lv_font_montserrat_48,
                               0);  // Use a large font for big text
    big_labels.a_label = big_label;

    // Create the small label (smaller text)
    lv_obj_t *small_label = lv_label_create(row_container);
    lv_label_set_text_fmt(small_label, "%s", "A");
    lv_obj_set_style_text_font(small_label, &lv_font_montserrat_16,
                               0);  // Use a smaller font for small text
  });

  create_row_container(left_ctr_grid, 1, [this](lv_obj_t *row_container) {  // Set up the row container with flex layout
    // Create the first label (align left)
    lv_obj_set_style_pad_hor(row_container, 10, LV_PART_MAIN);
    lv_obj_t *big_label_left = lv_label_create(row_container);
    lv_label_set_text(big_label_left, "Setpoint:");
    lv_obj_set_style_text_font(big_label_left, &lv_font_montserrat_16, 0);
    lv_obj_set_flex_grow(big_label_left, 3);

    create_row_container(row_container, 3, [this](lv_obj_t *row_container) {
      lv_obj_set_style_bg_color(row_container, lv_palette_main(LV_PALETTE_BLUE), 0);
      lv_obj_set_style_bg_opa(row_container, LV_OPA_TRANSP, 0);

      lv_obj_set_style_radius(row_container, 0, LV_PART_MAIN);
      lv_obj_set_size(row_container, LV_PCT(100), LV_PCT(100));
      lv_obj_set_flex_align(row_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
      // Create the third label (align right)
      lv_obj_t *small_label_right = lv_label_create(row_container);
      lv_label_set_text_fmt(small_label_right, "0W");
      lv_obj_set_style_text_font(small_label_right, &lv_font_montserrat_16, 0);
      highlightable_containers.setpoint.container = row_container;
      highlightable_containers.setpoint.label     = small_label_right;
    });

    // Create the third label (align right)
    lv_obj_t *small_label_right = lv_label_create(row_container);
    lv_label_set_text_fmt(small_label_right, "(%d%s)", 0, "%");
    lv_obj_set_style_text_font(small_label_right, &lv_font_montserrat_16, 0);
    lv_obj_set_flex_grow(small_label_right, 2);
  });

  create_row_container(left_ctr_grid, 1, [this](lv_obj_t *row_container) {
    lv_obj_set_style_pad_hor(row_container, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_ver(row_container, 5, LV_PART_MAIN);
    lv_obj_t *source_label = lv_label_create(row_container);
    lv_label_set_text(source_label, "Source:");
    lv_obj_set_style_text_font(source_label, &lv_font_montserrat_16, 0);
    lv_obj_set_flex_grow(source_label, 2);

    create_row_container(row_container, 1, [this](lv_obj_t *row_container) {
      lv_obj_set_style_bg_color(row_container, lv_palette_main(LV_PALETTE_BLUE), 0);
      lv_obj_set_style_bg_opa(row_container, LV_OPA_TRANSP, 0);

      lv_obj_set_style_radius(row_container, 0, LV_PART_MAIN);
      lv_obj_set_size(row_container, LV_PCT(100), LV_PCT(100));
      lv_obj_set_flex_align(row_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
      // Create the third label (align right)
      lv_obj_t *small_label_right = lv_label_create(row_container);
      lv_label_set_text_fmt(small_label_right, "AC");
      lv_obj_set_style_text_font(small_label_right, &lv_font_montserrat_16, 0);
      highlightable_containers.source_ac.container = row_container;
      highlightable_containers.source_ac.label     = small_label_right;
    });
    create_row_container(row_container, 1, [this](lv_obj_t *row_container) {
      lv_obj_set_style_bg_color(row_container, lv_palette_main(LV_PALETTE_BLUE), 0);
      lv_obj_set_style_bg_opa(row_container, LV_OPA_TRANSP, 0);

      lv_obj_set_style_radius(row_container, 0, LV_PART_MAIN);
      lv_obj_set_size(row_container, LV_PCT(100), LV_PCT(100));
      lv_obj_set_flex_align(row_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
      // Create the third label (align right)
      lv_obj_t *small_label_right = lv_label_create(row_container);
      lv_label_set_text_fmt(small_label_right, "DC");
      lv_obj_set_style_text_font(small_label_right, &lv_font_montserrat_16, 0);
      highlightable_containers.source_dc.container = row_container;
      highlightable_containers.source_dc.label     = small_label_right;
    });
    create_row_container(row_container, 1, [this](lv_obj_t *row_container) {
      lv_obj_set_style_bg_color(row_container, lv_palette_main(LV_PALETTE_BLUE), 0);
      lv_obj_set_style_bg_opa(row_container, LV_OPA_TRANSP, 0);

      lv_obj_set_style_radius(row_container, 0, LV_PART_MAIN);
      lv_obj_set_size(row_container, LV_PCT(100), LV_PCT(100));
      lv_obj_set_flex_align(row_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
      // Create the third label (align right)
      lv_obj_t *small_label_right = lv_label_create(row_container);
      lv_label_set_text_fmt(small_label_right, "OFF");
      lv_obj_set_style_text_font(small_label_right, &lv_font_montserrat_16, 0);
      highlightable_containers.source_off.container = row_container;
      highlightable_containers.source_off.label     = small_label_right;
    });
  });

  lv_obj_t *right_ctr_grid = lv_obj_create(center_grid);
  lv_obj_set_size(right_ctr_grid, lv_pct(50),
                  lv_pct(100));  // Set the size of the grid
  lv_obj_set_style_radius(right_ctr_grid, 0,
                          LV_PART_MAIN);  // Set border radius to 0
  lv_obj_set_style_border_width(right_ctr_grid, 2, LV_PART_MAIN);
  lv_obj_set_style_border_side(right_ctr_grid, LV_BORDER_SIDE_LEFT, 0);
  lv_obj_set_style_border_color(right_ctr_grid, lv_palette_main(LV_PALETTE_BLUE), 0);
  lv_obj_set_style_pad_all(right_ctr_grid, 0,
                           LV_PART_MAIN);  // Ensure no padding in the bottom_button_grid
  lv_obj_set_style_pad_row(right_ctr_grid, 0,
                           LV_PART_MAIN);  // Remove row padding
  lv_obj_set_style_pad_column(right_ctr_grid, 0,
                              LV_PART_MAIN);  // Remove column padding
  lv_obj_align(right_ctr_grid, LV_ALIGN_RIGHT_MID, 0, 0);

  // Set the flex layout for the right_ctr_grid to arrange children in rows
  lv_obj_set_flex_flow(right_ctr_grid,
                       LV_FLEX_FLOW_COLUMN);  // Arrange in a column (vertical)
  lv_obj_set_flex_align(right_ctr_grid, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);  // Align children
  lv_obj_clear_flag(right_ctr_grid, LV_OBJ_FLAG_SCROLLABLE);

  create_row_container(right_ctr_grid, 6, [this](lv_obj_t *row_container) {
    lv_obj_set_style_pad_hor(row_container, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_right(row_container, 35, LV_PART_MAIN);
    // Create the big label (larger text)
    lv_obj_t *big_label = lv_label_create(row_container);
    lv_label_set_text_fmt(big_label, "0", 1);
    lv_obj_set_style_text_font(big_label, &lv_font_montserrat_48,
                               0);  // Use a large font for big text
    big_labels.w_label = big_label;

    // Create the small label (smaller text)
    lv_obj_t *small_label = lv_label_create(row_container);
    lv_label_set_text_fmt(small_label, "%s", "W");
    lv_obj_set_style_text_font(small_label, &lv_font_montserrat_16,
                               0);  // Use a smaller font for small text
  });

  create_row_container(right_ctr_grid, 6, [this](lv_obj_t *row_container) {
    lv_obj_set_style_pad_hor(row_container, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_right(row_container, 35, LV_PART_MAIN);
    lv_obj_set_style_pad_top(row_container, 10, LV_PART_MAIN);

    // Create the big label (larger text)
    lv_obj_t *big_label = lv_label_create(row_container);
    lv_label_set_text_fmt(big_label, "0", 1);
    lv_obj_set_style_text_font(big_label, &lv_font_montserrat_48,
                               0);  // Use a large font for big text
    big_labels.wh_label = big_label;

    // Create the small label (smaller text)
    lv_obj_t *small_label = lv_label_create(row_container);
    lv_label_set_text_fmt(small_label, "%s", "Wh");
    lv_obj_set_style_text_font(small_label, &lv_font_montserrat_16,
                               0);  // Use a smaller font for small text
  });

  create_row_container(right_ctr_grid, 2, [this](lv_obj_t *row_container) {
    lv_obj_set_style_pad_hor(row_container, 10, LV_PART_MAIN);

    // Create the small label (smaller text)
    lv_obj_t *cutoff_label = lv_label_create(row_container);
    lv_label_set_text_fmt(cutoff_label, "Cut-Off V:");
    lv_obj_set_style_text_font(cutoff_label, &lv_font_montserrat_16,
                               0);  // Use a smaller font for small text
    lv_obj_set_flex_grow(cutoff_label, 3);

    create_row_container(row_container, 2, [this](lv_obj_t *row_container) {
      lv_obj_set_style_bg_color(row_container, lv_palette_main(LV_PALETTE_BLUE), 0);
      lv_obj_set_style_bg_opa(row_container, LV_OPA_TRANSP, 0);

      lv_obj_set_style_radius(row_container, 0, LV_PART_MAIN);
      lv_obj_set_size(row_container, LV_PCT(100), LV_PCT(100));
      lv_obj_set_flex_align(row_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
      lv_obj_clear_flag(row_container, LV_OBJ_FLAG_SCROLLABLE);
      // Create the third label (align right)
      lv_obj_t *small_label_right = lv_label_create(row_container);
      lv_label_set_text_fmt(small_label_right, "Off");
      lv_obj_set_style_text_font(small_label_right, &lv_font_montserrat_16, 0);
      highlightable_containers.cutoff_v.container = row_container;
      highlightable_containers.cutoff_v.label     = small_label_right;
    });

    // Create the third label (align right)
    lv_obj_t *small_label_right = lv_label_create(row_container);
    lv_label_set_text_fmt(small_label_right, "V");
    lv_obj_set_style_text_font(small_label_right, &lv_font_montserrat_16, 0);
    lv_obj_set_flex_grow(small_label_right, 1);
  });

  create_row_container(right_ctr_grid, 2, [this](lv_obj_t *row_container) {
    lv_obj_set_style_pad_hor(row_container, 10, LV_PART_MAIN);

    // Create the small label (smaller text)
    lv_obj_t *cutoff_label = lv_label_create(row_container);
    lv_label_set_text_fmt(cutoff_label, "Cut-Off E:");
    lv_obj_set_style_text_font(cutoff_label, &lv_font_montserrat_16,
                               0);  // Use a smaller font for small text
    lv_obj_set_flex_grow(cutoff_label, 3);

    create_row_container(row_container, 2, [this](lv_obj_t *row_container) {
      lv_obj_set_style_bg_color(row_container, lv_palette_main(LV_PALETTE_BLUE), 0);
      lv_obj_set_style_bg_opa(row_container, LV_OPA_TRANSP, 0);

      lv_obj_set_style_radius(row_container, 0, LV_PART_MAIN);
      lv_obj_set_size(row_container, LV_PCT(100), LV_PCT(100));
      lv_obj_set_flex_align(row_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
      lv_obj_clear_flag(row_container, LV_OBJ_FLAG_SCROLLABLE);
      // Create the third label (align right)
      lv_obj_t *small_label_right = lv_label_create(row_container);
      lv_label_set_text_fmt(small_label_right, "Off");
      lv_obj_set_style_text_font(small_label_right, &lv_font_montserrat_16, 0);
      highlightable_containers.cutoff_e.container = row_container;
      highlightable_containers.cutoff_e.label     = small_label_right;
    });

    // Create the third label (align right)
    lv_obj_t *small_label_right = lv_label_create(row_container);
    lv_label_set_text_fmt(small_label_right, "Wh");
    lv_obj_set_style_text_font(small_label_right, &lv_font_montserrat_16, 0);
    lv_obj_set_flex_grow(small_label_right, 1);
  });

  create_row_container(right_ctr_grid, 2, [this](lv_obj_t *row_container) {
    lv_obj_set_style_pad_hor(row_container, 10, LV_PART_MAIN);

    // Create the small label (smaller text)
    lv_obj_t *cutoff_label = lv_label_create(row_container);
    lv_label_set_text_fmt(cutoff_label, "Timer:");
    lv_obj_set_style_text_font(cutoff_label, &lv_font_montserrat_16,
                               0);  // Use a smaller font for small text
    lv_obj_set_flex_grow(cutoff_label, 3);

    create_row_container(row_container, 3, [this](lv_obj_t *row_container) {
      lv_obj_set_style_bg_color(row_container, lv_palette_main(LV_PALETTE_BLUE), 0);
      lv_obj_set_style_bg_opa(row_container, LV_OPA_TRANSP, 0);

      lv_obj_set_style_radius(row_container, 0, LV_PART_MAIN);
      lv_obj_set_size(row_container, LV_PCT(100), LV_PCT(100));
      lv_obj_set_flex_align(row_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
      lv_obj_clear_flag(row_container, LV_OBJ_FLAG_SCROLLABLE);
      // Create the third label (align right)
      lv_obj_t *small_label_right = lv_label_create(row_container);
      lv_label_set_text_fmt(small_label_right, "Off");
      lv_obj_set_style_text_font(small_label_right, &lv_font_montserrat_16, 0);
      highlightable_containers.timer.container = row_container;
      highlightable_containers.timer.label     = small_label_right;
    });
  });

  // Set the flex layout for the right_ctr_grid to arrange children in rows
  lv_obj_set_flex_flow(right_ctr_grid,
                       LV_FLEX_FLOW_COLUMN);  // Arrange in a column (vertical)
  lv_obj_set_flex_align(right_ctr_grid, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);  // Align children
  lv_obj_clear_flag(right_ctr_grid, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_align(right_ctr_grid, LV_ALIGN_RIGHT_MID, 0, 0);

  lv_obj_t *bottom_button_grid = lv_obj_create(scr_home);  // Create bottom_button_grid as a child of the screen
  lv_obj_set_size(bottom_button_grid, lv_pct(100),
                  bottom_grid_height);  // Set the bottom_button_grid size
  lv_obj_set_style_radius(bottom_button_grid, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(bottom_button_grid, 0,
                           LV_PART_MAIN);  // Ensure no padding in the bottom_button_grid
  lv_obj_set_style_pad_row(bottom_button_grid, 0,
                           LV_PART_MAIN);  // Remove row padding
  lv_obj_set_style_pad_column(bottom_button_grid, 0,
                              LV_PART_MAIN);  // Remove column padding

  lv_obj_align(bottom_button_grid, LV_ALIGN_BOTTOM_MID, 0,
               0);  // Align bottom_button_grid to bottom

  static lv_coord_t bottom_col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
  static lv_coord_t bottom_row_dsc[] = {LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
  lv_obj_set_grid_dsc_array(bottom_button_grid, bottom_col_dsc, bottom_row_dsc);

  for (int i = 0; i < 5; i++) {
    lv_obj_t *btn = lv_btn_create(bottom_button_grid);  // Create a button
    lv_obj_set_grid_cell(btn, LV_GRID_ALIGN_STRETCH, i, 1, LV_GRID_ALIGN_STRETCH, 0, 1);

    // Style the button
    lv_obj_set_style_radius(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_margin_all(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_margin_hor(btn, 1, LV_PART_MAIN);
    lv_obj_set_height(btn, 40);

    // Add a label to the button
    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text_fmt(label, "%s", bottom_home_btns[i]);
    lv_obj_center(label);  // Center the label on the button
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
    lv_obj_add_event_cb(
        btn,
        [](lv_event_t *e) {
          // Call the member function through the captured 'this' pointer
          static_cast<LVGL_App *>(lv_event_get_user_data(e))->button_event_handler(e);
        },
        LV_EVENT_ALL, this);
  }
  set_setting_highlight(Setpoint, true);
  set_source_highlight(Source_Off, true);
}

void LVGL_App::app_update(const Big_Labels_Value &big_labels_value, const Setting_Labels_Value &setting_labels_value,
                          const Status_Labels_Value &status_labels_value) {
  static Big_Labels_Value     prev_big_labels_value;
  static Setting_Labels_Value prev_setting_labels_value;
  static Status_Labels_Value  prev_status_labels_value;
  static char                 temp_str[64];

  if (big_labels_value.v != prev_big_labels_value.v) {
    snprintf(temp_str, sizeof(temp_str), "%.2f", big_labels_value.v);
    lv_label_set_text(big_labels.v_label, temp_str);
  }

  if (big_labels_value.a != prev_big_labels_value.a) {
    snprintf(temp_str, sizeof(temp_str), "%.2f", big_labels_value.a);
    lv_label_set_text(big_labels.a_label, temp_str);
  }

  if (big_labels_value.w != prev_big_labels_value.w) {
    snprintf(temp_str, sizeof(temp_str), "%.1f", big_labels_value.w);
    lv_label_set_text(big_labels.w_label, temp_str);
  }

  if (big_labels_value.wh != prev_big_labels_value.wh) {
    snprintf(temp_str, sizeof(temp_str), "%.0f", big_labels_value.wh);
    lv_label_set_text(big_labels.wh_label, temp_str);
  }

  if (setting_labels_value.setpoint != prev_setting_labels_value.setpoint) {
    snprintf(temp_str, sizeof(temp_str), "%.1fW", setting_labels_value.setpoint);
    lv_label_set_text(highlightable_containers.setpoint.label, temp_str);
  }

  if (setting_labels_value.cutoff_v != prev_setting_labels_value.cutoff_v) {
    snprintf(temp_str, sizeof(temp_str), "%.1fV", setting_labels_value.cutoff_v);
    lv_label_set_text(highlightable_containers.cutoff_v.label, temp_str);
  }

  if (setting_labels_value.cutoff_e != prev_setting_labels_value.cutoff_e) {
    snprintf(temp_str, sizeof(temp_str), "%.0fWh", setting_labels_value.cutoff_e);
    lv_label_set_text(highlightable_containers.cutoff_e.label, temp_str);
  }

  if (setting_labels_value.timer != prev_setting_labels_value.timer) {
    snprintf(temp_str, sizeof(temp_str), "%02d:%02d:%02d", setting_labels_value.timer / 3600, (setting_labels_value.timer % 3600) / 60,
             setting_labels_value.timer % 60);
    lv_label_set_text(highlightable_containers.timer.label, temp_str);
  }

  prev_big_labels_value     = big_labels_value;
  prev_setting_labels_value = setting_labels_value;
  prev_status_labels_value  = status_labels_value;
}

void LVGL_App::button_event_handler(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t       *obj  = (lv_obj_t *) lv_event_get_target(e);

  if (code == LV_EVENT_CLICKED) {
    if (obj == bottom_grid_buttons.setpoint) {
      set_setting_highlight(Setpoint, true);
    } else if (obj == bottom_grid_buttons.timer) {
      set_setting_highlight(Timer, true);
    } else if (obj == bottom_grid_buttons.cutoff_v) {
      set_setting_highlight(CutOff_V, true);
    } else if (obj == bottom_grid_buttons.cutoff_e) {
      set_setting_highlight(CutOff_E, true);
    } else if (obj == bottom_grid_buttons.settings) {
      modal_create_alert("Settings are not available yet", "Coming soon!");
    }
  }
}

void LVGL_App::set_highlight_container(lv_obj_t *container, bool highlight) {
  if (highlight) {
    lv_obj_set_style_bg_opa(container, LV_OPA_COVER, 0);
  } else {
    lv_obj_set_style_bg_opa(container, LV_OPA_TRANSP, 0);
  }
}

void LVGL_App::set_setting_highlight(Setting_Highlighted_Container container, bool highlight) {
  clear_setting_highlight();
  setting_highlight = container;
  switch (container) {
  case Setpoint:
    set_highlight_container(highlightable_containers.setpoint.container, highlight);
    break;
  case Timer:
    set_highlight_container(highlightable_containers.timer.container, highlight);
    break;
  case CutOff_V:
    set_highlight_container(highlightable_containers.cutoff_v.container, highlight);
    break;
  case CutOff_E:
    set_highlight_container(highlightable_containers.cutoff_e.container, highlight);
    break;
  }
}

void LVGL_App::set_source_highlight(Source_Highlighted_Container container, bool highlight) {
  clear_source_highlight();
  source_highlight = container;
  switch (container) {
  case Source_AC:
    set_highlight_container(highlightable_containers.source_ac.container, highlight);
    break;
  case Source_DC:
    set_highlight_container(highlightable_containers.source_dc.container, highlight);
    break;
  case Source_Off:
    set_highlight_container(highlightable_containers.source_off.container, highlight);
    break;
  }
}

void LVGL_App::clear_setting_highlight() {
  set_highlight_container(highlightable_containers.setpoint.container, false);
  set_highlight_container(highlightable_containers.cutoff_v.container, false);
  set_highlight_container(highlightable_containers.cutoff_e.container, false);
  set_highlight_container(highlightable_containers.timer.container, false);
  //   set_highlight_container(highlightable_containers.start_stop.container,
  //   false);
}

void LVGL_App::clear_source_highlight() {
  set_highlight_container(highlightable_containers.source_ac.container, false);
  set_highlight_container(highlightable_containers.source_dc.container, false);
  set_highlight_container(highlightable_containers.source_off.container, false);
}

lv_obj_t *LVGL_App::lvc_create_overlay() {
  lv_obj_t *overlay = lv_obj_create(lv_scr_act());
  lv_obj_set_size(overlay, 480, 320);
  lv_obj_set_style_border_width(overlay, 0, 0);
  lv_obj_set_style_radius(overlay, 0, 0);
  lv_obj_set_style_bg_color(overlay, bs_dark, 0);
  lv_obj_set_style_bg_opa(overlay, LV_OPA_90, 0);  // 30% opacity
  return overlay;
}

lv_obj_t *LVGL_App::lvc_btn_init(lv_obj_t *btn, const char *labelText, lv_align_t align, lv_coord_t offsetX, lv_coord_t offsetY,
                                 const lv_font_t *font, lv_color_t bgColor, lv_color_t textColor, lv_text_align_t alignText,
                                 lv_label_long_mode_t longMode, lv_coord_t labelWidth, lv_coord_t btnSizeX, lv_coord_t btnSizeY) {
  lv_obj_t *label = lv_label_create(btn);
  lv_label_set_text_static(label, labelText);
  lv_obj_align(btn, align, offsetX, offsetY);
  lv_obj_set_style_text_font(label, font, 0);
  lv_obj_set_style_bg_color(btn, bgColor, 0);
  lv_obj_set_style_text_color(label, textColor, 0);
  lv_obj_set_style_text_align(label, alignText, 0);
  lv_label_set_long_mode(label, longMode);
  if (labelWidth != 0)
    lv_obj_set_width(label, labelWidth);
  lv_obj_center(label);  // Center the label
  if (labelWidth != 0)
    lv_obj_set_width(label, labelWidth);
  if (btnSizeX != 0)
    lv_obj_set_width(btn, btnSizeX);
  if (btnSizeY != 0)
    lv_obj_set_height(btn, btnSizeY);
  return label;
}

void LVGL_App::lvc_label_init(lv_obj_t *label, const lv_font_t *font, lv_align_t align, lv_coord_t offsetX, lv_coord_t offsetY, lv_color_t textColor,
                              lv_text_align_t alignText, lv_label_long_mode_t longMode, lv_coord_t textWidth) {
  lv_obj_set_style_text_color(label, textColor, 0);
  lv_obj_set_style_text_font(label, font, 0);
  lv_obj_set_style_text_align(label, alignText, 0);
  lv_obj_align(label, align, offsetX, offsetY);
  if (longMode != LV_LABEL_LONG_WRAP)  // Set long mode if set value is not defaulted
    lv_label_set_long_mode(label, longMode);
  if (textWidth != 0)  // Only set label width if passed textWidth value is not 0
    lv_obj_set_width(label, textWidth);
}

lv_obj_t *
LVGL_App::modal_create_confirm(WidgetParameterData *modalConfirmData, const char *message, const char *headerText, const lv_font_t *headerFont,
                               const lv_font_t *messageFont, lv_color_t headerTextColor, lv_color_t textColor, lv_color_t headerColor,
                               const char *confirmButtonText, const char *cancelButtonText, lv_coord_t xSize, lv_coord_t ySize) {
  lv_obj_t *modal =
      modal_create_alert(message, headerText, headerFont, messageFont, headerTextColor, textColor, headerColor, cancelButtonText, xSize, ySize);
  lv_obj_t *okButton = lv_button_create(modal);
  lvc_btn_init(okButton, confirmButtonText, LV_ALIGN_BOTTOM_RIGHT, -120, -15);
  lv_obj_add_event_cb(
      okButton,
      [](lv_event_t *e) {
        WidgetParameterData *modalConfirmData = (WidgetParameterData *) lv_event_get_user_data(e);
        lv_obj_t            *btn              = (lv_obj_t *) lv_event_get_target(e);
        lv_obj_send_event(modalConfirmData->issuer, LV_EVENT_REFRESH, modalConfirmData);
        lv_obj_delete(lv_obj_get_parent(lv_obj_get_parent(btn)));
      },
      LV_EVENT_CLICKED, modalConfirmData);
  return modal;
}

lv_obj_t *LVGL_App::modal_create_alert(const char *message, const char *headerText, const lv_font_t *headerFont, const lv_font_t *messageFont,
                                       lv_color_t headerTextColor, lv_color_t textColor, lv_color_t headerColor, const char *buttonText,
                                       lv_coord_t xSize, lv_coord_t ySize) {
  lv_obj_t *overlay = lvc_create_overlay();

  lv_obj_t *modal = lv_obj_create(overlay);
  lv_obj_center(modal);
  lv_obj_set_size(modal, xSize, ySize);
  lv_obj_set_style_pad_all(modal, 0, 0);

  lv_obj_t *modalHeader = lv_obj_create(modal);
  lv_obj_align(modalHeader, LV_ALIGN_TOP_MID, 0, 0);
  lv_obj_set_size(modalHeader, lv_pct(100), LV_SIZE_CONTENT);
  lv_obj_set_style_radius(modalHeader, 0, 0);

  lv_obj_set_style_bg_color(modalHeader, headerColor, 0);

  lv_obj_t *warningLabel = lv_label_create(modalHeader);
  lvc_label_init(warningLabel, &lv_font_montserrat_20, LV_ALIGN_TOP_LEFT, 0, 0, headerTextColor, LV_TEXT_ALIGN_LEFT, LV_LABEL_LONG_WRAP, lv_pct(100));
  lv_label_set_text_static(warningLabel, headerText);

  lv_obj_t *error = lv_label_create(modal);
  lvc_label_init(error, &lv_font_montserrat_14, LV_ALIGN_CENTER, 0, 0, textColor, LV_TEXT_ALIGN_CENTER, LV_LABEL_LONG_WRAP, lv_pct(100));
  lv_label_set_text_static(error, message);

  lv_obj_t *okButton = lv_button_create(modal);
  lvc_btn_init(okButton, buttonText, LV_ALIGN_BOTTOM_RIGHT, -15, -15);
  lv_obj_add_event_cb(okButton, [](lv_event_t *e) { lv_obj_delete((lv_obj_t *) lv_event_get_user_data(e)); }, LV_EVENT_CLICKED, overlay);
  return modal;
}