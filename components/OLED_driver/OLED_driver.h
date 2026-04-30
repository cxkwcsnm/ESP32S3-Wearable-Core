#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "driver/i2c.h"
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define OLED_I2C_ADDRESS 0x3C
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define OLED_PAGES (OLED_HEIGHT / 8)
#define OLED_FONT_WIDTH_6 6
#define OLED_FONT_HEIGHT_8 8

    /**
     * @brief 初始化 SSD1306 OLED 屏幕
     *
     * 这个函数初始化 I2C 总线，并且设置 OLED 的基础显示参数。
     * 适用于地址为 0x3C、分辨率 128x64 的 OLED 屏幕。
     *
     * @param port I2C 端口号，例如 I2C_NUM_0
     * @param sda_io SDA 引脚
     * @param scl_io SCL 引脚
     * @return ESP_OK 初始化成功
     */
    esp_err_t oled_init(i2c_port_t port, gpio_num_t sda_io, gpio_num_t scl_io);

    /**
     * @brief 反初始化 OLED 驱动
     *
     * 释放 I2C 驱动资源，停止 OLED 显示刷新。
     */
    esp_err_t oled_deinit(void);

    /**
     * @brief 清空 OLED 屏幕缓存，并立即刷新显示
     */
    esp_err_t oled_clear(void);

    /**
     * @brief 将缓存内容写入 OLED 屏幕
     */
    esp_err_t oled_refresh(void);

    /**
     * @brief 向 OLED 屏幕写入一个命令
     *
     * 该函数用于发送 SSD1306 命令，例如设置显示模式、行地址、列地址等。
     */
    esp_err_t oled_send_command(uint8_t command);

    /**
     * @brief 向 OLED 屏幕写入数据
     *
     * 用于发送显示数据，通常是图像缓存或文字数据。
     */
    esp_err_t oled_send_data(const uint8_t *data, size_t length);

    /**
     * @brief 打开或关闭 OLED 显示
     *
     * @param on true 时打开显示，false 时关闭显示
     */
    esp_err_t oled_display_power(bool on);

    /**
     * @brief 设置显示对比度
     *
     * @param contrast 对比度值，0~255
     */
    esp_err_t oled_set_contrast(uint8_t contrast);

    /**
     * @brief 在缓存中设置一个点
     *
     * @param x 水平坐标，范围 0~127
     * @param y 垂直坐标，范围 0~63
     * @param color true 表示点亮，false 表示熄灭
     */
    esp_err_t oled_draw_pixel(uint8_t x, uint8_t y, bool color);

    /**
     * @brief 在屏幕上显示 ASCII 字符串
     *
     * @param col 字符起始列，单位为像素，范围 0~122
     * @param row 字符行号，单位为 8 像素行，范围 0~7
     * @param str ASCII 字符串
     */
    esp_err_t oled_show_string(uint8_t col, uint8_t row, const char *str);

    /**
     * @brief 在缓存中显示一个 ASCII 字符
     *
     * @param col 字符起始列，单位为像素
     * @param row 字符行号，单位为 8 像素行
     * @param chr ASCII 字符
     */
    esp_err_t oled_show_char(uint8_t col, uint8_t row, char chr);

    /**
     * @brief 在屏幕上显示图像字模
     *
     * @param col 图像左上角起始列，单位为像素
     * @param row 图像左上角起始行，单位为 8 像素行
     * @param image 图像字模数据指针
     * @param width 图像宽度，单位为像素
     * @param height 图像高度，单位为像素
     */
    esp_err_t oled_draw_image(uint8_t col, uint8_t row, const uint8_t *image, uint8_t width, uint8_t height);

#ifdef __cplusplus
}
#endif
