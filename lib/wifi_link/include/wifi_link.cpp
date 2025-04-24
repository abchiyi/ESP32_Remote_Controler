
#include "wifi_link.h"

#include "config.h"
#include "radio.h"
#include "tool.h"

#include "esp_wifi.h"
#include "esp_now.h"
#include "esp_log.h"
#include "esp_err.h"

#include "vector"

#include "WiFi.h"

#define TAG "wifi_link"
#define TimeOutMs 80 // 链路通信超时

#define HEADER_NAME "ESP" // 从机SSID名称头，以此起始的AP将视为从机

typedef std::array<unsigned char, ESP_NOW_ETH_ALEN> mac_t; // MAC 地址

static esp_now_peer_info_t peer_info;       // 对等对象
static TickType_t wifiSendInterval = 0;     // 上次成功发包时间
QueueHandle_t queue_recv = nullptr;         // 数据接收队列
QueueHandle_t crtpPacketDelivery = nullptr; // 数据分发队列
// static bool init = false;                   // 是否初始化成功

esp_err_t pack_recv(radio_packet_t *rp);
esp_err_t pack_send(radio_packet_t *rp);
bool is_connected();
esp_err_t start();
esp_err_t rest();

typedef struct AP_info
{
    uint8_t CHANNEL;
    int8_t RSSI;
    String SSID;
    mac_t MAC;

    bool operator<(const AP_info &obj) { return RSSI < obj.RSSI; };

    bool isValidMac() const
    {
        bool allZero = true;
        bool allFF = true;
        for (auto byte : MAC)
        {
            if (byte != 0)
                allZero = false;
            if (byte != 0xFF)
                allFF = false;
        }
        return !(allZero || allFF);
    }

    String toStr()
    {
        const char Template_[] = "SSID: %s, MAC:" MACSTR ", RSSI: %d, Channel: %d";
        char temp[std::strlen(Template_) + 30];
        sprintf(temp, Template_, SSID, MAC2STR(MAC), RSSI, CHANNEL);
        return String(temp);
    }
} ap_info_t;

enum enum_wifi_link_status
{
    wl_DISCONNECTED,
    wl_CONNECTED,
};

static struct
{
    int16_t RSSI;
    enum_wifi_link_status status = wl_DISCONNECTED;
} LINK_INFO;

static ap_info_t DeviceAPInfo = {};

radio_link_operation_t _RLOP{
    .recv = pack_recv,
    .send = pack_send,
    .is_connected = is_connected,
    .start = start,
    .rest = rest};

void promiscuous_rx_cb(void *buff, wifi_promiscuous_pkt_type_t type);

void esp_now_on_recv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
    if (incomingData == nullptr)
        return;

    ESP_LOGD(TAG, "Recv on " MACSTR "", MAC2STR(mac));

    static radio_packet_t rp;
    if (len > sizeof(rp))
    {
        ESP_LOGD(TAG, "Recv data 长度超过 32字节, len:%d", len);
        return;
    }

    memset(&rp, 0, sizeof(rp));
    memcpy(&rp, incomingData, sizeof(rp));

    static auto ret = xQueueSend(queue_recv, &rp, 0);
    if (ret != pdPASS)
        ESP_LOGE(TAG, "发送到队列失败, error code:%s", esp_err_to_name(ret));
}

void esp_now_on_send(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    static TickType_t lastRecvTime = 0;
    TickType_t currentTime = xTaskGetTickCount();

    if (lastRecvTime != 0)
        wifiSendInterval = currentTime - lastRecvTime;
    if (status == ESP_NOW_SEND_SUCCESS)
        lastRecvTime = currentTime;
    // else
    //     ESP_LOGE(TAG, "Send to " MACSTR " FAIl", MAC2STR(mac_addr));

    // ESP_LOGI(TAG, "Send to " MACSTR " SUCCESS", MAC2STR(mac_addr));
}

/**
 *  处理设备配对和收包
 */
