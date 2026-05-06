#include "max30102.h"

static const char *TAG = "MAX30102";
static i2c_master_dev_handle_t max30102_dev = NULL;

static uint32_t aun_ir_buffer[IR_BUF_LEN];
static uint32_t aun_red_buffer[RED_BUF_LEN];
static int32_t n_spo2;
static int8_t ch_spo2_valid;
static int32_t n_heart_rate;
static int8_t ch_hr_valid;

esp_err_t Max30102_Init(void)
{
    ESP_ERROR_CHECK(myiic_add_device(MAX30102_ADDR, &max30102_dev));
    vTaskDelay(pdMS_TO_TICKS(100));

    Max30102_Write_Reg(REG_MODE_CONFIG, 0x40);
    vTaskDelay(pdMS_TO_TICKS(10));
    Max30102_Write_Reg(REG_INTR_ENABLE_1, 0xC0);
    Max30102_Write_Reg(REG_INTR_ENABLE_2, 0x00);
    Max30102_Write_Reg(REG_FIFO_WR_PTR, 0x00);
    Max30102_Write_Reg(REG_OVF_COUNTER, 0x00);
    Max30102_Write_Reg(REG_FIFO_RD_PTR, 0x00);
    Max30102_Write_Reg(REG_FIFO_CONFIG, 0x0F);
    Max30102_Write_Reg(REG_MODE_CONFIG, 0x03);
    Max30102_Write_Reg(REG_SPO2_CONFIG, 0x27);
    Max30102_Write_Reg(REG_LED1_PA, 0x24);
    Max30102_Write_Reg(REG_LED2_PA, 0x24);
    Max30102_Write_Reg(REG_PILOT_PA, 0x7f);

    return ESP_OK;
}

esp_err_t Max30102_Write_Reg(uint8_t reg, uint8_t data)
{
    uint8_t buf[2] = {reg, data};
    return myiic_write(max30102_dev, MAX30102_ADDR, buf, 2);
}

esp_err_t Max30102_Read_Reg(uint8_t reg, uint8_t *data)
{
    return myiic_write_read(max30102_dev, MAX30102_ADDR, &reg, 1, data, 1);
}

esp_err_t Max30102_Read_Fifo(uint8_t *buffer, uint8_t count)
{
    uint8_t reg_addr = REG_FIFO_DATA;
    return myiic_write_read(max30102_dev, MAX30102_ADDR, &reg_addr, 1, buffer, count);
}

uint32_t Max30102_Get_Heart_Rate(void)
{
    return n_heart_rate;
}

uint32_t Max30102_Get_Spo2(void)
{
    return n_spo2;
}

static float max30102_calculate_spo2(float *ir_data, float *red_data, uint16_t count)
{
    float ir_max = *ir_data, ir_min = *ir_data;
    float red_max = *red_data, red_min = *red_data;

    for (uint16_t i = 1; i < count; i++)
    {
        ir_max = ir_data[i] > ir_max ? ir_data[i] : ir_max;
        ir_min = ir_data[i] < ir_min ? ir_data[i] : ir_min;
        red_max = red_data[i] > red_max ? red_data[i] : red_max;
        red_min = red_data[i] < red_min ? red_data[i] : red_min;
    }

    float ir_range = ir_max - ir_min;
    float red_range = red_max - red_min;
    float ir_sum = ir_max + ir_min;
    float red_sum = red_max + red_min;

    if (ir_range < 1e-6 || red_sum < 1e-6)
    {
        return -1.0f;
    }

    float R = (ir_sum * red_range) / (red_sum * ir_range);
    return (-45.060f * R * R) + (30.354f * R) + 94.845f;
}

static void find_peaks(int32_t *locs, int32_t *npks, int32_t *x, int32_t size, int32_t min_h, int32_t min_d, int32_t max_n)
{
    int32_t i = 1, count = 0;
    int32_t temp_locs[15] = {0};

    while (i < size - 1 && count < 15)
    {
        if (x[i] > min_h && x[i] > x[i - 1] && x[i] > x[i + 1])
        {
            temp_locs[count++] = i;
            while (i < size - 1 && x[i] >= x[i + 1])
                i++;
        }
        i++;
    }

    for (i = 1; i < count; i++)
    {
        for (int32_t j = i; j > 0 && x[temp_locs[j]] > x[temp_locs[j - 1]]; j--)
        {
            int32_t temp = temp_locs[j];
            temp_locs[j] = temp_locs[j - 1];
            temp_locs[j - 1] = temp;
        }
    }

    int32_t final_count = 0;
    memset(locs, 0, sizeof(int32_t) * 15);
    for (i = 0; i < count && final_count < max_n; i++)
    {
        bool too_close = false;
        for (int32_t j = 0; j < final_count; j++)
        {
            if (llabs((long)temp_locs[i] - (long)locs[j]) < min_d)
            {
                too_close = true;
                break;
            }
        }
        if (!too_close)
        {
            locs[final_count++] = temp_locs[i];
        }
    }

    for (i = 1; i < final_count; i++)
    {
        for (int32_t j = i; j > 0 && locs[j] < locs[j - 1]; j--)
        {
            int32_t temp = locs[j];
            locs[j] = locs[j - 1];
            locs[j - 1] = temp;
        }
    }

    *npks = final_count;
}

