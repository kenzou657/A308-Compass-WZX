/**
 * @file app_camera_uart.c
 * @brief 摄像头 UART 通信应用层实现
 * 
 * 功能：
 * - 解析摄像头数据帧
 * - 计算并验证校验和
 * - 提供摄像头数据访问接口
 */

#include "app_camera_uart.h"
#include "../drivers/drv_uart.h"
#include <string.h>

/* ============ 全局变量定义 ============ */
camera_uart_t g_camera_uart = {0};

/* ============ 初始化函数 ============ */

/**
 * @brief 初始化摄像头通信模块
 */
void camera_uart_init(void)
{
    memset(&g_camera_uart, 0, sizeof(camera_uart_t));
    g_camera_uart.initialized = 1;
}

/* ============ 校验和计算函数 ============ */

/**
 * @brief 计算校验和
 * 
 * 对前 9 个字节进行累加和计算（取低 8 位）
 * 
 * @param data 数据指针
 * @return 校验和值
 */
uint8_t camera_uart_calc_checksum(const uint8_t *data)
{
    uint16_t sum = 0;
    
    // 累加前 9 个字节
    for (int i = 0; i < 9; i++) {
        sum += data[i];
    }
    
    // 取低 8 位
    return (uint8_t)(sum & 0xFF);
}

/* ============ 接收处理函数 ============ */

/**
 * @brief 摄像头接收中断回调函数
 * 
 * 由 UART 中断处理函数调用，解析摄像头数据帧
 * 功能：
 * - 获取完整帧数据
 * - 验证帧头帧尾
 * - 计算并验证校验和
 * - 解析帧数据（Mode、ID、Data_X、Data_Y）
 * - 更新接收统计信息
 */
void camera_uart_rx_callback(void)
{
    uint8_t buffer[CAMERA_FRAME_SIZE];
    uint16_t len = 0;
    int ret;
    
    // 尝试获取完整帧（包含帧头帧尾）
    ret = uart_get_full_frame(buffer, &len);
    
    if (ret == 0 && len == CAMERA_FRAME_SIZE) {
        // 成功获取帧数据
        
        // 验证帧头
        if (buffer[0] != CAMERA_FRAME_HEADER_1 || buffer[1] != CAMERA_FRAME_HEADER_2) {
            g_camera_uart.last_rx_status = -1;  // 帧头错误
            g_camera_uart.rx_error_count++;
            return;
        }
        
        // 验证帧尾
        if (buffer[10] != CAMERA_FRAME_TAIL_1 || buffer[11] != CAMERA_FRAME_TAIL_2) {
            g_camera_uart.last_rx_status = -2;  // 帧尾错误
            g_camera_uart.rx_error_count++;
            return;
        }
        
        // 验证校验和
        uint8_t calc_checksum = camera_uart_calc_checksum(buffer);
        if (calc_checksum != buffer[9]) {
            g_camera_uart.last_rx_status = -3;  // 校验和错误
            g_camera_uart.rx_checksum_error_count++;
            g_camera_uart.rx_error_count++;
            return;
        }
        
        // 保存原始帧
        memcpy(g_camera_uart.rx_raw_frame, buffer, CAMERA_FRAME_SIZE);
        
        // 解析帧数据
        g_camera_uart.rx_frame.mode = buffer[2];
        g_camera_uart.rx_frame.id = buffer[3];
        
        // 解析 Data_X（字节 4-5，大端序）
        g_camera_uart.rx_frame.data_x = (int16_t)((buffer[4] << 8) | buffer[5]);
        
        // 解析 Data_Y（字节 6-7，大端序）
        g_camera_uart.rx_frame.data_y = (int16_t)((buffer[6] << 8) | buffer[7]);
        
        g_camera_uart.rx_frame.reserved = buffer[8];
        g_camera_uart.rx_frame.checksum = buffer[9];
        g_camera_uart.rx_frame.valid = 1;
        
        // 更新统计信息
        g_camera_uart.rx_frame_count++;
        g_camera_uart.last_rx_status = 0;  // 成功
        
        // 保存到历史记录
        memcpy(&g_camera_uart.rx_frame_history[g_camera_uart.rx_history_index],
               &g_camera_uart.rx_frame,
               sizeof(camera_frame_data_t));
        g_camera_uart.rx_history_index = (g_camera_uart.rx_history_index + 1) % CAMERA_RX_HISTORY_SIZE;
    }
}

