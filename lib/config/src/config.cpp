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
            ESP_LOGE(TAG, "Json, %s", doc.as<String>().c_str());

            if (!areAllKeysPresent(doc, {"ssid",
                                         "pass",
                                         "radio_mode",
                                         "control_mode",

                                         "THRUST",
                                         "PITCH",
                                         "ROLL",
                                         "YAW",
                                         "breaker",
                                         "Reverse",

                                         "ROLL_FLIP",
                                         "PITCH_FLIP",
                                         "YAW_FLIP",
                                         "THRUST_FLIP",
                                         "breaker_FLIP",
                                         "Reverse_FLIP"}))
            {
                ESP_LOGE(TAG, "未知数据, %s", doc.as<String>().c_str());
                return;
            }

            strlcpy(CONFIG.WIFI_SSID, doc["ssid"], sizeof(CONFIG.WIFI_SSID));
            strlcpy(CONFIG.WIFI_PASS, doc["pass"], sizeof(CONFIG.WIFI_PASS));

            // 从 doc["breaker"] 取 2 字节数组到 CONFIG.breaker
            JsonArray breakerArr = doc["breaker"].as<JsonArray>();
            if (breakerArr.size() == 2)
                for (size_t i = 0; i < 2; ++i)
                    CONFIG.breaker[i] = breakerArr[i];
            else
                ESP_LOGE(TAG, "breaker 数组长度错误: %d", breakerArr.size());

            JsonArray breakerFlipArr = doc["breaker_FLIP"].as<JsonArray>();
            if (breakerFlipArr.size() == 2)
                for (size_t i = 0; i < 2; ++i)
                    CONFIG.breaker_FLIP[i] = breakerFlipArr[i];
            else
                ESP_LOGE(TAG, "breaker_FLIP 数组长度错误: %d", breakerFlipArr.size());

            CONFIG.radio_mode = doc["radio_mode"];
            CONFIG.control_mode = doc["control_mode"];
            CONFIG.THRUST = doc["THRUST"];
            CONFIG.PITCH = doc["PITCH"];
            CONFIG.ROLL = doc["ROLL"];
            CONFIG.YAW = doc["YAW"];
            CONFIG.Reverse = doc["Reverse"];

            CONFIG.ROLL_FLIP = doc["ROLL_FLIP"];
            CONFIG.PITCH_FLIP = doc["PITCH_FLIP"];
            CONFIG.YAW_FLIP = doc["YAW_FLIP"];
            CONFIG.THRUST_FLIP = doc["THRUST_FLIP"];
            CONFIG.Reverse_FLIP = doc["Reverse_FLIP"];

            CONFIG.print();
            CONFIG.save()
                ? ESP_LOGI(TAG, "保存成功")
                : ESP_LOGE(TAG, "保存失败");
        }
        else
        {
            JsonDocument doc;
            doc["ssid"] = CONFIG.WIFI_SSID;
            doc["pass"] = CONFIG.WIFI_PASS;
            doc["radio_mode"] = CONFIG.radio_mode;
            doc["control_mode"] = CONFIG.control_mode;
            doc["THRUST"] = CONFIG.THRUST;
            doc["PITCH"] = CONFIG.PITCH;
            doc["ROLL"] = CONFIG.ROLL;
            doc["YAW"] = CONFIG.YAW;
            doc["Reverse"] = CONFIG.Reverse;

            doc["ROLL_FLIP"] = CONFIG.ROLL_FLIP;
            doc["PITCH_FLIP"] = CONFIG.PITCH_FLIP;
            doc["YAW_FLIP"] = CONFIG.YAW_FLIP;
            doc["THRUST_FLIP"] = CONFIG.THRUST_FLIP;
            doc["Reverse_FLIP"] = CONFIG.Reverse_FLIP;

            // breaker& breaker_FLIP 是数组，需要逐个赋值
            JsonArray breakerArr = doc["breaker"].to<JsonArray>();
            for (size_t i = 0; i < 2; ++i)
                breakerArr.add(CONFIG.breaker[i]);

            JsonArray breakerFlipArr = doc["breaker_FLIP"].to<JsonArray>();
            for (size_t i = 0; i < 2; ++i)
                breakerFlipArr.add(CONFIG.breaker_FLIP[i]);

            String JsonString;
            serializeJson(doc, JsonString);
            web_console_send(JsonString);
            CONFIG.print();
        }
    };

    register_web_console_callback(WEB_PORT_CONFIG, web_cbfn);

    this->print();
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

void Config::print()
{
    ESP_LOGI(TAG, "=== Current Configuration ===");
    ESP_LOGI(TAG, "Data valid: %s", data_is_alive ? "Yes" : "No");
    ESP_LOGI(TAG, "WIFI SSID: %s", WIFI_SSID);
    ESP_LOGI(TAG, "WIFI Password: [Length: %d]", strlen(WIFI_PASS));
    switch (radio_mode)
    {
    case ESP_NOW:
        ESP_LOGI(TAG, "Radio Mode: ESP_NOW");
        break;
    case BT_CONTROLLER:
        ESP_LOGI(TAG, "Radio Mode: BT_CONTROLLER");
        break;
    default:
        ESP_LOGI(TAG, "Radio Mode: Unknown (%d)", radio_mode);
        break;
    }
    ESP_LOGI(TAG, "Control Mode: %s", control_mode == SLAVE ? "SLAVE" : "MASTER");

    ESP_LOGI(TAG, "THRUST: %d", THRUST);
    ESP_LOGI(TAG, "PITCH: %d", PITCH);
    ESP_LOGI(TAG, "ROLL: %d", ROLL);
    ESP_LOGI(TAG, "YAW: %d", YAW);
    ESP_LOGI(TAG, "breaker: [%d, %d]", breaker[0], breaker[1]);

    ESP_LOGI(TAG, "Reverse: %d", Reverse);
    ESP_LOGI(TAG, "ROLL_FLIP: %d", ROLL_FLIP);
    ESP_LOGI(TAG, "PITCH_FLIP: %d", PITCH_FLIP);
    ESP_LOGI(TAG, "YAW_FLIP: %d", YAW_FLIP);
    ESP_LOGI(TAG, "THRUST_FLIP: %d", THRUST_FLIP);
    ESP_LOGI(TAG, "breaker_FLIP: [%d, %d]", breaker_FLIP[0], breaker_FLIP[1]);
    ESP_LOGI(TAG, "Reverse_FLIP: %d", Reverse_FLIP);
    ESP_LOGI(TAG, "=========================");
}
