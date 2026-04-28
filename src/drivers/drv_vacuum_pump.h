/*
 * 真空泵驱动模块头文件
 * 
 * 功能：
 * - 管理真空泵和电磁阀的 PWM 输出
 * - 提供脉宽设置接口
 * - 硬件初始化
 */

#ifndef DRV_VACUUM_PUMP_H
#define DRV_VACUUM_PUMP_H

#include <stdint.h>
#include <stdbool.h>

/* ==================== PWM 脉宽值定义 ==================== */

/* PWM 周期计数值 = 400, 50Hz, 20ms周期 */
#define VACUUM_PUMP_PULSE_MIN       10     /* 0.5ms (最小脉宽) */
#define VACUUM_PUMP_PULSE_MAX       50     /* 2.5ms (最大脉宽) */
#define VACUUM_PUMP_PULSE_CLOSE     10     /* 关闭脉宽 (0.5ms) */
#define VACUUM_PUMP_PULSE_OPEN      50     /* 打开脉宽 (2.5ms) */

#define SOLENOID_VALVE_PULSE_MIN    10     /* 0.5ms (最小脉宽) */
#define SOLENOID_VALVE_PULSE_MAX    50     /* 2.5ms (最大脉宽) */
#define SOLENOID_VALVE_PULSE_CLOSE  10     /* 关闭脉宽 (0.5ms) */
#define SOLENOID_VALVE_PULSE_OPEN   50     /* 打开脉宽 (2.5ms) */

/* ==================== 初始化函数 ==================== */

/*
 * 初始化真空泵驱动模块
 * 
 * 功能：
 * - 初始化 TIMG8 PWM
 * - 配置 PA26 (真空泵) 和 PA30 (电磁阀) GPIO
 * - 设置初始脉宽值
 */
void VacuumPump_Init(void);

/* ==================== 真空泵控制函数 ==================== */

/*
 * 设置真空泵 PWM 脉宽
 * 
 * 参数：
 *   pulse - PWM 脉宽值 (10-50)
 *           10 = 0.5ms (关闭)
 *           50 = 2.5ms (打开)
 */
void VacuumPump_SetPulse(uint16_t pulse);

/*
 * 获取真空泵当前 PWM 脉宽
 * 
 * 返回：
 *   当前脉宽值 (10-50)
 */
uint16_t VacuumPump_GetPulse(void);

/* ==================== 电磁阀控制函数 ==================== */

/*
 * 设置电磁阀 PWM 脉宽
 * 
 * 参数：
 *   pulse - PWM 脉宽值 (10-50)
 *           10 = 0.5ms (关闭)
 *           50 = 2.5ms (打开)
 */
void SolenoidValve_SetPulse(uint16_t pulse);

/*
 * 获取电磁阀当前 PWM 脉宽
 * 
 * 返回：
 *   当前脉宽值 (10-50)
 */
uint16_t SolenoidValve_GetPulse(void);

#endif /* DRV_VACUUM_PUMP_H */
