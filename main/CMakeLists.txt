idf_component_register(SRCS "main.c"
                            "screen.cpp"
                            "files.cpp"
                            "text/cf_grammar_gen.cpp"
                            "mpu6050/motion.c"
                            "gc9a01/tft.cpp"
                            "gc9a01/font.cpp"
                            "gc9a01/layout.cpp"
                    INCLUDE_DIRS "." 
                            "headers"
                    REQUIRES esp_driver_gpio esp_driver_i2c freertos spiffs)
spiffs_create_partition_image(storage ./files FLASH_IN_PROJECT)
