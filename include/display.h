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

const byte BACKLIGHT_PIN = 15;

static bool on_color_trans_done(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx);

class TFT_Parallel : public Adafruit_GFX {
private:
    uint16_t *buffer;
    static esp_lcd_panel_handle_t panel_handle;
    static volatile bool trans_done;
    friend bool on_color_trans_done(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx);
public:
    TFT_Parallel(uint16_t w, uint16_t h) : Adafruit_GFX(w, h), buffer(nullptr) {}

    void init() {
        pinMode(EXAMPLE_PIN_LCD_RD, INPUT_PULLUP);
        pinMode(EXAMPLE_PIN_NUM_POWER, OUTPUT);
        digitalWrite(EXAMPLE_PIN_NUM_POWER, EXAMPLE_LCD_BK_LIGHT_ON_LEVEL);

        Serial.println("Initialize Intel 8080 bus");
        esp_lcd_i80_bus_handle_t i80_bus = NULL;
        esp_lcd_i80_bus_config_t bus_config = {
            .dc_gpio_num = EXAMPLE_PIN_NUM_DC,
            .wr_gpio_num = EXAMPLE_PIN_NUM_PCLK,
            .clk_src = LCD_CLK_SRC_PLL160M /* LCD_CLK_SRC_DEFAULT */,
            .data_gpio_nums = {
                EXAMPLE_PIN_NUM_DATA0,
                EXAMPLE_PIN_NUM_DATA1,
                EXAMPLE_PIN_NUM_DATA2,
                EXAMPLE_PIN_NUM_DATA3,
                EXAMPLE_PIN_NUM_DATA4,
                EXAMPLE_PIN_NUM_DATA5,
                EXAMPLE_PIN_NUM_DATA6,
                EXAMPLE_PIN_NUM_DATA7,
            },
            .bus_width = CONFIG_EXAMPLE_LCD_I80_BUS_WIDTH,
            .max_transfer_bytes = EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES * sizeof(uint16_t),
            .psram_trans_align = EXAMPLE_PSRAM_DATA_ALIGNMENT,
            .sram_trans_align = 4,
        };
        ESP_ERROR_CHECK(esp_lcd_new_i80_bus(&bus_config, &i80_bus));
        esp_lcd_panel_io_handle_t io_handle = NULL;
        esp_lcd_panel_io_i80_config_t io_config = {
            .cs_gpio_num = EXAMPLE_PIN_NUM_CS,
            .pclk_hz = EXAMPLE_LCD_PIXEL_CLOCK_HZ,
            .trans_queue_depth = 20,
            .on_color_trans_done = on_color_trans_done,
            .user_ctx = NULL,
            .lcd_cmd_bits = EXAMPLE_LCD_CMD_BITS,
            .lcd_param_bits = EXAMPLE_LCD_PARAM_BITS,
            .dc_levels = {
                .dc_idle_level = 0,
                .dc_cmd_level = 0,
                .dc_dummy_level = 0,
                .dc_data_level = 1,
            },
            .flags = {
                .swap_color_bytes = 1, // Swap can be done in LvGL (default) or DMA
            }
        };
        ESP_ERROR_CHECK(esp_lcd_new_panel_io_i80(i80_bus, &io_config, &io_handle));

        Serial.println("Install LCD driver of st7789");
        esp_lcd_panel_dev_config_t panel_config = {
            .reset_gpio_num = EXAMPLE_PIN_NUM_RST,
            // .rgb_endian = LCD_RGB_ENDIAN_RGB,
            .color_space = ESP_LCD_COLOR_SPACE_RGB,
            .bits_per_pixel = 16,
        };
        ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle));

        esp_lcd_panel_reset(panel_handle);
        esp_lcd_panel_init(panel_handle);
        // Set inversion, x/y coordinate order, x/y mirror according to your LCD module spec
        // the gap is LCD panel specific, even panels with the same driver IC, can have different gap value
        ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));
        ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel_handle, true));
        ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, false, true));
        ESP_ERROR_CHECK(esp_lcd_panel_set_gap(panel_handle, 0, 35));

        pinMode(BACKLIGHT_PIN, OUTPUT);
        digitalWrite(BACKLIGHT_PIN, HIGH);
        pinMode(38, OUTPUT);
        digitalWrite(38, HIGH);

        ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

        if (buffer = (uint16_t*)heap_caps_malloc(WIDTH * HEIGHT * sizeof(uint16_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA)) {
            memset(buffer, 0, WIDTH * HEIGHT * sizeof(uint16_t));
        }
        else {
            Serial.println("Failed to allocate display buffer");
            while(1);
        }
    }

    virtual void drawPixel(int16_t x, int16_t y, uint16_t color) {
    if (x >= 0 && x < EXAMPLE_LCD_H_RES && y >= 0 && y < EXAMPLE_LCD_V_RES)
        buffer[x + y*EXAMPLE_LCD_H_RES] = color;
    }

    void clear() {
        memset(buffer, 0, WIDTH * HEIGHT * sizeof(uint16_t));
    }

    void refresh() {
        trans_done = false;
        ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES, buffer));
    }

    bool done_refreshing() {
        return trans_done;
    }
};

static bool on_color_trans_done(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx) {
    TFT_Parallel::trans_done = true;
    return false;
}

esp_lcd_panel_handle_t TFT_Parallel::panel_handle = nullptr;
volatile bool TFT_Parallel::trans_done = true;

constexpr uint16_t color_rgb(byte r, byte g, byte b) {
    return (r >> 3) << 11 | (g >> 2) << 5 | (b >> 3);
}