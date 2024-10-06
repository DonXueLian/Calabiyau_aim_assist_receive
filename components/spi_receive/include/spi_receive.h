#ifndef __SPI_RECELVE_H__
#define __SPI_RECELVE_H__

#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#define RCV_HOST    SPI2_HOST

#define GPIO_HANDSHAKE      2
#define GPIO_MOSI           12
#define GPIO_MISO           13
#define GPIO_SCLK           15
#define GPIO_CS             14

extern QueueHandle_t mouse_data_queue;

typedef struct
{
    int16_t x_pos;
    int16_t y_pos;
    uint8_t button_val;
    uint8_t is_spi;
    int8_t whirl;
}mouse_data_t;

void spi_receive_task(void * param);

#endif
