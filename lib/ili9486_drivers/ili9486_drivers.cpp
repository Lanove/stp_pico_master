/**
 * @file ili9486_drivers.cpp
 * @brief ILI9486 SPI driver implementation (8-bit mode)
 */
#include "ili9486_drivers.h"

ILI9486Drivers::ILI9486Drivers(spi_inst_t *spi, uint8_t pin_cs,
                                 uint8_t pin_dc, uint8_t pin_rst, uint8_t mosi,
                                 uint8_t miso, uint8_t sck, uint32_t spi_clock,
                                 bool dma_used)
    : spi(spi), pin_cs(pin_cs), pin_dc(pin_dc), pin_rst(pin_rst),
      pin_mosi(mosi), pin_miso(miso), pin_sck(sck), spi_clock(spi_clock),
      dma_used(dma_used) {}

void ILI9486Drivers::init() {
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
  const uint8_t *initCommand_ptr = init_commands;
  while (true) {
    // Check for end marker
    if (initCommand_ptr[0] == 0xFF && initCommand_ptr[1] == 0xFF)
      break;

    uint8_t cmd = *initCommand_ptr++;
    uint8_t next = *initCommand_ptr++;

    if (next == CMD_Delay) {
      // Handle delay
      uint8_t delay_ms = *initCommand_ptr++;
      write_command(cmd);
      sleep_ms(delay_ms);
    } else {
      // Handle parameters
      uint8_t param_count = next;
      write_command(cmd);
      for (uint8_t i = 0; i < param_count; i++) {
        uint8_t data = *initCommand_ptr++;
        write_data(data);
      }
    }
  }
  set_rotation(LANDSCAPE);
  fill_screen((uint32_t)0); // Clear screen
}

void ILI9486Drivers::draw_pixel(int16_t x, int16_t y, uint32_t color) {
  if ((x < 0) || (x >= _width) || (y < 0) || (y >= _height))
    return;

  set_address_window(x, y, x, y);
  push_block(color, 1);
}

void ILI9486Drivers::set_rotation(Rotations rotation) {
  _rot = rotation;
  uint8_t madctl = 0; // BGR filter is always enabled

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

  start_transaction();
  write_command(CMD_MemoryAccessControl);
  write_data(madctl);
  end_transaction();

  // Update dimensions based on orientation
  _width = swap_dims ? panel_height : panel_width;
  _height = swap_dims ? panel_width : panel_height;
}

void ILI9486Drivers::write_command(uint8_t cmd) {
  start_transaction();
  gpio_put(pin_dc, 0);
  spi_write_blocking(spi, &cmd, 1);
  end_transaction();
}

void ILI9486Drivers::write_data(uint8_t data) {
  start_transaction();
  gpio_put(pin_dc, 1);
  spi_write_blocking(spi, &data, 1);
  end_transaction();
}

void ILI9486Drivers::set_window(int32_t x0, int32_t y0, int32_t x1,
                                int32_t y1) {
  write_command(CMD_ColumnAddressSet);
  write_data(x0 >> 8);
  write_data(x0 & 0xFF);
  write_data(x1 >> 8);
  write_data(x1 & 0xFF);

  write_command(CMD_PageAddressSet);
  write_data(y0 >> 8);
  write_data(y0 & 0xFF);
  write_data(y1 >> 8);
  write_data(y1 & 0xFF);

  write_command(CMD_MemoryWrite);
}

void ILI9486Drivers::push_block(uint32_t color, uint32_t len) {
  uint8_t color_buf[3] = {static_cast<uint8_t>(color >> 16),
                          static_cast<uint8_t>(color >> 8),
                          static_cast<uint8_t>(color & 0xFF)};
  start_transaction();
  gpio_put(pin_dc, 1);
  for (uint32_t i = 0; i < len; i++) {
    spi_write_blocking(spi, color_buf, 3);
  }
  end_transaction();
}

void ILI9486Drivers::fill_screen(uint32_t color) {
  set_address_window(0, 0, _width, _height);
  push_block(color, _width * _height);
}

void ILI9486Drivers::push_colors(uint32_t *color, uint32_t len) {
  start_transaction();
  gpio_put(pin_dc, 1);
  spi_write_blocking(spi, reinterpret_cast<uint8_t *>(color), len * 3);
  end_transaction();
}

void ILI9486Drivers::dma_init(void (*onComplete_cb)(void)) {
  dma_tx_channel = dma_claim_unused_channel(true);
  dma_tx_config = dma_channel_get_default_config(dma_tx_channel);
  channel_config_set_transfer_data_size(&dma_tx_config, DMA_SIZE_8);
  channel_config_set_dreq(&dma_tx_config, spi_get_dreq(spi, true));
  uint irq_num = (dma_tx_channel <= 3) ? DMA_IRQ_0 : DMA_IRQ_1;
  static int captured_dma_tx_channel = dma_tx_channel; // Capture the channel
  static void (*user_callback)(void) = onComplete_cb;
  irq_handler_t wrapper = [](void) {
    if (dma_channel_get_irq0_status(captured_dma_tx_channel)) {
      dma_channel_acknowledge_irq0(captured_dma_tx_channel);
      if (user_callback) {
        user_callback();
      }
    }
  };

  irq_remove_handler(irq_num, wrapper);
  irq_set_exclusive_handler(irq_num, wrapper);

  dma_channel_set_irq0_enabled(dma_tx_channel, true);
  irq_set_enabled(irq_num, true);

  dma_used = true;
}

void ILI9486Drivers::push_colors_dma(uint32_t *colors, uint32_t len) {
  if (!dma_used)
    return;
  gpio_put(pin_dc, 1);

  // Configure DMA transfer
  uint8_t *src = reinterpret_cast<uint8_t *>(colors);
  const uint32_t transfer_size = len * 3; // 3 bytes per pixel

  dma_channel_configure(dma_tx_channel, &dma_tx_config,
                        &spi_get_hw(spi)->dr, // SPI data register (destination)
                        src,                  // Source buffer
                        transfer_size,        // Transfer size in bytes
                        true                  // Start immediately
  );
}

uint32_t ILI9486Drivers::create_888_color(uint8_t r, uint8_t g, uint8_t b) {
  return (b << 16) | (g << 8) | r;
}

uint32_t ILI9486Drivers::create_666_color(uint8_t r, uint8_t g, uint8_t b) {
  return ((b & 0x3F) << 18) | ((g & 0x3F) << 10) | ((r & 0x3F) << 2);
}

void ILI9486Drivers::set_address_window(int32_t x, int32_t y, int32_t w,
                                       int32_t h) {
  set_window(x, y, x + w - 1, y + h - 1);
}

// Transaction management
void ILI9486Drivers::start_transaction() { gpio_put(pin_cs, 0); }
void ILI9486Drivers::end_transaction() { gpio_put(pin_cs, 1); }