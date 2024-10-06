idf_component_register(
    SRCS "src/mouse_report.c"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES spi_receive mouse_imitate esp_tinyusb
)


