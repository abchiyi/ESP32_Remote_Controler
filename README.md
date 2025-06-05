# ESP32 Remote Controller

基于ESP32的多功能遥控器项目，支持XBOX蓝牙控制器输入和Web控制台监控 。

## 功能特点

### 硬件控制
- XBOX蓝牙控制器支持
  - 完整按键映射（A/B/X/Y/LB/RB等）
  - 双摇杆模拟输入
  - LT/RT模拟扳机
  - 方向键支持

### 无线通信
- ESP-NOW
  - 低延迟点对点通信
  - RSSI信号强度监测

### Web控制台
- 基于Vue 3 + TypeScript开发
- 实时姿态显示
  - 俯仰角（Pitch）[TODO]
  - 横滚角（Roll）[TODO]
  - 偏航角（Yaw）[TODO]
- 设置界面

## 项目结构

```
ESP32_Remote_Controler/
├── src/                    # 主程序源码
├── lib/                    # 库文件
│   ├── BAT/               # 电池管理模块
│   ├── ESP_BT_Controller/ # 蓝牙控制器模块
│   ├── esp_crtp/          # 通信协议模块
│   ├── LED/               # LED控制模块
│   ├── motor/             # 电机控制模块
│   ├── radio/             # 无线通信模块
│   ├── Tool/              # 工具函数
│   └── wifi_link/         # WiFi连接模块
└── ESP_Receiver_WEB_colsole/  # Web控制台
    ├── src/
    │   ├── component/     # Vue组件
    │   └── view/          # 页面视图
    └── public/            # 静态资源
```

## 快速开始

### 硬件要求
- ESP32开发板
- XBOX蓝牙控制器[可选]

### 开发环境
- PlatformIO
- Node.js >= 14.0.0
- Vue 3
- TypeScript
- ESP32 Arduino Core

### 构建步骤

1. 克隆项目
```bash
git clone [repository-url]
cd ESP32_Remote_Controler
```

2. 编译固件
```bash
pio run
pio run --target upload
```

3. 部署Web控制台
```bash
cd ESP_Receiver_WEB_colsole
npm install
npm run build
```

## 使用说明

1. 上电后，设备自动进入配置的工作模式
2. 长按XBOX按钮开启控制器，等待配对
3. LED指示灯显示连接状态
4. 通过Web控制台监控设备状态

## 许可证

本项目采用修改版的 GNU 许可证。
完整的许可证文本请参见：[LICENSE](LICENSE)

## 贡献

欢迎提交Issue和Pull Request。

## 致谢

感谢所有为本项目做出贡献的开发者。
