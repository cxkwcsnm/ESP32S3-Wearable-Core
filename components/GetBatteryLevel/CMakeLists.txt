
idf_component_register(
    SRCS 
        "GetBatteryLevel.c"
        

    INCLUDE_DIRS "." 
        
        
    REQUIRES esp_adc driver OLED_Data
)