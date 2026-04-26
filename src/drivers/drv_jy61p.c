#include "drv_jy61p.h"
#include "ti_msp_dl_config.h"

/* ============ 全局变量定义 ============ */
jy61p_rx_t g_jy61p_rx = {0};

/* ============ 内部函数声明 ============ */
static uint8_t jy61p_verify_checksum(const uint8_t *frame);
static void jy61p_parse_frame(const uint8_t *frame);

/* ============ 初始化函数 ============ */

/**
 * @brief 初始化 JY61P 驱动
 * 
 * 配置环形缓冲 DMA 接收，每次接收 11 字节
 * 使用流式扫描自动恢复帧同步
 */
void jy61p_init(void)
{
    // 初始化接收状态
    g_jy61p_rx.write_pos = 0;
    g_jy61p_rx.frame_ready = 0;
    g_jy61p_rx.frame_type = 0;
    g_jy61p_rx.frame_count = 0;
    g_jy61p_rx.error_count = 0;
    
    // 初始化数据更新标志
    g_jy61p_rx.acc_updated = 0;
    g_jy61p_rx.gyro_updated = 0;
    g_jy61p_rx.angle_updated = 0;
    
    // 配置 DMA 源地址（UART IMU RX FIFO）
    DL_DMA_setSrcAddr(DMA, DMA_IMU_RX_CHAN_ID, 
                      (uint32_t)(&UART_IMU_INST->RXDATA));
    
    // 配置 DMA 目标地址（环形缓冲起始位置）
    DL_DMA_setDestAddr(DMA, DMA_IMU_RX_CHAN_ID, 
                       (uint32_t)&g_jy61p_rx.buffer[0]);
    
    // 配置 DMA 传输大小（每次 11 字节）
    DL_DMA_setTransferSize(DMA, DMA_IMU_RX_CHAN_ID, JY61P_DMA_TRANSFER_SIZE);
    
    // 启用 DMA 通道
    DL_DMA_enableChannel(DMA, DMA_IMU_RX_CHAN_ID);
    
    // 确认 DMA 通道已启用
    while (false == DL_DMA_isChannelEnabled(DMA, DMA_IMU_RX_CHAN_ID)) {
        ;
    }
    
    // 启用 UART IMU 中断
    NVIC_EnableIRQ(UART_IMU_INST_INT_IRQN);
}

/* ============ 中断处理函数 ============ */

/**
 * @brief JY61P UART 中断处理函数
 * 
 * 处理 DMA_DONE_RX 中断：
 * 1. 在环形缓冲中流式扫描帧头（搜索范围：最近 22 字节）
 * 2. 验证帧头和校验和
 * 3. 如果有效，解析数据并设置更新标志
 * 4. 更新 write_pos
 * 5. 重新配置 DMA 继续接收下一个 11 字节
 */
