/**
 * @file drv_jy61p.c
 * @brief JY61P 陀螺仪 UART 驱动实现（简化版）
 * 
 * 功能：
 * - 简单的 UART1 中断接收
 * - 流式帧扫描和解析
 * - 偏航角数据提取
 * - 零点校准功能
 */

#include "drv_jy61p.h"
#include "ti_msp_dl_config.h"
#include <string.h>

/* ============ 全局变量定义 ============ */
jy61p_rx_t g_jy61p_rx = {0};

/* ============ 校准相关变量 ============ */
static uint32_t s_calibration_target = 0;  // 目标采样次数

/* ============ 初始化函数 ============ */
void jy61p_init(void)
{
    memset(&g_jy61p_rx, 0, sizeof(jy61p_rx_t));
    
    // 调用 SysConfig 生成的 UART1 初始化函数
    SYSCFG_DL_UART_IMU_init();
    
    // 清除待处理中断
    NVIC_ClearPendingIRQ(UART_IMU_INST_INT_IRQN);
    
    // 启用 NVIC 中断
    NVIC_EnableIRQ(UART_IMU_INST_INT_IRQN);
    
    // 初始化零点偏移
    g_jy61p_rx.angle.yaw_offset = 0;
}

/* ============ 中断处理函数 ============ */
void jy61p_isr_handler(void)
{
    // 获取中断类型
    uint32_t interruptStatus = DL_UART_Main_getPendingInterrupt(UART_IMU_INST);
    
    // 如果是接收中断
    if (interruptStatus & DL_UART_MAIN_IIDX_RX) {
        // 读取接收到的字节
        uint8_t byte = DL_UART_Main_receiveData(UART_IMU_INST);
        
        // 存储到接收缓冲
        if (g_jy61p_rx.write_pos < JY61P_RX_BUFFER_SIZE) {
            g_jy61p_rx.buffer[g_jy61p_rx.write_pos++] = byte;
        } else {
            // 缓冲满，重置
            g_jy61p_rx.write_pos = 0;
            g_jy61p_rx.buffer[g_jy61p_rx.write_pos++] = byte;
        }
        
        // 流式扫描帧头
        if (g_jy61p_rx.write_pos >= JY61P_FRAME_SIZE) {
            // 检查最后 11 个字节是否构成有效帧
            uint16_t scan_pos = g_jy61p_rx.write_pos - JY61P_FRAME_SIZE;
            
            // 检查帧头
            if (g_jy61p_rx.buffer[scan_pos] == JY61P_FRAME_HEADER) {
                uint8_t frame_type = g_jy61p_rx.buffer[scan_pos + 1];
                
                // 只处理欧拉角帧（0x53）
                if (frame_type == JY61P_FRAME_TYPE_ANGLE) {
                    // 验证校验和（前 10 字节之和 = 第 11 字节）
                    uint8_t sum = 0;
                    for (int i = 0; i < 10; i++) {
                        sum += g_jy61p_rx.buffer[scan_pos + i];
                    }
                    
                    if (sum == g_jy61p_rx.buffer[scan_pos + 10]) {
                        // 有效帧，保存数据
                        memcpy(g_jy61p_rx.frame_data, 
                               &g_jy61p_rx.buffer[scan_pos], 
                               JY61P_FRAME_SIZE);
                        g_jy61p_rx.frame_ready = 1;
                        g_jy61p_rx.frame_type = frame_type;
                        g_jy61p_rx.frame_count++;
                        
                        // 解析欧拉角（只需偏航角）
                        // 帧格式：[0x55][0x53][YAW_L][YAW_H][...][CHECKSUM]
                        // 偏航角在字节 2-3（小端序）
                        g_jy61p_rx.angle.yaw = (int16_t)((g_jy61p_rx.frame_data[7] << 8) | g_jy61p_rx.frame_data[6]);
                        
                        // 整数运算：角度 = 原始值 * 18000 / 32768（单位：0.01°）
                        g_jy61p_rx.angle.yaw_deg_x100 = ((int32_t)g_jy61p_rx.angle.yaw * 18000) / 32768;
                        
                        // 如果正在校准，累加样本
                        if (g_jy61p_rx.calibration_count > 0 && !g_jy61p_rx.calibration_done) {
                            g_jy61p_rx.calibration_sum += g_jy61p_rx.angle.yaw_deg_x100;
                            g_jy61p_rx.calibration_count--;
                            
                            // 校准完成，计算平均值作为零点偏移
                            if (g_jy61p_rx.calibration_count == 0) {
                                // 计算平均值
                                g_jy61p_rx.angle.yaw_offset = g_jy61p_rx.calibration_sum / s_calibration_target;
                                g_jy61p_rx.calibration_done = 1;
                            }
                        }
                        
                        g_jy61p_rx.angle_updated = 1;
                    } else {
                        g_jy61p_rx.error_count++;
                    }
                }
            }
        }
    }
}

/* ============ 数据获取函数 ============ */
int jy61p_get_angle(jy61p_angle_t *angle)
{
    if (g_jy61p_rx.angle_updated == 0) {
        return -1;
    }
    
    angle->yaw = g_jy61p_rx.angle.yaw;
    // 应用零点偏移
    angle->yaw_deg_x100 = g_jy61p_rx.angle.yaw_deg_x100 - g_jy61p_rx.angle.yaw_offset;
    angle->yaw_offset = g_jy61p_rx.angle.yaw_offset;
    
    g_jy61p_rx.angle_updated = 0;
    
    return 0;
}

void jy61p_start_calibration(uint32_t sample_count)
{
    // 重置校准状态
    g_jy61p_rx.calibration_done = 0;
    g_jy61p_rx.calibration_count = 0;
    g_jy61p_rx.calibration_sum = 0;
    
    // 设置目标采样次数
    if (sample_count == 0) {
        sample_count = 10;  // 默认 10 次采样
    }
    s_calibration_target = sample_count;
    g_jy61p_rx.calibration_count = sample_count;
}

uint8_t jy61p_is_calibration_done(void)
{
    return g_jy61p_rx.calibration_done;
}

void jy61p_get_stats(uint32_t *frame_count, uint32_t *error_count)
{
    if (frame_count != NULL) {
        *frame_count = g_jy61p_rx.frame_count;
    }
    if (error_count != NULL) {
        *error_count = g_jy61p_rx.error_count;
    }
}