void Max30102_Algorithm_Calculate(uint32_t *ir_buffer, int32_t buffer_len, uint32_t *red_buffer,
                                  int32_t *spo2, int8_t *spo2_valid,
                                  int32_t *heart_rate, int8_t *hr_valid)
{
    static int32_t an_dx[MAX30102_BUFFER_SIZE];
    static int32_t an_x[MAX30102_BUFFER_SIZE];
    static float ir_float[MAX30102_BUFFER_SIZE];
    static float red_float[MAX30102_BUFFER_SIZE];
    static int32_t last_hr = 0;

    uint32_t ir_mean = 0;
    int32_t k, n_npks;
    int32_t peak_locs[15] = {0};

    for (k = 0; k < buffer_len; k++)
    {
        ir_mean += ir_buffer[k];
    }
    ir_mean /= buffer_len;

    for (k = 0; k < buffer_len; k++)
    {
        an_x[k] = ir_buffer[k] - ir_mean;
    }

    for (k = 0; k < buffer_len - 8; k++)
    {
        an_x[k] = (an_x[k] + an_x[k + 1] + an_x[k + 2] + an_x[k + 3] +
                   an_x[k + 4] + an_x[k + 5] + an_x[k + 6] + an_x[k + 7]) /
                  8;
    }

    for (k = 0; k < buffer_len - 9; k++)
    {
        an_dx[k] = an_x[k + 1] - an_x[k];
    }

    int32_t abs_sum = 0;
    int32_t data_len = buffer_len - 14;
    for (k = 0; k < data_len; k++)
    {
        abs_sum += llabs(an_dx[k]);
    }
    // 使用较低的阈值以更容易检测到峰值
    int32_t th = abs_sum / data_len;
    if (th < 10)
        th = 10; // 设置最小阈值

    ESP_LOGD(TAG, "Peak detection - threshold: %ld, data_len: %ld", (long)th, (long)data_len);

    find_peaks(peak_locs, &n_npks, an_dx, data_len, th, 10, 4);

    ESP_LOGD(TAG, "Peak detection - found %ld peaks", (long)n_npks);

    if (n_npks >= 2)
    {
        int32_t intervals[10] = {0}, cnt = 0;
        for (k = 1; k < n_npks; k++)
        {
            int32_t interval = peak_locs[k] - peak_locs[k - 1];
            if (interval > 5 && interval < 50)
            {
                intervals[cnt++] = interval;
            }
        }

        if (cnt >= 2)
        {
            for (int i = 1; i < cnt; i++)
            {
                for (int j = i; j > 0 && intervals[j] < intervals[j - 1]; j--)
                {
                    int32_t temp = intervals[j];
                    intervals[j] = intervals[j - 1];
                    intervals[j - 1] = temp;
                }
            }

            int32_t current_hr = 12000 / intervals[cnt / 2];

            if (current_hr >= HEART_RATE_MIN_VALID && current_hr <= HEART_RATE_MAX_VALID)
            {
                if (last_hr > 0 && llabs(current_hr - last_hr) > 50)
                {
                    current_hr = last_hr;
                }
                last_hr = current_hr;
                *heart_rate = current_hr;
                *hr_valid = 1;
            }
            else
            {
                *hr_valid = 0;
                *spo2_valid = 0;
                return;
            }
        }
        else
        {
            *hr_valid = 0;
            *spo2_valid = 0;
            return;
        }
    }
    else
    {
        *hr_valid = 0;
        *spo2_valid = 0;
        return;
    }

    for (k = 0; k < buffer_len; k++)
    {
        ir_float[k] = (float)ir_buffer[k];
        red_float[k] = (float)red_buffer[k];
    }

    float spo2_result = max30102_calculate_spo2(ir_float, red_float, (uint16_t)buffer_len);
    if (spo2_result >= 0.0f && spo2_result <= 100.0f)
    {
        *spo2 = (int32_t)spo2_result;
        *spo2_valid = 1;
    }
    else
    {
        *spo2_valid = 0;
    }
}

