/**
 * @file isr_jy61p.c
 * @brief JY61P 陀螺仪 UART 中断服务函数
 */

#include "ti_msp_dl_config.h"
#include "../drivers/drv_jy61p.h"

/**
 * @brief UART IMU (JY61P) 中断服务函数
 * 
 * 处理 UART1 的 DMA 接收完成中断
 */
void UART_IMU_INST_IRQHandler(void)
{
    jy61p_isr_handler();
}