void wifi_link_task(void *pvParameters)
{
    static auto AP = &DeviceAPInfo;

    static radio_packet_t rp = {};

    while (true)
    {
        switch (LINK_INFO.status)
        {
        case wl_DISCONNECTED: // 没有连接时寻找设备连接
            [&]()
            {
                std::vector<ap_info_t> aps;

                while (true) // 扫描信道0~13
                {
                    for (size_t ch = 1; ch < 14; ch++)
                    {
                        auto scanResults = WiFi.scanNetworks(0, 0, 0, 50, ch);
                        if (!scanResults) // 没有扫描到AP跳转到下一信道扫描
                            continue;
                        for (size_t i = 0; i < scanResults; i++)

                            // 从扫描结果中筛选出符合要求的AP
                            if (WiFi.SSID(i).indexOf(HEADER_NAME) == 0)
                            {
                                ap_info_t ap;
                                ap.RSSI = WiFi.RSSI(i),
                                ap.SSID = WiFi.SSID(i),
                                ap.CHANNEL = WiFi.channel(i),
                                memcpy(&ap.MAC, WiFi.BSSID(i), sizeof(mac_t));

                                aps.push_back(ap);
                            };

                        WiFi.scanDelete(); // 清空当前信道扫描结果
                    }

                    if (!aps.empty())
                        break;
                    ESP_LOGI(TAG, "No device found, try again");
                }

                std::sort(aps.begin(), aps.end());
                *AP = aps[aps.size() - 1]; // 选择信号最强的AP

                ESP_LOGI(TAG, "WL_FIND_DEVICE: %s, RSSI: %d, CHANNEL: %d",
                         AP->SSID.c_str(), AP->RSSI, AP->CHANNEL);

                memset(&peer_info, 0, sizeof(esp_now_peer_info_t));
                memcpy(peer_info.peer_addr, AP->MAC.data(), ESP_NOW_ETH_ALEN);
                peer_info.channel = AP->CHANNEL;
                peer_info.ifidx = WIFI_IF_STA;
                peer_info.encrypt = false;

                auto ret = esp_now_add_peer(&peer_info);
                if (ret != ESP_OK)
                {
                    ESP_LOGE(TAG, "Add peer fail, error code:%s", esp_err_to_name(ret));
                    memset(&peer_info, 0, sizeof(esp_now_peer_info_t));
                }

                ESP_ERROR_CHECK(
                    esp_wifi_set_channel(AP->CHANNEL, WIFI_SECOND_CHAN_NONE));

                LINK_INFO.status = wl_CONNECTED; // 连接成功
                ESP_LOGI(TAG, "Connected devices '%s' success",
                         AP->SSID.c_str());
            }();
            break;
        case wl_CONNECTED:
            if (xQueueReceive(queue_recv, &rp, portMAX_DELAY) == pdTRUE)
            {
                // TODO 如果数据来自 esp-now 则不校验
                // 验证数据是否有效
                if (rp.checksum != calculate_cksum(rp.data, sizeof(rp.data) - 1))
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

void wifi_link_init()
{
    // set wifi
    ESP_LOGI(TAG, "Init wifi");
    WiFi.enableLongRange(true);

    // 设置模式 STA
    if (WiFi.mode(WIFI_STA))
        ESP_LOGI(TAG, "WIFI Start in STA,MAC: %s, CHANNEL: %u",
                 WiFi.macAddress().c_str(), WiFi.channel());
    else
        esp_system_abort("WIFI Start FAIL");

    // 开启混杂模式
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(&promiscuous_rx_cb));

    // 设置最大 TX Power 到 20db
    ESP_ERROR_CHECK(esp_wifi_set_max_tx_power(84));

    // 设置 ESPNOW 通讯速率
    esp_wifi_config_espnow_rate(WIFI_IF_STA, WIFI_PHY_RATE_LORA_500K) == ESP_OK
        ? ESP_LOGI(TAG, "Set ESPNOW WIFI_PHY_RATE_LORA_500K")
        : ESP_LOGI(TAG, "Set ESPNOW RATE FAIL");

    // 设置 ESPNOW
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_recv_cb(esp_now_on_recv));
    ESP_ERROR_CHECK(esp_now_register_send_cb(esp_now_on_send));

    queue_recv = xQueueCreate(10, sizeof(radio_packet_t));
    if (queue_recv == nullptr)
        esp_system_abort("Create queue_recv fail");

    xTaskCreate(wifi_link_task, "wifi_link_task", 4096, NULL, TP_H, NULL);

    ESP_LOGI(TAG, "wifi link init success");
}

IRAM_ATTR void promiscuous_rx_cb(void *buff, wifi_promiscuous_pkt_type_t type)
{
    if (type != WIFI_PKT_MGMT)
        return;

    static wifi_promiscuous_pkt_t *ppkt = (wifi_promiscuous_pkt_t *)buff;
    static mac_t mac = {};

    memcpy(mac.data(), (ppkt->payload + 10), sizeof(mac_t));
    // 满足条件时更新 RSSI
    if (DeviceAPInfo.isValidMac() && DeviceAPInfo.MAC == mac)
        LINK_INFO.RSSI = ppkt->rx_ctrl.rssi;
}

// link operation
IRAM_ATTR esp_err_t pack_recv(radio_packet_t *rp)
{
    xQueueReceive(queue_recv, rp, portMAX_DELAY);
    return ESP_OK;
}

IRAM_ATTR esp_err_t pack_send(radio_packet_t *rp)
{
    static BaseType_t ret;
    if (LINK_INFO.status != wl_CONNECTED)
        return ESP_ERR_NOT_FINISHED;

    return esp_now_send(peer_info.peer_addr, rp->raw, sizeof(*rp));
    // ret = esp_now_send(peer_info.peer_addr, rp->raw, sizeof(*rp));
    // if (ret != ESP_OK)
    // {
    //     ESP_LOGE(TAG, "esp_now_send fail, error: %s", esp_err_to_name(ret));
    //     ESP_LOGE(TAG, "Devices info: Mac " MACSTR ", Channel %d",
    //              MAC2STR(peer_info.peer_addr),
    //              peer_info.channel);
    // }
    //
    // return ret;
}

esp_err_t start()
{
    wifi_link_init();

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

void start_wifi_link()
{
    init_radio(&_RLOP);
}

esp_err_t rest()
{

    return ESP_OK;
};

radio_link_operation_t *get_link()
{
    return &_RLOP;
}
