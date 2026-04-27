#include "drv_uart.h"
#include "ti_msp_dl_config.h"

/* ============ 全局变量定义 ============ */
uart_rx_t g_uart_rx = {0};
uart_tx_t g_uart_tx = {0};

/* ============ 初始化函数 ============ */

/**
 * @brief 初始化 UART 驱动
 * 
 * 配置环形缓冲 DMA 接收，每次接收 8 字节
 * 使用流式扫描自动恢复帧同步
 */
void uart_init(void)
{
    // 初始化接收状态
    g_uart_rx.write_pos = 0;
    g_uart_rx.frame_ready = 0;
    g_uart_rx.frame_error = 0;
    g_uart_rx.frame_count = 0;
    g_uart_rx.error_count = 0;
    
    // 初始化发送状态
    g_uart_tx.tx_len = 0;
    g_uart_tx.tx_busy = 0;
    
    // 配置 DMA 源地址（UART RX FIFO）
    DL_DMA_setSrcAddr(DMA, DMA_UART0_RX_CHAN_ID, 
                      (uint32_t)(&UART_CAM_INST->RXDATA));
    
    // 配置 DMA 目标地址（环形缓冲起始位置）
    DL_DMA_setDestAddr(DMA, DMA_UART0_RX_CHAN_ID, 
                       (uint32_t)&g_uart_rx.buffer[0]);
    
    // 配置 DMA 传输大小（每次 8 字节）
    DL_DMA_setTransferSize(DMA, DMA_UART0_RX_CHAN_ID, UART_DMA_TRANSFER_SIZE);
    
    // 启用 DMA 通道
    DL_DMA_enableChannel(DMA, DMA_UART0_RX_CHAN_ID);
    
    // 确认 DMA 通道已启用
    while (false == DL_DMA_isChannelEnabled(DMA, DMA_UART0_RX_CHAN_ID)) {
        ;
    }
    
    // 启用 UART0 中断
    NVIC_EnableIRQ(UART_CAM_INST_INT_IRQN);
}

/* ============ 中断处理函数 ============ */

/**
 * @brief UART0 中断处理函数
 * 
 * 处理 DMA_DONE_RX 中断：
 * 1. 在环形缓冲中流式扫描帧头（搜索范围：最近 16 字节）
 * 2. 验证帧头帧尾
 * 3. 如果有效，复制到 frame_data 并设置 frame_ready 标志
 * 4. 更新 write_pos
 * 5. 重新配置 DMA 继续接收下一个 8 字节
 */
void uart_isr_handler(void)
{
    switch (DL_UART_Main_getPendingInterrupt(UART_CAM_INST)) {
        case DL_UART_MAIN_IIDX_DMA_DONE_RX:
        {
            // 当前 DMA 写入位置（刚写完的 8 字节的起始位置）
            uint16_t current_write_pos = g_uart_rx.write_pos;
            
            // 流式扫描：在最近 24 字节范围内搜索帧头
            // 搜索范围：[write_pos-12, write_pos+11]（环形）
            // 这样可以处理帧边界跨越 DMA 传输边界的情况
            uint8_t frame_found = 0;
            
            for (int offset = 0; offset <= 12; offset++) {
                // 计算帧头可能的起始位置（环形索引）
                uint16_t frame_start = (current_write_pos + offset) % UART_RX_BUFFER_SIZE;
                
                // 计算帧内各字节的环形索引
                uint16_t idx0 = frame_start;
                uint16_t idx1 = (frame_start + 1) % UART_RX_BUFFER_SIZE;
                uint16_t idx10 = (frame_start + 10) % UART_RX_BUFFER_SIZE;
                uint16_t idx11 = (frame_start + 11) % UART_RX_BUFFER_SIZE;
                
                // 验证帧头帧尾
                // 帧格式：[AA 55] [数据 8 字节] [0D 0A]
                if (g_uart_rx.buffer[idx0] == FRAME_HEADER_1 &&
                    g_uart_rx.buffer[idx1] == FRAME_HEADER_2 &&
                    g_uart_rx.buffer[idx10] == FRAME_TAIL_1 &&
                    g_uart_rx.buffer[idx11] == FRAME_TAIL_2) {
                    
                    // 找到有效帧，复制到 frame_data
                    for (int i = 0; i < UART_FRAME_SIZE; i++) {
                        uint16_t idx = (frame_start + i) % UART_RX_BUFFER_SIZE;
                        g_uart_rx.frame_data[i] = g_uart_rx.buffer[idx];
                    }
                    
                    g_uart_rx.frame_ready = 1;
                    g_uart_rx.frame_error = 0;
                    g_uart_rx.frame_count++;
                    frame_found = 1;
                    break;
                }
            }
            
            // 如果未找到有效帧，设置错误标志
            if (!frame_found) {
                g_uart_rx.frame_error = 1;
                g_uart_rx.error_count++;
            }
            
            // 更新写入位置（环形）
            g_uart_rx.write_pos = (g_uart_rx.write_pos + UART_DMA_TRANSFER_SIZE) % UART_RX_BUFFER_SIZE;
            
            // 重新配置 DMA 接收下一个 12 字节
            DL_DMA_setDestAddr(DMA, DMA_UART0_RX_CHAN_ID, 
                              (uint32_t)&g_uart_rx.buffer[g_uart_rx.write_pos]);
            DL_DMA_setTransferSize(DMA, DMA_UART0_RX_CHAN_ID, UART_DMA_TRANSFER_SIZE);
            DL_DMA_enableChannel(DMA, DMA_UART0_RX_CHAN_ID);
            
            break;
        }
        default:
            break;
    }
}

