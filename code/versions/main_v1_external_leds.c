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

/* 软件空循环延时函数。
 * t表示需要延时的毫秒数；外层循环每执行一次，约延时1 ms。
 * volatile和__NOP()可防止编译器把无实际结果的空循环优化掉。
 * 该延时会受系统时钟和编译优化等级影响，8000是按当前72 MHz工程选用的近似值。
 */
static void Delay_ms(volatile unsigned int t)
{
    volatile unsigned int i;

    while (t--)
    {
        for (i = 0u; i < 8000u; i++)
        {
            __NOP(); /* 执行一条空操作指令，只消耗CPU时间，不改变寄存器状态。 */
        }
    }
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
        Delay_ms(1000u);             /* 软件延时约1000 ms。 */
        GPIOC_BSRR_REG = (1u << 31); /* BR15=1，蓝灯灭。 */

        GPIOA_BSRR_REG = (1u << 0);  /* BS0=1，红灯亮。 */
        Delay_ms(1000u);             /* 软件延时约1000 ms。 */
        GPIOA_BSRR_REG = (1u << 16); /* BR0=1，红灯灭。 */

        GPIOB_BSRR_REG = (1u << 0);  /* BS0=1，绿灯亮。 */
        Delay_ms(1000u);             /* 软件延时约1000 ms。 */
        GPIOB_BSRR_REG = (1u << 16); /* BR0=1，绿灯灭。 */
    }
}
