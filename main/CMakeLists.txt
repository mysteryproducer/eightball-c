idf_component_register(SRCS "source/main.c" 
                        "source/tft.cpp" 
                        "source/cf_grammar_gen.cpp" 
                        "source/font.cpp" 
                        "source/files.cpp" 
                        "source/accelerometer.cpp"
                        "source/coordinator.cpp"
                    INCLUDE_DIRS "includes")
spiffs_create_partition_image(storage ../files FLASH_IN_PROJECT)