idf_component_register(SRCS "source/main.c" 
                        "source/screen/tft.cpp" 
                        "source/cf_grammar_gen.cpp" 
                        "source/screen/font.cpp" 
                        "source/files.cpp" 
                        "source/accelerometer.cpp"
                        "source/coordinator.cpp"
                        "source/screen/layout.cpp"
                    INCLUDE_DIRS "includes")
spiffs_create_partition_image(storage ../files FLASH_IN_PROJECT)