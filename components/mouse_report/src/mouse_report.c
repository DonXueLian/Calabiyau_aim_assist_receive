#include "mouse_report.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "tusb.h"

#include "mouse_imitate.h"
#include "spi_receive.h"

static const char *TAG = "mouse_report";

void mouse_report_task(void * param){
    mouse_init();

    /*用于回报率检测*/
    uint32_t last_time = esp_log_timestamp();
    int8_t last_x_delta = 0;
    int8_t last_y_delta = 0;
    uint16_t times = 0;

    mouse_data_t mouse_data;
    while(NULL == mouse_data_queue);    // 等待mouse_data_queue创建成功
    while(1){
        /*回报率检测*/
        if(esp_log_timestamp() - last_time >= 1000){
            ESP_LOGI(TAG, "%d", times);
            times = 0;
            last_time = esp_log_timestamp();
        }
        /*发送鼠标数据到电脑*/
        if(pdTRUE == xQueueReceive(mouse_data_queue, &mouse_data, portMAX_DELAY)){
            times++;
            mouse_report(mouse_data.button_val, mouse_data.x_pos, mouse_data.y_pos, mouse_data.is_spi, mouse_data.whirl);
        }
    }
}