void jy61p_isr_handler(void)
{
    switch (DL_UART_Main_getPendingInterrupt(UART_IMU_INST)) {
        case DL_UART_MAIN_IIDX_DMA_DONE_RX:
        {
            // 当前 DMA 写入位置（刚写完的 11 字节的起始位置）
            uint16_t current_write_pos = g_jy61p_rx.write_pos;
            
            // 流式扫描：在最近 22 字节范围内搜索帧头
            // 搜索范围：[write_pos, write_pos+11]（环形）
            // 这样可以处理帧边界跨越 DMA 传输边界的情况
            uint8_t frame_found = 0;
            
            for (int offset = 0; offset <= 11; offset++) {
                // 计算帧头可能的起始位置（环形索引）
                uint16_t frame_start = (current_write_pos + offset) % JY61P_RX_BUFFER_SIZE;
                
                // 计算帧内各字节的环形索引
                uint16_t idx0 = frame_start;
                uint16_t idx1 = (frame_start + 1) % JY61P_RX_BUFFER_SIZE;
                
                // 验证帧头（0x55 + 帧类型）
                if (g_jy61p_rx.buffer[idx0] == JY61P_FRAME_HEADER) {
                    uint8_t frame_type = g_jy61p_rx.buffer[idx1];
                    
                    // 检查帧类型是否有效
                    if (frame_type == JY61P_FRAME_TYPE_ACC ||
                        frame_type == JY61P_FRAME_TYPE_GYRO ||
                        frame_type == JY61P_FRAME_TYPE_ANGLE) {
                        
                        // 复制完整帧到临时缓冲
                        uint8_t temp_frame[JY61P_FRAME_SIZE];
                        for (int i = 0; i < JY61P_FRAME_SIZE; i++) {
                            uint16_t idx = (frame_start + i) % JY61P_RX_BUFFER_SIZE;
                            temp_frame[i] = g_jy61p_rx.buffer[idx];
                        }
                        
                        // 验证校验和
                        if (jy61p_verify_checksum(temp_frame)) {
                            // 找到有效帧，复制到 frame_data
                            memcpy(g_jy61p_rx.frame_data, temp_frame, JY61P_FRAME_SIZE);
                            g_jy61p_rx.frame_ready = 1;
                            g_jy61p_rx.frame_type = frame_type;
                            g_jy61p_rx.frame_count++;
                            
                            // 解析数据
                            jy61p_parse_frame(temp_frame);
                            
                            frame_found = 1;
                            break;
                        }
                    }
                }
            }
            
            // 如果未找到有效帧，设置错误标志
            if (!frame_found) {
                g_jy61p_rx.error_count++;
            }
            
            // 更新写入位置（环形）
            g_jy61p_rx.write_pos = (g_jy61p_rx.write_pos + JY61P_DMA_TRANSFER_SIZE) % JY61P_RX_BUFFER_SIZE;
            
            // 重新配置 DMA 接收下一个 11 字节
            DL_DMA_setDestAddr(DMA, DMA_IMU_RX_CHAN_ID, 
                              (uint32_t)&g_jy61p_rx.buffer[g_jy61p_rx.write_pos]);
            DL_DMA_setTransferSize(DMA, DMA_IMU_RX_CHAN_ID, JY61P_DMA_TRANSFER_SIZE);
            DL_DMA_enableChannel(DMA, DMA_IMU_RX_CHAN_ID);
            
            break;
        }
        default:
            break;
    }
}

/* ============ 内部函数实现 ============ */

/**
 * @brief 验证帧校验和
 * @param frame 帧数据指针（11 字节）
 * @return 1: 校验通过，0: 校验失败
 */
static uint8_t jy61p_verify_checksum(const uint8_t *frame)
{
    uint8_t sum = 0;
    
    // 计算前 10 字节的和
    for (int i = 0; i < 10; i++) {
        sum += frame[i];
    }
    
    // 比较计算值与帧中的校验和
    return (sum == frame[10]);
}

/**
 * @brief 解析帧数据
 * @param frame 帧数据指针（11 字节）
 */
