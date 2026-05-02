/*
 * 按键逻辑处理模块实现
 * 
 * 功能：
 * - 按键事件处理
 * - 任务选择状态机
 * - 系统状态管理（IDLE/RUNNING）
 * - 与任务管理器交互
 */

#include "key_logic.h"
#include "drv_key.h"
#include "app_task_manager.h"
#include "drv_led.h"
#include "drv_buzzer.h"
#include "app_task_1_line_tracking.h"
#include "app_task_2_dual_point.h"
#include "app_task_3_round_trip.h"

/* ==================== 全局变量 ==================== */

static SystemState_t g_system_state = SYSTEM_STATE_IDLE;
static ParamSetting_t g_param_setting = {
    .destination_1 = 1,
    .destination_2 = 1,
    .current_param = PARAM_DESTINATION_1,
};

/* ==================== 参数设置辅助函数声明 ==================== */

/*
 * 增加目的地数值（1 → 2 → 3 → 4 → 5 → 1）
 */
static void ParamSetting_IncreaseDestination(uint8_t *destination);

/* ==================== 按键逻辑实现 ==================== */

void Key_Logic_Init(void)
{
    /* 初始化系统状态为IDLE */
    g_system_state = SYSTEM_STATE_IDLE;
}

void Key_Logic_Process(void)
{
    /* 读取按键事件 */
    KeyEvent_t k1_event = Key_GetEvent(KEY_ID_K1);
    KeyEvent_t k2_event = Key_GetEvent(KEY_ID_K2);
    KeyEvent_t k3_event = Key_GetEvent(KEY_ID_K3);
    
    /* 根据系统状态处理按键 */
    switch (g_system_state) {
        case SYSTEM_STATE_IDLE:
            /* ========== IDLE状态：任务选择模式 ========== */
            
            /* K1: 上一个任务 */
            if (k1_event == KEY_EVENT_PRESSED) {
                TaskManager_PrevTask();
                
                /* 可选：蜂鸣器提示音 */
                // beep_short();
                
                /* 可选：LED指示 */
                // LEDG_TOGGLE();
            }
            
            /* K2: 下一个任务 */
            if (k2_event == KEY_EVENT_PRESSED) {
                TaskManager_NextTask();
                
                /* 可选：蜂鸣器提示音 */
                // beep_short();
                
                /* 可选：LED指示 */
                // LEDG_TOGGLE();
            }
            
            /* K3: 启动当前任务或进入参数设置 */
            if (k3_event == KEY_EVENT_PRESSED) {
                /* 获取当前任务ID */
                TaskID_t current_task = TaskManager_GetCurrentTaskID();
                
                /* 根据任务类型设置参数 */
                if (current_task == TASK_ID_1_LINE_TRACKING) {
                    /* 任务1：设置单个目的地参数 */
                    Task1_SetTargetZone(g_param_setting.destination_1);
                }
                else if (current_task == TASK_ID_2_DUAL_POINT) {
                    /* 任务2：设置两个目的地参数 */
                    Task2_SetTargetZones(g_param_setting.destination_1, g_param_setting.destination_2);
                }
                else if (current_task == TASK_ID_3_ROUND_TRIP) {
                    /* 任务3：设置两个目的地参数 */
                    Task3_SetTargetZones(g_param_setting.destination_1, g_param_setting.destination_2);
                }
                
                /* 启动任务 */
                TaskManager_StartTask();
                
                /* 切换到运行状态 */
                g_system_state = SYSTEM_STATE_RUNNING;
                
                /* 可选：蜂鸣器提示音（长音） */
                // beep_long();
                
                /* 可选：LED指示 */
                // LEDR_ON();
            }
            
            /* K2长按：进入参数设置模式 */
            if (k2_event == KEY_EVENT_LONG_PRESSED) {
                Key_Logic_EnterParamSetting();
            }
            break;
            
        case SYSTEM_STATE_RUNNING:
            /* ========== RUNNING状态：任务执行中 ========== */
            
            /* K3: 停止当前任务 */
            if (k3_event == KEY_EVENT_PRESSED) {
                /* 停止任务 */
                TaskManager_StopTask();
                
                /* 切换到空闲状态 */
                g_system_state = SYSTEM_STATE_IDLE;
                
                /* 可选：蜂鸣器提示音 */
                // beep_short();
                
                /* 可选：LED指示 */
                // LEDR_OFF();
            }
            
            /* 在运行状态下，K1和K2不响应 */
            break;
            
        case SYSTEM_STATE_PARAM_SETTING:
            /* ========== PARAM_SETTING状态：参数设置模式 ========== */
            
            // /* K1: 切换任务（在参数设置模式下也可用） */
            // if (k1_event == KEY_EVENT_PRESSED) {
            //     TaskManager_PrevTask();
            // }
            
            /* K2短按：增加当前参数的数值 */
            if (k2_event == KEY_EVENT_PRESSED) {
                if (g_param_setting.current_param == PARAM_DESTINATION_1) {
                    ParamSetting_IncreaseDestination(&g_param_setting.destination_1);
                } else {
                    ParamSetting_IncreaseDestination(&g_param_setting.destination_2);
                }
            }
            
            /* K2长按：切换参数A/B */
            if (k2_event == KEY_EVENT_LONG_PRESSED) {
                if (g_param_setting.current_param == PARAM_DESTINATION_1) {
                    g_param_setting.current_param = PARAM_DESTINATION_2;
                    LEDG_ON();  /* 参数B被选中时，打开绿灯 */
                } else {
                    g_param_setting.current_param = PARAM_DESTINATION_1;
                    LEDG_OFF();  /* 参数A被选中时，关闭绿灯 */
                }
            }
            
            /* K3: 确认并退出参数设置 */
            if (k3_event == KEY_EVENT_PRESSED) {
                Key_Logic_ExitParamSetting();
            }
            break;
            
        default:
            /* 异常状态，重置为IDLE */
            g_system_state = SYSTEM_STATE_IDLE;
            break;
    }
}

/* ==================== 参数设置辅助函数实现 ==================== */

/*
 * 增加目的地数值（1 → 2 → 3 → 4 → 5 → 1）
 */
static void ParamSetting_IncreaseDestination(uint8_t *destination)
{
    if (*destination >= 5) {
        *destination = 1;
    } else {
        (*destination)++;
    }
}

SystemState_t Key_Logic_GetSystemState(void)
{
    return g_system_state;
}

void Key_Logic_SetSystemState(SystemState_t state)
{
    g_system_state = state;
}

ParamSetting_t* Key_Logic_GetParamSetting(void)
{
    return &g_param_setting;
}

void Key_Logic_EnterParamSetting(void)
{
    g_system_state = SYSTEM_STATE_PARAM_SETTING;
    g_param_setting.current_param = PARAM_DESTINATION_1;
    LEDG_OFF();  /* 进入参数设置时，关闭绿灯 */
}

void Key_Logic_ExitParamSetting(void)
{
    g_system_state = SYSTEM_STATE_IDLE;
    LEDG_OFF();  /* 退出参数设置时，关闭绿灯 */
}
