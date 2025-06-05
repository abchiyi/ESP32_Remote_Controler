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

本项目采用修改版的 MIT 许可证。这意味着您可以：

- 自由使用、复制、修改、合并、发布、分发、再许可和/或出售本软件的副本
- 在软件副本中包含版权声明和许可声明
- 选择使用其他许可证重新许可本软件

但是您必须：

1. 在所有副本中包含原始的版权声明和许可声明
2. 在分发软件时，告知接收者本软件可以免费获取，并提供获取原始软件的信息。这个要求：
   - 适用于所有分发形式，包括使用其他许可证重新许可的情况
   - 必须包含在任何新的许可条款中
   - 在软件被重新许可或再许可后仍然有效

这个修改版的MIT许可证保持了原许可证的宽松特性，同时确保了软件的免费可得性信息能够传递给所有后续用户，无论软件是以原始许可证还是新的许可证形式分发。

完整的许可证文本请参见：[LICENSE](LICENSE)

## 贡献

欢迎提交Issue和Pull Request。

## 致谢

感谢所有为本项目做出贡献的开发者。
