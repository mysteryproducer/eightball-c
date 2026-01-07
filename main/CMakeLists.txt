idf_component_register(SRCS "main.c"
                            "mpu6050/motion.c"
                            "cf_grammar_gen.cpp"
                            "screen.c"
                    INCLUDE_DIRS "." 
                            "headers"
                            "mpu6050"
                    REQUIRES driver freertos spiffs)
spiffs_create_partition_image(storage ./files FLASH_IN_PROJECT)
