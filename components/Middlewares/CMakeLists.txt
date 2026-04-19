# components/Middlewares/CMakeLists.txt
idf_component_register(
    SRCS "GetBatteryLevel/GetBatteryLevel.c"
    INCLUDE_DIRS "GetBatteryLevel"
    REQUIRES esp_adc driver
)