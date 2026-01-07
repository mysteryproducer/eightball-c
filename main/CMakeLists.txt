idf_component_register(SRCS "main.c" "mpu6050/motion.c"
                    INCLUDE_DIRS "." "headers" "mpu6050"
                    REQUIRES driver freertos)

