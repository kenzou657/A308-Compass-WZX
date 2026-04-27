#ifndef APP_CAMERA_UART_H
#define APP_CAMERA_UART_H

#include <stdint.h>

/* ============ 摄像头通信配置 ============ */
#define CAMERA_FRAME_SIZE           12      // 摄像头数据帧大小
#define CAMERA_RX_HISTORY_SIZE      10      // 接收帧历史记录数
#define CAMERA_TX_HISTORY_SIZE      10      // 发送帧历史记录数

/* ============ 摄像头数据帧格式 ============ */
// 帧格式：[AA 55] [Mode] [ID] [Data_X(2)] [Data_Y(2)] [Reserved] [Checksum] [0D 0A]
// 字节偏移：0    1     2      3    4-5       6-7        8         9         10  11

#define CAMERA_FRAME_HEADER_1       0xAA    // 帧头第一字节
#define CAMERA_FRAME_HEADER_2       0x55    // 帧头第二字节
#define CAMERA_FRAME_TAIL_1         0x0D    // 帧尾第一字节（\r）
#define CAMERA_FRAME_TAIL_2         0x0A    // 帧尾第二字节（\n）

/* 摄像头工作模式 */
#define CAMERA_MODE_LINE_TRACKING   0x01    // 寻线模式
#define CAMERA_MODE_DIGIT_RECOG     0x02    // 数字识别模式

/* ============ 摄像头接收数据结构体 ============ */
typedef struct {
    uint8_t mode;                           // 工作模式（0x01 或 0x02）
    uint8_t id;                             // 识别的数字 ID（模式 2）
    int16_t data_x;                         // X 坐标或偏移（放大 1000 倍）
    int16_t data_y;                         // Y 坐标或角度偏移（放大 1000 倍）
    uint8_t reserved;                       // 预留位
    uint8_t checksum;                       // 校验和
    uint8_t valid;                          // 帧有效标志
} camera_frame_data_t;

/* ============ 摄像头通信统计结构体 ============ */
typedef struct {
    // 接收相关
    camera_frame_data_t rx_frame;                           // 最新接收的帧数据
    uint8_t rx_raw_frame[CAMERA_FRAME_SIZE];                // 原始接收帧
    uint32_t rx_frame_count;                                // 接收帧计数
    uint32_t rx_error_count;                                // 接收错误计数
    uint32_t rx_checksum_error_count;                       // 校验和错误计数
    camera_frame_data_t rx_frame_history[CAMERA_RX_HISTORY_SIZE];  // 接收帧历史
    uint8_t rx_history_index;                               // 历史记录索引
    
    // 发送相关
    uint8_t tx_raw_frame[CAMERA_FRAME_SIZE];                // 原始发送帧
    uint32_t tx_frame_count;                                // 发送帧计数
    uint8_t tx_history_index;                               // 发送历史索引
    
    // 状态标志
    uint8_t initialized;                                    // 初始化标志
    uint8_t last_rx_status;                                 // 最后接收状态
    uint8_t last_tx_status;                                 // 最后发送状态
} camera_uart_t;

/* ============ 全局变量声明 ============ */
extern camera_uart_t g_camera_uart;

/* ============ 函数声明 ============ */

/**
 * @brief 初始化摄像头通信模块
 */
void camera_uart_init(void);

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
void camera_uart_rx_callback(void);

/**
 * @brief 获取最新接收的摄像头数据
 * 
 * @param frame 输出帧数据指针
 * @return 0: 成功，-1: 无有效数据
 */
int camera_uart_get_rx_frame(camera_frame_data_t *frame);

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
int camera_uart_send_frame(uint8_t mode, uint8_t id, int16_t data_x, int16_t data_y);

/**
 * @brief 计算校验和
 * 
 * 对前 9 个字节进行累加和计算（取低 8 位）
 * 
 * @param data 数据指针
 * @return 校验和值
 */
uint8_t camera_uart_calc_checksum(const uint8_t *data);

/**
 * @brief 获取接收统计信息
 * 
 * @param frame_count 接收帧计数指针
 * @param error_count 错误帧计数指针
 * @param checksum_error_count 校验和错误计数指针
 */
void camera_uart_get_rx_stats(uint32_t *frame_count, uint32_t *error_count, uint32_t *checksum_error_count);

/**
 * @brief 获取接收帧历史记录
 * 
 * @param index 历史记录索引（0-9）
 * @param frame 输出帧数据指针
 * @return 0: 成功，-1: 索引无效
 */
int camera_uart_get_rx_history(uint8_t index, camera_frame_data_t *frame);

/**
 * @brief 清空通信统计数据
 */
void camera_uart_clear_stats(void);

#endif // APP_CAMERA_UART_H
