#ifndef DRV_JY61P_H
#define DRV_JY61P_H

#include <stdint.h>
#include <string.h>

/* ============ JY61P 配置常量 ============ */
#define JY61P_FRAME_SIZE        11          // 固定帧长（包含帧头和校验）
#define JY61P_RX_BUFFER_SIZE    55          // 环形接收缓冲大小（5 个完整帧）
#define JY61P_DMA_TRANSFER_SIZE 11          // DMA 每次传输大小

/* 帧格式定义 */
#define JY61P_FRAME_HEADER      0x55        // 帧头
#define JY61P_FRAME_TYPE_ACC    0x51        // 加速度帧
#define JY61P_FRAME_TYPE_GYRO   0x52        // 角速度帧
#define JY61P_FRAME_TYPE_ANGLE  0x53        // 欧拉角帧

/* ============ JY61P 数据结构体 ============ */

/**
 * @brief 加速度数据结构
 */
typedef struct {
    int16_t ax;         // X 轴加速度（原始值）
    int16_t ay;         // Y 轴加速度（原始值）
    int16_t az;         // Z 轴加速度（原始值）
    int16_t temp;       // 温度（原始值）
    float ax_g;         // X 轴加速度（g）
    float ay_g;         // Y 轴加速度（g）
    float az_g;         // Z 轴加速度（g）
    float temp_c;       // 温度（°C）
} jy61p_acc_t;

/**
 * @brief 角速度数据结构
 */
typedef struct {
    int16_t wx;         // X 轴角速度（原始值）
    int16_t wy;         // Y 轴角速度（原始值）
    int16_t wz;         // Z 轴角速度（原始值）
    uint16_t voltage;   // 电压（原始值）
    float wx_dps;       // X 轴角速度（°/s）
    float wy_dps;       // Y 轴角速度（°/s）
    float wz_dps;       // Z 轴角速度（°/s）
    float voltage_v;    // 电压（V）
} jy61p_gyro_t;

/**
 * @brief 欧拉角数据结构
 */
typedef struct {
    int16_t roll;       // 横滚角（原始值）
    int16_t pitch;      // 俯仰角（原始值）
    int16_t yaw;        // 偏航角（原始值）
    uint16_t version;   // 版本号
    float roll_deg;     // 横滚角（°）
    float pitch_deg;    // 俯仰角（°）
    float yaw_deg;      // 偏航角（°）
} jy61p_angle_t;

/**
 * @brief JY61P 接收结构体
 */
typedef struct {
    uint8_t buffer[JY61P_RX_BUFFER_SIZE];   // 环形缓冲
    uint16_t write_pos;                     // DMA 写入位置（0-54）
    uint8_t frame_data[JY61P_FRAME_SIZE];   // 有效帧数据
    uint8_t frame_ready;                    // 帧就绪标志
    uint8_t frame_type;                     // 帧类型（0x51/0x52/0x53）
    uint32_t frame_count;                   // 成功接收帧计数
    uint32_t error_count;                   // 错误帧计数
    
    // 解析后的数据
    jy61p_acc_t acc;                        // 加速度数据
    jy61p_gyro_t gyro;                      // 角速度数据
    jy61p_angle_t angle;                    // 欧拉角数据
    
    // 数据更新标志
    uint8_t acc_updated;                    // 加速度数据已更新
    uint8_t gyro_updated;                   // 角速度数据已更新
    uint8_t angle_updated;                  // 欧拉角数据已更新
} jy61p_rx_t;

/* ============ 全局变量声明 ============ */
extern jy61p_rx_t g_jy61p_rx;

/* ============ 函数声明 ============ */

/**
 * @brief 初始化 JY61P 驱动
 * 
 * 配置环形缓冲 DMA 接收，每次接收 11 字节
 * 使用流式扫描自动恢复帧同步
 */
void jy61p_init(void);

/**
 * @brief JY61P UART 中断处理函数
 * 
 * 处理 DMA_DONE_RX 中断：
 * - 在环形缓冲中流式扫描帧头
 * - 验证帧头和校验和
 * - 自动恢复帧同步
 * - 解析数据
 * - 重新配置 DMA
 */
void jy61p_isr_handler(void);

/**
 * @brief 获取加速度数据
 * @param acc 输出加速度数据指针
 * @return 0: 成功，-1: 无新数据
 */
int jy61p_get_acc(jy61p_acc_t *acc);

/**
 * @brief 获取角速度数据
 * @param gyro 输出角速度数据指针
 * @return 0: 成功，-1: 无新数据
 */
int jy61p_get_gyro(jy61p_gyro_t *gyro);

/**
 * @brief 获取欧拉角数据
 * @param angle 输出欧拉角数据指针
 * @return 0: 成功，-1: 无新数据
 */
int jy61p_get_angle(jy61p_angle_t *angle);

/**
 * @brief 获取接收统计信息
 * @param frame_count 成功接收帧数
 * @param error_count 错误帧数
 */
void jy61p_get_stats(uint32_t *frame_count, uint32_t *error_count);

#endif // DRV_JY61P_H