/* ============ 帧获取函数 ============ */

/**
 * @brief 获取接收到的帧数据（不含帧头帧尾）
 * 
 * @param buffer 输出缓冲，用于存储帧数据
 * @param len 输出长度指针，返回帧数据长度
 * @return 0: 成功，-1: 无帧数据
 */
int uart_get_frame(uint8_t *buffer, uint16_t *len)
{
    if (g_uart_rx.frame_ready == 0) {
        return -1;  // 无帧数据
    }
    
    // 提取数据部分（跳过帧头，去掉帧尾）
    // 帧格式：[AA 55] [数据 8 字节] [0D 0A]
    uint16_t data_len = UART_FRAME_SIZE - 4;  // 12 - 2(帧头) - 2(帧尾) = 8
    
    if (data_len > 0) {
        memcpy(buffer, &g_uart_rx.frame_data[2], data_len);
        *len = data_len;
    } else {
        *len = 0;
    }
    
    // 清除帧就绪标志
    g_uart_rx.frame_ready = 0;
    
    return 0;
}

/**
 * @brief 获取完整帧数据（包含帧头帧尾）
 * 
 * @param buffer 输出缓冲，用于存储完整帧
 * @param len 输出长度指针，返回帧长度
 * @return 0: 成功，-1: 无帧数据
 */
int uart_get_full_frame(uint8_t *buffer, uint16_t *len)
{
    if (g_uart_rx.frame_ready == 0) {
        return -1;  // 无帧数据
    }
    
    // 复制完整帧
    memcpy(buffer, g_uart_rx.frame_data, UART_FRAME_SIZE);
    *len = UART_FRAME_SIZE;
    
    // 清除帧就绪标志
    g_uart_rx.frame_ready = 0;
    
    return 0;
}

/* ============ 发送函数 ============ */

/**
 * @brief 发送单个字节
 * @param byte 字节数据
 */
void uart_send_byte(uint8_t byte)
{
    // 等待 TX FIFO 非满
    while (DL_UART_Main_isBusy(UART_CAM_INST));
    DL_UART_Main_transmitData(UART_CAM_INST, byte);
}

/**
 * @brief 发送字节数组
 * @param data 数据指针
 * @param len 数据长度
 */
void uart_send_data(const uint8_t *data, uint16_t len)
{
    uint16_t i;
    for (i = 0; i < len; i++) {
        uart_send_byte(data[i]);
    }
}

/**
 * @brief 发送数据帧（自动添加帧头帧尾）
 * 
 * 帧格式：[AA 55] [数据] [0D 0A]
 * 
 * @param data 数据指针
 * @param len 数据长度
 * @return 0: 成功，-1: 发送忙，-2: 数据过长
 */
int uart_send_frame(const uint8_t *data, uint16_t len)
{
    uint16_t total_len;
    uint16_t i;
    
    if (g_uart_tx.tx_busy) {
        return -1;  // 发送忙
    }
    
    if (len + 4 > UART_TX_BUFFER_SIZE) {
        return -2;  // 数据过长
    }
    
    // 构建帧：[AA 55] [数据] [0D 0A]
    g_uart_tx.tx_buffer[0] = FRAME_HEADER_1;
    g_uart_tx.tx_buffer[1] = FRAME_HEADER_2;
    
    if (len > 0) {
        memcpy(&g_uart_tx.tx_buffer[2], data, len);
    }
    
    g_uart_tx.tx_buffer[len + 2] = FRAME_TAIL_1;
    g_uart_tx.tx_buffer[len + 3] = FRAME_TAIL_2;
    
    total_len = len + 4;
    g_uart_tx.tx_len = total_len;
    g_uart_tx.tx_busy = 1;
    
    // 发送数据
    for (i = 0; i < total_len; i++) {
        uart_send_byte(g_uart_tx.tx_buffer[i]);
    }
    
    g_uart_tx.tx_busy = 0;
    
    return 0;
}

/* ============ 统计函数 ============ */

/**
 * @brief 获取接收统计信息
 * @param frame_count 成功接收帧数
 * @param error_count 错误帧数
 */
void uart_get_stats(uint32_t *frame_count, uint32_t *error_count)
{
    if (frame_count != NULL) {
        *frame_count = g_uart_rx.frame_count;
    }
    
    if (error_count != NULL) {
        *error_count = g_uart_rx.error_count;
    }
}
