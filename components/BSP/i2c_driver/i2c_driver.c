/**
 * @file i2c_driver.c
 * @brief ESP32-S3 I2C 通信驱动实现
 *
 * 这个文件实现了 I2C 主机和从机模式的驱动函数。
 * I2C 是一种两线串行通信协议，用于连接微控制器和外围设备。
 * 作为初学者，你需要了解：
 * - SDA（数据线）和 SCL（时钟线）是 I2C 的两条信号线。
 * - 主机控制通信，从机响应主机。
 * - 通信基于地址和数据字节。
 *
 * 使用前，确保硬件连接正确：SDA 和 SCL 引脚连接到设备，并添加上拉电阻（通常 4.7kΩ）。
 */

#include "i2c_driver.h"
#include <string.h>
#include "esp_log.h"

static const char *TAG = "BSP_I2C";  /**< 日志标签，用于调试输出 */

/**
 * @brief 将 BSP 主机总线配置转换为 ESP-IDF 标准配置
 *
 * 这个辅助函数将我们定义的配置结构体转换为 ESP-IDF 内部使用的结构体。
 * 这样可以保持 API 的简洁性，同时兼容底层实现。
 *
 * @param src BSP 配置结构体
 * @param dst ESP-IDF 配置结构体
 */
static void fill_master_bus_cfg(const i2c_bsp_master_config_t *src, i2c_master_bus_config_t *dst)
{
    memset(dst, 0, sizeof(*dst));  // 清空结构体，确保没有未初始化的值
    dst->i2c_port = src->i2c_port;
    dst->sda_io_num = src->sda_io_num;
    dst->scl_io_num = src->scl_io_num;
    dst->clk_source = src->clk_source;
    dst->glitch_ignore_cnt = src->glitch_ignore_cnt;
    dst->intr_priority = src->intr_priority;
    dst->trans_queue_depth = src->trans_queue_depth;
    dst->flags.enable_internal_pullup = src->enable_internal_pullup ? 1 : 0;  // 布尔值转换为整数
    dst->flags.allow_pd = src->allow_pd ? 1 : 0;
}

/**
 * @brief 将 BSP 设备配置转换为 ESP-IDF 标准配置
 *
 * @param src BSP 配置
 * @param dst ESP-IDF 配置
 */
static void fill_master_dev_cfg(const i2c_bsp_device_config_t *src, i2c_device_config_t *dst)
{
    memset(dst, 0, sizeof(*dst));
    dst->dev_addr_length = src->dev_addr_length;
    dst->device_address = src->device_address;
    dst->scl_speed_hz = src->scl_speed_hz;
    dst->scl_wait_us = src->scl_wait_us;
}

/**
 * @brief 将 BSP 从机配置转换为 ESP-IDF 标准配置
 *
 * @param src BSP 配置
 * @param dst ESP-IDF 配置
 */
static void fill_slave_cfg(const i2c_bsp_slave_config_t *src, i2c_slave_config_t *dst)
{
    memset(dst, 0, sizeof(*dst));
    dst->i2c_port = src->i2c_port;
    dst->sda_io_num = src->sda_io_num;
    dst->scl_io_num = src->scl_io_num;
    dst->clk_source = src->clk_source;
    dst->send_buf_depth = src->send_buf_depth;
    dst->slave_addr = src->slave_addr;
    dst->addr_bit_len = src->addr_bit_len;
    dst->intr_priority = src->intr_priority;
}

esp_err_t i2c_bsp_master_init(const i2c_bsp_master_config_t *config, i2c_master_bus_handle_t *bus_handle)
{
    if (config == NULL || bus_handle == NULL) {
        return ESP_ERR_INVALID_ARG;  // 参数检查：确保指针不为空
    }

    i2c_master_bus_config_t bus_config;
    fill_master_bus_cfg(config, &bus_config);  // 转换配置格式

    esp_err_t err = i2c_new_master_bus(&bus_config, bus_handle);  // 调用 ESP-IDF API 初始化总线
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize I2C master bus, err=%s", esp_err_to_name(err));  // 记录错误日志
    }
    return err;
}

