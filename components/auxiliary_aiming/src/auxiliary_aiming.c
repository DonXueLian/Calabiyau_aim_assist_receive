#include "auxiliary_aiming.h"

#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"

#include "my_uart.h"
#include "spi_receive.h"
#include "PD_controller.h"
#include "mouse_imitate.h"

static const char *TAG = "aiming";


void auxiliary_aiming_task(void * param){
    /*用于定期执行任务*/
    TickType_t xLastWakeTime = xTaskGetTickCount();
    /*用于自瞄持续时间判断*/
    uint32_t auto_aiming_start_time = esp_log_timestamp();

    while(NULL == mouse_data_queue);    // 等待鼠标队列创建完成
    mouse_data_t mouse_data;
    while(1){
        /*处理串口接收到的数据*/
        if(pdTRUE == xSemaphoreTake(is_receive_data, pdMS_TO_TICKS(5))){
            /*检测到敌人*/
            // if(-1000 != x_delta){
            //     /*检测到敌人且准星在敌人身上，持续自瞄*/
            //     if(is_fire){
            //         auto_aiming_start_time = esp_log_timestamp();
            //     }
            //     /*检测到敌人且准星不在敌人身上，一段时间后停止自瞄*/
            //     else{
            //         if(esp_log_timestamp() - auto_aiming_start_time >= 1000){
            //             x_delta = 0;
            //             y_delta = 0;
            //         }
            //     }
            // }
            // /*未检测到敌人 或者 未使能自瞄*/
            // else{
            //     x_delta = 0;
            //     y_delta = 0;
            // }
            if(-1000 == x_delta){
                x_delta = 0;
                y_delta = 0;
            }
        }

        /*PD计算鼠标速度*/
        PD_controller(x_delta, y_delta, &(mouse_data.x_pos), &(mouse_data.y_pos));
        
        /*将鼠标数据将加入到鼠标队列*/
        mouse_data.is_spi = 0;      // 非原始鼠标信息
        if(mouse_data.x_pos != 0 && mouse_data.y_pos != 0)
            if(pdTRUE != xQueueSend(mouse_data_queue, &mouse_data, 1))
                    ESP_LOGI(TAG, "fail send to queue");

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(10)); 
    }

}

