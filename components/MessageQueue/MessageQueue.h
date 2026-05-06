#ifndef MESSAGE_QUEUE_H
#define MESSAGE_QUEUE_H

#include <stdint.h>
#include <stdbool.h>
#include <esp_err.h>
#include <esp_log.h>

// 警报类型结构体
typedef struct {
    bool fall_detected;      // 跌倒检测
    bool convulsion_detected; // 抽搐检测
    bool heart_rate_warning;  // 心率预警
} alert_message_t;

// 心率数据结构体
typedef struct {
    uint32_t heart_rate;       // 心率值
    uint32_t spo2;             // 血氧值
    uint32_t baseline;         // 基准心率
    bool warning_active;       // 是否有预警
} heart_rate_data_t;

// 初始化消息队列
esp_err_t Message_Queue_Init(void);

// 发送警报消息
void Message_Queue_Send_Alert(bool fall, bool convulsion, bool heart_rate);

// 发送心率数据消息
void Message_Queue_Send_Heart_Rate(uint32_t heart_rate, uint32_t spo2, uint32_t baseline, bool warning);

// 接收警报消息
bool Message_Queue_Receive_Alert(alert_message_t *msg, uint32_t timeout_ms);

// 检查是否有等待的消息
bool Message_Queue_Has_Messages(void);

// 消息处理任务（消费者任务）
void Message_Queue_Process_Task(void *pvParameters);

#endif