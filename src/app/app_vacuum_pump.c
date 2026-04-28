/*
 * 真空泵应用模块实现
 * 
 * 功能：
 * - 实现完整的吸物/卸物流程
 * - 管理状态转移和时序
 * - 超时保护
 * - 根据任务产生的标志位驱动状态转移
 */

#include "app_vacuum_pump.h"
#include "drv_vacuum_pump.h"
#include "../utils/timer.h"

/* ==================== 全局变量 ==================== */

/* 当前状态 */
static VacuumPumpState_t g_vacuum_state = VACUUM_STATE_IDLE;

/* 待处理的控制指令 */
static VacuumPumpCmd_t g_vacuum_cmd = VACUUM_CMD_IDLE;

/* 状态转移计时器 */
static uint32_t g_state_timer = 0;

/* 总运行时间计时器 */
static uint32_t g_total_timer = 0;

/* ==================== 初始化函数实现 ==================== */

void VacuumPump_App_Init(void)
{
    /* 初始化驱动层 */
    VacuumPump_Init();
    
    /* 初始化状态 */
    g_vacuum_state = VACUUM_STATE_IDLE;
    g_vacuum_cmd = VACUUM_CMD_IDLE;
    g_state_timer = 0;
    g_total_timer = 0;
    
    /* 设置初始 PWM 值（全部关闭） */
    VacuumPump_SetPulse(VACUUM_PUMP_PULSE_CLOSE);
    SolenoidValve_SetPulse(SOLENOID_VALVE_PULSE_CLOSE);
}

/* ==================== 更新函数实现 ==================== */

void VacuumPump_App_Update(void)
{
    /* 获取当前时间戳 */
    uint32_t current_tick = MW_Timer_GetTick();
    
    switch (g_vacuum_state) {
        case VACUUM_STATE_IDLE:
            /* 待命状态：等待控制指令 */
            if (g_vacuum_cmd == VACUUM_CMD_SUCTION) {
                /* 转移到吸物流程 */
                g_vacuum_state = VACUUM_STATE_SUCTION_CLOSE_VALVE;
                g_state_timer = current_tick;
                g_total_timer = current_tick;
                g_vacuum_cmd = VACUUM_CMD_IDLE;  /* 清除指令 */
            }
            break;
            
        case VACUUM_STATE_SUCTION_CLOSE_VALVE:
            /* 关闭电磁阀 (100ms) */
            SolenoidValve_SetPulse(SOLENOID_VALVE_PULSE_CLOSE);
            
            if (current_tick - g_state_timer >= SUCTION_CLOSE_VALVE_TIME) {
                /* 时间到，转移到启动真空泵 */
                g_vacuum_state = VACUUM_STATE_SUCTION_START_PUMP;
                g_state_timer = current_tick;
            }
            break;
            
        case VACUUM_STATE_SUCTION_START_PUMP:
            /* 启动真空泵 (100ms) */
            VacuumPump_SetPulse(VACUUM_PUMP_PULSE_OPEN);
            
            if (current_tick - g_state_timer >= SUCTION_START_PUMP_TIME) {
                /* 时间到，转移到吸物状态 */
                g_vacuum_state = VACUUM_STATE_SUCKING;
                g_state_timer = current_tick;
            }
            break;
            
        case VACUUM_STATE_SUCKING:
            /* 吸物中：真空泵运行 */
            VacuumPump_SetPulse(VACUUM_PUMP_PULSE_OPEN);
            SolenoidValve_SetPulse(SOLENOID_VALVE_PULSE_CLOSE);
            
            /* 检查超时 (60秒) */
            if (current_tick - g_total_timer >= SUCTION_MAX_TIME) {
                /* 超时，自动停止 */
                g_vacuum_state = VACUUM_STATE_IDLE;
                VacuumPump_SetPulse(VACUUM_PUMP_PULSE_CLOSE);
                SolenoidValve_SetPulse(SOLENOID_VALVE_PULSE_CLOSE);
            }
            /* 检查释放指令 */
            else if (g_vacuum_cmd == VACUUM_CMD_RELEASE) {
                /* 转移到卸物流程 */
                g_vacuum_state = VACUUM_STATE_RELEASE_STOP_PUMP;
                g_state_timer = current_tick;
                g_total_timer = current_tick;
                g_vacuum_cmd = VACUUM_CMD_IDLE;  /* 清除指令 */
            }
            break;
            
        case VACUUM_STATE_RELEASE_STOP_PUMP:
            /* 停止真空泵 (100ms) */
            VacuumPump_SetPulse(VACUUM_PUMP_PULSE_CLOSE);
            
            if (current_tick - g_state_timer >= RELEASE_STOP_PUMP_TIME) {
                /* 时间到，转移到打开电磁阀 */
                g_vacuum_state = VACUUM_STATE_RELEASE_OPEN_VALVE;
                g_state_timer = current_tick;
            }
            break;
            
        case VACUUM_STATE_RELEASE_OPEN_VALVE:
            /* 打开电磁阀 (2秒) */
            SolenoidValve_SetPulse(SOLENOID_VALVE_PULSE_OPEN);
            
            if (current_tick - g_state_timer >= RELEASE_OPEN_VALVE_TIME) {
                /* 时间到，转移到卸物状态 */
                g_vacuum_state = VACUUM_STATE_RELEASING;
                g_state_timer = current_tick;
            }
            break;
            
        case VACUUM_STATE_RELEASING:
            /* 卸物中：电磁阀开启 */
            VacuumPump_SetPulse(VACUUM_PUMP_PULSE_CLOSE);
            SolenoidValve_SetPulse(SOLENOID_VALVE_PULSE_OPEN);
            
            /* 检查超时 (10秒) */
            if (current_tick - g_total_timer >= RELEASE_MAX_TIME) {
                /* 超时，自动停止 */
                g_vacuum_state = VACUUM_STATE_IDLE;
                VacuumPump_SetPulse(VACUUM_PUMP_PULSE_CLOSE);
                SolenoidValve_SetPulse(SOLENOID_VALVE_PULSE_CLOSE);
            }
            /* 检查吸物指令 */
            else if (g_vacuum_cmd == VACUUM_CMD_SUCTION) {
                /* 转移回吸物流程 */
                g_vacuum_state = VACUUM_STATE_SUCTION_CLOSE_VALVE;
                g_state_timer = current_tick;
                g_total_timer = current_tick;
                g_vacuum_cmd = VACUUM_CMD_IDLE;  /* 清除指令 */
            }
            break;
            
        default:
            /* 异常状态，回到待命 */
            g_vacuum_state = VACUUM_STATE_IDLE;
            VacuumPump_SetPulse(VACUUM_PUMP_PULSE_CLOSE);
            SolenoidValve_SetPulse(SOLENOID_VALVE_PULSE_CLOSE);
            break;
    }
}

/* ==================== 控制函数实现 ==================== */

void VacuumPump_App_SetCommand(VacuumPumpCmd_t cmd)
{
    g_vacuum_cmd = cmd;
}

/* ==================== 查询函数实现 ==================== */

VacuumPumpState_t VacuumPump_App_GetState(void)
{
    return g_vacuum_state;
}
