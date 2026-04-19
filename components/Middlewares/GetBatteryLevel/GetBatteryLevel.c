#include "GetBatteryLevel.h"

// ========== 全局变量 ==========
static const char *TAG = "BATTERY";
static adc_oneshot_unit_handle_t adc1_handle = NULL;  // ADC 句柄
static bool is_initialized = false;

// ========== 初始化函数（只调用一次） ==========
esp_err_t battery_adc_init(void)
{
    if (is_initialized) {
        return ESP_OK;  // 防止重复初始化
    }

    // 1️⃣ 创建 ADC 单元句柄（相当于"打开设备"）
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,              // 使用 ADC1
        .ulp_mode = ADC_ULP_MODE_DISABLE,   // 不需要 ULP 模式
    };
    
    esp_err_t ret = adc_oneshot_new_unit(&init_config, &adc1_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "创建 ADC 句柄失败: %s", esp_err_to_name(ret));
        return ret;
    }

    // 2️⃣ 配置通道参数（分辨率 + 衰减）
    adc_oneshot_chan_cfg_t chan_config = {
        .atten = ADC_ATTEN_DB_11,    // 11dB 衰减 → 最大测量 ~3.1V
        .bitwidth = ADC_BITWIDTH_12, // 12位分辨率 → 0~4095
    };
    
    ret = adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_7, &chan_config);  // GPIO1 = ADC1 Channel 0
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "配置通道失败: %s", esp_err_to_name(ret));
        adc_oneshot_del_unit(adc1_handle);  // 清理资源
        return ret;
    }

    
    is_initialized = true;
    ESP_LOGI(TAG, "ADC 初始化成功");
    return ESP_OK;
}

// ========== 读取函数（可重复调用） ==========
esp_err_t battery_adc_read(uint32_t *out_value)
{
    // 安全检查
    if (!is_initialized) {
        ESP_LOGE(TAG, "请先调用 battery_adc_init()");
        return ESP_ERR_INVALID_STATE;
    }
    if (out_value == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    // 3️⃣ 读取原始值（核心函数）
    int raw_value;
    esp_err_t ret = adc_oneshot_read(adc1_handle, ADC_CHANNEL_7, &raw_value);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "读取失败: %s", esp_err_to_name(ret));
        return ret;
    }

    *out_value = (uint32_t)raw_value;  // 输出结果
    return ESP_OK;
}

// ========== 可选：反初始化（释放资源） ==========
void battery_adc_deinit(void)
{
    if (adc1_handle != NULL) {
        adc_oneshot_del_unit(adc1_handle);
        adc1_handle = NULL;
        is_initialized = false;
        ESP_LOGI(TAG, "ADC 资源已释放");
    }
}