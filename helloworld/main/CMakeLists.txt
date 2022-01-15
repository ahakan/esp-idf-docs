idf_component_register(
    SRCS "src/main.cpp"
    INCLUDE_DIRS "inc/"
    REQUIRES freertos esp_common nvs_flash bt
)
