/*
 * 按键驱动头文件
 * 
 * 功能：
 * - 按键硬件抽象
 * - 按键消抖处理（20ms延迟）
 * - 按键状态检测（按下/释放事件）
 * 
 * 硬件配置：
 * - K1: GPIOB.21 (Pin 20) - 上一个任务
 * - K2: GPIOB.23 (Pin 22) - 下一个任务
 * - K3: GPIOB.24 (Pin 23) - 确认/启动/停止
 */

#ifndef DRV_KEY_H
#define DRV_KEY_H

#include <stdint.h>
#include <stdbool.h>

/* ==================== 按键ID定义 ==================== */

typedef enum {
    KEY_ID_K1 = 0,      /* K1: 上一个任务 */
    KEY_ID_K2 = 1,      /* K2: 下一个任务 */
    KEY_ID_K3 = 2,      /* K3: 确认/启动/停止 */
    KEY_ID_COUNT = 3,
} KeyID_t;

/* ==================== 按键状态定义 ==================== */

typedef enum {
    KEY_STATE_RELEASED = 0,     /* 按键释放状态 */
    KEY_STATE_PRESSED = 1,      /* 按键按下状态 */
} KeyState_t;

/* ==================== 按键事件定义 ==================== */

typedef enum {
    KEY_EVENT_NONE = 0,         /* 无事件 */
    KEY_EVENT_PRESSED,          /* 按键按下事件 */
    KEY_EVENT_RELEASED,         /* 按键释放事件 */
} KeyEvent_t;

/* ==================== 按键驱动接口 ==================== */

/*
 * 初始化按键驱动
 * 
 * 功能：
 * - 初始化GPIO（已在SysConfig中配置）
 * - 初始化内部状态机
 * - 清空事件缓冲
 */
void Key_Init(void);

/*
 * 扫描按键状态
 * 
 * 功能：
 * - 读取GPIO电平
 * - 消抖处理（20ms延迟）
 * - 生成按键事件
 * 
 * 调用周期：建议10ms（在主循环中定期调用）
 */
void Key_Scan(void);

/*
 * 获取按键当前状态
 * 
 * 参数：
 * - key_id: 按键ID
 * 
 * 返回：
 * - KEY_STATE_RELEASED: 按键释放
 * - KEY_STATE_PRESSED: 按键按下
 */
KeyState_t Key_GetState(KeyID_t key_id);

/*
 * 获取按键事件（读取后自动清除）
 * 
 * 参数：
 * - key_id: 按键ID
 * 
 * 返回：
 * - KEY_EVENT_NONE: 无事件
 * - KEY_EVENT_PRESSED: 按键按下事件
 * - KEY_EVENT_RELEASED: 按键释放事件
 * 
 * 注意：事件读取后会自动清除，避免重复处理
 */
KeyEvent_t Key_GetEvent(KeyID_t key_id);

/*
 * 清除按键事件
 * 
 * 参数：
 * - key_id: 按键ID
 */
void Key_ClearEvent(KeyID_t key_id);

/*
 * 清除所有按键事件
 */
void Key_ClearAllEvents(void);

#endif /* DRV_KEY_H */
