#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_log.h"

#include "epaper.h"

static const char *TAG = "epaper";

// SPI 핸들러
static spi_device_handle_t spi;

int16_t _current_page = -1;
int rotation = 0;

// 내부 함수 선언
static void epaper_reset(void);
static void epaper_wait_busy(void);
static void epaper_send_command(uint8_t cmd);
static void epaper_send_data(uint8_t data);

void swap(int16_t a, int16_t b)
{
    int16_t t = a;
    a = b;
    b = t;
}

// 전역 프레임 버퍼 선언
static uint8_t frame_buffer[EPD_ARRAY];

int get_rotation() {
    return rotation;
}

void set_rotation(int rot) {
    rotation = rot;
}

esp_err_t epaper_spi_init(void)
{
    esp_err_t ret;

    // GPIO 초기화
    gpio_set_direction(EPD_PIN_DC, GPIO_MODE_OUTPUT);
    gpio_set_direction(EPD_PIN_RST, GPIO_MODE_OUTPUT);
    gpio_set_direction(EPD_PIN_BUSY, GPIO_MODE_INPUT);

    // SPI 버스 설정
    spi_bus_config_t buscfg = {
        .mosi_io_num = EPD_PIN_MOSI,
        .miso_io_num = -1,
        .sclk_io_num = EPD_PIN_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };

    // SPI 디바이스 설정
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 10*1000*1000,
        .mode = 0,
        .spics_io_num = EPD_PIN_CS,
        .queue_size = 7,
    };

    // SPI 버스 초기화
    ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
    ESP_ERROR_CHECK(ret);

    // SPI 디바이스 추가
    ret = spi_bus_add_device(SPI2_HOST, &devcfg, &spi);
    ESP_ERROR_CHECK(ret);

    vTaskDelay(pdMS_TO_TICKS(1000));
    ESP_LOGI(TAG, "E-paper initialized");

    return ESP_OK;
}

esp_err_t epaper_display_init(void)
{
    // E-paper 초기화 시퀀스
    // Nomal operation flow
    epaper_reset();

    epaper_send_command(EPD_BOOSTER_SOFT_START);
    epaper_send_data(0x17);
    epaper_send_data(0x17);
    epaper_send_data(0x17);

    epaper_send_command(EPD_POWER_SETTING);
    epaper_send_data(0x03);           
    epaper_send_data(0x00);
    epaper_send_data(0x2b);
    epaper_send_data(0x2b);
    epaper_send_data(0x03);

    epaper_send_command(EPD_POWER_ON);
    epaper_wait_busy();

    epaper_send_command(EPD_PANEL_SETTING);
    epaper_send_data(0x1F);
    epaper_send_data(0x0D);

    epaper_send_command(EPD_PLL_CONTROL);
    epaper_send_data(0x3C);

    epaper_send_command(EPD_RESOLUTION_SETTING);
    epaper_send_data(EPD_WIDTH);
    epaper_send_data(0);
    epaper_send_data(EPD_HEIGHT%256); 
    
    epaper_send_command(EPD_VCOM_AND_DATA_INTERVAL_SETTING);     //VCOM AND DATA INTERVAL SETTING      
    epaper_send_data(0x97);    //WBmode:VBDF 17|D7 VBDW 97 VBDB 57   WBRmode:VBDF F7 VBDW 77 VBDB 37  VBDR B7

    return ESP_OK;
}

esp_err_t epaper_fill_screen(uint8_t color_hex)
{
    unsigned int i;

    for(i = 0; i < EPD_ARRAY; i++)
    {
      frame_buffer[i] = color_hex;
    }

    return ESP_OK;
}

esp_err_t epaper_draw_px(int16_t x, int16_t y, uint16_t color)
{
    if (x >= EPD_WIDTH || y >= EPD_HEIGHT) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // 픽셀의 위치 계산
    uint32_t addr = x / 8 + y * (EPD_WIDTH / 8);
    uint8_t bit = 7 - (x % 8);

    // 색상에 따라 비트 설정
    if (color == 0x00) {
        frame_buffer[addr] |= (1 << bit);
    } else {
        frame_buffer[addr] &= ~(1 << bit);
    }

    return ESP_OK;
}

void epaper_draw_pixel(int16_t x, int16_t y, uint16_t color)
{
  if ((x < 0) || (x >= EPD_WIDTH) || (y < 0) || (y >= EPD_HEIGHT)) return;

  // check rotation, move pixel around if necessary
  switch (rotation)
  {
    case 1:
      swap(x, y);
      x = EPD_WIDTH - x - 1;
      break;
    case 2:
      x = EPD_WIDTH - x - 1;
      y = EPD_HEIGHT - y - 1;
      break;
    case 3:
      swap(x, y);
      y = EPD_HEIGHT - y - 1;
      break;
  }
  uint16_t i = x / 8 + y * EPD_WIDTH / 8;
  if (_current_page < 1)
  {
    if (i >= sizeof(frame_buffer)) return;
  }
  else
  {
    y -= _current_page * EPD_HEIGHT;
    if ((y < 0) || (y >= EPD_HEIGHT)) return;
    i = x / 8 + y * EPD_WIDTH / 8;
  }

  if (!color)
    frame_buffer[i] = (frame_buffer[i] | (1 << (7 - x % 8)));
  else
    frame_buffer[i] = (frame_buffer[i] & (0xFF ^ (1 << (7 - x % 8))));
}

esp_err_t epaper_update(void)
{
    unsigned int i;
    //Write Data
    epaper_send_command(EPD_DATA_START_TRANSMISSION_1);        //Transfer old data
    for(i = 0; i < EPD_ARRAY; i++)
    { 
      epaper_send_data(0xFF);
    }

    epaper_send_command(EPD_DATA_START_TRANSMISSION_2);        //Transfer new data
    for(i = 0; i < EPD_ARRAY; i++)
    {
        epaper_send_data(frame_buffer[i]);
    }

    epaper_send_command(EPD_DISPLAY_REFRESH);
    vTaskDelay(pdMS_TO_TICKS(1));
    epaper_wait_busy();

    return ESP_OK;
}

esp_err_t epaper_deep_sleep()
{
    epaper_send_command(EPD_VCOM_AND_DATA_INTERVAL_SETTING);  // VCOM AND DATA INTERVAL SETTING     
    epaper_send_data(0xF7); // WBmode:VBDF 17|D7 VBDW 97 VBDB 57    WBRmode:VBDF F7 VBDW 77 VBDB 37  VBDR B7  

    epaper_send_command(EPD_POWER_OFF);     // power off
    vTaskDelay(pdMS_TO_TICKS(100));         // 100ms
    epaper_send_command(EPD_DEEP_SLEEP);    // deep sleep
    epaper_send_data(0xA5);

    return ESP_OK;
}

static void epaper_reset(void)
{
    gpio_set_level(EPD_PIN_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(EPD_PIN_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
}

static void epaper_wait_busy(void)
{
    while (gpio_get_level(EPD_PIN_BUSY) == 1) {
        vTaskDelay(pdMS_TO_TICKS(100));
        ESP_LOGI(TAG, "E-paper busy...");
    }
}

static void epaper_send_command(uint8_t cmd)
{
    gpio_set_level(EPD_PIN_DC, 0);
    spi_transaction_t t = {
        .length = 8,
        .tx_buffer = &cmd,
    };
    spi_device_polling_transmit(spi, &t);
}

static void epaper_send_data(uint8_t data)
{
    gpio_set_level(EPD_PIN_DC, 1);
    spi_transaction_t t = {
        .length = 8,
        .tx_buffer = &data,
    };
    spi_device_polling_transmit(spi, &t);
}
