#include "lv_drivers.h"

// Display buffer configuration
#define DISP_HOR_RES 480
#define DISP_VER_RES 320
#define DISP_BUF_SIZE (DISP_HOR_RES * 10) // 40 lines buffer
#define BYTE_PER_PIXEL (LV_COLOR_FORMAT_GET_SIZE(LV_COLOR_FORMAT_RGB888))

LV_ATTRIBUTE_MEM_ALIGN
static uint8_t disp_buf[DISP_BUF_SIZE * BYTE_PER_PIXEL];
static ili9486_drivers *tft_driver = nullptr;
static XPT2046 *touch_driver = nullptr;
lv_display_t *disp;
// Flush callback for LVGL

static void disp_flush(lv_display_t *dispf, const lv_area_t *area,
                       uint8_t *px_map) {
  uint32_t num_pixels = (area->x2 - area->x1 + 1) * (area->y2 - area->y1 + 1);

  tft_driver->setAddressWindow(area->x1, area->y1, area->x2 - area->x1 + 1,
                               area->y2 - area->y1 + 1);
  tft_driver->startTransaction();
  tft_driver->pushColorsDMA(reinterpret_cast<uint32_t *>(px_map), num_pixels);
}

static void touch_cb(lv_indev_t *indev, lv_indev_data_t *data) {
  if(touch_driver == nullptr) return;
  if (touch_driver->isTouched()) {
    uint16_t x, y;
    touch_driver->getTouch(x, y);
    data->point.x = x;
    data->point.y = y;
    data->state = LV_INDEV_STATE_PRESSED;
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

void lvgl_display_init(ili9486_drivers &driver, XPT2046 &touch) {
  tft_driver = &driver;
  touch_driver = &touch;

  tft_driver->dmaInit([]() {
    lv_display_flush_ready(disp);
    tft_driver->endTransaction();
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
}