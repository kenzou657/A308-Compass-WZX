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

/* ==================== 全局变量 ==================== */

static SystemState_t g_system_state = SYSTEM_STATE_IDLE;

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
            
            /* K3: 启动当前任务 */
            if (k3_event == KEY_EVENT_PRESSED) {
                /* 启动任务 */
                TaskManager_StartTask();
                
                /* 切换到运行状态 */
                g_system_state = SYSTEM_STATE_RUNNING;
                
                /* 可选：蜂鸣器提示音（长音） */
                // beep_long();
                
                /* 可选：LED指示 */
                // LEDR_ON();
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
            
        default:
            /* 异常状态，重置为IDLE */
            g_system_state = SYSTEM_STATE_IDLE;
            break;
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
