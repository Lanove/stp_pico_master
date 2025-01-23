/**
 * @file ili9486_drivers.cpp
 * @brief ILI9486 SPI driver implementation (8-bit mode)
 */
#include "ili9486_drivers.h"

ili9486_drivers::ili9486_drivers(spi_inst_t *spi, uint8_t pin_cs,
                                 uint8_t pin_dc, uint8_t pin_rst, uint8_t mosi,
                                 uint8_t miso, uint8_t sck, uint32_t spi_clock,
                                 bool dma_used)
    : spi(spi), pin_cs(pin_cs), pin_dc(pin_dc), pin_rst(pin_rst),
      pin_mosi(mosi), pin_miso(miso), pin_sck(sck), spi_clock(spi_clock),
      dma_used(dma_used) {}

void ili9486_drivers::init() {
  // Initialize GPIOs
  gpio_init(pin_cs);
  gpio_init(pin_dc);
  gpio_init(pin_rst);

  // Configure SPI pins
  gpio_set_function(pin_mosi, GPIO_FUNC_SPI);
  gpio_set_function(pin_sck, GPIO_FUNC_SPI);
  gpio_set_function(pin_miso, GPIO_FUNC_SPI);

  // Set control pins as outputs
  gpio_set_dir(pin_cs, GPIO_OUT);
  gpio_set_dir(pin_dc, GPIO_OUT);
  gpio_set_dir(pin_rst, GPIO_OUT);

  // Default states
  gpio_put(pin_cs, 1);
  gpio_put(pin_rst, 1);

  // Hardware reset
  gpio_put(pin_rst, 1);
  sleep_ms(50);
  gpio_put(pin_rst, 0);
  sleep_ms(50);
  gpio_put(pin_rst, 1);
  sleep_ms(150);

  // SPI initialization (8-bit mode)
  spi_init(spi, spi_clock);
  spi_set_format(spi, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

  printf("ILI9486 initialization\n");
  printf("SPI clock: %d\n", spi_clock);
  printf("Panel width: %d\n", panel_width);
  printf("Panel height: %d\n", panel_height);
  printf("DMA used: %d\n", dma_used);

  // Send initialization commands
  const uint8_t *initCommand_ptr = initCommands;
  while (true) {
    // Check for end marker
    if (initCommand_ptr[0] == 0xFF && initCommand_ptr[1] == 0xFF)
      break;

    uint8_t cmd = *initCommand_ptr++;
    uint8_t next = *initCommand_ptr++;

    if (next == CMD_Delay) {
      // Handle delay
      uint8_t delay_ms = *initCommand_ptr++;
      writeCommand(cmd);
      sleep_ms(delay_ms);
    } else {
      // Handle parameters
      uint8_t param_count = next;
      writeCommand(cmd);
      for (uint8_t i = 0; i < param_count; i++) {
        uint8_t data = *initCommand_ptr++;
        writeData(data);
      }
    }
  }
  setRotation(LANDSCAPE);
  fillScreen((uint32_t)0); // Clear screen
}
void ili9486_drivers::setRotation(Rotations rotation) {
  _rot = rotation;
  uint8_t madctl = MASK_BGR; // BGR filter is always enabled
  bool swap_dims = false;

  switch (_rot) {
  case PORTRAIT:
    madctl |= MASK_MX;
    break;
  case LANDSCAPE:
    madctl |= MASK_MV;
    swap_dims = true;
    break;
  case INVERTED_PORTRAIT:
    madctl |= MASK_MY;
    break;
  case INVERTED_LANDSCAPE:
    madctl |= (MASK_MV | MASK_MX | MASK_MY);
    swap_dims = true;
    break;
  default:
    // Handle invalid rotations
    madctl |= MASK_MX;
    break;
  }

  startTransaction();
  writeCommand(CMD_MemoryAccessControl);
  writeData(madctl);
  endTransaction();

  // Update dimensions based on orientation
  _width = swap_dims ? panel_height : panel_width;
  _height = swap_dims ? panel_width : panel_height;

  printf("Screen width: %d\n", _width);
  printf("Screen height: %d\n", _height);
}

void ili9486_drivers::writeCommand(uint8_t cmd) {
  startTransaction();
  gpio_put(pin_dc, 0);
  spi_write_blocking(spi, &cmd, 1);
  endTransaction();
}

void ili9486_drivers::writeData(uint8_t data) {
  startTransaction();
  gpio_put(pin_dc, 1);
  spi_write_blocking(spi, &data, 1);
  endTransaction();
}

void ili9486_drivers::setWindow(int32_t x0, int32_t y0, int32_t x1,
                                int32_t y1) {
  writeCommand(CMD_ColumnAddressSet);
  writeData(x0 >> 8);
  writeData(x0 & 0xFF);
  writeData(x1 >> 8);
  writeData(x1 & 0xFF);

  writeCommand(CMD_PageAddressSet);
  writeData(y0 >> 8);
  writeData(y0 & 0xFF);
  writeData(y1 >> 8);
  writeData(y1 & 0xFF);

  writeCommand(CMD_MemoryWrite);
}

void ili9486_drivers::pushBlock(uint16_t color, uint32_t len) {
  uint8_t color_buf[2] = {static_cast<uint8_t>(color >> 8),
                          static_cast<uint8_t>(color & 0xFF)};
  startTransaction();
  gpio_put(pin_dc, 1);
  for (uint32_t i = 0; i < len; i++) {
    spi_write_blocking(spi, color_buf, 2);
  }
  endTransaction();
}

void ili9486_drivers::pushBlock(uint32_t color, uint32_t len) {
  uint8_t color_buf[3] = {static_cast<uint8_t>(color >> 16),
                          static_cast<uint8_t>(color >> 8),
                          static_cast<uint8_t>(color & 0xFF)};
  startTransaction();
  gpio_put(pin_dc, 1);
  for (uint32_t i = 0; i < len; i++) {
    spi_write_blocking(spi, color_buf, 3);
  }
  endTransaction();
}

void ili9486_drivers::fillScreen(uint16_t color) {
  setAddressWindow(0, 0, _width, _height);
  pushBlock(color, _width * _height);
}

void ili9486_drivers::fillScreen(uint32_t color) {
  setAddressWindow(0, 0, _width, _height);
  pushBlock(color, _width * _height);
}

void ili9486_drivers::pushColors(uint16_t *color, uint32_t len) {
  startTransaction();
  gpio_put(pin_dc, 1);
  spi_write_blocking(spi, reinterpret_cast<uint8_t *>(color), len * 2);
  endTransaction();
}

void ili9486_drivers::pushColors(uint32_t *color, uint32_t len) {
  startTransaction();
  gpio_put(pin_dc, 1);
  spi_write_blocking(spi, reinterpret_cast<uint8_t *>(color), len * 3);
  endTransaction();
}

void ili9486_drivers::dmaInit(void (*onComplete_cb)(void)) {
  dma_tx_channel = dma_claim_unused_channel(true);
  dma_tx_config = dma_channel_get_default_config(dma_tx_channel);
  channel_config_set_transfer_data_size(&dma_tx_config, DMA_SIZE_8);
  channel_config_set_dreq(&dma_tx_config, spi_get_dreq(spi, true));
  dma_channel_set_irq0_enabled(dma_tx_channel, true);
  irq_set_exclusive_handler(DMA_IRQ_0, onComplete_cb);
  irq_set_enabled(DMA_IRQ_0, true);
  dma_used = true;
}

void ili9486_drivers::pushColorsDMA(uint16_t *colors, uint32_t len) {
  if (!dma_used)
    return;
  startTransaction();
  gpio_put(pin_dc, 1);
  dma_channel_configure(dma_tx_channel, &dma_tx_config, &spi_get_hw(spi)->dr,
                        reinterpret_cast<uint8_t *>(colors), len * 2, true);
}

uint16_t ili9486_drivers::create565Color(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0x1F) << 11) | ((g & 0x3F) << 5) | (b & 0x1F);
}

uint32_t ili9486_drivers::create888Color(uint8_t r, uint8_t g, uint8_t b) {
  return (r << 16) | (g << 8) | b;
}

void ili9486_drivers::setAddressWindow(int32_t x, int32_t y, int32_t w,
                                       int32_t h) {
  setWindow(x, y, x + w - 1, y + h - 1);
}

// Transaction management
void ili9486_drivers::startTransaction() { gpio_put(pin_cs, 0); }
void ili9486_drivers::endTransaction() { gpio_put(pin_cs, 1); }