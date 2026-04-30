#pragma once

#include <stdbool.h>
#include "esp_err.h"
#include "driver/i2c_master.h"
#include "driver/i2c_slave.h"
#include "driver/i2c_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief I2C 主机总线配置结构体
 *
 * 这个结构体定义了初始化 I2C 主机总线时需要的配置参数。
 * I2C（Inter-Integrated Circuit）是一种串行通信协议，用于连接微控制器和外围设备。
 * ESP32-S3 支持多个 I2C 端口，每个端口可以配置不同的参数。
 */
typedef struct {
    i2c_port_num_t i2c_port;              /**< I2C 端口号，选择使用哪个 I2C 控制器（例如 I2C_NUM_0 或 I2C_NUM_1） */
    gpio_num_t sda_io_num;                /**< SDA 引脚号，串行数据线，用于数据传输 */
    gpio_num_t scl_io_num;                /**< SCL 引脚号，串行时钟线，用于同步时钟 */
    i2c_clock_source_t clk_source;        /**< 时钟源选择，例如使用默认时钟或 APB 时钟 */
    uint8_t glitch_ignore_cnt;            /**< 毛刺忽略计数，用于滤除线上的短暂干扰 */
    int intr_priority;                    /**< 中断优先级，影响 I2C 事件的处理速度 */
    size_t trans_queue_depth;             /**< 传输队列深度，用于异步传输时的缓冲 */
    bool enable_internal_pullup;          /**< 是否启用内部上拉电阻，减少外部元件 */
    bool allow_pd;                        /**< 是否允许在睡眠模式下关闭电源以节省功耗 */
} i2c_bsp_master_config_t;

/**
 * @brief I2C 设备配置结构体
 *
 * 这个结构体定义了连接到 I2C 总线的从设备参数。
 * 每个 I2C 从设备有唯一的地址，用于主机识别和通信。
 */
typedef struct {
    i2c_addr_bit_len_t dev_addr_length;   /**< 设备地址位长度，7 位或 10 位 */
    uint16_t device_address;              /**< 设备地址，主机用来寻址从设备 */
    uint32_t scl_speed_hz;                /**< SCL 时钟频率，单位 Hz（例如 100000 表示 100 kHz） */
    uint32_t scl_wait_us;                 /**< SCL 等待时间，单位微秒，用于处理从设备的响应延迟 */
} i2c_bsp_device_config_t;

/**
 * @brief I2C 从机设备配置结构体
 *
 * 这个结构体定义了 ESP32-S3 作为 I2C 从设备时的配置。
 * 从机模式下，ESP32 响应主机的请求，提供数据或接收数据。
 */
typedef struct {
    i2c_port_num_t i2c_port;              /**< I2C 端口号 */
    gpio_num_t sda_io_num;                /**< SDA 引脚号 */
    gpio_num_t scl_io_num;                /**< SCL 引脚号 */
    i2c_clock_source_t clk_source;        /**< 时钟源 */
    uint32_t send_buf_depth;              /**< 发送缓冲区深度，用于存储待发送的数据 */
    uint16_t slave_addr;                  /**< 从机地址，主机用来寻址这个从设备 */
    i2c_addr_bit_len_t addr_bit_len;      /**< 地址位长度 */
    int intr_priority;                    /**< 中断优先级 */
} i2c_bsp_slave_config_t;

/**
 * @brief 初始化 I2C 主机总线
 *
 * 这个函数根据配置创建并初始化 I2C 主机总线。
 * 主机总线是 I2C 通信的基础，所有设备都挂载在总线上。
 *
 * @param config 指向主机总线配置的指针
 * @param bus_handle 返回的总线句柄，用于后续操作
 * @return ESP_OK 表示成功，其他值表示错误
 */
esp_err_t i2c_bsp_master_init(const i2c_bsp_master_config_t *config, i2c_master_bus_handle_t *bus_handle);

/**
 * @brief 添加 I2C 主机设备到总线
 *
 * 在总线上添加一个从设备，使主机可以与它通信。
 *
 * @param bus_handle 总线句柄
 * @param device_config 设备配置
 * @param dev_handle 返回的设备句柄
 * @return ESP_OK 表示成功
 */
esp_err_t i2c_bsp_master_add_device(i2c_master_bus_handle_t bus_handle, const i2c_bsp_device_config_t *device_config, i2c_master_dev_handle_t *dev_handle);

