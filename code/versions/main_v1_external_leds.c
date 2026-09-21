#include "stm32f10x.h"

/* 将指定地址转换为可读写的32位硬件寄存器。
 * volatile用于防止编译器省略或合并对硬件寄存器的访问。
 */
#define REG32(address)              (*(volatile unsigned int *)(address))

/* RCC与GPIO寄存器的绝对地址，均由“外设基地址+寄存器偏移地址”得到。 */
#define RCC_APB2ENR_REG             REG32(0x40021018u) /* APB2时钟使能。 */
#define GPIOA_CRL_REG               REG32(0x40010800u) /* 配置PA0～PA7。 */
#define GPIOB_CRL_REG               REG32(0x40010C00u) /* 配置PB0～PB7。 */
#define GPIOC_CRH_REG               REG32(0x40011004u) /* 配置PC8～PC15。 */
#define GPIOA_BSRR_REG              REG32(0x40010810u) /* 控制GPIOA输出。 */
#define GPIOB_BSRR_REG              REG32(0x40010C10u) /* 控制GPIOB输出。 */
#define GPIOC_BSRR_REG              REG32(0x40011010u) /* 控制GPIOC输出。 */

/* Cortex-M3 SysTick寄存器的绝对地址。 */
#define SYSTICK_CTRL_REG            REG32(0xE000E010u) /* 控制及状态寄存器。 */
#define SYSTICK_LOAD_REG            REG32(0xE000E014u) /* 重装载值寄存器。 */
#define SYSTICK_VALUE_REG           REG32(0xE000E018u) /* 当前计数值寄存器。 */

/* 使用SysTick产生约1秒的阻塞延时。 */
static void delay_1s(void)
{
    SYSTICK_CTRL_REG = 0u; /* 清除ENABLE位，配置前先关闭SysTick。 */

    /* 72 MHz除以8得到9 MHz；计数器从LOAD递减到0共经历LOAD+1个周期，
     * 因此1秒延时的LOAD值为9 000 000-1。
     */
    SYSTICK_LOAD_REG = SystemCoreClock / 8u - 1u;
    SYSTICK_VALUE_REG = 0u; /* 清零当前计数器和COUNTFLAG。 */

    /* ENABLE=1启动计数，TICKINT=0不产生中断，CLKSOURCE=0选择HCLK/8。 */
    SYSTICK_CTRL_REG = 1u;

    /* CTRL寄存器位16为COUNTFLAG，计数到0后由硬件置1。 */
    while ((SYSTICK_CTRL_REG & (1u << 16)) == 0u)
    {
    }

    SYSTICK_CTRL_REG = 0u;                        /* 延时结束后关闭SysTick。 */
}

int main(void)
{
    /* 位2～位4分别控制GPIOA～GPIOC；“|=”可保留寄存器其他位。 */
    RCC_APB2ENR_REG |= (1u << 2) | (1u << 3) | (1u << 4);

    /* PA0对应CRL位3:0。先清除原配置，再写入0010：CNF=00、MODE=10。 */
    GPIOA_CRL_REG &= ~(0x0Fu << 0);
    GPIOA_CRL_REG |=  (0x02u << 0);

    /* PB0同样对应GPIOB_CRL位3:0，配置为2 MHz通用推挽输出。 */
    GPIOB_CRL_REG &= ~(0x0Fu << 0);
    GPIOB_CRL_REG |=  (0x02u << 0);

    /* PC15对应GPIOC_CRH位31:28，因此掩码和配置值均左移28位。 */
    GPIOC_CRH_REG &= ~(0x0Fu << 28);
    GPIOC_CRH_REG |=  (0x02u << 28);

    /* BSRR高16位BR写1可输出低电平，初始化时先熄灭全部LED。 */
    GPIOA_BSRR_REG = (1u << 16); /* BR0=1，PA0输出低电平。 */
    GPIOB_BSRR_REG = (1u << 16); /* BR0=1，PB0输出低电平。 */
    GPIOC_BSRR_REG = (1u << 31); /* BR15=1，PC15输出低电平。 */

    while (1)
    {
        GPIOC_BSRR_REG = (1u << 15); /* BS15=1，蓝灯亮。 */
        delay_1s();
        GPIOC_BSRR_REG = (1u << 31); /* BR15=1，蓝灯灭。 */

        GPIOA_BSRR_REG = (1u << 0);  /* BS0=1，红灯亮。 */
        delay_1s();
        GPIOA_BSRR_REG = (1u << 16); /* BR0=1，红灯灭。 */

        GPIOB_BSRR_REG = (1u << 0);  /* BS0=1，绿灯亮。 */
        delay_1s();
        GPIOB_BSRR_REG = (1u << 16); /* BR0=1，绿灯灭。 */
    }
}
