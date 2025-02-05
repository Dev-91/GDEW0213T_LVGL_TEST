#include <stdio.h>
#include "esp_log.h"
#include "esp_err.h"
#include "esp_timer.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"

#include "epaper.h"
#include "lvgl.h"

#include "ui/ui.h"

// 디스플레이 해상도 정의
#define EPD_WIDTH   104
#define EPD_HEIGHT  212

#define BUF_SIZE ((EPD_HEIGHT * EPD_WIDTH))

#define LV_TICK_PERIOD_MS 10

// GPIO 정의
#define EPD_PIN_BUSY    4
#define EPD_PIN_RST     16
#define EPD_PIN_DC      17
#define EPD_PIN_CS      5
#define EPD_PIN_CLK     18
#define EPD_PIN_MOSI    23

#define BUTTON_PIN 39   // 버튼 GPIO 핀

#define SWAP(a, b) do { \
    typeof(a) _tmp = (a); \
    (a) = (b);        \
    (b) = _tmp;       \
} while (0)

// #define LVGL_8
#define LVGL_9

static QueueHandle_t gpio_evt_queue = NULL;

#if defined(LVGL_8)
static lv_disp_drv_t disp;
#elif defined(LVGL_9)
static lv_display_t *disp = NULL;
#endif

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

uint8_t rgb565_brightness(uint16_t color) {
    // RGB565 형식에서 각 색상 성분 추출
    uint8_t r = (color >> 11) & 0x1F;  // 5비트
    uint8_t g = (color >> 5) & 0x3F;   // 6비트
    uint8_t b = color & 0x1F;          // 5비트

    // 5비트, 6비트 값을 8비트로 변환
    r = (r << 3) | (r >> 2);  // 5비트 -> 8비트
    g = (g << 2) | (g >> 4);  // 6비트 -> 8비트
    b = (b << 3) | (b >> 2);  // 5비트 -> 8비트

    // ITU-R BT.709 공식 사용하여 밝기 계산
    // Y = 0.2126*R + 0.7152*G + 0.0722*B
    return (uint8_t)((r * 54 + g * 183 + b * 19) >> 8);
}

#if defined(LVGL_8)
void my_disp_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p) {
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
    lv_disp_flush_ready(disp_drv);
}

#elif defined(LVGL_9)
static void my_disp_flush(lv_display_t * disp_drv, const lv_area_t * area, void * px_map)
{
    epaper_display_init();
    
    int rotation = lv_disp_get_rotation(disp);

    uint16_t * buf16 = (uint16_t *)px_map; /*Let's say it's a 16 bit (RGB565) display*/
    for(int y = area->y1; y <= area->y2; y++) {
        for(int x = area->x1; x <= area->x2; x++) {
            
            // black & white
            uint8_t brightness = rgb565_brightness(*buf16);
            uint16_t mono_color = (brightness > 128) ? 0x00 : 0xFF;  // 임계값 128
            
            epaper_draw_pixel(x, y, mono_color, rotation);

            buf16++;
        }
    }

    epaper_update();
    lv_display_flush_ready(disp_drv);
}
#endif

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

    #if defined(LVGL_8)
    /*Create a white label, set its text and align it to the center*/
    lv_obj_t * label = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_color(label, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_label_set_text(label, "Hello World");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

    #elif defined(LVGL_9)
    /*Change the active screen's background color*/
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0xFFFFFF), LV_PART_MAIN);

    /*Create a white label, set its text and align it to the center*/
    lv_obj_t * label = lv_label_create(lv_screen_active());
    lv_label_set_text(label, "Hello World");
    lv_obj_set_style_text_color(label, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    #endif
}

void lvgl_setup() {
#if defined(LVGL_8)
    lv_init();

    static lv_disp_draw_buf_t draw_buf;
    static lv_color_t buf[BUF_SIZE];
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, BUF_SIZE);

    lv_disp_drv_init(&disp);
    disp.hor_res = EPD_WIDTH;
    disp.ver_res = EPD_HEIGHT;
    disp.flush_cb = my_disp_flush;
    disp.draw_buf = &draw_buf;
    disp.full_refresh = 1;

    disp.rotated = 0;
    set_rotation(0);
    
    lv_disp_drv_register(&disp);
#elif defined(LVGL_9)
    lv_init();

    static lv_draw_buf_t *buf[BUF_SIZE];
    disp = lv_display_create(EPD_WIDTH, EPD_HEIGHT);

    lv_display_set_flush_cb(disp, (lv_display_flush_cb_t) my_disp_flush);
    // lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
    
    lv_display_set_buffers(disp, buf, NULL, sizeof(buf), LV_DISPLAY_RENDER_MODE_PARTIAL);
    
    lv_disp_set_rotation(disp, LV_DISP_ROTATION_90);
#endif

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

    for (int i = 0; i < 5; i++) {
        ESP_LOGI(TAG, "%d seconds", i+1);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    while(1) {
        for (int i = 0; i < 4; i++) {
            lv_disp_set_rotation(disp, i);
            create_lvgl_ui();
            vTaskDelay(pdMS_TO_TICKS(5000));
        }
    }

    // ui_init();
}
