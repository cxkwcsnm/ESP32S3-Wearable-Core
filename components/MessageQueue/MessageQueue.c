#include "MessageQueue.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_log.h"

static const char *TAG = "MessageQueue";
static QueueHandle_t alert_queue = NULL;
static QueueHandle_t heart_rate_queue = NULL;

esp_err_t Message_Queue_Init(void)
{
    if (alert_queue != NULL && heart_rate_queue != NULL)
    {
        ESP_LOGW(TAG, "消息队列已初始化");
        return ESP_OK;
    }

    alert_queue = xQueueCreate(10, sizeof(alert_message_t));
    heart_rate_queue = xQueueCreate(10, sizeof(heart_rate_data_t));

    if (alert_queue == NULL || heart_rate_queue == NULL)
    {
        ESP_LOGE(TAG, "消息队列创建失败");
        return ESP_FAIL;
    }
    else
    {
        ESP_LOGI(TAG, "消息队列初始化成功");
    }
    return ESP_OK;
}

void Message_Queue_Send_Alert(bool fall, bool convulsion, bool heart_rate)
{
    if (alert_queue == NULL)
    {
        ESP_LOGE(TAG, "消息队列未初始化");
        return;
    }

    alert_message_t msg = {
        .fall_detected = fall,
        .convulsion_detected = convulsion,
        .heart_rate_warning = heart_rate};

    BaseType_t ret = xQueueSend(alert_queue, &msg, pdMS_TO_TICKS(100));
    if (ret != pdPASS)
    {
        ESP_LOGW(TAG, "消息发送失败，队列已满");
    }
    else
    {
        ESP_LOGI(TAG, "警报消息发送成功: fall=%d, convulsion=%d, heart_rate=%d",
                 fall, convulsion, heart_rate);
    }
}

bool Message_Queue_Receive_Alert(alert_message_t *msg, uint32_t timeout_ms)
{
    if (alert_queue == NULL || msg == NULL)
    {
        return false;
    }

    BaseType_t ret = xQueueReceive(alert_queue, msg, pdMS_TO_TICKS(timeout_ms));
    return (ret == pdPASS);
}

void Message_Queue_Send_Heart_Rate(uint32_t heart_rate, uint32_t spo2, uint32_t baseline, bool warning)
{
    if (heart_rate_queue == NULL)
    {
        ESP_LOGE(TAG, "消息队列未初始化");
        return;
    }

    heart_rate_data_t msg = {
        .heart_rate = heart_rate,
        .spo2 = spo2,
        .baseline = baseline,
        .warning_active = warning};

    BaseType_t ret = xQueueSend(heart_rate_queue, &msg, pdMS_TO_TICKS(100));
    if (ret != pdPASS)
    {
        ESP_LOGW(TAG, "心率消息发送失败，队列已满");
    }
    else
    {
        ESP_LOGI(TAG, "心率消息发送成功: hr=%lu, spo2=%lu, baseline=%lu, warning=%d",
                 (unsigned long)heart_rate, (unsigned long)spo2, (unsigned long)baseline, warning);
    }
}

bool Message_Queue_Receive_Heart_Rate(heart_rate_data_t *msg, uint32_t timeout_ms)
{
    if (heart_rate_queue == NULL || msg == NULL)
    {
        return false;
    }

    BaseType_t ret = xQueueReceive(heart_rate_queue, msg, pdMS_TO_TICKS(timeout_ms));
    return (ret == pdPASS);
}

bool Message_Queue_Has_Messages(void)
{
    if (alert_queue == NULL && heart_rate_queue == NULL)
    {
        return false;
    }

    UBaseType_t alert_count = (alert_queue != NULL) ? uxQueueMessagesWaiting(alert_queue) : 0;
    UBaseType_t hr_count = (heart_rate_queue != NULL) ? uxQueueMessagesWaiting(heart_rate_queue) : 0;

    return (alert_count > 0 || hr_count > 0);
}

// --- 消息处理任务 ---
void Message_Queue_Process_Task(void *pvParameters)
{
    ESP_LOGI(TAG, "消息处理任务启动");

    alert_message_t alert_msg;
    heart_rate_data_t hr_msg;
    TickType_t last_wake_time = xTaskGetTickCount();

    while (1)
    {
        // 处理警报消息
        while (Message_Queue_Receive_Alert(&alert_msg, 0))
        {
            ESP_LOGI(TAG, "收到警报消息: 跌倒=%d, 抽搐=%d, 心率异常=%d",
                     alert_msg.fall_detected,
                     alert_msg.convulsion_detected,
                     alert_msg.heart_rate_warning);

            // 在这里添加警报处理逻辑
            if (alert_msg.fall_detected)
            {
                ESP_LOGW(TAG, "⚠️ 检测到跌倒事件!");
            }
            if (alert_msg.convulsion_detected)
            {
                ESP_LOGW(TAG, "⚠️ 检测到抽搐事件!");
            }
            if (alert_msg.heart_rate_warning)
            {
                ESP_LOGW(TAG, "⚠️ 心率异常警告!");
            }
        }

        // 处理心率消息
        while (Message_Queue_Receive_Heart_Rate(&hr_msg, 0))
        {
            ESP_LOGI(TAG, "收到心率消息: HR=%lu bpm, SpO2=%lu%%, 基准=%lu bpm, 警告=%d",
                     (unsigned long)hr_msg.heart_rate,
                     (unsigned long)hr_msg.spo2,
                     (unsigned long)hr_msg.baseline,
                     hr_msg.warning_active);
        }

        // 每100ms检查一次队列
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(100));
    }
}