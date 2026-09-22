# STM32F103C8T6寄存器流水灯实验

本项目使用STM32F103C8T6 Blue Pill核心板，通过C语言直接操作RCC和GPIO寄存器，并使用软件空循环延时，实现三只外接LED流水灯，在第二版程序中加入PC13板载LED。

## 实验内容

- 第一版：PA0、PB0、PC15控制三只外接LED，以1秒为间隔循环点亮。
- 第二版：加入PC13板载LED，并根据数据手册动态切换PC13与PC15的输入、输出模式。
- 开发环境：Keil MDK、ST-Link。

## 项目文件

- [Markdown实验报告](STM32F103C8T6寄存器流水灯实验.md)
- [PDF实验报告](output/pdf/STM32F103C8T6寄存器流水灯实验.pdf)
- [第一版Keil工程ZIP](output/zip/STM32F103C8T6_三只外接LED_第一版.zip)
- [第二版Keil工程ZIP](output/zip/STM32F103C8T6_加入PC13_第二版.zip)
- `code/`：当前Keil工程及两版源代码
- `images/`：接线图片、寄存器截图和实验现象素材

## 实验博客

[CSDN：STM32F103C8T6寄存器实现流水灯](https://blog.csdn.net/2401_89763261/article/details/166245904)
