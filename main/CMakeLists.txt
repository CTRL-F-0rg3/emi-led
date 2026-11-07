idf_component_register(SRCS "main.c" "led.c"
                       INCLUDE_DIRS "."
                       REQUIRES led_strip nvs_flash bt)