/* ============ 数据获取函数 ============ */

/**
 * @brief 获取最新接收的摄像头数据
 * 
 * @param frame 输出帧数据指针
 * @return 0: 成功，-1: 无有效数据
 */
int camera_uart_get_rx_frame(camera_frame_data_t *frame)
{
    if (g_camera_uart.rx_frame.valid == 0) {
        return -1;  // 无有效数据
    }
    
    // 复制数据
    memcpy(frame, &g_camera_uart.rx_frame, sizeof(camera_frame_data_t));
    
    // 清除有效标志（可选，根据需求决定是否清除）
    // g_camera_uart.rx_frame.valid = 0;
    
    return 0;
}

/* ============ 发送函数 ============ */

/**
 * @brief 发送摄像头命令帧
 * 
 * 构建并发送摄像头命令帧
 * 
 * @param mode 工作模式
 * @param id 数字 ID
 * @param data_x X 坐标/偏移
 * @param data_y Y 坐标/偏移
 * @return 0: 成功，-1: 发送失败
 */
int camera_uart_send_frame(uint8_t mode, uint8_t id, int16_t data_x, int16_t data_y)
{
    uint8_t frame[CAMERA_FRAME_SIZE];
    
    // 构建帧：[AA 55] [Mode] [ID] [Data_X(2)] [Data_Y(2)] [Reserved] [Checksum] [0D 0A]
    frame[0] = CAMERA_FRAME_HEADER_1;
    frame[1] = CAMERA_FRAME_HEADER_2;
    frame[2] = mode;
    frame[3] = id;
    
    // Data_X（大端序）
    frame[4] = (uint8_t)((data_x >> 8) & 0xFF);
    frame[5] = (uint8_t)(data_x & 0xFF);
    
    // Data_Y（大端序）
    frame[6] = (uint8_t)((data_y >> 8) & 0xFF);
    frame[7] = (uint8_t)(data_y & 0xFF);
    
    frame[8] = 0x00;  // Reserved
    
    // 计算校验和
    frame[9] = camera_uart_calc_checksum(frame);
    
    frame[10] = CAMERA_FRAME_TAIL_1;
    frame[11] = CAMERA_FRAME_TAIL_2;
    
    // 保存发送帧
    memcpy(g_camera_uart.tx_raw_frame, frame, CAMERA_FRAME_SIZE);
    
    // 发送数据（使用驱动层的发送函数）
    uart_send_data(frame, CAMERA_FRAME_SIZE);
    
    // 更新统计信息
    g_camera_uart.tx_frame_count++;
    g_camera_uart.last_tx_status = 0;
    
    return 0;
}

/* ============ 统计函数 ============ */

/**
 * @brief 获取接收统计信息
 * 
 * @param frame_count 接收帧计数指针
 * @param error_count 错误帧计数指针
 * @param checksum_error_count 校验和错误计数指针
 */
void camera_uart_get_rx_stats(uint32_t *frame_count, uint32_t *error_count, uint32_t *checksum_error_count)
{
    if (frame_count != NULL) {
        *frame_count = g_camera_uart.rx_frame_count;
    }
    
    if (error_count != NULL) {
        *error_count = g_camera_uart.rx_error_count;
    }
    
    if (checksum_error_count != NULL) {
        *checksum_error_count = g_camera_uart.rx_checksum_error_count;
    }
}

/**
 * @brief 获取发送统计信息
 * 
 * @param frame_count 发送帧计数指针
 */
void camera_uart_get_tx_stats(uint32_t *frame_count)
{
    if (frame_count != NULL) {
        *frame_count = g_camera_uart.tx_frame_count;
    }
}
