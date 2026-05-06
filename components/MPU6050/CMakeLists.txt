idf_component_register(
    SRCS "MPU6050.c"
    INCLUDE_DIRS "."
    REQUIRES BSP driver MessageQueue
)