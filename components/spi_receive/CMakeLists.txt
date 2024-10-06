idf_component_register(
    SRCS "src/spi_receive.c"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES esp_driver_spi esp_driver_gpio esp_timer
)