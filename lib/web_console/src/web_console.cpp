#include "web_console.h"
#include "config.h"
#include "tool.h"

#include "Arduino.h"

#include "WebSocketsServer.h"
#include "WebServer.h"
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"

#include "wifi_link.h"
#include "radio.h"
#include "bat.h"

#include "vector"

#define TAG "Web Console"
#define PORT_HTTP 80
#define PORT_WS 81
#define WEB_SOCKET_FEQ 120 // Hz

bool areAllKeysPresent(const json_doc &doc,
                       const std::vector<const char *> &indices)
{
    bool all_present = true;

    for (const auto &key : indices)
        if (doc[key].isNull())
        {
            ESP_LOGW(TAG, "缺少配置项: %s", key);
            all_present = false;
            break;
        }

    return all_present;
}

/**
 * 一个二维数组，第一维表示端口号，第二维表示回调函数列表，
 * 可以支持在任意端口注册多个回调函数
 */
typedef std::vector<web_console_callback_t> callback_list_t;
callback_list_t web_console_callback[__PORT_MAX];

/**
 * * @brief 注册回调函数
 */
bool register_web_console_callback(web_console_port port, web_console_callback_t cb)
{
    if (port >= __PORT_MAX)
    {
        ESP_LOGE(TAG, "端口号错误 %d", port);
        return false;
    }
    web_console_callback[port].push_back(cb);
    return true;
}

/**
 * web console 服务器
 */
WebSocketsServer webSocket = WebSocketsServer(PORT_WS);
AsyncWebServer server(PORT_HTTP);

/**
 * 在这里处理收到的数据 & 处理回调
 */
void handleIncomingJson(uint8_t num, const char *jsonString)
{
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, jsonString);

    if (error)
    {
        Serial.printf("JSON解析失败: %s\n", error.c_str());
        return;
    }

    if (doc["port"].isNull())
        ESP_LOGE(TAG, "未知数据格式: \"%s\"(String)", jsonString);
    else
    {
        int port = doc["port"];
        if (port < __PORT_MAX)
            for (auto &callback : web_console_callback[port])
                callback(doc);
        else
            ESP_LOGE(TAG, "无效的端口号: %d", port);
    }
}
// WS 事件处理函数
void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{
    switch (type)
    {
    case WStype_DISCONNECTED:
        Serial.printf("[%u] 断开连接!\n", num);
        break;
    case WStype_CONNECTED:
    {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] 已连接 from %d.%d.%d.%d\n", num, ip[0], ip[1], ip[2], ip[3]);
    }
    break;

    case WStype_TEXT:
        handleIncomingJson(num, (const char *)payload);
        break;
    }
}

void web_console_send(String &jsonString)
{
    webSocket.broadcastTXT(jsonString);
}

void init_web_console()
{
    WiFi_init(); // 初始化WiFi
    if (!SPIFFS.begin(true))
    {
        ESP_LOGE(TAG, "SPIFFS mount failed!");
        return;
    }

    // 设置静态资源服务器
    server
        .serveStatic("/", SPIFFS, "/")   // 设置静态文件目录
        .setCacheControl("max-age=3600") // 设置缓存时间
        .setDefaultFile("index.html");   // 页面入口

    server.onNotFound([](AsyncWebServerRequest *request) // 处理404错误
                      { request->send(404, "text/plain", "404 Not Found"); });
    server.begin(); // 启动HTTP服务器

    webSocket.onEvent(webSocketEvent); // 设置WebSocket事件处理函数
    webSocket.begin();                 // 启动WebSocket服务器

    // WS 通讯任务
    auto wct = [](void *pvParameters)
    {
        TickType_t xLastWakeTime = xTaskGetTickCount();
        const TickType_t xFrequency = HZ2TICKS(WEB_SOCKET_FEQ);
        while (true)
        {
            webSocket.loop(); // 处理WebSocket事件
            vTaskDelayUntil(&xLastWakeTime, xFrequency);
        }
    };

    auto ret = xTaskCreate(wct, "WebConsole", 4096, NULL, TP_N, NULL);
    ESP_ERROR_CHECK(ret == pdPASS ? ESP_OK : ESP_FAIL);

    auto ip = WiFi.getMode() == WIFI_MODE_STA
                  ? WiFi.localIP()
                  : WiFi.softAPIP();
    ESP_LOGI(TAG, "On http://%s", ip.toString().c_str());
}
