idf_component_register(SRCS "speaker.c"
                    INCLUDE_DIRS "."
                    REQUIRES freertos bt driver nvs_flash)
