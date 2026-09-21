#include "stm32f10x.h"

/* 将指定地址转换为可读写的32位硬件寄存器。
 * volatile用于防止编译器省略或合并对硬件寄存器的访问。
 */
#define REG32(address)              (*(volatile unsigned int *)(address))

/* RCC与GPIO寄存器的绝对地址，均由“外设基地址+寄存器偏移地址”得到。 */
#define RCC_APB2ENR_REG             REG32(0x40021018u) /* 0x40021000+0x18，APB2时钟使能。 */
#define GPIOA_CRL_REG               REG32(0x40010800u) /* 0x40010800+0x00，配置PA0～PA7。 */
#define GPIOB_CRL_REG               REG32(0x40010C00u) /* 0x40010C00+0x00，配置PB0～PB7。 */
#define GPIOC_CRH_REG               REG32(0x40011004u) /* 0x40011000+0x04，配置PC8～PC15。 */
#define GPIOA_BSRR_REG              REG32(0x40010810u) /* 0x40010800+0x10，控制GPIOA输出。 */
#define GPIOB_BSRR_REG              REG32(0x40010C10u) /* 0x40010C00+0x10，控制GPIOB输出。 */
#define GPIOC_BSRR_REG              REG32(0x40011010u) /* 0x40011000+0x10，控制GPIOC输出。 */

/* Cortex-M3 SysTick寄存器的绝对地址。 */
#define SYSTICK_CTRL_REG            REG32(0xE000E010u) /* 控制及状态寄存器。 */
#define SYSTICK_LOAD_REG            REG32(0xE000E014u) /* 计数重装载值寄存器。 */
#define SYSTICK_VALUE_REG           REG32(0xE000E018u) /* 当前计数值寄存器。 */

/* GPIOC_CRH中PC13和PC15配置字段的起始位。 */
#define PC13_CONFIG_SHIFT           20u
#define PC15_CONFIG_SHIFT           28u

/* STM32F1每个GPIO引脚使用4位配置。
 * 0x2：CNF=00、MODE=10，表示2 MHz通用推挽输出。
 * 0x4：CNF=01、MODE=00，表示浮空输入。
 */
#define GPIO_OUTPUT_2MHZ_PP         0x02u
#define GPIO_FLOATING_INPUT         0x04u

/* 使用SysTick产生约1秒的阻塞延时。
 * 当前SystemCoreClock为72 MHz，SysTick选择HCLK/8作为时钟源，
 * 因此计数频率为9 MHz，计数9 000 000次所需时间为1秒。
 */
static void delay_1s(void)
{
    SYSTICK_CTRL_REG = 0u; /* 清除ENABLE位，配置前先关闭SysTick。 */

    /* SystemCoreClock=72 MHz，选择HCLK/8后计数频率为9 MHz。
     * 计数器从LOAD递减到0共经历LOAD+1个时钟周期，
     * 所以1秒延时的LOAD值应为9 000 000-1。
     */
    SYSTICK_LOAD_REG = SystemCoreClock / 8u - 1u;
    SYSTICK_VALUE_REG = 0u; /* 写入任意值都会清零当前计数器和COUNTFLAG。 */

    /* CTRL位0 ENABLE=1：启动计数；位1 TICKINT=0：不产生中断；
     * 位2 CLKSOURCE=0：选择HCLK/8作为SysTick时钟源。
     */
    SYSTICK_CTRL_REG = 1u;

    /* CTRL寄存器位16为COUNTFLAG，计数到0后由硬件置1。 */
    while ((SYSTICK_CTRL_REG & (1u << 16)) == 0u)
    {
    }

    SYSTICK_CTRL_REG = 0u;                       /* 延时结束后关闭SysTick。 */
}

/* 修改GPIOC_CRH中的一个4位配置字段。
 * shift表示目标字段最低位的位置，config表示要写入的4位配置值。
 * 先清除目标字段，再写入新值，不会改变GPIOC其他引脚的配置。
 */
static void gpio_c_set_config(unsigned int shift, unsigned int config)
{
    GPIOC_CRH_REG &= ~(0x0Fu << shift);
    GPIOC_CRH_REG |=  (config << shift);
}

