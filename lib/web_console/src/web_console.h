#ifndef _WEB_CONSOLE_H_
#define _WEB_CONSOLE_H_

#include "ArduinoJson.h"
#include "functional"
#include <vector>

typedef enum
{
    WEB_PORT_CONFIG, // 配置端口
    __PORT_MAX,      // 端口最大值,仅作为占位符
} web_console_port;

#define WEB_CONSOLE_R 0X00
#define WEB_CONSOLE_W 0X01

typedef JsonDocument json_doc;
typedef std::function<void(json_doc)> web_console_callback_t;
bool areAllKeysPresent(const json_doc &doc,
                       const std::vector<const char *> &indices);

bool register_web_console_callback(web_console_port, web_console_callback_t);
void init_web_console(void);

/**
 * * @brief 发送数据到
 */
void web_console_send(String &JsonString);

#endif
