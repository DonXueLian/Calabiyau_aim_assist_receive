#ifndef __MY_UART_H__
#define __MY_UART_H__

#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

/*创建信号量*/
extern SemaphoreHandle_t is_receive_data;

/*接收到的数据*/
extern int16_t x_delta;      // x_bias == -1000 表示没有检测到敌人，正常范围-100到100
extern int16_t y_delta;      // y_bias == -1000 表示没有检测到敌人，正常范围-100到100
extern uint8_t is_fire;        // 1 表示准星在敌人身上
extern uint8_t is_main_wepon;  // 1 表示主武器，2 表示副武器，0 表示没检测到
extern uint8_t is_open_scope;  // 1 表示大狙开镜，0 表示大狙没开镜


void uart_event_task(void * param);

#endif
