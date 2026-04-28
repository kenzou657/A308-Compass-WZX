/*
 * 按键逻辑处理模块头文件
 * 
 * 功能：
 * - 按键事件处理
 * - 任务选择状态机
 * - 系统状态管理（IDLE/RUNNING）
 * - 与任务管理器交互
 */

#ifndef KEY_LOGIC_H
#define KEY_LOGIC_H

#include <stdint.h>
#include <stdbool.h>

/* ==================== 系统状态定义 ==================== */

typedef enum {
    SYSTEM_STATE_IDLE = 0,      /* 空闲状态（任务选择模式） */
    SYSTEM_STATE_RUNNING = 1,   /* 运行状态（任务执行中） */
} SystemState_t;

/* ==================== 按键逻辑接口 ==================== */

/*
 * 初始化按键逻辑模块
 * 
 * 功能：
 * - 初始化系统状态为IDLE
 * - 清空内部状态
 */
void Key_Logic_Init(void);

/*
 * 处理按键事件
 * 
 * 功能：
 * - 读取按键事件
 * - 根据系统状态处理按键
 * - 调用任务管理器接口
 * 
 * 调用位置：主循环中定期调用
 * 
 * 按键功能：
 * - IDLE状态：
 *   - K1: 上一个任务
 *   - K2: 下一个任务
 *   - K3: 启动当前任务
 * - RUNNING状态：
 *   - K3: 停止当前任务
 */
void Key_Logic_Process(void);

/*
 * 获取系统状态
 * 
 * 返回：
 * - SYSTEM_STATE_IDLE: 空闲状态
 * - SYSTEM_STATE_RUNNING: 运行状态
 */
SystemState_t Key_Logic_GetSystemState(void);

/*
 * 设置系统状态（内部使用）
 * 
 * 参数：
 * - state: 系统状态
 */
void Key_Logic_SetSystemState(SystemState_t state);

#endif /* KEY_LOGIC_H */
