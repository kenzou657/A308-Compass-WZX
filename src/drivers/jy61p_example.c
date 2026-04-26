/**
 * @file jy61p_example.c
 * @brief JY61P 陀螺仪驱动使用示例
 */

#include "drivers/drv_jy61p.h"
#include "ti_msp_dl_config.h"

/**
 * @brief JY61P 使用示例
 */
void jy61p_example(void)
{
    jy61p_acc_t acc;
    jy61p_gyro_t gyro;
    jy61p_angle_t angle;
    uint32_t frame_count, error_count;
    
    // 1. 初始化 JY61P 驱动
    jy61p_init();
    
    // 2. 主循环中读取数据
    while (1) {
        // 读取加速度数据
        if (jy61p_get_acc(&acc) == 0) {
            // 成功获取新的加速度数据
            // acc.ax_g, acc.ay_g, acc.az_g: 加速度（g）
            // acc.temp_c: 温度（°C）
            
            // 示例：打印加速度
            // printf("加速度: X=%.3f g, Y=%.3f g, Z=%.3f g, 温度=%.2f°C\n",
            //        acc.ax_g, acc.ay_g, acc.az_g, acc.temp_c);
        }
        
        // 读取角速度数据
        if (jy61p_get_gyro(&gyro) == 0) {
            // 成功获取新的角速度数据
            // gyro.wx_dps, gyro.wy_dps, gyro.wz_dps: 角速度（°/s）
            // gyro.voltage_v: 电压（V）
            
            // 示例：打印角速度
            // printf("角速度: X=%.2f°/s, Y=%.2f°/s, Z=%.2f°/s, 电压=%.2fV\n",
            //        gyro.wx_dps, gyro.wy_dps, gyro.wz_dps, gyro.voltage_v);
        }
        
        // 读取欧拉角数据
        if (jy61p_get_angle(&angle) == 0) {
            // 成功获取新的欧拉角数据
            // angle.roll_deg: 横滚角（°）
            // angle.pitch_deg: 俯仰角（°）
            // angle.yaw_deg: 偏航角（°）
            
            // 示例：打印欧拉角
            // printf("欧拉角: Roll=%.2f°, Pitch=%.2f°, Yaw=%.2f°\n",
            //        angle.roll_deg, angle.pitch_deg, angle.yaw_deg);
        }
        
        // 定期获取统计信息（可选）
        jy61p_get_stats(&frame_count, &error_count);
        // printf("统计: 成功帧=%u, 错误帧=%u\n", frame_count, error_count);
        
        // 延时
        delay_ms(10);
    }
}

/**
 * @brief 简单的姿态监控示例
 */
void jy61p_attitude_monitor(void)
{
    jy61p_angle_t angle;
    
    jy61p_init();
    
    while (1) {
        if (jy61p_get_angle(&angle) == 0) {
            // 检查小车是否倾斜
            if (angle.pitch_deg > 30.0f || angle.pitch_deg < -30.0f) {
                // 俯仰角过大，可能翻车
                // 采取保护措施...
            }
            
            if (angle.roll_deg > 30.0f || angle.roll_deg < -30.0f) {
                // 横滚角过大，可能翻车
                // 采取保护措施...
            }
        }
        
        delay_ms(20);
    }
}

/**
 * @brief 角速度积分示例（用于航向估计）
 */
void jy61p_heading_estimation(void)
{
    jy61p_gyro_t gyro;
    float heading = 0.0f;  // 航向角（°）
    uint32_t last_time = 0;
    uint32_t current_time;
    float dt;
    
    jy61p_init();
    
    while (1) {
        if (jy61p_get_gyro(&gyro) == 0) {
            // 获取当前时间
            current_time = get_system_time_ms();
            dt = (current_time - last_time) / 1000.0f;  // 转换为秒
            last_time = current_time;
            
            // 积分 Z 轴角速度得到航向角
            heading += gyro.wz_dps * dt;
            
            // 限制在 0-360° 范围
            if (heading > 360.0f) heading -= 360.0f;
            if (heading < 0.0f) heading += 360.0f;
            
            // 使用航向角进行导航...
        }
        
        delay_ms(10);
    }
}