esp_err_t i2c_bsp_master_add_device(i2c_master_bus_handle_t bus_handle, const i2c_bsp_device_config_t *device_config, i2c_master_dev_handle_t *dev_handle)
{
    if (bus_handle == NULL || device_config == NULL || dev_handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    i2c_device_config_t cfg;
    fill_master_dev_cfg(device_config, &cfg);  // 转换设备配置

    esp_err_t err = i2c_master_bus_add_device(bus_handle, &cfg, dev_handle);  // 添加设备到总线
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add I2C device, err=%s", esp_err_to_name(err));
    }
    return err;
}

esp_err_t i2c_bsp_master_remove_device(i2c_master_dev_handle_t dev_handle)
{
    if (dev_handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    return i2c_master_bus_rm_device(dev_handle);  // 移除设备
}

esp_err_t i2c_bsp_master_deinit(i2c_master_bus_handle_t bus_handle)
{
    if (bus_handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    return i2c_del_master_bus(bus_handle);  // 释放总线资源
}

esp_err_t i2c_bsp_master_write(i2c_master_dev_handle_t dev_handle, const uint8_t *data, size_t length, int timeout_ms)
{
    if (dev_handle == NULL || data == NULL || length == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    return i2c_master_transmit(dev_handle, data, length, timeout_ms);  // 发送数据
}

esp_err_t i2c_bsp_master_read(i2c_master_dev_handle_t dev_handle, uint8_t *data, size_t length, int timeout_ms)
{
    if (dev_handle == NULL || data == NULL || length == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    return i2c_master_receive(dev_handle, data, length, timeout_ms);  // 接收数据
}

esp_err_t i2c_bsp_master_write_read(i2c_master_dev_handle_t dev_handle, const uint8_t *write_data, size_t write_length, uint8_t *read_data, size_t read_length, int timeout_ms)
{
    if (dev_handle == NULL || write_data == NULL || write_length == 0 || read_data == NULL || read_length == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    return i2c_master_transmit_receive(dev_handle, write_data, write_length, read_data, read_length, timeout_ms);  // 先写后读
}

esp_err_t i2c_bsp_master_probe(i2c_master_bus_handle_t bus_handle, uint16_t address, int timeout_ms)
{
    if (bus_handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    return i2c_master_probe(bus_handle, address, timeout_ms);  // 探测设备是否存在
}

esp_err_t i2c_bsp_master_reset_bus(i2c_master_bus_handle_t bus_handle)
{
    if (bus_handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    return i2c_master_bus_reset(bus_handle);  // 重置总线，解决通信问题
}

esp_err_t i2c_bsp_master_get_handle(i2c_port_num_t port_num, i2c_master_bus_handle_t *bus_handle)
{
    if (bus_handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    return i2c_master_get_bus_handle(port_num, bus_handle);  // 获取已初始化的总线句柄
}

esp_err_t i2c_bsp_slave_init(const i2c_bsp_slave_config_t *config, i2c_slave_dev_handle_t *slave_handle)
{
    if (config == NULL || slave_handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    i2c_slave_config_t slave_cfg;
    fill_slave_cfg(config, &slave_cfg);  // 转换从机配置

    esp_err_t err = i2c_new_slave_device(&slave_cfg, slave_handle);  // 初始化从机
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize I2C slave device, err=%s", esp_err_to_name(err));
    }
    return err;
}

esp_err_t i2c_bsp_slave_register_callbacks(i2c_slave_dev_handle_t slave_handle, const i2c_slave_event_callbacks_t *callbacks, void *user_data)
{
    if (slave_handle == NULL || callbacks == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    return i2c_slave_register_event_callbacks(slave_handle, callbacks, user_data);  // 注册事件回调，如数据请求
}

esp_err_t i2c_bsp_slave_deinit(i2c_slave_dev_handle_t slave_handle)
{
    if (slave_handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    return i2c_del_slave_device(slave_handle);  // 释放从机资源
}
