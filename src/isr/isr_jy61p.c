/**
 * @file isr_jy61p.c
 * @brief JY61P 陀螺仪 UART 中断服务函数
 *
 * 功能：
 * - 处理 UART1 的接收中断
 * - 调用驱动层处理函数
 */

#include "ti_msp_dl_config.h"
#include "../drivers/drv_jy61p.h"

/**
 * @brief UART1 (IMU) 中断服务函数
 *
 * 处理 UART1 的接收中断
 */
void UART1_IRQHandler(void)
{
    // 调用驱动层处理接收
    jy61p_isr_handler();
}
