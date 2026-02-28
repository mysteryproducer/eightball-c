idf_component_register(SRCS "main.c"
                            "main.cpp"
                            "files.cpp"
                            "usbmsc.cpp"
                            "text/GrammarGenerator.cpp"
                            "text/LineReader.cpp"
                            "mpu6050/motion.c"
                            "gc9a01/font.cpp"
                            "gc9a01/functions.cpp"
                            "gc9a01/EightBallScreen.cpp"
                            "gc9a01/EightBallScreen_layout.cpp"
                    INCLUDE_DIRS "." 
                            "headers"
                    REQUIRES esp_driver_gpio esp_driver_i2c freertos 
                    json_parser 
                    fatfs esp_tinyusb console wear_levelling esp_partition spi_flash
                    )
# fatfs_create_spiflash_image(storage ./files FLASH_IN_PROJECT)
# fatfs_create_rawflash_image(storage ./files FLASH_IN_PROJECT)
