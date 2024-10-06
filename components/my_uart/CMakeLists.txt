idf_component_register(
    SRCS "src/my_uart.c"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES esp_driver_uart PD_controller
)