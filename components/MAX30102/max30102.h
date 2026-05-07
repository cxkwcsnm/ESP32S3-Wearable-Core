#ifndef MAX30102_H
#define MAX30102_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "myiic.h"
#include "MessageQueue.h"

#define MAX30102_INT_GPIO 6
#define MAX30102_ADDR 0x57

#define REG_INTR_STATUS_1 0x00
#define REG_INTR_STATUS_2 0x01
#define REG_INTR_ENABLE_1 0x02
#define REG_INTR_ENABLE_2 0x03
#define REG_FIFO_WR_PTR 0x04
#define REG_OVF_COUNTER 0x05
#define REG_FIFO_RD_PTR 0x06
#define REG_FIFO_DATA 0x07
#define REG_FIFO_CONFIG 0x08
#define REG_MODE_CONFIG 0x09
#define REG_SPO2_CONFIG 0x0A
#define REG_LED1_PA 0x0C
#define REG_LED2_PA 0x0D
#define REG_PILOT_PA 0x10
#define REG_PART_ID 0xFF

#define MAX30102_BUFFER_SIZE 500
#define IR_BUF_LEN 500
#define RED_BUF_LEN 500

#define HEART_RATE_MIN_VALID 40
#define HEART_RATE_MAX_VALID 180

esp_err_t Max30102_Init(void);
esp_err_t Max30102_Write_Reg(uint8_t reg, uint8_t data);
esp_err_t Max30102_Read_Reg(uint8_t reg, uint8_t *data);
esp_err_t Max30102_Read_Fifo(uint8_t *buffer, uint8_t count);

uint32_t Max30102_Get_Heart_Rate(void);
uint32_t Max30102_Get_Spo2(void);
bool Max30102_Is_Measuring(void);
uint32_t Max30102_Get_Elapsed_Seconds(void);
bool Max30102_Has_Valid_Data(void);

void Max30102_Algorithm_Calculate(uint32_t *ir_buffer, int32_t buffer_len, uint32_t *red_buffer,
                                  int32_t *spo2, int8_t *spo2_valid,
                                  int32_t *heart_rate, int8_t *hr_valid);

void Max30102_Task(void *pvParameters);

#endif // MAX30102_H