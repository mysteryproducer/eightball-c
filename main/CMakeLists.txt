set(COMPONENT_SRCS "main.c"
                    "main.cpp"
                    "files.cpp"
                    "text/GrammarGenerator.cpp"
                    "text/LineReader.cpp"
                    "mpu6050/motion.c"
                    "gc9a01/font.cpp"
                    "gc9a01/EightBallScreen.cpp"
                    "gc9a01/EightBallScreen_layout.cpp")
set(REQUIRED_COMPONENTS esp_driver_gpio esp_driver_i2c freertos 
                        json_parser 
                        fatfs console)
if(CONFIG_IDF_TARGET_ESP32S3) 
        list(APPEND COMPONENT_SRCS "usbmsc.cpp")
        list(APPEND REQUIRED_COMPONENTS esp_tinyusb wear_levelling esp_partition spi_flash)
endif()
idf_component_register(SRCS ${COMPONENT_SRCS}
                    INCLUDE_DIRS "." 
                            "headers"
                    REQUIRES ${REQUIRED_COMPONENTS}
                    )
if(CONFIG_IDF_TARGET_ESP32S3) 
        fatfs_create_spiflash_image(storage ./files FLASH_IN_PROJECT)
endif()
# fatfs_create_rawflash_image(storage ./files FLASH_IN_PROJECT)
