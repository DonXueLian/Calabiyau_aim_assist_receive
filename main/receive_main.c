/* SPI Slave example, receiver (uses SPI Slave driver to communicate with sender)

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tusb.h"

#include "driver/spi_slave.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include "my_uart.h"
#include "spi_receive.h"
#include "auxiliary_aiming.h"
#include "mouse_report.h"

static const char *TAG = "receive_main";

void app_main(void)
{
    xTaskCreatePinnedToCore(uart_event_task, "uart_raceive", 2048*2, NULL, 3, NULL, 0);
    xTaskCreatePinnedToCore(spi_receive_task, "spi_receive", 2048*3, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(auxiliary_aiming_task, "auxiliary_aiming", 2048*2, NULL, 2, NULL, 0);
    xTaskCreatePinnedToCore(mouse_report_task, "mouse_report", 2048*3, NULL, 4, NULL, 1);
    // xTaskCreatePinnedToCore(my_usb, "my_usb", 2048*2, NULL, 6, NULL, 0);
}

/*
void my_usb(void * param){
    TickType_t xLastWakeTime = xTaskGetTickCount();
    uint32_t last_time = esp_log_timestamp();
    uint16_t times = 0;
    while(1){
        times++;
        if(esp_log_timestamp() - last_time >= 1000){
            ESP_LOGI(TAG, "%d", times);
            times = 0;
            last_time = esp_log_timestamp();
        }
        // last_time = esp_log_timestamp();
        tud_task();
        // ESP_LOGI(TAG, "%d", (int)(esp_log_timestamp() - last_time));
        // vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(2));
    }
}
*/
