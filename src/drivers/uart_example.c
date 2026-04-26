#include "ti_msp_dl_config.h"
#include "src/drivers/drv_uart.h"

/**
 * @file uart_example.c
 * @brief UART 驱动使用示例
 * 
 * 演示如何在应用中使用 UART 驱动进行数据接收和发送
 */

/* ============ 示例 1：基础接收和发送 ============ */

/**
 * @brief 处理接收到的帧数据
 * @param data 帧数据指针
 * @param len 帧数据长度
 */
void uart_handle_frame(const uint8_t *data, uint16_t len)
{
    // TODO: 在此处添加应用层处理逻辑
    // 例如：解析 OpenMV 数据、IMU 数据等
    
    // 示例：打印接收到的数据
    // printf("收到帧，长度：%d\n", len);
    // for (int i = 0; i < len; i++) {
    //     printf("0x%02X ", data[i]);
    // }
    // printf("\n");
}

/**
 * @brief 主循环中的 UART 处理
 * 
 * 在 main() 的主循环中调用此函数
 */
void uart_main_loop(void)
{
    // 检查是否有帧就绪
    if (g_uart_rx.frame_ready) {
        // 流式解析接收缓冲中的数据
        int result = uart_parse_stream();
        
        if (result == 1) {
            // ========== 帧有效 ==========
            uint8_t frame_data[UART_FRAME_MAX_SIZE];
            uint16_t frame_len;
            
            // 获取帧数据
            uart_get_frame(frame_data, &frame_len);
            
            // 处理帧数据
            uart_handle_frame(frame_data, frame_len);
            
            // 重启 DMA 接收
            uart_restart_dma();
            
        } else if (result == -1) {
            // ========== 帧错误 ==========
            // 帧数据过长或格式错误，重启 DMA
            uart_restart_dma();
            
        } else {
            // ========== 未找到完整帧 ==========
            // result == 0，继续等待更多数据
            // 不需要重启 DMA，继续接收
        }
    }
}

/* ============ 示例 2：发送响应数据 ============ */

/**
 * @brief 发送状态响应
 * @param status 状态码
 */
void uart_send_status(uint8_t status)
{
    uint8_t response[2] = {status, 0x00};
    uart_send_frame(response, 2);
}

/**
 * @brief 发送多字节数据
 * @param data 数据指针
 * @param len 数据长度
 */
void uart_send_response(const uint8_t *data, uint16_t len)
{
    uart_send_frame(data, len);
}

/* ============ 示例 3：完整的应用集成 ============ */

/**
 * @brief 应用层 UART 初始化
 */
void app_uart_init(void)
{
    // 初始化 UART 驱动
    uart_init();
    
    // 发送初始化完成信号
    uint8_t init_msg[] = {0x01};
    uart_send_frame(init_msg, 1);
}

/**
 * @brief 应用层 UART 处理
 * 
 * 在主循环中调用
 */
void app_uart_process(void)
{
    if (g_uart_rx.frame_ready) {
        int result = uart_parse_stream();
        
        if (result == 1) {
            uint8_t frame_data[UART_FRAME_MAX_SIZE];
            uint16_t frame_len;
            uart_get_frame(frame_data, &frame_len);
            
            // 根据帧数据进行处理
            if (frame_len > 0) {
                uint8_t cmd = frame_data[0];
                
                switch (cmd) {
                    case 0x01:
                        // 命令 1：获取状态
                        uart_send_status(0x00);  // 返回状态 0
                        break;
                    
                    case 0x02:
                        // 命令 2：执行操作
                        // TODO: 执行相应操作
                        uart_send_status(0x01);  // 返回成功
                        break;
                    
                    case 0x03:
                        // 命令 3：获取数据
                        {
                            uint8_t response[] = {0x03, 0xAA, 0xBB, 0xCC};
                            uart_send_response(response, 4);
                        }
                        break;
                    
                    default:
                        // 未知命令
                        uart_send_status(0xFF);  // 返回错误
                        break;
                }
            }
            
            // 重启 DMA
            uart_restart_dma();
            
        } else if (result == -1) {
            // 帧错误，重启 DMA
            uart_restart_dma();
        }
    }
}

