/**
 * @file wifi_link.cpp
 * @brief ESP32远程控制器的WiFi链路管理模块，基于ESP-NOW协议实现主从设备的自动配对与数据通信。
 *
 * 本模块负责：
 *  - 设备主从配对流程的实现（广播、握手、确认等）
 *  - 通过ESP-NOW进行无线数据包的收发
 *  - 链路状态的维护与检测（连接/断开）
 *  - WiFi模式的自动切换（AP/STA）
 *  - 数据包的CRC校验
 *  - 信道切换与重试机制
 *  - 与FreeRTOS队列和任务的集成
 *
 * 主要类型与函数说明：
 *  - mac_t：MAC地址类型，长度为ESP_NOW_ETH_ALEN
 *  - bcast_packet_t：广播包结构体，包含主从MAC地址和原始数据
 *  - linkStatus_t：链路状态枚举（已连接/未连接）
 *  - pack_recv/pack_send：无线数据包的接收与发送接口
 *  - is_connected：检测链路是否保持连接
 *  - start：初始化WiFi与ESP-NOW，启动链路管理任务
 *  - disconnect_at_master/disconnect_at_slave：主/从设备配对流程
 *  - wifi_link_task：链路管理主任务，处理配对与数据分发
 *  - packetCrcCheck：数据包CRC校验
 *  - data_recv/data_sent：QuickESPNow回调，处理数据收发事件
 *
 * 注意事项：
 *  - 需确保CONFIG、radio.h、tool.h等依赖已正确配置
 *  - 需在支持ESP-NOW的ESP32平台上编译运行
 *  - 任务栈大小和队列长度可根据实际需求调整
 *
 * @author meme_me2019
 * @date 2025-06-04
 */
#include "wifi_link.h"

#include "config.h"
#include "radio.h"
#include "tool.h"

#include "esp_wifi.h"
#include "esp_now.h"
#include "esp_log.h"
#include "esp_err.h"

#include "vector"
#include "atomic"

#include "WiFi.h"
#include "Arduino.h"
#include "QuickEspNow.h"

#define TAG "wifi_link"
typedef std::array<unsigned char, ESP_NOW_ETH_ALEN> mac_t; // MAC地址类型
typedef struct
{
    union
    {
        struct // 主机和从机的MAC地址
        {
            mac_t MAC_MASTER;
            mac_t MAC_SLAVE;
        };
        uint8_t raw[CRTP_MAX_DATA_SIZE]; // raw数据
    };
} __attribute__((packed)) bcast_packet_t; // 广播包结构类型
typedef enum
{
    wl_DISCONNECTED,
    wl_CONNECTED,
} linkStatus_t;
static std::atomic<linkStatus_t> linkStatus(wl_DISCONNECTED); // 链路状态
static std::atomic<int16_t> lastRSSI(0);                      // 最后的RSSI值

#define TimeOutMs 80                               // 链路通信超时
static mac_t MAC_TARGET;                           // 目标MAC地址
static TickType_t wifiSendInterval = 0;            // 上次成功发包时间
static QueueHandle_t radioPackRecv = nullptr;      // 数据接收队列
static QueueHandle_t crtpPacketDelivery = nullptr; // 数据分发队列

esp_err_t pack_recv(radio_packet_t *rp);
esp_err_t pack_send(radio_packet_t *rp);
bool is_connected();
esp_err_t start();
esp_err_t rest();

radio_link_operation_t _RLOP{
    .recv = pack_recv,
    .send = pack_send,
    .is_connected = is_connected,
    .start = start,
    .rest = rest};

/**
 * @brief 检查数据包的CRC校验和
 *
 * @param rp 待检查的无线数据包结构体引用
 * @return true 校验和匹配
 * @return false 校验和不匹配
 */
bool packetCrcCheck(const radio_packet_t &rp)
{
    auto crc = calculate_cksum((void *)rp.raw, sizeof(rp.raw) - 1);
    auto ret = rp.checksum == crc;
    if (!ret)
        ESP_LOGE(TAG, "CRC ERROR, expected: %d, got: %d", crc, rp.checksum);

    return ret;
}

