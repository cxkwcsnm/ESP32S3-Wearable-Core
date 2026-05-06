
#ifndef MYIIC_H
#define MYIIC_H

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_err.h"
#include "esp_log.h"
#include <stdint.h>
#include <stddef.h>

/* 引脚与相关参数定义 */
#define IIC_NUM_PORT I2C_NUM_0       /* IIC0 */
#define IIC_SPEED_CLK 400000         /* 速率400K（降低速率提高稳定性） */
#define IIC_SDA_GPIO_PIN GPIO_NUM_20 /* IIC0_SDA引脚 */
#define IIC_SCL_GPIO_PIN GPIO_NUM_21 /* IIC0_SCL引脚 */

/* 设备地址定义 */
#define MAX30102_ADDR 0x57           /* MAX30102 心率传感器 */
#define MPU6050_ADDR  0x68           /* MPU6050 六轴传感器 */
#define OLED_ADDR     0x3C           /* OLED 显示屏 */

extern i2c_master_bus_handle_t bus_handle; /* 总线句柄 */

/* 函数声明 */
esp_err_t myiic_init(void);                                                                                                                       /* 初始化MYIIC */
esp_err_t myiic_deinit(void);                                                                                                                     /* 反初始化MYIIC */
esp_err_t myiic_add_device(uint16_t dev_addr, i2c_master_dev_handle_t *dev_handle);                                                               /* 添加I2C设备 */
esp_err_t myiic_remove_device(i2c_master_dev_handle_t dev_handle);                                                                                /* 移除I2C设备 */
esp_err_t myiic_write(i2c_master_dev_handle_t dev_handle, uint16_t dev_addr, const uint8_t *data, size_t len);                                   /* 写入数据 */
esp_err_t myiic_read(i2c_master_dev_handle_t dev_handle, uint16_t dev_addr, uint8_t *data, size_t len);                                          /* 读取数据 */
esp_err_t myiic_write_read(i2c_master_dev_handle_t dev_handle, uint16_t dev_addr, const uint8_t *write_data, size_t write_len, uint8_t *read_data, size_t read_len); /* 写后读 */

#endif
