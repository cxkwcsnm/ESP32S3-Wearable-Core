#include "myiic.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

static const char *TAG = "MYIIC";
i2c_master_bus_handle_t bus_handle;        /* 总线句柄 */
static bool bus_initialized = false;       /* 总线初始化状态 */
static SemaphoreHandle_t i2c_mutex = NULL; /* I2C总线互斥锁 */

/* 设备地址到名称的映射 */
static const char *get_device_name(uint16_t dev_addr)
{
    switch (dev_addr)
    {
    case MAX30102_ADDR:
        return "MAX30102 (心率传感器)";
    case MPU6050_ADDR:
        return "MPU6050 (六轴传感器)";
    case OLED_ADDR:
        return "OLED (显示屏)";
    default:
        return "未知设备";
    }
}

/**
 * @brief       初始化MYIIC
 * @param       无
 * @retval      ESP_OK:初始化成功
 */
esp_err_t myiic_init(void)
{
    if (bus_initialized)
    {
        ESP_LOGW(TAG, "I2C总线 已初始化");
        return ESP_OK;
    }

    i2c_master_bus_config_t i2c_bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,    /* 时钟源 */
        .i2c_port = IIC_NUM_PORT,             /* I2C端口 */
        .scl_io_num = IIC_SCL_GPIO_PIN,       /* SCL管脚 */
        .sda_io_num = IIC_SDA_GPIO_PIN,       /* SDA管脚 */
        .glitch_ignore_cnt = 7,               /* 故障周期 */
        .flags.enable_internal_pullup = true, /* 内部上拉 */
    };
    /* 新建I2C总线 */
    esp_err_t err = i2c_new_master_bus(&i2c_bus_config, &bus_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "I2C总线 初始化失败: %s", esp_err_to_name(err));
        return err;
    }

    /* 创建I2C总线互斥锁 */
    i2c_mutex = xSemaphoreCreateMutex();
    if (i2c_mutex == NULL)
    {
        ESP_LOGE(TAG, "I2C总线 互斥锁创建失败");
        i2c_del_master_bus(bus_handle);
        return ESP_ERR_NO_MEM;
    }

    bus_initialized = true;
    ESP_LOGI(TAG, "I2C总线 初始化成功 (SDA: %d, SCL: %d)", IIC_SDA_GPIO_PIN, IIC_SCL_GPIO_PIN);
    return ESP_OK;
}

/**
 * @brief       反初始化MYIIC
 * @param       无
 * @retval      ESP_OK:反初始化成功
 */
esp_err_t myiic_deinit(void)
{
    if (!bus_initialized)
    {
        return ESP_OK;
    }

    esp_err_t err = i2c_del_master_bus(bus_handle);
    if (err == ESP_OK)
    {
        bus_initialized = false;
        ESP_LOGI(TAG, "I2C总线 反初始化成功");
    }
    return err;
}

/**
 * @brief       添加I2C设备到总线
 * @param       dev_addr: 设备地址
 * @param       dev_handle: 返回的设备句柄
 * @retval      ESP_OK:添加成功
 */
esp_err_t myiic_add_device(uint16_t dev_addr, i2c_master_dev_handle_t *dev_handle)
{
    if (!bus_initialized)
    {
        ESP_LOGE(TAG, "I2C总线 未初始化");
        return ESP_ERR_INVALID_STATE;
    }

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = dev_addr,
        .scl_speed_hz = IIC_SPEED_CLK,
    };

    esp_err_t err = i2c_master_bus_add_device(bus_handle, &dev_cfg, dev_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "I2C总线 添加设备 0x%02X 失败: %s", dev_addr, esp_err_to_name(err));
    }
    else
    {
        ESP_LOGI(TAG, "I2C总线 添加设备 0x%02X 成功", dev_addr);
    }
    return err;
}

/**
 * @brief       从总线移除I2C设备
 * @param       dev_handle: 设备句柄
 * @retval      ESP_OK:移除成功
 */
esp_err_t myiic_remove_device(i2c_master_dev_handle_t dev_handle)
{
    esp_err_t err = i2c_master_bus_rm_device(dev_handle);
    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "I2C总线 移除设备成功");
    }
    else
    {
        ESP_LOGE(TAG, "I2C总线 移除设备失败: %s", esp_err_to_name(err));
    }
    return err;
}

/**
 * @brief       向I2C设备写入数据
 * @param       dev_handle: 设备句柄
 * @param       data: 数据指针
 * @param       len: 数据长度
 * @retval      ESP_OK:写入成功
 */
