#include "OLED_driver.h"


static const char *TAG = "OLED_DRIVER";
static uint8_t s_oled_buffer[OLED_WIDTH * OLED_PAGES];
static bool s_oled_initialized = false;
static i2c_master_dev_handle_t s_oled_dev_handle = NULL;

static esp_err_t oled_i2c_write_raw(const uint8_t *data, size_t len, uint8_t control)
{
    if (!s_oled_initialized || data == NULL || len == 0)
    {
        return ESP_ERR_INVALID_STATE;
    }

    /* 控制字节 + 最多128字节数据 (SSD1306 单页最大) */
    if (len > OLED_WIDTH)
    {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t write_buf[OLED_WIDTH + 1];
    write_buf[0] = control;
    memcpy(&write_buf[1], data, len);

    return myiic_write(s_oled_dev_handle, OLED_ADDR, write_buf, len + 1);
}

esp_err_t oled_init(void)
{
    if (s_oled_initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err = myiic_add_device(OLED_I2C_ADDRESS, &s_oled_dev_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to add OLED device: %s", esp_err_to_name(err));
        return err;
    }

    const uint8_t init_cmds[] = {
        0xAE,       // Display off
        0xD5, 0x80, // Clock divide ratio
        0xA8, 0x3F, // Multiplex ratio 64
        0xD3, 0x00, // Display offset
        0x40,       // Set display start line
        0x8D, 0x14, // Charge pump enable
        0x20, 0x00, // Memory addressing mode: horizontal
        0xA1,       // Segment remap
        0xC8,       // COM output scan direction reverse
        0xDA, 0x12, // COM pins hardware configuration
        0x81, 0xCF, // Contrast control
        0xD9, 0xF1, // Pre-charge period
        0xDB, 0x40, // VCOMH deselect level
        0xA4,       // Display all on resume
        0xA6,       // Normal display
        0xAF        // Display on
    };

    memset(s_oled_buffer, 0, sizeof(s_oled_buffer));
    s_oled_initialized = true;

    err = oled_i2c_write_raw(init_cmds, sizeof(init_cmds), 0x00);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "OLED init commands failed: %s", esp_err_to_name(err));
        s_oled_initialized = false;
        myiic_remove_device(s_oled_dev_handle);
        s_oled_dev_handle = NULL;
        return err;
    }

    ESP_LOGI(TAG, "OLED initialized successfully");
    return oled_refresh();
}

esp_err_t oled_deinit(void)
{
    if (!s_oled_initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err = myiic_remove_device(s_oled_dev_handle);
    if (err == ESP_OK)
    {
        s_oled_initialized = false;
        s_oled_dev_handle = NULL;
        ESP_LOGI(TAG, "OLED deinitialized");
    }
    return err;
}

esp_err_t oled_send_command(uint8_t command)
{
    return oled_i2c_write_raw(&command, 1, 0x00);
}

esp_err_t oled_send_data(const uint8_t *data, size_t length)
{
    return oled_i2c_write_raw(data, length, 0x40);
}

esp_err_t oled_display_power(bool on)
{
    uint8_t cmd = on ? 0xAF : 0xAE;
    return oled_send_command(cmd);
}

esp_err_t oled_set_contrast(uint8_t contrast)
{
    const uint8_t cmds[2] = {0x81, contrast};
    return oled_i2c_write_raw(cmds, sizeof(cmds), 0x00);
}

esp_err_t oled_clear_buffer(void)
{
    if (!s_oled_initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }
    memset(s_oled_buffer, 0, sizeof(s_oled_buffer));
    return ESP_OK;
}
/*
1. oled_clear() 函数会 先清空缓冲区，然后立即刷新屏幕 （显示黑色）
2. 然后在缓冲区中绘制新内容
3. 最后再次刷新屏幕显示新内容
这样就造成了： 黑色 → 新内容 的闪烁效果
*/

esp_err_t oled_clear(void)
{
    if (!s_oled_initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }
    memset(s_oled_buffer, 0, sizeof(s_oled_buffer));
    return oled_refresh();
}


static esp_err_t oled_set_page_address(uint8_t page)
{
    if (page >= OLED_PAGES)
    {
        return ESP_ERR_INVALID_ARG;
    }
    const uint8_t cmds[3] = {0xB0 | page, 0x00, 0x10};
    return oled_i2c_write_raw(cmds, sizeof(cmds), 0x00);
}

esp_err_t oled_refresh(void)
{
    if (!s_oled_initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }

    for (uint8_t page = 0; page < OLED_PAGES; page++)
    {
        esp_err_t err = oled_set_page_address(page);
        if (err != ESP_OK)
        {
            return err;
        }

        const uint8_t *page_data = &s_oled_buffer[page * OLED_WIDTH];
        err = oled_send_data(page_data, OLED_WIDTH);
        if (err != ESP_OK)
        {
            return err;
        }
    }
    return ESP_OK;
}

esp_err_t oled_draw_pixel(uint8_t x, uint8_t y, bool color)
{
    if (x >= OLED_WIDTH || y >= OLED_HEIGHT)
    {
        return ESP_ERR_INVALID_ARG;
    }

    uint16_t index = (y / 8) * OLED_WIDTH + x;
    uint8_t bit = 1 << (y % 8);
    if (color)
    {
        s_oled_buffer[index] |= bit;
    }
    else
    {
        s_oled_buffer[index] &= ~bit;
    }
    return ESP_OK;
}

esp_err_t oled_show_char(uint8_t col, uint8_t row, char chr)
{
    if (col >= OLED_WIDTH || row >= OLED_PAGES)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (chr < 32 || chr > 127)
    {
        chr = '?';
    }

    const uint8_t *font = OLED_F6x8[chr - 32];
    uint16_t offset = row * OLED_WIDTH + col;
    if (col + OLED_FONT_WIDTH_6 > OLED_WIDTH)
    {
        return ESP_ERR_INVALID_ARG;
    }

    for (uint8_t i = 0; i < OLED_FONT_WIDTH_6; i++)
    {
        s_oled_buffer[offset + i] = font[i];
    }
    return ESP_OK;
}

esp_err_t oled_show_string(uint8_t col, uint8_t row, const char *str)
{
    if (str == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t x = col;
    while (*str != '\0')
    {
        if (x + OLED_FONT_WIDTH_6 > OLED_WIDTH)
        {
            break; // 超过当前行宽度则停止
        }
        esp_err_t err = oled_show_char(x, row, *str);
        if (err != ESP_OK)
        {
            return err;
        }
        x += OLED_FONT_WIDTH_6;
        str++;
    }
    return ESP_OK;
}

esp_err_t oled_draw_image(uint8_t col, uint8_t row, const uint8_t *image, uint8_t width, uint8_t height)
{
    if (!s_oled_initialized || image == NULL || col >= OLED_WIDTH || row >= OLED_PAGES)
    {
        return ESP_ERR_INVALID_ARG;
    }

    // 计算需要显示的页数（每页8像素高）
    uint8_t pages = (height + 7) / 8;

    for (uint8_t page = 0; page < pages && (row + page) < OLED_PAGES; page++)
    {
        for (uint8_t x = 0; x < width && (col + x) < OLED_WIDTH; x++)
        {
            uint16_t buffer_idx = (row + page) * OLED_WIDTH + col + x;
            uint16_t image_idx = page * width + x;
            s_oled_buffer[buffer_idx] = image[image_idx];
        }
    }

    return ESP_OK;
}
