/* mw_timer.c */
#include "ti_msp_dl_config.h"
#include "timer.h"

// 模拟 ST 的 uwTick
volatile uint32_t uwTick = 0; // 系统时钟计数器
volatile unsigned int delay_times = 0;

// 在 ti_msp_dl_config.c 中对应的 SysTick 中断服务函数中调用
void SysTick_Handler(void) {
    uwTick++;

    if( delay_times != 0 )
    {
            delay_times--;
    }
}

// 封装接口供 Application 层调用
uint32_t MW_Timer_GetTick(void) {
    return uwTick;
}

void delay_ms(unsigned int ms)
{
        delay_times = ms;
        while( delay_times != 0 );
}

/**
 * @brief 检查指定时间是否已经过去（非阻塞式）
 * @param start_tick 起始时刻
 * @param ms 需要等待的毫秒数
 * @return 1 表示时间已到，0 表示时间未到
 */
uint8_t MW_Timer_IsTimeout(uint32_t start_tick, uint32_t ms) {
    return ((uwTick - start_tick) >= ms) ? 1 : 0;
}