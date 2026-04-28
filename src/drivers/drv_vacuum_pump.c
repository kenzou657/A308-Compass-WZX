/*
 * 真空泵驱动模块实现
 * 
 * 功能：
 * - 管理真空泵和电磁阀的 PWM 输出
 * - 提供脉宽设置接口
 * - 硬件初始化
 */

#include "drv_vacuum_pump.h"
#include "ti_msp_dl_config.h"

/* ==================== 全局变量 ==================== */

/* 真空泵当前脉宽值 */
static uint16_t g_vacuum_pump_pulse = VACUUM_PUMP_PULSE_CLOSE;

/* 电磁阀当前脉宽值 */
static uint16_t g_solenoid_valve_pulse = SOLENOID_VALVE_PULSE_CLOSE;

/* ==================== 初始化函数实现 ==================== */

void VacuumPump_Init(void)
{
    /* TIMG8 已在 SysConfig 中初始化，此处无需重复初始化 */
    
    /* 设置初始脉宽值 */
    g_vacuum_pump_pulse = VACUUM_PUMP_PULSE_CLOSE;
    g_solenoid_valve_pulse = SOLENOID_VALVE_PULSE_CLOSE;
    
    /* 更新 PWM 输出 */
    DL_TimerG_setCaptureCompareValue(PWM_VACUUM_INST, g_vacuum_pump_pulse, DL_TIMER_CC_0_INDEX);
    DL_TimerG_setCaptureCompareValue(PWM_VACUUM_INST, g_solenoid_valve_pulse, DL_TIMER_CC_1_INDEX);
}

/* ==================== 真空泵控制函数实现 ==================== */

void VacuumPump_SetPulse(uint16_t pulse)
{
    /* 限制脉宽范围 */
    if (pulse < VACUUM_PUMP_PULSE_MIN) {
        pulse = VACUUM_PUMP_PULSE_MIN;
    }
    if (pulse > VACUUM_PUMP_PULSE_MAX) {
        pulse = VACUUM_PUMP_PULSE_MAX;
    }
    
    g_vacuum_pump_pulse = pulse;
    
    /* 更新 PWM 输出 (TIMG8-CH0, PA26) */
    DL_TimerG_setCaptureCompareValue(PWM_VACUUM_INST, pulse, DL_TIMER_CC_0_INDEX);
}

uint16_t VacuumPump_GetPulse(void)
{
    return g_vacuum_pump_pulse;
}

/* ==================== 电磁阀控制函数实现 ==================== */

void SolenoidValve_SetPulse(uint16_t pulse)
{
    /* 限制脉宽范围 */
    if (pulse < SOLENOID_VALVE_PULSE_MIN) {
        pulse = SOLENOID_VALVE_PULSE_MIN;
    }
    if (pulse > SOLENOID_VALVE_PULSE_MAX) {
        pulse = SOLENOID_VALVE_PULSE_MAX;
    }
    
    g_solenoid_valve_pulse = pulse;
    
    /* 更新 PWM 输出 (TIMG8-CH1, PA30) */
    DL_TimerG_setCaptureCompareValue(PWM_VACUUM_INST, pulse, DL_TIMER_CC_1_INDEX);
}

uint16_t SolenoidValve_GetPulse(void)
{
    return g_solenoid_valve_pulse;
}
