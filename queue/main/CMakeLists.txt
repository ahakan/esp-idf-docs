idf_component_register(
    SRCS "main.c" "example_queue.c"
    INCLUDE_DIRS "../include/"
    REQUIRES freertos esp_common)
