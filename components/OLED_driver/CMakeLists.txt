
idf_component_register(
    SRCS 
        "OLED_driver.c"

    INCLUDE_DIRS "."
    REQUIRES OLED_Data driver
)