#include "ti_msp_dl_config.h"
#include "../drivers/drv_uart.h"
#include "../app/app_camera_uart.h"

/**
 * @brief UART0 中断服务函数
 *
 * 由 SysConfig 自动调用，连接到 UART0_IRQHandler
 * 处理 UART 接收中断：
 * - RXD_NEG_EDGE：帧开始
 * - RXD_POS_EDGE：帧结束（空闲中断）
 * - DMA_DONE_RX：DMA 缓冲满
 *
 * 中断处理流程：
 * 1. 调用驱动层 uart_isr_handler() 处理接收
 * 2. 调用应用层 camera_uart_rx_callback() 解析摄像头数据
 */
void UART0_IRQHandler(void)
{
    uart_isr_handler();
    camera_uart_rx_callback();
}
