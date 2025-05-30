#include "config.h"
#include "esp_log.h"
#include "nvs_flash.h"

#define TAG "Config"
config_t CONFIG;

#define NAMESPACE "config"
#define KEY "CONFIG_DATA_RAW"
#define WIFI_SSID_KEY "WIFI_SSID"
#include "web_console.h"

bool saveBytesToNVS(const uint8_t *data, size_t dataSize)
{
    nvs_handle handle;
    esp_err_t err = nvs_open(NAMESPACE, NVS_READWRITE, &handle);
    if (err == ESP_OK)
    {
        err = nvs_set_blob(handle, KEY, data, dataSize);
        if (err != ESP_OK)
            nvs_commit(handle);
    }

    nvs_close(handle);
    if (err == ESP_OK)
        return true;
    else
    {
        ESP_LOGE(TAG, "NVS Error, code: %s\n", esp_err_to_name(err));
        return false;
    }
}

// 从 NVS 读取 bytes 数据
bool loadBytesFromNVS(uint8_t *data, size_t dataSize)
{
    nvs_handle handle;
    esp_err_t err = nvs_open(NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error opening NVS: %s", esp_err_to_name(err));
        return false;
    }

    // 获取二进制数据大小
    size_t requiredSize = 0;
    err = nvs_get_blob(handle, KEY, NULL, &requiredSize);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
    {
        ESP_LOGE(TAG, "Error getting blob size from NVS: %s", esp_err_to_name(err));
        nvs_close(handle);
        return false;
    }

    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        ESP_LOGI(TAG, "No data found in NVS!");
        nvs_close(handle);
        return false;
    }

    if (requiredSize != dataSize)
    {
        ESP_LOGE(TAG, "Data size mismatch! Expected %zu, got %zu", dataSize, requiredSize);
        nvs_close(handle);
        return false;
    }

    // 读取二进制数据
    err = nvs_get_blob(handle, KEY, data, &dataSize);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error reading bytes from NVS: %s", esp_err_to_name(err));
        nvs_close(handle);
        return false;
    }

    nvs_close(handle);
    return true;
}
void Config::configCheckTask(void *parameter)
{
    Config *config = (Config *)parameter;
    while (1)
    {
        // 检查raw数组是否发生变化
        if (memcmp(config->raw, config->previous_raw, sizeof(config->raw)) != 0)
        {
            // 如果发生变化，保存配置
            if (config->save())
            {
                // 更新previous_raw
                memcpy(config->previous_raw, config->raw, sizeof(config->raw));
                ESP_LOGI(TAG, "Config changed and saved successfully");
            }
            else
                ESP_LOGE(TAG, "Failed to save changed config");
        }
        // 每秒检查一次
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void Config::begin()
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    /**
     * 读取数据到临时缓冲区校验其是否有效，
     * 如数据无效则放弃缓冲区中的数据
     */
    uint8_t temp_raw[sizeof(this->raw)];
    loadBytesFromNVS(temp_raw, sizeof(this->raw));
    uint8_t cksum = calculateChecksum(temp_raw, sizeof(temp_raw));

    if (temp_raw[0] != 0)
        if (cksum == temp_raw[0])
        {
            // 如果校验和匹配，则将数据复制到raw数组中
            memcpy(this->raw, temp_raw, sizeof(this->raw));
            ESP_LOGI(TAG, "Loaded config from NVS successfully");
        }
        else
        {
            ESP_LOGE(TAG, "Checksum mismatch, using default values");
            ESP_LOGI(TAG, "Checksum: %d, temp_raw[0]: %d", cksum, temp_raw[0]);
        }
    else
        ESP_LOGW(TAG, "No data found in NVS");
    // 初始化previous_raw
    memcpy(previous_raw, raw, sizeof(raw));

    /**
     * 注册web控制台回调函数
     */
    auto web_cbfn = [](json_doc doc)
    {
        if (!areAllKeysPresent(doc, {"port", "rw"}))
        {
            ESP_LOGE(TAG, "未知数据, %s", doc.as<String>().c_str());
            return;
        }

        if (doc["rw"] == WEB_CONSOLE_W)
        {
            if (!areAllKeysPresent(doc, {"ssid", "pass"}))
            {
                ESP_LOGE(TAG, "未知数据, %s", doc.as<String>().c_str());
                return;
            }

            strlcpy(CONFIG.WIFI_SSID,
                    doc["ssid"],
                    sizeof(CONFIG.WIFI_SSID));

            strlcpy(CONFIG.WIFI_PASS,
                    doc["pass"],
                    sizeof(CONFIG.WIFI_PASS));

            ESP_LOGI(TAG, "SSID: %s, PASS: %s",
                     CONFIG.WIFI_SSID, CONFIG.WIFI_PASS);

            CONFIG.save()
                ? ESP_LOGI(TAG, "保存成功")
                : ESP_LOGE(TAG, "保存失败");
        }
        else
        {
            JsonDocument doc;
            doc["ssid"] = CONFIG.WIFI_SSID;
            doc["pass"] = CONFIG.WIFI_PASS;
            String JsonString;
            serializeJson(doc, JsonString);
            web_console_send(JsonString);
        }
    };

    register_web_console_callback(WEB_PORT_CONFIG, web_cbfn);

    // // 创建配置检查任务
    // xTaskCreate(
    //     configCheckTask, // 任务函数
    //     "ConfigCheck",   // 任务名称
    //     2048,            // 堆栈大小
    //     this,            // 传递this指针作为参数
    //     TP_L,            // 使用最低优先级
    //     &checkTaskHandle // 保存任务句柄
    // );
}

Config::~Config()
{
    // 如果任务存在，删除它
    if (checkTaskHandle == NULL)
        return;
    vTaskDelete(checkTaskHandle);
    checkTaskHandle = NULL;
}

uint8_t Config::calculateChecksum(uint8_t *data, size_t dataSize)
{
    uint8_t checksum = 0;
    // 计算除0位之外所有数据的校验和
    for (size_t i = 1; i < dataSize; i++)
        checksum ^= data[i]; // 使用异或运算计算校验和
    return checksum;
}

bool Config::save()
{
    // 计算并存储校验和到data_is_alive
    this->raw[0] = calculateChecksum(this->raw, sizeof(this->raw));
    return saveBytesToNVS(this->raw, sizeof(this->raw));
}