int main(void)
{
    /* RCC_APB2ENR的位2、位3和位4分别为IOPAEN、IOPBEN和IOPCEN。
     * 使用“|=”只把这三位置1，并保留寄存器中其他位的原值。
     */
    RCC_APB2ENR_REG |= (1u << 2) | (1u << 3) | (1u << 4);

    /* 每个GPIO引脚占4个配置位，高2位为CNF，低2位为MODE。
     * PA0对应GPIOA_CRL位3:0。先用0xF的反码清除原配置，
     * 再写入二进制0010，即CNF=00、MODE=10，配置为2 MHz通用推挽输出。
     */
    GPIOA_CRL_REG &= ~(0x0Fu << 0);
    GPIOA_CRL_REG |=  (0x02u << 0);

    /* PB0同样对应GPIOB_CRL位3:0，配置值也为0x2。 */
    GPIOB_CRL_REG &= ~(0x0Fu << 0);
    GPIOB_CRL_REG |=  (0x02u << 0);

    /* 数据手册规定PC13～PC15同一时刻只能有一个引脚作为输出。
     * 初始化时先将PC13和PC15都设置为浮空输入，随后按流水灯阶段动态切换。
     */
    gpio_c_set_config(PC13_CONFIG_SHIFT, GPIO_FLOATING_INPUT);
    gpio_c_set_config(PC15_CONFIG_SHIFT, GPIO_FLOATING_INPUT);

    /* 外接LED均为高电平点亮，因此PA0、PB0和PC15预置低电平。
     * 板载LED为低电平点亮，因此PC13预置高电平使其保持熄灭。
     * 即使引脚当前是输入模式，写BSRR仍可预先设置对应ODR锁存位。
     */
    GPIOA_BSRR_REG = (1u << 16); /* BR0=1，使PA0输出低电平。 */
    GPIOB_BSRR_REG = (1u << 16); /* BR0=1，使PB0输出低电平。 */
    GPIOC_BSRR_REG = (1u << 31); /* BR15=1，使PC15输出低电平。 */
    GPIOC_BSRR_REG = (1u << 13); /* BS13=1，使PC13预置高电平。 */

    while (1)
    {
        /* 阶段1：点亮PC15控制的外接蓝灯。
         * 此时PC13保持浮空输入，只将PC15切换为2 MHz推挽输出。
         */
        gpio_c_set_config(PC13_CONFIG_SHIFT, GPIO_FLOATING_INPUT);
        gpio_c_set_config(PC15_CONFIG_SHIFT, GPIO_OUTPUT_2MHZ_PP);
        GPIOC_BSRR_REG = (1u << 15);             /* BS15=1，蓝灯亮。 */
        delay_1s();
        GPIOC_BSRR_REG = (1u << 31);             /* BR15=1，蓝灯灭。 */
        gpio_c_set_config(PC15_CONFIG_SHIFT, GPIO_FLOATING_INPUT);

        /* 阶段2：点亮PA0控制的外接红灯。 */
        GPIOA_BSRR_REG = (1u << 0);              /* BS0=1，红灯亮。 */
        delay_1s();
        GPIOA_BSRR_REG = (1u << 16);             /* BR0=1，红灯灭。 */

        /* 阶段3：点亮PB0控制的外接绿灯。 */
        GPIOB_BSRR_REG = (1u << 0);              /* BS0=1，绿灯亮。 */
        delay_1s();
        GPIOB_BSRR_REG = (1u << 16);             /* BR0=1，绿灯灭。 */

        /* 阶段4：点亮PC13控制的板载LED。
         * 先确保PC15为浮空输入，再将PC13切换为2 MHz推挽输出。
         * 板载LED低电平有效，因此向BR13写1时点亮，向BS13写1时熄灭。
         */
        gpio_c_set_config(PC15_CONFIG_SHIFT, GPIO_FLOATING_INPUT);
        gpio_c_set_config(PC13_CONFIG_SHIFT, GPIO_OUTPUT_2MHZ_PP);
        GPIOC_BSRR_REG = (1u << 29);             /* BR13=1，板载LED亮。 */
        delay_1s();
        GPIOC_BSRR_REG = (1u << 13);             /* BS13=1，板载LED灭。 */
        gpio_c_set_config(PC13_CONFIG_SHIFT, GPIO_FLOATING_INPUT);
    }
}
