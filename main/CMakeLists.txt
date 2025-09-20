idf_component_register(SRCS "source/main.c" "source/tft.c" "source/cf_grammar_gen.cpp"
                    INCLUDE_DIRS "includes")
spiffs_create_partition_image(storage ../files FLASH_IN_PROJECT)