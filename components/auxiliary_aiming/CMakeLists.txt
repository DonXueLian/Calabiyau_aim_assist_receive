idf_component_register(
    SRCS "src/auxiliary_aiming.c"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES esp_timer mouse_imitate my_uart PD_controller spi_receive
)