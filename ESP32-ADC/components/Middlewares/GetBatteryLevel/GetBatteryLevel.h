#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"  // ← 新版 ADC 头文件

#define VOLTAGE_DIVIDER_RATIO    0.5f             // 电压分压比 = r2/(r1 + r2)

esp_err_t battery_adc_init(void);

esp_err_t battery_adc_read(uint32_t *out_value);

void battery_adc_deinit(void);