esp_err_t myiic_write(i2c_master_dev_handle_t dev_handle, uint16_t dev_addr, const uint8_t *data, size_t len)
{
    const char *dev_name = get_device_name(dev_addr);

    // 获取I2C总线互斥锁
    if (xSemaphoreTake(i2c_mutex, pdMS_TO_TICKS(1000)) != pdTRUE)
    {
        ESP_LOGE(TAG, "I2C总线 [0x%02X %s] 获取互斥锁超时", dev_addr, dev_name);
        return ESP_ERR_TIMEOUT;
    }

    // 打印调试信息：写入的数据内容（前8字节）
    ESP_LOGD(TAG, "I2C 写入准备 [0x%02X %s]: 长度=%d, 数据=[", dev_addr, dev_name, len);
    for (size_t i = 0; i < len && i < 8; i++)
    {
        ESP_LOGD(TAG, "0x%02X ", data[i]);
    }
    if (len > 8)
        ESP_LOGD(TAG, "...");
    ESP_LOGD(TAG, "]");

    esp_err_t err = i2c_master_transmit(dev_handle, data, len, pdMS_TO_TICKS(1000));
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "I2C总线 [0x%02X %s] 写入 %d 字节数据失败: %s", dev_addr, dev_name, len, esp_err_to_name(err));
        ESP_LOGE(TAG, "可能原因: 1.设备未连接 2.地址错误 3.总线被占用 4.缺少上拉电阻");
    }

    // 释放I2C总线互斥锁
    xSemaphoreGive(i2c_mutex);
    return err;
}

/**
 * @brief       从I2C设备读取数据
 * @param       dev_handle: 设备句柄
 * @param       data: 数据指针
 * @param       len: 数据长度
 * @retval      ESP_OK:读取成功
 */
esp_err_t myiic_read(i2c_master_dev_handle_t dev_handle, uint16_t dev_addr, uint8_t *data, size_t len)
{
    const char *dev_name = get_device_name(dev_addr);

    // 获取I2C总线互斥锁
    if (xSemaphoreTake(i2c_mutex, pdMS_TO_TICKS(1000)) != pdTRUE)
    {
        ESP_LOGE(TAG, "I2C总线 [0x%02X %s] 获取互斥锁超时", dev_addr, dev_name);
        return ESP_ERR_TIMEOUT;
    }

    ESP_LOGD(TAG, "I2C 读取准备 [0x%02X %s]: 长度=%d", dev_addr, dev_name, len);

    esp_err_t err = i2c_master_receive(dev_handle, data, len, pdMS_TO_TICKS(1000));
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "I2C总线 [0x%02X %s] 读取 %d 字节数据失败: %s", dev_addr, dev_name, len, esp_err_to_name(err));
        ESP_LOGE(TAG, "可能原因: 1.设备未响应 2.地址错误 3.数据长度错误");
    }
    else
    {
        // 打印读取到的数据（前8字节）
        ESP_LOGD(TAG, "I2C 读取成功 [0x%02X %s]: 数据=[", dev_addr, dev_name);
        for (size_t i = 0; i < len && i < 8; i++)
        {
            ESP_LOGD(TAG, "0x%02X ", data[i]);
        }
        if (len > 8)
            ESP_LOGD(TAG, "...");
        ESP_LOGD(TAG, "]");
    }

    // 释放I2C总线互斥锁
    xSemaphoreGive(i2c_mutex);
    return err;
}

/**
 * @brief       向I2C设备写入数据后读取数据
 * @param       dev_handle: 设备句柄
 * @param       write_data: 写入数据指针
 * @param       write_len: 写入数据长度
 * @param       read_data: 读取数据指针
 * @param       read_len: 读取数据长度
 * @retval      ESP_OK:操作成功
 */
esp_err_t myiic_write_read(i2c_master_dev_handle_t dev_handle, uint16_t dev_addr, const uint8_t *write_data, size_t write_len, uint8_t *read_data, size_t read_len)
{
    const char *dev_name = get_device_name(dev_addr);

    // 获取I2C总线互斥锁
    if (xSemaphoreTake(i2c_mutex, pdMS_TO_TICKS(1000)) != pdTRUE)
    {
        ESP_LOGE(TAG, "I2C总线 [0x%02X %s] 获取互斥锁超时", dev_addr, dev_name);
        return ESP_ERR_TIMEOUT;
    }

    // 打印调试信息
    ESP_LOGD(TAG, "I2C 写后读准备 [0x%02X %s]: 写入长度=%d, 读取长度=%d", dev_addr, dev_name, write_len, read_len);
    ESP_LOGD(TAG, "写入数据=[");
    for (size_t i = 0; i < write_len && i < 8; i++)
    {
        ESP_LOGD(TAG, "0x%02X ", write_data[i]);
    }
    if (write_len > 8)
        ESP_LOGD(TAG, "...");
    ESP_LOGD(TAG, "]");

    esp_err_t err = i2c_master_transmit_receive(dev_handle, write_data, write_len, read_data, read_len, pdMS_TO_TICKS(1000));
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "I2C总线 [0x%02X %s] 写入读取失败: %s", dev_addr, dev_name, esp_err_to_name(err));
        ESP_LOGE(TAG, "可能原因: 1.设备未连接 2.地址错误 3.寄存器不存在");
    }
    else
    {
        ESP_LOGD(TAG, "I2C 写后读成功 [0x%02X %s]: 读取数据=[", dev_addr, dev_name);
        for (size_t i = 0; i < read_len && i < 8; i++)
        {
            ESP_LOGD(TAG, "0x%02X ", read_data[i]);
        }
        if (read_len > 8)
            ESP_LOGD(TAG, "...");
        ESP_LOGD(TAG, "]");
    }

    // 释放I2C总线互斥锁
    xSemaphoreGive(i2c_mutex);
    return err;
}