static void jy61p_parse_frame(const uint8_t *frame)
{
    uint8_t frame_type = frame[1];
    
    switch (frame_type) {
        case JY61P_FRAME_TYPE_ACC:  // 加速度帧
        {
            // 提取原始值
            g_jy61p_rx.acc.ax = (int16_t)((frame[3] << 8) | frame[2]);
            g_jy61p_rx.acc.ay = (int16_t)((frame[5] << 8) | frame[4]);
            g_jy61p_rx.acc.az = (int16_t)((frame[7] << 8) | frame[6]);
            g_jy61p_rx.acc.temp = (int16_t)((frame[9] << 8) | frame[8]);
            
            // 转换为物理单位
            g_jy61p_rx.acc.ax_g = (float)g_jy61p_rx.acc.ax / 32768.0f * 16.0f;
            g_jy61p_rx.acc.ay_g = (float)g_jy61p_rx.acc.ay / 32768.0f * 16.0f;
            g_jy61p_rx.acc.az_g = (float)g_jy61p_rx.acc.az / 32768.0f * 16.0f;
            g_jy61p_rx.acc.temp_c = (float)g_jy61p_rx.acc.temp / 100.0f;
            
            // 设置更新标志
            g_jy61p_rx.acc_updated = 1;
            break;
        }
        
        case JY61P_FRAME_TYPE_GYRO:  // 角速度帧
        {
            // 提取原始值
            g_jy61p_rx.gyro.wx = (int16_t)((frame[3] << 8) | frame[2]);
            g_jy61p_rx.gyro.wy = (int16_t)((frame[5] << 8) | frame[4]);
            g_jy61p_rx.gyro.wz = (int16_t)((frame[7] << 8) | frame[6]);
            g_jy61p_rx.gyro.voltage = (uint16_t)((frame[9] << 8) | frame[8]);
            
            // 转换为物理单位
            g_jy61p_rx.gyro.wx_dps = (float)g_jy61p_rx.gyro.wx / 32768.0f * 2000.0f;
            g_jy61p_rx.gyro.wy_dps = (float)g_jy61p_rx.gyro.wy / 32768.0f * 2000.0f;
            g_jy61p_rx.gyro.wz_dps = (float)g_jy61p_rx.gyro.wz / 32768.0f * 2000.0f;
            g_jy61p_rx.gyro.voltage_v = (float)g_jy61p_rx.gyro.voltage / 100.0f;
            
            // 设置更新标志
            g_jy61p_rx.gyro_updated = 1;
            break;
        }
        
        case JY61P_FRAME_TYPE_ANGLE:  // 欧拉角帧
        {
            // 提取原始值
            g_jy61p_rx.angle.roll = (int16_t)((frame[3] << 8) | frame[2]);
            g_jy61p_rx.angle.pitch = (int16_t)((frame[5] << 8) | frame[4]);
            g_jy61p_rx.angle.yaw = (int16_t)((frame[7] << 8) | frame[6]);
            g_jy61p_rx.angle.version = (uint16_t)((frame[9] << 8) | frame[8]);
            
            // 转换为物理单位
            g_jy61p_rx.angle.roll_deg = (float)g_jy61p_rx.angle.roll / 32768.0f * 180.0f;
            g_jy61p_rx.angle.pitch_deg = (float)g_jy61p_rx.angle.pitch / 32768.0f * 180.0f;
            g_jy61p_rx.angle.yaw_deg = (float)g_jy61p_rx.angle.yaw / 32768.0f * 180.0f;
            
            // 设置更新标志
            g_jy61p_rx.angle_updated = 1;
            break;
        }
        
        default:
            break;
    }
}

/* ============ 数据获取函数 ============ */

/**
 * @brief 获取加速度数据
 * @param acc 输出加速度数据指针
 * @return 0: 成功，-1: 无新数据
 */
int jy61p_get_acc(jy61p_acc_t *acc)
{
    if (g_jy61p_rx.acc_updated == 0) {
        return -1;  // 无新数据
    }
    
    // 复制数据
    memcpy(acc, &g_jy61p_rx.acc, sizeof(jy61p_acc_t));
    
    // 清除更新标志
    g_jy61p_rx.acc_updated = 0;
    
    return 0;
}

/**
 * @brief 获取角速度数据
 * @param gyro 输出角速度数据指针
 * @return 0: 成功，-1: 无新数据
 */
int jy61p_get_gyro(jy61p_gyro_t *gyro)
{
    if (g_jy61p_rx.gyro_updated == 0) {
        return -1;  // 无新数据
    }
    
    // 复制数据
    memcpy(gyro, &g_jy61p_rx.gyro, sizeof(jy61p_gyro_t));
    
    // 清除更新标志
    g_jy61p_rx.gyro_updated = 0;
    
    return 0;
}

/**
 * @brief 获取欧拉角数据
 * @param angle 输出欧拉角数据指针
 * @return 0: 成功，-1: 无新数据
 */
int jy61p_get_angle(jy61p_angle_t *angle)
{
    if (g_jy61p_rx.angle_updated == 0) {
        return -1;  // 无新数据
    }
    
    // 复制数据
    memcpy(angle, &g_jy61p_rx.angle, sizeof(jy61p_angle_t));
    
    // 清除更新标志
    g_jy61p_rx.angle_updated = 0;
    
    return 0;
}

/* ============ 统计函数 ============ */

/**
 * @brief 获取接收统计信息
 * @param frame_count 成功接收帧数
 * @param error_count 错误帧数
 */
void jy61p_get_stats(uint32_t *frame_count, uint32_t *error_count)
{
    if (frame_count != NULL) {
        *frame_count = g_jy61p_rx.frame_count;
    }
    
    if (error_count != NULL) {
        *error_count = g_jy61p_rx.error_count;
    }
}
