#include "my_uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "string.h"

#include "PD_controller.h"

#define UART_PORT 0     // 使用几号串口（0或1）
 
#if UART_PORT == 1
    #define UART_NUM UART_NUM_1
    #define UART_TX_PIN 17      // UART_NUM_1 管脚：tx:17, rx:18
    #define UART_RX_PIN 18
#else
    #define UART_NUM UART_NUM_0
    #define UART_TX_PIN UART_PIN_NO_CHANGE
    #define UART_RX_PIN UART_PIN_NO_CHANGE
#endif

#define BUF_SIZE (1024)

static const char *TAG = "my_uart";

/*初始化UART
 */
void init_uart() {
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    uart_driver_install(UART_NUM, BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM, &uart_config);
    uart_set_pin(UART_NUM, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}


/*定义信号量*/
SemaphoreHandle_t is_receive_data = NULL;
/*接收到的数据*/
int16_t x_delta = 0;      // percent_x == -1000 表示没有检测到敌人，正常范围-100到100
int16_t y_delta = 0;      // percent_y == -1000 表示没有检测到敌人，正常范围-100到100
uint8_t is_fire = 0;        // 1 表示准星在敌人身上
uint8_t is_main_wepon = 0;      // 1 表示主武器，2 表示副武器，0 表示没检测到
uint8_t is_open_scope = 0;  // 1 表示大狙开镜，0 表示大狙没开镜

/*UART接收主任务
 */
void uart_event_task(void *pvParameters) {
    uint8_t *data = (uint8_t *) malloc(BUF_SIZE);

    /*初始化信号量*/
    is_receive_data = xSemaphoreCreateBinary();

    init_uart();

    uint8_t fps_counter = 0;
    uint32_t last_time = esp_log_timestamp();
    uint32_t now_time;
    while(1){
        int len = uart_read_bytes(UART_NUM, data, BUF_SIZE, pdMS_TO_TICKS(10));
        data[len] = '\0'; // 确保字符串结束
        // 根据开头字符区分数据类型
        if (len > 0 && data[0] == 'A'){
            char *token = strtok((char *)(data + 1), ","); // 从'A'后开始解析
            if (token) x_delta = atoi(token);

            token = strtok(NULL, ",");
            if (token) y_delta = atoi(token);

            token = strtok(NULL, ",");
            if (token) is_fire = atoi(token);

            token = strtok(NULL, ",");
            if (token) is_main_wepon = atoi(token);

            token = strtok(NULL, ",");
            if (token) is_open_scope = atoi(token);

            xSemaphoreGive(is_receive_data);    //给信号
        }
        else if (len > 0 && data[0] == 'P'){
            char *token = strtok((char *)(data + 1), ","); // 从'P'后开始解析
            if (token != NULL) kp = atoi(token); // 解析 kp
            
            token = strtok(NULL, ",");
            if (token != NULL) kd = atoi(token); // 解析 kd
        }

        now_time = esp_log_timestamp();
        if(now_time - last_time >= 1000){
            last_time = now_time;
            ESP_LOGI(TAG, "fps:%d", fps_counter);
            fps_counter = 0;
        }
        // vTaskDelay(pdMS_TO_TICKS(2));
    }
    free(data); // 调用vTaskDelete(NULL)终止任务后此代码将会执行
}
