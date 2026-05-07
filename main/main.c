#include <stdio.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"

#include "GetBatteryLevel.h"
#include "OLED_driver.h"
#include "OLED_Data.h"
#include "RTC_time.h"
#include "myiic.h"

#include "system_oled_show.h"
#include "WIFI_manager.h"
#include "MessageQueue.h"

#define WIFI_SSID "DESKTOP-HTLNPUV 4127"
#define WIFI_PASSWORD "88888888"

wifi_connect_params_t wifi_params = {
    .ssid = WIFI_SSID,
    .password = WIFI_PASSWORD
};

void app_main(void)
{
    esp_log_level_set("wifi", ESP_LOG_WARN);

    esp_err_t ret;
    
    ret = nvs_flash_init();     /* 初始化NVS */
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }
    
    ESP_ERROR_CHECK(myiic_init());
    ESP_ERROR_CHECK(battery_adc_init());
    ESP_ERROR_CHECK(oled_init());
    ESP_ERROR_CHECK(wifi_scan());
    //ESP_ERROR_CHECK(RTC_init());
    ESP_ERROR_CHECK(RTC_init_with_sntp("CST-8"));
    ESP_ERROR_CHECK(Max30102_Init());
    ESP_ERROR_CHECK(Message_Queue_Init());

    xTaskCreate(Max30102_Task, "Max30102_Task", 4096, NULL, 5, NULL);
    xTaskCreate(Task_Mpu6050_Monitor, "Mpu6050_Task", 4096, NULL, 5, NULL);
    xTaskCreate(wifi_connect_task, "wifi_connect_task", 4096, &wifi_params, 5, NULL);
    xTaskCreate(OLEDShowTask, "OLEDShowTask", 4096, NULL, 5, NULL);
    xTaskCreate(Message_Queue_Process_Task, "MessageQueueTask", 4096, NULL, 5, NULL);
}
