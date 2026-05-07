#include "system_oled_show.h"


#define MEASUREMENT_TOTAL_SECONDS 20
#define PROGRESS_BAR_WIDTH 20  /* 字符数，每字符6像素 = 120像素 */

static void oled_show_measurement_screen(void)
{
    oled_clear_buffer();

    oled_show_string(12, 0, "= Measuring =");

    char buf[22];

    /* 心率：有数据时显示数值，否则显示等待提示 */
    uint32_t hr = Max30102_Get_Heart_Rate();
    if (hr > 0 && hr <= 200)
        snprintf(buf, sizeof(buf), "HR:  %3lu bpm", (unsigned long)hr);
    else
        snprintf(buf, sizeof(buf), "HR:  ... bpm");
    oled_show_string(0, 2, buf);

    /* 血氧 */
    uint32_t spo2 = Max30102_Get_Spo2();
    if (spo2 > 0 && spo2 <= 100)
        snprintf(buf, sizeof(buf), "SpO2: %3lu%%", (unsigned long)spo2);
    else
        snprintf(buf, sizeof(buf), "SpO2: ...%%");
    oled_show_string(0, 3, buf);

    /* 已用时间 / 总时间 */
    uint32_t elapsed = Max30102_Get_Elapsed_Seconds();
    snprintf(buf, sizeof(buf), "%2lu / %ds", (unsigned long)elapsed, MEASUREMENT_TOTAL_SECONDS);
    oled_show_string(0, 5, buf);

    /* 进度条 */
    int filled = (elapsed * PROGRESS_BAR_WIDTH) / MEASUREMENT_TOTAL_SECONDS;
    if (filled > PROGRESS_BAR_WIDTH)
        filled = PROGRESS_BAR_WIDTH;

    char bar[PROGRESS_BAR_WIDTH + 3];
    bar[0] = '[';
    for (int i = 0; i < PROGRESS_BAR_WIDTH; i++)
        bar[1 + i] = (i < filled) ? '#' : ' ';
    bar[1 + PROGRESS_BAR_WIDTH] = ']';
    bar[2 + PROGRESS_BAR_WIDTH] = '\0';
    oled_show_string(4, 7, bar);

    oled_refresh();
}

void OLEDShowTask(void *pvParameters)
{

    while (1)
    {
        if (Max30102_Is_Measuring())
        {
            oled_show_measurement_screen();
            vTaskDelay(pdMS_TO_TICKS(200));
            continue;
        }

        float voltage = 0.0f;
        battery_get_average_voltage(5, &voltage); // 采样 5 次，获取平均电压
        

        char voltage_str[20];
        snprintf(voltage_str, sizeof(voltage_str), "%.2f V", voltage);

        oled_clear_buffer();
        oled_show_string(0, 0, RTC_get_HH_MM_SS()); // 显示当前时分秒

        oled_show_string(0, 3, "V:  ");
        oled_show_string(48, 3, voltage_str);

        uint32_t heart_rate = Max30102_Get_Heart_Rate();
        char heart_rate_str[15];
        if (heart_rate > 0 && heart_rate <= 200)
        {
            snprintf(heart_rate_str, sizeof(heart_rate_str), "%lu bpm", heart_rate);
        }
        else
        {
            snprintf(heart_rate_str, sizeof(heart_rate_str), "-- bpm");
        }
        oled_show_string(0, 4, "HR:  ");
        oled_show_string(64, 4, heart_rate_str);

        uint32_t spo2 = Max30102_Get_Spo2();
        char spo2_str[15];
        if (spo2 > 0 && spo2 <= 100)
        {
            snprintf(spo2_str, sizeof(spo2_str), "%lu%%", spo2);
        }
        else
        {
            snprintf(spo2_str, sizeof(spo2_str), "--%%");
        }
        oled_show_string(0, 5, "SPO2: ");
        oled_show_string(64, 5, spo2_str);

        int16_t ax, ay, az;
        Mpu6050_Get_Accel_Data(&ax, &ay, &az);
        char accel_buf[22];
        snprintf(accel_buf, sizeof(accel_buf), "X:%+5d Y:%+5d", ax, ay);
        oled_show_string(0, 6, accel_buf);
        snprintf(accel_buf, sizeof(accel_buf), "Z:%+5d", az);
        oled_show_string(0, 7, accel_buf);

        if (wifi_is_connected())
        {
            oled_draw_image(80, 0, wifiImg.data, wifiImg.width, wifiImg.height);
        }

        // 使用 oled_draw_image 显示电池图标
        oled_draw_image(100, 0, battery_pattern(voltage), 19, 8);

        oled_refresh();

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
