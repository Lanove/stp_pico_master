#include "lv_drivers.h"

// Display buffer configuration
#define DISP_HOR_RES 480
#define DISP_VER_RES 320
#define DISP_BUF_SIZE (DISP_HOR_RES * 10) // 40 lines buffer
#define BYTE_PER_PIXEL (LV_COLOR_FORMAT_GET_SIZE(LV_COLOR_FORMAT_RGB888))

LV_ATTRIBUTE_MEM_ALIGN
static uint8_t disp_buf[DISP_BUF_SIZE * BYTE_PER_PIXEL];
lv_display_t *disp;
static repeating_timer lv_tick_timer;

uint8_t tft_tx = 11;
uint8_t tft_rx = 8;
uint8_t tft_sck = 10;

uint8_t tft_cs = 13;
uint8_t tft_dc = 12;
uint8_t tft_reset = 9;
spi_inst_t *tft_spi = spi1;
spi_inst_t *touch_spi = spi0;

uint8_t T_CLK = 6;
uint8_t T_CS = 5;
uint8_t T_DIN = 3;
uint8_t T_DO = 0;
uint8_t T_IRQ = 1;

constexpr float tft_freq_mhz = 62.5;

static ILI9486Drivers tft(tft_spi, tft_cs, tft_dc, tft_reset, tft_tx, tft_rx,
                           tft_sck, tft_freq_mhz * 1000 * 1000);

static XPT2046 touch(touch_spi, T_DIN, T_DO, T_CLK, T_CS, T_IRQ);

static void disp_flush(lv_display_t *dispf, const lv_area_t *area,
                       uint8_t *px_map);
static void touch_cb(lv_indev_t *indev, lv_indev_data_t *data);

void lvgl_display_init() {
  tft.init();
  tft.set_rotation(LANDSCAPE);
  touch.begin(LANDSCAPE);

  tft.dma_init([]() {
    lv_display_flush_ready(disp);
    tft.end_transaction();
  });

  // Initialize LVGL
  lv_init();

  // Set up display buffer
  disp = lv_display_create(DISP_HOR_RES, DISP_VER_RES);
  //   lv_display_set_draw_buffers(disp, disp_buf, nullptr, DISP_BUF_SIZE);
  lv_display_set_buffers(disp, disp_buf, nullptr, sizeof(disp_buf),
                         LV_DISPLAY_RENDER_MODE_PARTIAL);

  // Configure display driver
  lv_display_set_flush_cb(disp, disp_flush);
  lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB888);
  lv_display_flush_ready(disp);

  // Initialize LVGL input device
  lv_indev_t *indev =
      lv_indev_create(); /* Create input device connected to Default Display. */
  lv_indev_set_type(
      indev, LV_INDEV_TYPE_POINTER); /* Touch pad is a pointer-like device. */
  lv_indev_set_read_cb(indev, touch_cb); /* Set driver function. */

  add_repeating_timer_ms(
      5,
      [](struct repeating_timer *t) -> bool {
        lv_tick_inc(5);
        return true;
      },
      NULL, &lv_tick_timer);
}


static void disp_flush(lv_display_t *dispf, const lv_area_t *area,
                       uint8_t *px_map) {
  uint32_t num_pixels = (area->x2 - area->x1 + 1) * (area->y2 - area->y1 + 1);

  tft.set_address_window(area->x1, area->y1, area->x2 - area->x1 + 1,
                       area->y2 - area->y1 + 1);
  tft.start_transaction();
  tft.push_colors_dma(reinterpret_cast<uint32_t *>(px_map), num_pixels);
}

static void touch_cb(lv_indev_t *indev, lv_indev_data_t *data) {

  if (touch.is_touched()) {
    uint16_t x, y;
    touch.get_touch(x, y);
    data->point.x = x;
    data->point.y = y;
    data->state = LV_INDEV_STATE_PRESSED;
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}
