/*
 * drv_gripper.c
 * 夹爪驱动实现
 * 
 * 功能：
 * - 协调舵机、真空泵、电磁阀的硬件操作
 * - 提供原子操作接口
 * - 管理时序延迟
 */

#include "drv_gripper.h"
#include "src/utils/timer.h"

/* ============ 初始化函数 ============ */

void Gripper_Init(void)
{
    /* 调用底层驱动初始化 */
    drv_servo_init();
    VacuumPump_Init();
    
    /* 设置初始位置 */
    drv_servo_set_angle(GRIPPER_IDLE_ANGLE);
    VacuumPump_SetPulse(VACUUM_PUMP_PULSE_CLOSE);
    SolenoidValve_SetPulse(SOLENOID_VALVE_PULSE_CLOSE);
}

/* ============ 舵机操作函数 ============ */

void Gripper_MoveServo(uint16_t angle, uint16_t delay_time)
{
    /* 设置舵机角度 */
    drv_servo_set_angle(angle);
    
    /* 等待舵机到位 */
    if (delay_time > 0) {
        delay_ms((unsigned int)delay_time);
    }
}

uint16_t Gripper_GetServoAngle(void)
{
    return drv_servo_get_angle();
}

/* ============ 真空泵操作函数 ============ */

void Gripper_StartPump(uint16_t delay_time)
{
    /* 启动真空泵 */
    VacuumPump_SetPulse(VACUUM_PUMP_PULSE_OPEN);
    
    /* 等待真空形成 */
    if (delay_time > 0) {
        delay_ms((unsigned int)delay_time);
    }
}

void Gripper_StopPump(uint16_t delay_time)
{
    /* 停止真空泵 */
    VacuumPump_SetPulse(VACUUM_PUMP_PULSE_CLOSE);
    
    /* 延迟 */
    if (delay_time > 0) {
        delay_ms((unsigned int)delay_time);
    }
}

uint16_t Gripper_GetPumpPulse(void)
{
    return VacuumPump_GetPulse();
}

/* ============ 电磁阀操作函数 ============ */

void Gripper_OpenValve(uint16_t delay_time)
{
    /* 打开电磁阀 */
    SolenoidValve_SetPulse(SOLENOID_VALVE_PULSE_OPEN);
    
    /* 等待物体释放 */
    if (delay_time > 0) {
        delay_ms((unsigned int)delay_time);
    }
}

void Gripper_CloseValve(uint16_t delay_time)
{
    /* 关闭电磁阀 */
    SolenoidValve_SetPulse(SOLENOID_VALVE_PULSE_CLOSE);
    
    /* 延迟 */
    if (delay_time > 0) {
        delay_ms((unsigned int)delay_time);
    }
}

uint16_t Gripper_GetValvePulse(void)
{
    return SolenoidValve_GetPulse();
}
