/*
 * 电机驱动中断处理
 * 
 * 功能：
 * - ENCODER1A中断：电机A编码器A相捕获
 * - ENCODER2A中断：电机B编码器A相捕获
 * - CLOCK中断：100Hz定时器中断，用于速度计算和PID控制
 */

#include "drivers/drv_motor.h"
#include "ti_msp_dl_config.h"

/* ==================== ENCODER1A中断处理 ==================== */

void ENCODER1A_IRQHandler(void)
{
    /* 清除中断标志 */
    DL_TimerG_clearInterruptStatus(ENCODER1A_INST, DL_TIMERG_INTERRUPT_CC0_DN_EVENT);
    
    /* 调用电机驱动的编码器中断处理 */
    Motor_Encoder1A_ISR();
}

/* ==================== ENCODER2A中断处理 ==================== */

void ENCODER2A_IRQHandler(void)
{
    /* 清除中断标志 */
    DL_TimerG_clearInterruptStatus(ENCODER2A_INST, DL_TIMERG_INTERRUPT_CC0_DN_EVENT);
    
    /* 调用电机驱动的编码器中断处理 */
    Motor_Encoder2A_ISR();
}

/* ==================== CLOCK中断处理 ==================== */

void CLOCK_IRQHandler(void)
{
    /* 清除中断标志 */
    DL_TimerA_clearInterruptStatus(CLOCK_INST, DL_TIMERA_INTERRUPT_ZERO_EVENT);
    
    /* 调用电机驱动的时钟中断处理 */
    Motor_Clock_ISR();
}
