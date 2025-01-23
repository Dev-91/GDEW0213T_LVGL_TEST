#pragma once

#include <stdbool.h>
#include "esp_err.h"

// 디스플레이 해상도 정의
#define EPD_WIDTH   104
#define EPD_HEIGHT  212

#define EPD_ARRAY  EPD_WIDTH*EPD_HEIGHT/8  

// GPIO 정의
#define EPD_PIN_BUSY    4
#define EPD_PIN_RST     16
#define EPD_PIN_DC      17
#define EPD_PIN_CS      5
#define EPD_PIN_CLK     18
#define EPD_PIN_MOSI    23

// 명령어 정의
#define EPD_PANEL_SETTING             0x00
#define EPD_POWER_SETTING             0x01
#define EPD_POWER_OFF                 0x02
#define EPD_POWER_ON                  0x04
#define EPD_BOOSTER_SOFT_START        0x06
#define EPD_DEEP_SLEEP                0x07
#define EPD_DATA_START_TRANSMISSION_1 0x10
#define EPD_DATA_STOP                 0x11
#define EPD_DISPLAY_REFRESH           0x12
#define EPD_DATA_START_TRANSMISSION_2 0x13
#define EPD_LUT_FOR_VCOM             0x20
#define EPD_LUT_WHITE_TO_WHITE       0x21
#define EPD_LUT_BLACK_TO_WHITE       0x22
#define EPD_LUT_WHITE_TO_BLACK       0x23
#define EPD_LUT_BLACK_TO_BLACK       0x24
#define EPD_PLL_CONTROL              0x30
#define EPD_VCOM_AND_DATA_INTERVAL_SETTING 0x50
#define EPD_TCON_SETTING             0x60
#define EPD_RESOLUTION_SETTING       0x61
#define EPD_GET_STATUS               0x71
#define EPD_AUTO_MEASURE_VCOM        0x80
#define EPD_READ_VCOM_VALUE          0x81
#define EPD_VCM_DC_SETTING           0x82
#define EPD_PARTIAL_WINDOW           0x90
#define EPD_PARTIAL_IN               0x91
#define EPD_PARTIAL_OUT              0x92

// LVGL
#define LCD_DRAW_BUFF_DOUBLE (1)
#define LCD_DRAW_BUFF_HEIGHT (30)

// 함수 선언
esp_err_t epaper_spi_init(void);
esp_err_t epaper_display_init(void);
esp_err_t epaper_update(void);
esp_err_t epaper_deep_sleep(void);
esp_err_t epaper_fill_screen(uint8_t color_hex);
esp_err_t epaper_draw_px(int16_t x, int16_t y, uint16_t color);
// void epaper_draw_pixel(int16_t x, int16_t y, uint16_t color);

void set_rotation(int rot);
int get_rotation();