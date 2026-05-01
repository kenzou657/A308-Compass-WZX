/**
 * @file isr_encoder.c
 * @brief 编码器中断服务函数
 * 
 * 实现两个中断处理函数：
 * 1. GROUP1_IRQHandler (GPIOB) - 编码器GPIO外部中断，处理A、B两相上升沿
 * 2. TIMA0_IRQHandler (TIMA0) - 10ms定时器，用于速度计算
 * 
 * 脉冲计数逻辑（四相编码）：
 * - A相上升沿触发中断，读取B相电平
 *   - B相=1：正转，计数+1
 *   - B相=0：反转，计数-1
 * - B相上升沿触发中断，读取A相电平
 *   - A相=0：正转，计数+1
 *   - A相=1：反转，计数-1
 */

#include "drv_encoder.h"

/* ============ GPIO中断处理（编码器脉冲计数）============ */

/**
 * @brief GROUP1中断服务函数（GPIOB）
 * @note 处理编码器A、B两相的上升沿中断
 * 
 * 中断优先级：0（最高，确保脉冲不丢失）
 */
void GROUP1_IRQHandler(void)
{
    // 获取中断状态
    uint32_t gpio_status = DL_GPIO_getEnabledInterruptStatus(
        GPIO_ENCODER_PORT, 
        GPIO_ENCODER_E1A_PIN | GPIO_ENCODER_E1B_PIN | 
        GPIO_ENCODER_E2A_PIN | GPIO_ENCODER_E2B_PIN
    );
    
    /* ========== 处理编码器A ========== */
    
    // 编码器A的A相上升沿
    if (gpio_status & GPIO_ENCODER_E1A_PIN) {
        EncoderState_t* state_a = EncoderGetState(ENCODER_A);
        if (state_a != NULL) {
            // 读取B相电平
            uint32_t b_phase = DL_GPIO_readPins(GPIO_ENCODER_PORT, GPIO_ENCODER_E1B_PIN);
            
            // 根据B相电平判断方向
            if (b_phase != 0) {
                // B相=1，正转，计数+1
                state_a->pulse_count++;
            } else {
                // B相=0，反转，计数-1
                state_a->pulse_count--;
            }
        }
        // 清除中断标志位
        DL_GPIO_clearInterruptStatus(GPIO_ENCODER_PORT, GPIO_ENCODER_E1A_PIN);
    }
    
    // 编码器A的B相上升沿
    if (gpio_status & GPIO_ENCODER_E1B_PIN) {
        EncoderState_t* state_a = EncoderGetState(ENCODER_A);
        if (state_a != NULL) {
            // 读取A相电平
            uint32_t a_phase = DL_GPIO_readPins(GPIO_ENCODER_PORT, GPIO_ENCODER_E1A_PIN);
            
            // 根据A相电平判断方向
            if (a_phase == 0) {
                // A相=0，正转，计数+1
                state_a->pulse_count++;
            } else {
                // A相=1，反转，计数-1
                state_a->pulse_count--;
            }
        }
        // 清除中断标志位
        DL_GPIO_clearInterruptStatus(GPIO_ENCODER_PORT, GPIO_ENCODER_E1B_PIN);
    }
    
    /* ========== 处理编码器B ========== */
    
    // 编码器B的A相上升沿
    if (gpio_status & GPIO_ENCODER_E2A_PIN) {
        EncoderState_t* state_b = EncoderGetState(ENCODER_B);
        if (state_b != NULL) {
            // 读取B相电平
            uint32_t b_phase = DL_GPIO_readPins(GPIO_ENCODER_PORT, GPIO_ENCODER_E2B_PIN);
            
            // 根据B相电平判断方向
            if (b_phase != 0) {
                // B相=1，正转，计数+1
                state_b->pulse_count++;
            } else {
                // B相=0，反转，计数-1
                state_b->pulse_count--;
            }
        }
        // 清除中断标志位
        DL_GPIO_clearInterruptStatus(GPIO_ENCODER_PORT, GPIO_ENCODER_E2A_PIN);
    }
    
    // 编码器B的B相上升沿
    if (gpio_status & GPIO_ENCODER_E2B_PIN) {
        EncoderState_t* state_b = EncoderGetState(ENCODER_B);
        if (state_b != NULL) {
            // 读取A相电平
            uint32_t a_phase = DL_GPIO_readPins(GPIO_ENCODER_PORT, GPIO_ENCODER_E2A_PIN);
            
            // 根据A相电平判断方向
            if (a_phase == 0) {
                // A相=0，正转，计数+1
                state_b->pulse_count++;
            } else {
                // A相=1，反转，计数-1
                state_b->pulse_count--;
            }
        }
        // 清除中断标志位
        DL_GPIO_clearInterruptStatus(GPIO_ENCODER_PORT, GPIO_ENCODER_E2B_PIN);
    }
}

/* ============ CLOCK中断处理（速度计算）============ */

/**
 * @brief CLOCK中断服务函数（TIMA0）
 * @note 20ms定时器中断，执行速度计算
 * 
 * 中断优先级：3（低于GPIO中断，可被打断）
 */
void TIMA0_IRQHandler(void)
{
    // 检查是否为零事件中断（计数器溢出）
    switch (DL_TimerA_getPendingInterrupt(CLOCK_INST)) {
        case DL_TIMERA_IIDX_ZERO: {
            // 调用速度计算函数
            EncoderCalculateSpeed();
            
            // 清除中断标志位
            DL_TimerA_clearInterruptStatus(CLOCK_INST, DL_TIMERA_INTERRUPT_ZERO_EVENT);
            break;
        }
        default:
            break;
    }
}