// QuickESPNow回调函数
void data_recv(uint8_t *mac, const uint8_t *data, uint8_t len, signed int rssi, bool broadcast)
{
    lastRSSI.store(rssi);
    if (data == nullptr)
        return;
    auto rp = (radio_packet_t *)data;

    // ESP_LOGI(TAG, "From " MACSTR ", len:%d, rssi:%d, isBroadcast:%d",
    //          MAC2STR(mac), len, rssi, broadcast);

    if (len > sizeof(radio_packet_t))
    {
        ESP_LOGW(TAG, "Recv data 长度超过 32字节, len:%d", len);
        return;
    }

    auto ret = xQueueSend(radioPackRecv, (radio_packet_t *)data, 0);
    if (ret != pdTRUE)
        ESP_LOGE(TAG, "发送到队列失败, error code:%s", esp_err_to_name(ret));
}

// QuickESPNow回调函数
void data_sent(uint8_t *address, uint8_t status)
{
    static TickType_t lastRecvTime = 0;
    TickType_t currentTime = xTaskGetTickCount();

    if (lastRecvTime != 0)
        wifiSendInterval = currentTime - lastRecvTime;
    if (status == COMMS_SEND_OK)
        lastRecvTime = currentTime;
}

void disconnect_at_master()
{
    while (linkStatus.load() == wl_DISCONNECTED)
    {

        radio_packet_t rp = {};
        auto cp = (CRTPPacket *)rp.data;
        auto bp = (bcast_packet_t *)cp->data;
        cp->channel = 0;
        cp->port = CRTP_PORT_LINK;
        WiFi.macAddress((uint8_t *)bp->MAC_MASTER.data());
        rp.checksum = calculate_cksum(rp.data, sizeof(rp.data) - 1);

        // * STEP 1 在当前信道广播数据包 100ms/次
        quickEspNow.sendBcast(rp.raw, sizeof(rp.raw));
        if (xQueueReceive(radioPackRecv, &rp, 100) == pdTRUE)
        {
            // * STEP 2 当收到从机回复时向从机原样返回数据包
            if (!packetCrcCheck(rp))
                ESP_LOGE(TAG, "Packet from SLAVE ,CRC ERROR");
            else
            {
                // * STEP 3 向从机发送数据包,发送失败重试3次否则回到广播模式
                uint8_t retry_count = 3;
                while (retry_count > 0) // 回复到slave
                {
                    if (quickEspNow.send(
                            bp->MAC_SLAVE.data(),
                            rp.raw,
                            sizeof(rp.raw)))
                        continue; // 如果发送失败则重试

                    // 当发送成功时设置标志位及相关事务
                    linkStatus.store(wl_CONNECTED);
                    MAC_TARGET = bp->MAC_SLAVE;
                    ESP_LOGI(TAG, "Connected to Slave: " MACSTR, MAC2STR(bp->MAC_SLAVE.data()));
                    break; // 成功连接，跳出循环
                }
            }
        };
    }
};

