/**
 * @file drv_jy61p.h
 * @brief JY61P 陀螺仪 UART 驱动头文件（简化版）
 * 
 * 功能：
 * - 简单的 UART1 中断接收
 * - 接收缓冲存储
 * - 偏航角数据提取
 */

#ifndef DRV_JY61P_H
#define DRV_JY61P_H

#include <stdint.h>
#include <string.h>

/* ============ JY61P 配置常量 ============ */
#define JY61P_FRAME_SIZE        11          // 固定帧长（包含帧头和校验）
#define JY61P_RX_BUFFER_SIZE    128         // 接收缓冲大小

/* 帧格式定义 */
#define JY61P_FRAME_HEADER      0x55        // 帧头
#define JY61P_FRAME_TYPE_ANGLE  0x53        // 欧拉角帧

/* ============ JY61P 数据结构体 ============ */

/**
 * @brief 欧拉角数据结构（只保留偏航角）
 */
typedef struct {
    int16_t yaw;                // 偏航角（原始值）
    int32_t yaw_deg_x100;       // 偏航角（°×100，避免浮点）
    int32_t yaw_offset;         // 偏航角零点偏移（用于校准）
} jy61p_angle_t;

/**
 * @brief JY61P 接收结构体
 */
typedef struct {
    uint8_t buffer[JY61P_RX_BUFFER_SIZE];   // 接收缓冲
    uint16_t write_pos;                     // 写入位置
    uint8_t frame_data[JY61P_FRAME_SIZE];   // 有效帧数据
    uint8_t frame_ready;                    // 帧就绪标志
    uint8_t frame_type;                     // 帧类型（0x53）
    uint32_t frame_count;                   // 成功接收帧计数
    uint32_t error_count;                   // 错误帧计数
    
    // 解析后的数据
    jy61p_angle_t angle;                    // 欧拉角数据
    
    // 数据更新标志
    uint8_t angle_updated;                  // 欧拉角数据已更新
    
    // 零点校准相关
    uint8_t calibration_done;               // 校准完成标志
    uint32_t calibration_count;             // 校准采样计数
    int64_t calibration_sum;                // 校准采样累加和
} jy61p_rx_t;

/* ============ 全局变量声明 ============ */
extern jy61p_rx_t g_jy61p_rx;

/* ============ 函数声明 ============ */

/**
 * @brief 初始化 JY61P 驱动
 */
void jy61p_init(void);

/**
 * @brief JY61P UART 中断处理函数
 */
void jy61p_isr_handler(void);

/**
 * @brief 获取欧拉角数据（偏航角）
 * @param angle 输出欧拉角数据指针
 * @return 0: 成功，-1: 无新数据
 */
int jy61p_get_angle(jy61p_angle_t *angle);

/**
 * @brief 启动零点校准（采集 N 个样本取平均）
 * @param sample_count 采样次数（建议 10-20）
 */
void jy61p_start_calibration(uint32_t sample_count);

/**
 * @brief 检查校准是否完成
 * @return 1: 完成，0: 未完成
 */
uint8_t jy61p_is_calibration_done(void);

/**
 * @brief 获取接收统计信息
 * @param frame_count 成功接收帧数
 * @param error_count 错误帧数
 */
void jy61p_get_stats(uint32_t *frame_count, uint32_t *error_count);

#endif // DRV_JY61P_H
