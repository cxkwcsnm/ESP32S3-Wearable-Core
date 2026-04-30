#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"  // ← 新版 ADC 头文件

#include "OLED_Data.h"


#define VOLTAGE_DIVIDER_RATIO    0.5f             // 电压分压比 = r2/(r1 + r2)


// ========== 全局变量 ==========
static const char *TAG = "BATTERY";
static adc_oneshot_unit_handle_t adc1_handle = NULL;  // ADC 句柄
static bool is_initialized = false;

esp_err_t battery_adc_init(void);

esp_err_t battery_adc_read(uint32_t *out_value);

void battery_adc_deinit(void);

/**
 * @brief 获取电池平均电压
 *
 * 读取多次 ADC 值，计算平均电压，并考虑分压比。
 *
 * @param samples 采样次数，默认 10 次
 * @param out_voltage 输出平均电压（伏特）
 * @return ESP_OK 表示成功
 */
esp_err_t battery_get_average_voltage(int samples, float *out_voltage);

const uint8_t * battery_pattern(float voltage);  // 根据电压返回对应的电池图标字符串