void Max30102_Task(void *pvParameters)
{
    esp_log_level_set("gpio", ESP_LOG_ERROR);
    esp_log_level_set("i2c", ESP_LOG_ERROR);

    uint8_t temp[6];
    int32_t buf_len = IR_BUF_LEN;
    uint8_t fifo_wp, fifo_rp;
    int samples_read, i;

    ESP_LOGI(TAG, "Task started");

    if (bus_handle == NULL)
    {
        ESP_LOGE(TAG, "I2C bus not initialized");
        vTaskDelete(NULL);
        return;
    }

    Max30102_Init();
    vTaskDelay(pdMS_TO_TICKS(100));

    uint8_t part_id = 0;
    esp_err_t ret = Max30102_Read_Reg(REG_PART_ID, &part_id);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "I2C read failed");
        vTaskDelete(NULL);
        return;
    }
    ESP_LOGI(TAG, "Part ID: 0x%02X", part_id);

    samples_read = 0;
    ESP_LOGI(TAG, "Collecting initial %ld samples...", (long)buf_len);

    while (samples_read < buf_len)
    {
        Max30102_Read_Reg(REG_FIFO_WR_PTR, &fifo_wp);
        Max30102_Read_Reg(REG_FIFO_RD_PTR, &fifo_rp);

        if (fifo_wp != fifo_rp)
        {
            if (Max30102_Read_Fifo(temp, 6) == ESP_OK)
            {
                aun_red_buffer[samples_read] = ((temp[0] & 0x03) << 16) | (temp[1] << 8) | temp[2];
                aun_ir_buffer[samples_read] = ((temp[3] & 0x03) << 16) | (temp[4] << 8) | temp[5];
                samples_read++;

                // 每100个样本打印进度
                if (samples_read % 100 == 0)
                {
                    ESP_LOGI(TAG, "Collected %ld/%ld samples", (long)samples_read, (long)buf_len);
                }
            }
        }
        else
        {
            vTaskDelay(pdMS_TO_TICKS(5));
        }
    }

    // 打印初始数据的一些统计信息
    uint32_t ir_min = UINT32_MAX, ir_max = 0;
    for (int i = 0; i < buf_len; i++)
    {
        ir_min = aun_ir_buffer[i] < ir_min ? aun_ir_buffer[i] : ir_min;
        ir_max = aun_ir_buffer[i] > ir_max ? aun_ir_buffer[i] : ir_max;
    }
    ESP_LOGI(TAG, "Initial IR data: min=%lu, max=%lu, range=%lu", ir_min, ir_max, ir_max - ir_min);

    ESP_LOGI(TAG, "Ready for monitoring");

    while (1)
    {
        for (i = 100; i < 500; i++)
        {
            aun_red_buffer[i - 100] = aun_red_buffer[i];
            aun_ir_buffer[i - 100] = aun_ir_buffer[i];
        }

        samples_read = 400;
        while (samples_read < 500)
        {
            vTaskDelay(pdMS_TO_TICKS(2));
            Max30102_Read_Reg(REG_FIFO_WR_PTR, &fifo_wp);
            Max30102_Read_Reg(REG_FIFO_RD_PTR, &fifo_rp);

            if (fifo_wp != fifo_rp && Max30102_Read_Fifo(temp, 6) == ESP_OK)
            {
                aun_red_buffer[samples_read] = ((temp[0] & 0x03) << 16) | (temp[1] << 8) | temp[2];
                aun_ir_buffer[samples_read] = ((temp[3] & 0x03) << 16) | (temp[4] << 8) | temp[5];
                samples_read++;
            }
        }

        Max30102_Algorithm_Calculate(aun_ir_buffer, buf_len, aun_red_buffer, &n_spo2, &ch_spo2_valid, &n_heart_rate, &ch_hr_valid);

        // 心率有效时打印并发送心率数据
        if (ch_hr_valid == 1)
        {
            ESP_LOGI(TAG, "HR: %ld bpm", (long)n_heart_rate);

            // 如果血氧也有效，一起发送
            if (ch_spo2_valid == 1)
            {
                ESP_LOGI(TAG, "SpO2: %ld%%", (long)n_spo2);
                Message_Queue_Send_Heart_Rate((uint32_t)n_heart_rate, (uint32_t)n_spo2, 0, false);
            }
            else
            {
                // 只发送心率数据
                Message_Queue_Send_Heart_Rate((uint32_t)n_heart_rate, 0, 0, false);
            }
        }
        else
        {
            // 调试：心率无效时打印状态
            ESP_LOGD(TAG, "Heart rate invalid - hr_valid: %d, spo2_valid: %d", ch_hr_valid, ch_spo2_valid);
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }

    vTaskDelete(NULL);
}