void disconnect_at_slave()
{
    radio_packet_t rp = {};
    static auto cp = (CRTPPacket *)rp.data;
    static auto bp = (bcast_packet_t *)cp->data;

    uint8_t channel = 13;
    while (linkStatus.load() == wl_DISCONNECTED)
    {
        // 循环切换信道
        auto ret = quickEspNow.setChannel(channel, WIFI_SECOND_CHAN_NONE);
        channel == 0 ? channel-- : channel = 13;

        // * STEP 1 在每个通道等待100ms, 以监听是否有主机在当前通道广播
        if (xQueueReceive(radioPackRecv, &rp, 100) == pdTRUE)
        {
            ESP_LOGI(TAG, "Received broadcast packet on channel %d", channel);
            // 如果数据包CRC校验失败则跳过
            if (!packetCrcCheck(rp))
            {
                ESP_LOGE(TAG, "CRC ERROR in broadcast receive");
                continue;
            }
            /**
             *  向主机回复信息并等待主机确认连接
             *  成功发送或重试次数超时则重新进入守听状态
             */

            // * STEP 2 当收到广播数据包时向包内写入本机的MAC地址并回复到主机

            // 临时存储主机的MAC地址，用于比对两次接收的包是否来自同一主机
            mac_t temp_target_mac = {0};
            temp_target_mac = bp->MAC_MASTER;
            uint16_t retry_count = 3;
            WiFi.macAddress(bp->MAC_SLAVE.data()); // 设置从机的MAC地址
            rp.checksum = calculate_cksum(rp.data, sizeof(rp.data) - 1);
            while (retry_count > 0)
            {
                ESP_LOGI(TAG, "Return data to master");

                // 如果发送失败则重试
                if (quickEspNow
                        .send(bp->MAC_MASTER.data(), rp.raw, sizeof(rp.raw)))
                {
                    ESP_LOGW(TAG, "Failed to send data to master, retrying");
                    vTaskDelay(10);
                    retry_count--;
                    continue;
                }

                // * STEP 3 等待主机回复以确认连接,等待3次,总计600ms
                // * 如超时则判断当前连接无效,重新进入守听模式
                if (xQueueReceive(radioPackRecv, &rp, 200) == pdTRUE)
                {
                    ESP_LOGI(TAG, "Received response from master: " MACSTR,
                             MAC2STR(bp->MAC_MASTER.data()));

                    mac_t this_device_mac = {0};
                    WiFi.macAddress(this_device_mac.data());
                    auto crcCheckOk = packetCrcCheck(rp);
                    auto macCheckOk_master = bp->MAC_MASTER == temp_target_mac;
                    auto macCheckOk_slave = bp->MAC_SLAVE == this_device_mac;

                    // * STEP 4 如果收到主机回复，则连接成功
                    if (crcCheckOk && macCheckOk_master && macCheckOk_slave)
                    {
                        linkStatus.store(wl_CONNECTED);
                        MAC_TARGET = bp->MAC_MASTER;
                        ESP_LOGI(TAG, "Connected to master: " MACSTR, MAC2STR(bp->MAC_MASTER.data()));
                        break; // 成功连接，跳出循环
                    }
                    else
                        ESP_LOGE(TAG, "Master data check failed, CRC: %s, MAC_MASTER:[" MACSTR "], MAC_SLAVE:[" MACSTR "]",
                                 crcCheckOk ? "OK" : "ERROR",
                                 MAC2STR(bp->MAC_MASTER.data()),
                                 MAC2STR(bp->MAC_SLAVE.data()));
                }

                ESP_LOGE(TAG, "No response from master, retrying[%d]",
                         retry_count);
                retry_count--;
            };
        };
    };
};

/**
 *  处理设备配对和收包
 */
void wifi_link_task(void *pvParameters)
{
    static radio_packet_t rp = {};

    while (true)
    {
        switch (linkStatus.load())
        {
        case wl_DISCONNECTED:
            ESP_LOGI(TAG, "Control Mode:[ %s ]",
                     CONFIG.control_mode == MASTER ? "MASTER" : "SLAVE");
            CONFIG.control_mode == MASTER
                ? disconnect_at_master()
                : disconnect_at_slave();
            break;

        case wl_CONNECTED:
            if (xQueueReceive(radioPackRecv, &rp, portMAX_DELAY) == pdTRUE)
            {
                // TODO 如果数据来自 esp-now 则不校验
                // 验证数据是否有效
                if (!packetCrcCheck(rp))
                {
                    ESP_LOGE(TAG, "CRC ERROR");
                    continue;
                }
                xQueueSend(crtpPacketDelivery, &rp.data, pdMS_TO_TICKS(5));
            }
            break;
        default:
            break;
        }
    }
}

