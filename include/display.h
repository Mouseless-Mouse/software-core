#pragma once

#include <Arduino.h>
#include "Adafruit_GFX.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "hal/lcd_types.h"
#include "esp_err.h"
#include "esp_log.h"

#define EXAMPLE_LCD_PIXEL_CLOCK_HZ (16 * 1000 * 1000)
#define CONFIG_EXAMPLE_LCD_I80_BUS_WIDTH 8

#define EXAMPLE_LCD_BK_LIGHT_ON_LEVEL 1
#define EXAMPLE_LCD_BK_LIGHT_OFF_LEVEL !EXAMPLE_LCD_BK_LIGHT_ON_LEVEL
#define EXAMPLE_PIN_NUM_DATA0 (gpio_num_t)39    // 6
#define EXAMPLE_PIN_NUM_DATA1 (gpio_num_t)40    // 7
#define EXAMPLE_PIN_NUM_DATA2 (gpio_num_t)41    // 8
#define EXAMPLE_PIN_NUM_DATA3 (gpio_num_t)42    // 9
#define EXAMPLE_PIN_NUM_DATA4 (gpio_num_t)45    // 10
#define EXAMPLE_PIN_NUM_DATA5 (gpio_num_t)46    // 11
#define EXAMPLE_PIN_NUM_DATA6 (gpio_num_t)47    // 12
#define EXAMPLE_PIN_NUM_DATA7 (gpio_num_t)48    // 13
#define EXAMPLE_PIN_NUM_PCLK (gpio_num_t)8      // 5
#define EXAMPLE_PIN_NUM_CS (gpio_num_t)6        // 3
#define EXAMPLE_PIN_NUM_DC (gpio_num_t)7        // 4
#define EXAMPLE_PIN_NUM_RST (gpio_num_t)5       // 2
#define EXAMPLE_PIN_NUM_BK_LIGHT (gpio_num_t)38 // 1
#define EXAMPLE_PIN_NUM_POWER (gpio_num_t)15
#define EXAMPLE_PIN_LCD_RD (gpio_num_t)9

// The pixel number in horizontal and vertical
#define EXAMPLE_LCD_H_RES 320
#define EXAMPLE_LCD_V_RES 170
// Bit number used to represent command and parameter
#define EXAMPLE_LCD_CMD_BITS 8
#define EXAMPLE_LCD_PARAM_BITS 8

#define EXAMPLE_LVGL_TICK_PERIOD_MS 2

// Supported alignment: 16, 32, 64. A higher alignment can enables higher burst transfer size, thus a higher i80 bus throughput.
#define EXAMPLE_PSRAM_DATA_ALIGNMENT 32

#define BUFFER_SIZE_BYTES EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES * sizeof(uint16_t)

#define BACKLIGHT_PIN 38

static bool on_color_trans_done(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx);

class TFT_Parallel : public Adafruit_GFX {
private:
    uint16_t *buffer;
    static esp_lcd_panel_handle_t panel_handle;
    static volatile bool trans_done;
    friend bool on_color_trans_done(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx);
public:
    TFT_Parallel(uint16_t w, uint16_t h);

    void init();

    virtual void drawPixel(int16_t x, int16_t y, uint16_t color);

    void clear();

    void refresh();

    bool done_refreshing();

    uint16_t textWidth(const char *cstring);
};

static bool on_color_trans_done(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx) {
    TFT_Parallel::trans_done = true;
    return false;
}

constexpr uint16_t color_rgb(byte r, byte g, byte b) {
    return (r >> 3) << 11 | (g >> 2) << 5 | (b >> 3);
}