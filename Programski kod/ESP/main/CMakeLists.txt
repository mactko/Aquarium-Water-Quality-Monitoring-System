idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "include"
                    REQUIRES ds18b20 SEN0189 pump wifi freertos driver nvs_flash esp_timer mqtt)
