#include <stdio.h>
#include "esp_log.h"
#include "esp_err.h"
#include "esp_timer.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
// #include "esp_lcd_panel_io.h"
// #include "esp_lcd_panel_vendor.h"
// #include "esp_lcd_panel_ops.h"

#include "epaper.h"
#include "lvgl.h"

// 디스플레이 해상도 정의
#define EPD_WIDTH   104
#define EPD_HEIGHT  212

#define BUF_SIZE (EPD_HEIGHT * EPD_WIDTH)

#define LV_TICK_PERIOD_MS 100

// GPIO 정의
#define EPD_PIN_BUSY    4
#define EPD_PIN_RST     16
#define EPD_PIN_DC      17
#define EPD_PIN_CS      5
#define EPD_PIN_CLK     18
#define EPD_PIN_MOSI    23

#define BUTTON_PIN 39   // 버튼 GPIO 핀

static QueueHandle_t gpio_evt_queue = NULL;


static lv_disp_drv_t disp_drv;

static const char* TAG = "main";

// 버튼 인터럽트 핸들러
static void IRAM_ATTR button_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

// GPIO 이벤트 처리 태스크
static void gpio_task(void* arg)
{
    uint32_t gpio_num;
    while(1) {
        if(xQueueReceive(gpio_evt_queue, &gpio_num, portMAX_DELAY)) {
            ESP_LOGI(TAG, "Button pressed!");
            epaper_display_init();
            epaper_fill_screen(0xFF); // 흰색 화면 표시
            epaper_update();
            epaper_deep_sleep();
        }
    }
    vTaskDelete(NULL);
}

static void gpio_init(void)
{
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    
    xTaskCreate(gpio_task, "gpio_task", 1024, NULL, 10, NULL);

    // 버튼 GPIO 설정
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BUTTON_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE,
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    // 인터럽트 핸들러 설정
    ESP_ERROR_CHECK(gpio_install_isr_service(0));
    ESP_ERROR_CHECK(gpio_isr_handler_add(BUTTON_PIN, button_isr_handler, (void*) BUTTON_PIN));
}

void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    epaper_display_init();

    for (int y = area->y1; y <= area->y2; y++) {
        for (int x = area->x1; x <= area->x2; x++) {
            uint8_t brightness = lv_color_brightness(*color_p);
            uint16_t mono_color = (brightness > 128) ? 0x00 : 0xFF;  // 임계값 128

            epaper_draw_px(x, y, mono_color);
            // epaper_draw_pixel(x, y, mono_color);
            color_p++;
        }
    }

    epaper_update();
    lv_disp_flush_ready(disp);
}

static void lv_tick_task(void *arg)
{
    (void) arg;

    lv_tick_inc(LV_TICK_PERIOD_MS);
}

void lv_handler(void *pvParameter) {
    while(1) {
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    vTaskDelete(NULL);
}

void create_lvgl_ui() {
    ESP_LOGI("LVGL", "Hello World Start");
    /*Create a white label, set its text and align it to the center*/
    lv_obj_t * label = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_color(label, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_label_set_text(label, "Hello World");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    ESP_LOGI("LVGL", "Hello World End");
}

void lvgl_setup() {
    lv_init();

    static lv_disp_draw_buf_t draw_buf;
    static lv_color_t buf[BUF_SIZE];
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, BUF_SIZE);

    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = EPD_WIDTH;
    disp_drv.ver_res = EPD_HEIGHT;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    disp_drv.full_refresh = 1;

    disp_drv.rotated = 0;
    set_rotation(0);
    
    lv_disp_drv_register(&disp_drv);
    
    /* Create and start a periodic timer interrupt to call lv_tick_inc */
    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &lv_tick_task,
        .name = "periodic_gui"
    };
    esp_timer_handle_t periodic_timer;
    esp_timer_create(&periodic_timer_args, &periodic_timer);
    esp_timer_start_periodic(periodic_timer, LV_TICK_PERIOD_MS * 1000);

    vTaskDelay(pdMS_TO_TICKS(3000));
    xTaskCreate(&lv_handler, "lv_handler", 2048 * 8, NULL, 5, NULL);
}

void app_main(void)
{
    gpio_init();
    epaper_spi_init();

    lvgl_setup();
    ESP_LOGI(TAG, "LVGL Setup End");

    create_lvgl_ui();
}
