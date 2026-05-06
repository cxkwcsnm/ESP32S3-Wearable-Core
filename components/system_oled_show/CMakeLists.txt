
idf_component_register(
    SRCS 
        "system_oled_show.c"

    INCLUDE_DIRS "."
    REQUIRES OLED_Data driver GetBatteryLevel BSP OLED_driver RTC_time WIFI_manager MAX30102 MPU6050
)