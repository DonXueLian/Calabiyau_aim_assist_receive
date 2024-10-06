/*
 * SPDX-FileCopyrightText: 2022-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
#include "mouse_imitate.h"
#include <stdlib.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "tinyusb.h"
#include "class/hid/hid_device.h"
#include "tusb.h"


static const char *TAG = "mouse_imitate";

/************* TinyUSB descriptors ****************/

#define TUSB_DESC_TOTAL_LEN      (TUD_CONFIG_DESC_LEN + CFG_TUD_HID * TUD_HID_DESC_LEN)

/**
 * @brief HID report descriptor
 *
 * In this example we implement Keyboard + Mouse HID device,
 * so we must define both report descriptors
 */
const uint8_t hid_report_descriptor[] = {
    TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(HID_ITF_PROTOCOL_KEYBOARD)),
    TUD_HID_REPORT_DESC_MOUSE(HID_REPORT_ID(HID_ITF_PROTOCOL_MOUSE))
};

/**
 * @brief String descriptor
 */
const char* hid_string_descriptor[5] = {
    // array of pointer to string descriptors
    (char[]){0x09, 0x04},  // 0: is supported language is English (0x0409)
    "TinyUSB",             // 1: Manufacturer
    "TinyUSB Device",      // 2: Product
    "123456",              // 3: Serials, should use chip ID
    "Example HID interface",  // 4: HID
};

/**
 * @brief Configuration descriptor
 *
 * This is a simple configuration descriptor that defines 1 configuration and 1 HID interface
 */
static const uint8_t hid_configuration_descriptor[] = {
    // Configuration number, interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(1, 1, 0, TUSB_DESC_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),

    // Interface number, string index, boot protocol, report descriptor len, EP In address, size & polling interval
    TUD_HID_DESCRIPTOR(0, 4, false, sizeof(hid_report_descriptor), 0x81, 16, 1),
};

/********* TinyUSB HID callbacks ***************/

// Invoked when received GET HID REPORT DESCRIPTOR request
// Application return pointer to descriptor, whose contents must exist long enough for transfer to complete
uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance)
{
    // We use only one interface and one HID report descriptor, so we can ignore parameter 'instance'
    return hid_report_descriptor;
}

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen)
{
    (void) instance;
    (void) report_id;
    (void) report_type;
    (void) buffer;
    (void) reqlen;

    return 0;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize)
{
}

/********* Application ***************/

typedef enum {
    MOUSE_DIR_RIGHT,
    MOUSE_DIR_DOWN,
    MOUSE_DIR_LEFT,
    MOUSE_DIR_UP,
    MOUSE_DIR_MAX,
} mouse_dir_t;



void mouse_init(void){
    ESP_LOGI(TAG, "USB initializing...");
        const tinyusb_config_t tusb_cfg = {
            .device_descriptor = NULL,
            .string_descriptor = hid_string_descriptor,
            .string_descriptor_count = sizeof(hid_string_descriptor) / sizeof(hid_string_descriptor[0]),
            .external_phy = false,
    #if (TUD_OPT_HIGH_SPEED)
            .fs_configuration_descriptor = hid_configuration_descriptor, // HID configuration descriptor for full-speed and high-speed are the same
            .hs_configuration_descriptor = hid_configuration_descriptor,
            .qualifier_descriptor = NULL,
    #else
            .configuration_descriptor = hid_configuration_descriptor,
    #endif // TUD_OPT_HIGH_SPEED
    };

    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));
    ESP_LOGI(TAG, "USB initialization DONE");
}

/* buttoms：b'0000_0000 所有按键松开
            b'0000_0001 左键按下
            b'0000_0010 右键按下
            b'0000_0100 滚轮按下
            b'0000_1000 下侧键按下
            b'0001_0000 上侧键按下
    is_spi：1:鼠标原始数据
            0:非鼠标原始数据
 */
void mouse_report(uint8_t buttoms, int8_t delta_x, int8_t delta_y, uint8_t is_spi, int8_t whirl){
    /*用于储存鼠标原始按键数据*/
    static uint8_t last_buttoms;
    static int8_t x_direction, y_direction;
    /*连接上usb*/
    if(tud_mounted()){
        /*按键采用鼠标原始数据，不采用非原始数据*/
        if(is_spi)
            last_buttoms = buttoms;
        else
            buttoms = last_buttoms;
        /*判断自瞄移动方向与鼠标移动方向是否相同，相同则忽略自瞄移动，防止速度过快*/
        if(is_spi){
            x_direction = delta_x > 1 ? 1 : (delta_x < -1 ? -1 : 0);
            y_direction = delta_y > 1 ? 1 : (delta_x < -1 ? -1 : 0);
        }
        else{
            if(delta_x * x_direction >= 1) delta_x = 0; // 同方向, 忽略非鼠标原始数据
            if(delta_x * y_direction >= 1) delta_y = 0; // 同方向, 忽略非鼠标原始数据
        }
        /*判断是否按下左键，按下则开启自瞄*/
        if(is_spi){
            tud_hid_mouse_report(HID_ITF_PROTOCOL_MOUSE, buttoms, delta_x, delta_y, whirl, 0);
        }
        else{
            if(last_buttoms & 0x01){
                tud_hid_mouse_report(HID_ITF_PROTOCOL_MOUSE, buttoms, delta_x, delta_y, 0, 0);
            }
        }
    }
}

