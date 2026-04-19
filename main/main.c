#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"

#include "GetBatteryLevel.h"

// ========== app_main 入口 ==========
void app_main(void)
{
    // 1. 初始化
    ESP_ERROR_CHECK(battery_adc_init());

    // 2. 循环读取
    while (1)
    {
        uint32_t adc_raw;
        esp_err_t ret = battery_adc_read(&adc_raw);
        float average_voltage = 0;
        int times = 10; // 读取次数
        while (times--)
        {

            if (ret == ESP_OK)
            {
                average_voltage += (float)adc_raw * 3.3f / 4095.0f;
            }
        }
        average_voltage /= 10.0f;

        if (ret == ESP_OK)
        {

            ESP_LOGI("BATTERY", "ADC: %lu → 电压: %.2f V", adc_raw, average_voltage / VOLTAGE_DIVIDER_RATIO); // 注意：如果有分压电路，需除以分压比
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS); // 1秒一次
    }
}