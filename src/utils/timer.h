#ifndef TIMER_H_
#define TIMER_H_

#include <stdint.h>

/* 系统时钟计数器，由定时器中断定期更新 */
extern volatile uint32_t uwTick;
extern volatile unsigned int delay_times;

uint32_t MW_Timer_GetTick(void);
void delay_ms(unsigned int ms);

/**
 * @brief 检查指定时间是否已经过去（非阻塞式）
 * @param start_tick 起始时刻
 * @param ms 需要等待的毫秒数
 * @return 1 表示时间已到，0 表示时间未到
 */
uint8_t MW_Timer_IsTimeout(uint32_t start_tick, uint32_t ms);

#endif /* MW_TIMER_H_ */