IRAM_ATTR esp_err_t pack_recv(radio_packet_t *rp)
{
    xQueueReceive(crtpPacketDelivery, rp, portMAX_DELAY);
    return ESP_OK;
}

IRAM_ATTR esp_err_t pack_send(radio_packet_t *rp)
{
    static BaseType_t ret;
    if (linkStatus.load() != wl_CONNECTED)
        return ESP_ERR_NOT_FINISHED;

    return quickEspNow.send(MAC_TARGET.data(), rp->raw, sizeof(*rp));
}

esp_err_t start()
{
    /*** WiFi ***/
    /**
     *  选择WiFi 启动模式
     *   1. 未配置SSID和密码，启动AP模式
     *   2. 配置了SSID和密码，启动STA模式
     *   3. STA模式下连接失败，启动AP模式
     */
    auto start_on_AP = [&]()
    {
        mac_t mac = {0};
        char ssid[32] = {0};
        ESP_ERROR_CHECK(WiFi.mode(WIFI_AP_STA) ? ESP_OK : ESP_FAIL);
        WiFi.softAPmacAddress(mac.data()); // 设置AP的MAC地址
        sprintf(ssid, "ESP32-%02X:%02X:%02X", mac[3], mac[4], mac[5]);
        WiFi.softAP(ssid, NULL, 1, 0, 4, 0);
        ESP_LOGI(TAG, "Create open AP: %s, on channel: 1", ssid);
    };

    if (CONFIG.WIFI_SSID[0] != '\0')
    {
        ESP_ERROR_CHECK(WiFi.mode(WIFI_STA) ? ESP_OK : ESP_FAIL);
        WiFi.begin(CONFIG.WIFI_SSID, CONFIG.WIFI_PASS);

        uint8_t counter = 10;
        Serial.printf("Connecting to WiFi: %s", CONFIG.WIFI_SSID);
        while (WiFi.status() != WL_CONNECTED)
        {
            delay(500);
            counter--;
            if (!counter)
                break;
        }
        if (counter)
            ESP_LOGI(TAG, "Connected to WiFi: %s", CONFIG.WIFI_SSID);
        else
            start_on_AP();
    }
    else
        start_on_AP();

    WiFi.setTxPower(WIFI_POWER_19_5dBm); // 设置最大 TX Power 到 20db

    /*** ESP-NOW ***/
    quickEspNow.onDataRcvd(data_recv);
    quickEspNow.onDataSent(data_sent);
    quickEspNow.begin(WiFi.channel(), WIFI_IF_STA, false);

    // // 设置 ESPNOW 通讯速率
    // esp_wifi_config_espnow_rate(WIFI_IF_STA, WIFI_PHY_RATE_LORA_500K) == ESP_OK
    //     ? ESP_LOGI(TAG, "Set ESPNOW WIFI_PHY_RATE_LORA_500K")
    //     : ESP_LOGI(TAG, "Set ESPNOW RATE FAIL");
    // 设置 ESPNOW

    radioPackRecv = xQueueCreate(10, sizeof(radio_packet_t));
    configASSERT(radioPackRecv != nullptr);

    crtpPacketDelivery = xQueueCreate(10, sizeof(radio_packet_t::data));
    configASSERT(crtpPacketDelivery != nullptr);

    xTaskCreate(wifi_link_task, "wifi_link_task", 1024 * 10, NULL, TP_H, NULL);

    return ESP_OK;
}

/**
 * @brief 检查当前 Wi-Fi 连接状态。
 *
 * 该函数通过比较发送间隔时间与超时时间来判断连接是否保持。
 * 如果发送间隔时间不为 0 且不大于超时时间，则认为连接依然有效。
 *
 * @return true 表连接保持，false 表连接可能失效。
 */
bool is_connected()
{
    return wifiSendInterval <= TimeOutMs && wifiSendInterval;
}

esp_err_t rest()
{

    return ESP_OK;
};

radio_link_operation_t *WiFi_Esp_Now()
{
    return &_RLOP;
}
