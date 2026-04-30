#include <stdio.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"

#include "GetBatteryLevel.h"
#include "OLED_driver.h"
#include "OLED_Data.h"
void app_main(void)
{
    // 1. 初始化
    ESP_ERROR_CHECK(battery_adc_init());
    ESP_ERROR_CHECK(oled_init(I2C_NUM_0, GPIO_NUM_20, GPIO_NUM_21));

    // 2. 循环读取
    while (1)
    {
        float voltage = 0.0f;
        battery_get_average_voltage(5, &voltage); // 采样 5 次，获取平均电压
        ESP_LOGI(TAG, "平均电压: %.2f V", voltage);

        char voltage_str[20];
        snprintf(voltage_str, sizeof(voltage_str), "%.2f V", voltage);

        oled_clear();
        oled_show_string(0, 0, "Voltage:");
        oled_show_string(48, 0, voltage_str);

        // 使用 oled_draw_image 显示电池图标
        oled_draw_image(100, 0, battery_pattern(voltage), 19, 8);

        oled_refresh();

        vTaskDelay(1000 / portTICK_PERIOD_MS); // 延时 1 秒
    }
}
