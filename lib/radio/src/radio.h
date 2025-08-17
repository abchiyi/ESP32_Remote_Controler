#ifndef _RADIO_H_
#define _RADIO_H_
#pragma once
#include <Arduino.h>
#include <esp_now.h>
#include <cstring>
#include <vector>
#include <array>
#include <crtp.h>

// 通讯结构体
typedef struct
{
  union
  {
    struct
    {
      uint8_t data[sizeof(CRTPPacket::raw)];
      uint8_t checksum;
    };
    uint8_t raw[32];
  };

  int16_t rssi;         // 接收信号强度指示
  uint8_t is_broadcast; // 是否为广播包
} __attribute__((packed)) radio_packet_t;

typedef struct
{
  std::function<esp_err_t(radio_packet_t *)> recv; // 接收回调
  std::function<esp_err_t(radio_packet_t *)> send; // 发送回调
  std::function<bool(void)> is_connected;          // 验证连接
  std::function<esp_err_t(void)> start;            // 启动通信链路
  std::function<esp_err_t(void)> rest;             // 重置通讯链路
} radio_link_operation_t;

class RadioLink
{
public:
  virtual esp_err_t send(radio_packet_t *rp)
  {
    ESP_LOGD("RadioLink", "Not initialized, Check!!!");
    return ESP_FAIL;
  };
  virtual esp_err_t recv(radio_packet_t *rp)
  {
    ESP_LOGD("RadioLink", "Not initialized, Check!!!");
    return ESP_FAIL;
  };

  virtual bool is_connected()
  {
    ESP_LOGD("RadioLink", "Not initialized, Check!!!");
    return ESP_FAIL;
  };
  virtual esp_err_t start()
  {
    ESP_LOGD("RadioLink", "Not initialized, Check!!!");
    return ESP_FAIL;
  };
  virtual esp_err_t rest()
  {
    ESP_LOGD("RadioLink", "Not initialized, Check!!!");
    return ESP_FAIL;
  };
};

typedef std::function<void(radio_packet_t *)> radio_cb_fn;

void init_radio(); // 初始化无线通讯

esp_err_t radio_send_packet(radio_packet_t *rp);

bool radio_is_connected();

esp_err_t radio_set_port_callback(CRTPPort port, radio_cb_fn fn);

#endif
