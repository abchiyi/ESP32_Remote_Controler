#include "web_console.h"
#include "config.h"
#include "tool.h"

#include "Arduino.h"

#include "WebSocketsServer.h"
#include "WebServer.h"
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"

#define TAG "Web Console"
#define PORT_HTTP 80
#define PORT_WS 81

#define SSID "CCCP"
#define PASS "Madin133"

WebSocketsServer webSocket = WebSocketsServer(PORT_WS);
// WebServer server(PORT_HTTP);
AsyncWebServer server(PORT_HTTP);

// 内容类型映射表
const char *getContentType(String filename)
{
    if (filename.endsWith(".html"))
        return "text/html";
    if (filename.endsWith(".css"))
        return "text/css";
    if (filename.endsWith(".js"))
        return "application/javascript";
    if (filename.endsWith(".webp"))
        return "image/webp";
    if (filename.endsWith(".jpg") || filename.endsWith(".jpeg"))
        return "image/jpeg";
    if (filename.endsWith(".png"))
        return "image/png";
    if (filename.endsWith(".gif"))
        return "image/gif";
    if (filename.endsWith(".ico"))
        return "image/x-icon";
    if (filename.endsWith(".svg"))
        return "image/svg+xml";
    return "text/plain";
}

// // 处理静态文件请求
// bool handleFileRead(String path)
// {
//     if (path.endsWith("/"))
//         path += "index.html";

//     String contentType = getContentType(path);
//     ESP_LOGI(TAG, "contentType: %s", contentType.c_str());
//     if (SPIFFS.exists(path))
//     {
//         File file = SPIFFS.open(path, "r");
//         server.streamFile(file, contentType);
//         file.close();
//         return true;
//     }
//     return false;
// }

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
        // 可以在此处理客户端发送的消息
        break;
    }
}

// // HTTP 静态资源路由
// void match_all()
// {
//     ESP_LOGI(TAG, "url: %s", server.uri().c_str());
//     if (!handleFileRead(server.uri()))
//         server.send(404, "text/html", "404 not found");
// };

void init_web_console()
{
    if (!SPIFFS.begin(true))
    {
        ESP_LOGE(TAG, "SPIFFS mount failed!");
        return;
    }

    // 连接WiFi
    WiFi.begin(SSID, PASS);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
    }
    ESP_LOGI(TAG, "WiFi connected");
    ESP_LOGI(TAG, "Console on http://%s", WiFi.localIP().toString().c_str());

    // 设置静态资源服务器
    server
        .serveStatic("/", SPIFFS, "/")   // 设置静态文件目录
        .setCacheControl("max-age=3600") // 设置缓存时间
        .setDefaultFile("index.html");

    server.onNotFound([](AsyncWebServerRequest *request) // 处理404错误
                      { request->send(404, "text/plain", "404 Not Found"); });
    server.begin(); // 启动HTTP服务器

    ESP_LOGI(TAG, "HTTP server started, listening on port %d", PORT_HTTP);
    webSocket.onEvent(webSocketEvent); // 设置WebSocket事件处理函数
    webSocket.begin();                 // 启动WebSocket服务器

    // WEB 控制台任务
    auto wct = [](void *pvParameters)
    {
        TickType_t xLastWakeTime = xTaskGetTickCount();
        const TickType_t xFrequency = HZ2TICKS(200);
        while (true)
        {
            webSocket.loop();
            // server.handleClient();
            vTaskDelayUntil(&xLastWakeTime, xFrequency);
        }
    };
    auto ret = xTaskCreate(wct, "WebConsole", 4096, NULL, TP_HIGHEST, NULL);
    ESP_ERROR_CHECK(ret == pdPASS ? ESP_OK : ESP_FAIL);
}

/**
 *
 *
 *
 *
 *
 * void sendToAllClients(const char *message)
{
  webSocket.broadcastTXT(message);
}
 *
 *
 *  webSocket.loop();
  server.handleClient();

  // 模拟数据发送（实际使用时替换为真实传感器数据）
  static unsigned long lastSend = 0;
  if (millis() - lastSend > 1000)
  {
    String data = "当前值: " + String(random(0, 100));
    webSocket.broadcastTXT(data);
    lastSend = millis();
  }
 *
 */
