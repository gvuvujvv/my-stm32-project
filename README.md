# STM32 心电信号采集与 OLED 波形显示系统

## 1. 项目简介

本项目是一个基于 **STM32F103C8T6 微控制器** 的心电信号采集与实时显示系统。它通过 **AD8232 心电传感器** 采集人体心电信号，经过数字滤波处理后，在 **SSD1306 OLED 显示屏** 上实时绘制心电波形，并计算和显示心率信息。同时，系统还通过 **UART 串口** 输出处理后的心电数据和心率信息，方便上位机或调试。

## 2. 功能特性

*   **心电信号采集**：使用 AD8232 传感器，通过 STM32 的 ADC 模块以 500Hz 采样率进行高精度采集。
*   **数字滤波**：对原始心电信号进行高通、低通和陷波滤波处理，去除基线漂移、高频噪声和工频干扰。
*   **R 波检测**：实现 Pan-Tompkins 算法，准确检测心电信号中的 R 波峰值。
*   **心率计算**：根据 R-R 间期计算即时心率和平均心率，并判断心率状态（正常、心动过缓、心动过速）。
*   **OLED 实时显示**：在 128x64 像素的 SSD1306 OLED 屏幕上滚动显示滤波后的心电波形，并在屏幕上方显示实时心率。
*   **UART 数据输出**：通过串口实时输出处理后的心电数据（mV）和详细的心率信息，便于调试和数据分析。

## 3. 硬件连接

| 模块 | 引脚 | 描述 |
| :--- | :--- | :--- |
| **AD8232 OUTPUT** | STM32 **PA5** | 心电信号模拟输入 (ADC1 Channel 5) |
| **UART1 TX** | STM32 **PA9** | 串口发送 (连接 USB 转 TTL 模块的 RX) |
| **UART1 RX** | STM32 **PA10** | 串口接收 (连接 USB 转 TTL 模块的 TX) |
| **OLED SCL** | STM32 **PB6** | I2C1 时钟线 |
| **OLED SDA** | STM32 **PB7** | I2C1 数据线 |
| **OLED VCC/GND** | 3.3V/GND | OLED 供电 |

## 4. 软件环境

*   **开发工具**：Keil MDK (ARM Microcontroller Development Kit)
*   **微控制器**：STM32F103C8T6 (或其他 STM32F1 系列芯片)
*   **编程语言**：C 语言

## 5. 文件结构

```
my-stm32-project/
├── Library/                # STM32 标准外设库文件
├── Start/                  # 启动文件和 CMSIS 核心文件
├── User/                   # 用户应用程序代码
│   ├── main.c              # 主程序文件，系统初始化与主循环
│   ├── stm32f10x_conf.h    # STM32 库配置
│   ├── stm32f10x_it.c      # 中断服务函数实现
│   └── stm32f10x_it.h      # 中断服务函数声明
├── hd/                     # 硬件驱动和算法模块
│   ├── adc.c/.h            # ADC 驱动
│   ├── ecg_detector.c/.h   # R 波检测算法
│   ├── ecg_filter.c/.h     # 心电滤波算法
│   ├── ecg_hr.c/.h         # 心率计算
│   ├── key.c/.h            # 按键驱动 (如果存在)
│   ├── oled.c/.h           # SSD1306 OLED 驱动
│   ├── oled_wave.c/.h      # OLED 波形绘制逻辑
│   ├── oledfont.c/.h       # OLED 字体库 (如果存在)
│   ├── system.c/.h         # 系统时钟与延时函数
│   ├── timer.c/.h          # 定时器驱动 (用于采样中断)
│   └── uart.c/.h           # 串口通信驱动
├── Project.uvoptx          # Keil 工程选项文件
├── Project.uvprojx         # Keil 工程文件
└── README.md               # 项目说明文档
```

## 6. 如何使用

1.  **克隆仓库**：
    ```bash
    git clone https://github.com/gvuvujvv/my-stm32-project.git
    ```
2.  **打开 Keil 工程**：
    使用 Keil MDK 打开 `Project.uvprojx` 文件。
3.  **编译与烧录**：
    *   在 Keil 中配置好目标芯片和调试器。
    *   编译项目，生成 `.hex` 或 `.bin` 文件。
    *   将固件烧录到 STM32F103C8T6 微控制器。
4.  **连接硬件**：
    按照上述“硬件连接”部分连接 AD8232 传感器、SSD1306 OLED 显示屏和 USB 转 TTL 模块。
5.  **观察结果**：
    *   OLED 屏幕将实时显示心电波形和心率。
    *   通过串口调试助手（波特率 115200）可以接收到详细的心电数据和心率信息。

## 7. 贡献

欢迎对本项目提出改进建议或提交 Pull Request。如果您发现任何问题，请在 GitHub Issues 中提出。

## 8. 许可证

本项目采用 MIT 许可证。详情请参阅 `LICENSE` 文件 (如果存在)。
