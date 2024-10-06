#include "spi_receive.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "driver/spi_slave.h"
#include "driver/gpio.h"
#include <string.h>

static const char *TAG = "spi_receive";

QueueHandle_t mouse_data_queue = NULL;

//Called after a transaction is queued and ready for pickup by master. We use this to set the handshake line high.
static void my_post_setup_cb(spi_slave_transaction_t *trans)
{
    gpio_set_level(GPIO_HANDSHAKE, 1);
}

//Called after transaction is sent/received. We use this to set the handshake line low.
static void my_post_trans_cb(spi_slave_transaction_t *trans)
{
    gpio_set_level(GPIO_HANDSHAKE, 0);
}

void spi_receive_task(void * param){
    esp_err_t ret;

    //Configuration for the SPI bus
    spi_bus_config_t buscfg = {
        .mosi_io_num = GPIO_MOSI,
        .miso_io_num = GPIO_MISO,
        .sclk_io_num = GPIO_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };

    //Configuration for the SPI slave interface
    spi_slave_interface_config_t slvcfg = {
        .mode = 0,
        .spics_io_num = GPIO_CS,
        .queue_size = 3,
        .flags = 0,
        .post_setup_cb = my_post_setup_cb,
        .post_trans_cb = my_post_trans_cb
    };

    //Configuration for the handshake line
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = BIT64(GPIO_HANDSHAKE),
    };

    //Configure handshake line as output
    gpio_config(&io_conf);
    //Enable pull-ups on SPI lines so we don't detect rogue pulses when no master is connected.
    gpio_set_pull_mode(GPIO_MOSI, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(GPIO_SCLK, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(GPIO_CS, GPIO_PULLUP_ONLY);

    //Initialize SPI slave interface
    ret = spi_slave_initialize(RCV_HOST, &buscfg, &slvcfg, SPI_DMA_CH_AUTO);
    assert(ret == ESP_OK);

    WORD_ALIGNED_ATTR char sendbuf[21] = "";
    WORD_ALIGNED_ATTR char recvbuf[21] = "";
    memset(recvbuf, 0, 33);
    spi_slave_transaction_t t;
    memset(&t, 0, sizeof(t));

    mouse_data_queue = xQueueCreate(20, sizeof(mouse_data_t));

    while(1){
        //Clear receive buffer, set send buffer to something sane
        memset(recvbuf, 0xA5, 21);
        //Set up a transaction of 128 bytes to send/receive
        t.length = 21 * 8;
        t.tx_buffer = sendbuf;
        t.rx_buffer = recvbuf;
        /* This call enables the SPI slave interface to send/receive to the sendbuf and recvbuf. The transaction is
        initialized by the SPI master, however, so it will not actually happen until the master starts a hardware transaction
        by pulling CS low and pulsing the clock etc. In this specific example, we use the handshake line, pulled up by the
        .post_setup_cb callback that is called as soon as a transaction is ready, to let the master know it is free to transfer
        data.
        */
        ret = spi_slave_transmit(RCV_HOST, &t, portMAX_DELAY);
        //spi_slave_transmit does not return until the master has done a transmission, so by here we have sent our data and
        //received data from the master. Print it.

        mouse_data_t mouse_data;
        if(sscanf(recvbuf, "%hd,%hd,%hhu,%hhd", &(mouse_data.x_pos), &(mouse_data.y_pos), &(mouse_data.button_val), &(mouse_data.whirl)) == 4){
            mouse_data.is_spi = 1;
            if(pdTRUE != xQueueSend(mouse_data_queue, &mouse_data, pdMS_TO_TICKS(1)))
                ESP_LOGI(TAG, "fail send to queue");
        }
    }

}

