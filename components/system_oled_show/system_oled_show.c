#include "system_oled_show.h"

void OLEDShowTask(void *pvParameters)
{

    while (1)
    {
        float voltage = 0.0f;
        battery_get_average_voltage(5, &voltage); // 采样 5 次，获取平均电压
        ESP_LOGI("BATTERY", "平均电压: %.2f V", voltage);

        char voltage_str[20];
        snprintf(voltage_str, sizeof(voltage_str), "%.2f V", voltage);

        oled_clear_buffer();
        oled_show_string(0, 0, RTC_get_HH_MM_SS()); // 显示当前时分秒

        oled_show_string(0, 3, "Voltage:");
        oled_show_string(48, 3, voltage_str);

        uint32_t heart_rate = Max30102_Get_Heart_Rate();
        char heart_rate_str[15];
        if (heart_rate > 0 && heart_rate <= 200)
        {
            snprintf(heart_rate_str, sizeof(heart_rate_str), "%lu bpm", heart_rate);
        }
        else
        {
            snprintf(heart_rate_str, sizeof(heart_rate_str), "-- bpm");
        }
        oled_show_string(0, 4, "Heart Rate:");
        oled_show_string(64, 4, heart_rate_str);

        
        if (wifi_is_connected())
        {
            oled_draw_image(80, 0, wifiImg.data, wifiImg.width, wifiImg.height);
        }

        // 使用 oled_draw_image 显示电池图标
        oled_draw_image(100, 0, battery_pattern(voltage), 19, 8);

        oled_refresh();

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
