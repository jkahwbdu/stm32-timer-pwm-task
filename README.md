# STM32 定时器与 PWM 实验（Timer_PWM_Task）

第 4-5 周作业：使用 STM32F103C8T6 的通用定时器 TIM2 完成两个任务。

## 实验内容

| 任务 | 实现方式 | 效果 |
| --- | --- | --- |
| 任务一 | TIM2 更新中断，PSC=71、ARR=999（1 ms），累计 2000 次 | LED 以 **2 s 周期**亮-灭 |
| 任务二 | TIM2_CH2 PWM（1 kHz），主循环按二次曲线修改 CCR2 | LED **呼吸灯**，完整周期设计值 1.4 s（实测约 1.6 s） |

两个任务写在同一个工程中，通过 `Core/Src/main.c` 顶部的宏切换：

```c
#define APP_TASK   TASK_2_PWM_BREATH    /* 改为 TASK_1_TIMER_COUNT 即任务一 */
```

## 硬件连接

- MCU：STM32F103C8T6 最小系统板
- LED：**PA1**（TIM2_CH2），高电平点亮
- 调试/下载：DAP-Link（CMSIS-DAP）
  - SWDIO → PA13
  - SWCLK → PA14
  - GND → GND
  - 3V3 → 3V3

## 时钟与关键参数

- HSE 8 MHz × PLL9 = **72 MHz**
- APB1 分频 2 → PCLK1 = 36 MHz，TIM2CLK = **72 MHz**
- PSC = 71 → 计数时钟 1 MHz
- ARR = 999 → 更新周期 1 ms，PWM 频率 **1 kHz**
- 占空比 = CCR2 / 1000

## 目录结构

```
Timer_PWM_Task/
├── Timer_PWM_Task.ioc              # CubeMX 配置
├── Core/
│   ├── Inc/                        # main.h / hal_conf / it.h
│   └── Src/                        # main.c / msp.c / it.c
├── Drivers/                        # CMSIS + STM32F1xx HAL
├── MDK-ARM/
│   ├── Timer_PWM_Task.uvprojx      # Keil 工程（ARMCC V5）
│   └── Timer_PWM_Task/             # 编译产物（.hex/.axf）
└── docs/
    ├── STM32定时器与PWM实验.md      # 实验报告 Markdown 源码
    ├── STM32定时器与PWM实验.pdf     # 实验报告 PDF
    ├── images/                     # 波形图
    └── make_figs.ps1               # 波形图生成脚本
```

## 编译

Keil MDK-ARM V5 打开 `MDK-ARM/Timer_PWM_Task.uvprojx`，F7 编译。

命令行编译：

```powershell
$uv = 'C:\Keil_v5\UV4\UV4.exe'
Start-Process $uv -ArgumentList @('-r', 'MDK-ARM\Timer_PWM_Task.uvprojx', '-j0', '-o', 'build.log') -Wait
```

编译结果：`0 Error(s), 0 Warning(s)`，Code = 4668 字节。

## 实验现象

1. **任务一**：LED 亮 2 秒、灭 2 秒，循环往复。
2. **任务二**：LED 渐亮渐暗，一个呼吸周期设计值 1.4 秒、实测约 1.6 秒；PWM 载波为 1 kHz，25%/50%/75% 占空比对应高电平 0.25/0.5/0.75 ms。

详细原理、代码分析与实验数据见 [`docs/STM32定时器与PWM实验.pdf`](docs/STM32定时器与PWM实验.pdf)。

## 备注

- STM32F103C8T6 只有 TIM2/TIM3/TIM4，**没有 TIM5**。
- Keil 的 STM32F1 器件仿真不含 TIM/GPIO 行为模型，逻辑分析仪无法采到实际 PWM 波形；本实验用真板子 + Watch 窗口读取 `TIM2->CCR2/CNT/ARR` 进行验证。
