#include "ti_msp_dl_config.h"
#include "../drivers/drv_uart.h"

/**
 * @brief UART0 中断服务函数
 * 
 * 由 SysConfig 自动调用，连接到 UART0_IRQHandler
 * 处理 UART 接收中断：
 * - RXD_NEG_EDGE：帧开始
 * - RXD_POS_EDGE：帧结束（空闲中断）
 * - DMA_DONE_RX：DMA 缓冲满
 */
void UART0_IRQHandler(void)
{
    uart_isr_handler();
}