/* ============ 示例 4：调试辅助函数 ============ */

/**
 * @brief 打印 DMA 缓冲内容（调试用）
 */
void uart_debug_print_buffer(void)
{
    // printf("DMA 缓冲内容（%d 字节）：\n", g_uart_rx.dma_count);
    // for (int i = 0; i < g_uart_rx.dma_count; i++) {
    //     printf("0x%02X ", g_uart_rx.dma_buffer[i]);
    //     if ((i + 1) % 16 == 0) printf("\n");
    // }
    // printf("\n");
}

/**
 * @brief 打印接收状态（调试用）
 */
void uart_debug_print_state(void)
{
    // const char *state_names[] = {
    //     "IDLE", "HEADER_1", "HEADER_2", "DATA", "TAIL_1", "TAIL_2", "FRAME_READY"
    // };
    // printf("RX 状态：%s\n", state_names[g_uart_rx.state]);
    // printf("DMA 计数：%d，索引：%d\n", g_uart_rx.dma_count, g_uart_rx.dma_index);
    // printf("帧长度：%d，帧就绪：%d\n", g_uart_rx.frame_len, g_uart_rx.frame_ready);
}

/* ============ 示例 5：在 main() 中的集成 ============ */

/*
// 在 empty.c 中的 main() 函数中：

int main(void)
{
    SYSCFG_DL_init();
    
    // 初始化 UART
    app_uart_init();
    
    // 主循环
    while (1) {
        // 处理 UART 接收和发送
        app_uart_process();
        
        // 其他应用逻辑
        // ...
    }
}
*/

/* ============ 示例 6：高级用法 - 缓冲多帧 ============ */

#define MAX_FRAMES 10

typedef struct {
    uint8_t frames[MAX_FRAMES][UART_FRAME_MAX_SIZE];
    uint16_t lengths[MAX_FRAMES];
    uint8_t count;
    uint8_t read_index;
} frame_queue_t;

static frame_queue_t g_frame_queue = {0};

/**
 * @brief 将帧添加到队列
 */
void frame_queue_push(const uint8_t *data, uint16_t len)
{
    if (g_frame_queue.count < MAX_FRAMES) {
        memcpy(g_frame_queue.frames[g_frame_queue.count], data, len);
        g_frame_queue.lengths[g_frame_queue.count] = len;
        g_frame_queue.count++;
    }
}

/**
 * @brief 从队列中获取帧
 */
int frame_queue_pop(uint8_t *data, uint16_t *len)
{
    if (g_frame_queue.count == 0) {
        return -1;  // 队列为空
    }
    
    memcpy(data, g_frame_queue.frames[g_frame_queue.read_index], 
           g_frame_queue.lengths[g_frame_queue.read_index]);
    *len = g_frame_queue.lengths[g_frame_queue.read_index];
    
    g_frame_queue.read_index++;
    if (g_frame_queue.read_index >= g_frame_queue.count) {
        g_frame_queue.read_index = 0;
        g_frame_queue.count = 0;
    }
    
    return 0;
}

/**
 * @brief 使用帧队列的处理函数
 */
void app_uart_process_with_queue(void)
{
    if (g_uart_rx.frame_ready) {
        int result = uart_parse_stream();
        
        if (result == 1) {
            uint8_t frame_data[UART_FRAME_MAX_SIZE];
            uint16_t frame_len;
            uart_get_frame(frame_data, &frame_len);
            
            // 将帧添加到队列
            frame_queue_push(frame_data, frame_len);
            
            // 重启 DMA
            uart_restart_dma();
        } else if (result == -1) {
            uart_restart_dma();
        }
    }
    
    // 处理队列中的帧
    uint8_t frame_data[UART_FRAME_MAX_SIZE];
    uint16_t frame_len;
    if (frame_queue_pop(frame_data, &frame_len) == 0) {
        // 处理帧数据
        uart_handle_frame(frame_data, frame_len);
    }
}
