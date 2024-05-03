idf_component_register(
    SRCS "main.c" "example_neopixel.c"
    INCLUDE_DIRS "../include/"
    REQUIRES esp_common driver)