/**
 * @brief 从总线移除设备
 *
 * @param dev_handle 设备句柄
 * @return ESP_OK 表示成功
 */
esp_err_t i2c_bsp_master_remove_device(i2c_master_dev_handle_t dev_handle);

/**
 * @brief 释放 I2C 主机总线
 *
 * @param bus_handle 总线句柄
 * @return ESP_OK 表示成功
 */
esp_err_t i2c_bsp_master_deinit(i2c_master_bus_handle_t bus_handle);

/**
 * @brief 主机写入数据到从设备
 *
 * 发送数据到指定的从设备。
 *
 * @param dev_handle 设备句柄
 * @param data 要发送的数据缓冲区
 * @param length 数据长度
 * @param timeout_ms 超时时间（毫秒）
 * @return ESP_OK 表示成功
 */
esp_err_t i2c_bsp_master_write(i2c_master_dev_handle_t dev_handle, const uint8_t *data, size_t length, int timeout_ms);

/**
 * @brief 主机从从设备读取数据
 *
 * 从指定的从设备接收数据。
 *
 * @param dev_handle 设备句柄
 * @param data 接收数据的缓冲区
 * @param length 要读取的数据长度
 * @param timeout_ms 超时时间
 * @return ESP_OK 表示成功
 */
esp_err_t i2c_bsp_master_read(i2c_master_dev_handle_t dev_handle, uint8_t *data, size_t length, int timeout_ms);

/**
 * @brief 主机先写入后读取（复合操作）
 *
 * 常用于读取传感器数据：先发送命令，再读取响应。
 *
 * @param dev_handle 设备句柄
 * @param write_data 要写入的数据
 * @param write_length 写入长度
 * @param read_data 读取缓冲区
 * @param read_length 读取长度
 * @param timeout_ms 超时时间
 * @return ESP_OK 表示成功
 */
esp_err_t i2c_bsp_master_write_read(i2c_master_dev_handle_t dev_handle, const uint8_t *write_data, size_t write_length, uint8_t *read_data, size_t read_length, int timeout_ms);

/**
 * @brief 探测 I2C 设备是否存在
 *
 * 发送地址查询，检查设备是否响应。
 *
 * @param bus_handle 总线句柄
 * @param address 设备地址
 * @param timeout_ms 超时时间
 * @return ESP_OK 表示设备存在
 */
esp_err_t i2c_bsp_master_probe(i2c_master_bus_handle_t bus_handle, uint16_t address, int timeout_ms);

/**
 * @brief 重置 I2C 总线
 *
 * 用于解决通信故障。
 *
 * @param bus_handle 总线句柄
 * @return ESP_OK 表示成功
 */
esp_err_t i2c_bsp_master_reset_bus(i2c_master_bus_handle_t bus_handle);

/**
 * @brief 获取已初始化的总线句柄
 *
 * 如果总线已在其他地方初始化，可以通过端口号获取句柄。
 *
 * @param port_num 端口号
 * @param bus_handle 返回的句柄
 * @return ESP_OK 表示成功
 */
esp_err_t i2c_bsp_master_get_handle(i2c_port_num_t port_num, i2c_master_bus_handle_t *bus_handle);

/**
 * @brief 初始化 I2C 从机设备
 *
 * 配置 ESP32-S3 作为 I2C 从设备。
 *
 * @param config 从机配置
 * @param slave_handle 返回的从机句柄
 * @return ESP_OK 表示成功
 */
esp_err_t i2c_bsp_slave_init(const i2c_bsp_slave_config_t *config, i2c_slave_dev_handle_t *slave_handle);

/**
 * @brief 注册从机事件回调函数
 *
 * 当主机请求数据或发送数据时，会触发回调。
 *
 * @param slave_handle 从机句柄
 * @param callbacks 回调函数结构体
 * @param user_data 用户数据指针
 * @return ESP_OK 表示成功
 */
esp_err_t i2c_bsp_slave_register_callbacks(i2c_slave_dev_handle_t slave_handle, const i2c_slave_event_callbacks_t *callbacks, void *user_data);

/**
 * @brief 释放 I2C 从机设备
 *
 * @param slave_handle 从机句柄
 * @return ESP_OK 表示成功
 */
esp_err_t i2c_bsp_slave_deinit(i2c_slave_dev_handle_t slave_handle);

#ifdef __cplusplus
}
#endif
