#include "display.h"

TFT_Parallel::TFT_Parallel(uint16_t w, uint16_t h)
    : Adafruit_GFX(w, h)
    , buffer(nullptr)
{}

void TFT_Parallel::init() {
    pinMode(EXAMPLE_PIN_LCD_RD, INPUT_PULLUP);
    // pinMode(EXAMPLE_PIN_NUM_POWER, OUTPUT);
    // digitalWrite(EXAMPLE_PIN_NUM_POWER, EXAMPLE_LCD_BK_LIGHT_ON_LEVEL);

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

    ledcSetup(BACKLIGHT_LEDC_CHANNEL, 1000, 8);
    ledcAttachPin(BACKLIGHT_PIN, BACKLIGHT_LEDC_CHANNEL);
    ledcWrite(BACKLIGHT_LEDC_CHANNEL, 255);

    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    if (buffer = (uint16_t*)heap_caps_malloc(WIDTH * HEIGHT * sizeof(uint16_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA)) {
        memset(buffer, 0, WIDTH * HEIGHT * sizeof(uint16_t));
    }
    else {
        Serial.println("Failed to allocate display buffer");
        while(1);
    }
}

void TFT_Parallel::drawPixel(int16_t x, int16_t y, uint16_t color) {
    if (x >= 0 && x < _width && y >= 0 && y < _height)
        buffer[x + y*_width] = color;
}

void TFT_Parallel::writeFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) {
    if (y >= _height || y < 0 || w == 0)
        return;
    if (w < 0) {
        x += w;
        w = -w;
    }
    int16_t end = max((int16_t)0, (int16_t)(x + w));
    for (int16_t start = max((int16_t)0, x); start < end; ++start)
        buffer[start + y*_width] = color;
}

void TFT_Parallel::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    for (int16_t i = y; i < y + h; ++i)
        writeFastHLine(x, i, w, color);
}

void TFT_Parallel::clear() {
    memset(buffer, 0, WIDTH * HEIGHT * sizeof(uint16_t));
}

void TFT_Parallel::set_backlight(uint8_t brightness) {
    ledcWrite(BACKLIGHT_LEDC_CHANNEL, brightness);
}

void TFT_Parallel::refresh() {
    trans_done = false;
    ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES, buffer));
}

bool TFT_Parallel::done_refreshing() {
    return trans_done;
}

uint16_t TFT_Parallel::textWidth(const char *cstring) {
    int16_t dummyX, dummyY;
    uint16_t w, h;
    getTextBounds(cstring, 0, 0, &dummyX, &dummyY, &w, &h);
    return w;
}

esp_lcd_panel_handle_t TFT_Parallel::panel_handle = nullptr;
volatile bool TFT_Parallel::trans_done = true;