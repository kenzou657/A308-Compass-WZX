#ifndef DRV_UART_H
#define DRV_UART_H

#include <stdint.h>
#include <string.h>

/* ============ UART 配置常量 ============ */
#define UART_FRAME_SIZE         8           // 固定帧长（包含帧头帧尾）
#define UART_RX_BUFFER_SIZE     128         // 环形接收缓冲大小
#define UART_DMA_TRANSFER_SIZE  8           // DMA 每次传输大小
#define UART_TX_BUFFER_SIZE     128         // 发送缓冲大小

/* 帧格式定义 */
#define FRAME_HEADER_1          0xAA        // 帧头第一字节
#define FRAME_HEADER_2          0x55        // 帧头第二字节
#define FRAME_TAIL_1            0x0D        // 帧尾第一字节（\r）
#define FRAME_TAIL_2            0x0A        // 帧尾第二字节（\n）

/* ============ UART 接收结构体 ============ */
typedef struct {
    uint8_t buffer[UART_RX_BUFFER_SIZE];    // 环形缓冲
    uint16_t write_pos;                     // DMA 写入位置（0-127）
    uint8_t frame_data[UART_FRAME_SIZE];    // 有效帧数据
    uint8_t frame_ready;                    // 帧就绪标志
    uint8_t frame_error;                    // 帧错误标志
    uint32_t frame_count;                   // 成功接收帧计数
    uint32_t error_count;                   // 错误帧计数
} uart_rx_t;

/* ============ UART 发送结构体 ============ */
typedef struct {
    uint8_t tx_buffer[UART_TX_BUFFER_SIZE];
    uint16_t tx_len;
    uint8_t tx_busy;
} uart_tx_t;

/* ============ 全局变量声明 ============ */
extern uart_rx_t g_uart_rx;
extern uart_tx_t g_uart_tx;

/* ============ 函数声明 ============ */

/**
 * @brief 初始化 UART 驱动
 * 
 * 配置环形缓冲 DMA 接收，每次接收 8 字节
 * 使用流式扫描自动恢复帧同步
 */
void uart_init(void);

/**
 * @brief UART0 中断处理函数
 * 
 * 处理 DMA_DONE_RX 中断：
 * - 在环形缓冲中流式扫描帧头
 * - 验证帧头帧尾
 * - 自动恢复帧同步
 * - 重新配置 DMA
 */
void uart_isr_handler(void);

/**
 * @brief 获取接收到的帧数据（不含帧头帧尾）
 * 
 * 帧格式：[AA 55] [数据 4 字节] [0D 0A]
 * 返回：[数据 4 字节]
 * 
 * @param buffer 输出缓冲
 * @param len 输出长度指针
 * @return 0: 成功，-1: 无帧数据
 */
int uart_get_frame(uint8_t *buffer, uint16_t *len);

/**
 * @brief 获取完整帧数据（包含帧头帧尾）
 * 
 * 帧格式：[AA 55] [数据 4 字节] [0D 0A]
 * 返回：[AA 55] [数据 4 字节] [0D 0A]
 * 
 * @param buffer 输出缓冲
 * @param len 输出长度指针
 * @return 0: 成功，-1: 无帧数据
 */
int uart_get_full_frame(uint8_t *buffer, uint16_t *len);

/**
 * @brief 发送数据帧（自动添加帧头帧尾）
 * @param data 数据指针
 * @param len 数据长度
 * @return 0: 成功，-1: 发送忙，-2: 数据过长
 */
int uart_send_frame(const uint8_t *data, uint16_t len);

/**
 * @brief 发送单个字节
 * @param byte 字节数据
 */
void uart_send_byte(uint8_t byte);

/**
 * @brief 发送字节数组
 * @param data 数据指针
 * @param len 数据长度
 */
void uart_send_data(const uint8_t *data, uint16_t len);

/**
 * @brief 获取接收统计信息
 * @param frame_count 成功接收帧数
 * @param error_count 错误帧数
 */
void uart_get_stats(uint32_t *frame_count, uint32_t *error_count);

#endif // DRV_UART_H
