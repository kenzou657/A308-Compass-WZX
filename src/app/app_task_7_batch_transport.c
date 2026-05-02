/**
 * @file app_task_7_batch_transport.c
 * @brief Task 7: 全自动搬运 - 实现文件
 */

#include "app_task_7_batch_transport.h"
#include "../drivers/drv_chassis.h"
#include "../drivers/drv_buzzer.h"
#include "../utils/timer.h"

typedef struct {
    TaskState_t state;
    uint32_t start_time;
    uint32_t elapsed_time;
    uint32_t timeout_ms;
    
    uint8_t current_item;           /* 当前搬运的物品编号（1-5） */
    uint8_t items_transported;      /* 已搬运的物品数量 */
    uint8_t phase;                  /* 0: 搜索, 1: 搬运, 2: 放置, 3: 返回 */
    uint32_t phase_start_time;
    
    uint32_t update_count;
} Task7_Context_t;

static Task7_Context_t g_task7_ctx;
extern volatile uint32_t uwTick;

void Task7_Init(void)
{
    /* TODO: 实现初始化逻辑
     * 
     * 步骤：
     * 1. 初始化任务状态
     * 2. 设置超时时间（根据赛题要求）
     * 3. 初始化当前物品为 1
     * 4. 初始化已搬运物品数为 0
     * 5. 初始化阶段为 0（搜索）
     * 6. 蜂鸣器提示
     */
}

void Task7_Run(void)
{
    /* TODO: 实现主循环逻辑
     * 
     * 步骤：
     * 1. 检查任务状态
     * 2. 计算已运行时间
     * 3. 超时检查
     * 4. 根据阶段执行不同的逻辑：
     *    - 阶段0：搜索物品
     *    - 阶段1：搬运物品
     *    - 阶段2：放置物品到对应存放区
     *    - 阶段3：返回停车区
     * 5. 检查是否所有物品都已搬运
     */
}

void Task7_Stop(void)
{
    /* TODO: 实现停止逻辑 */
}

void Task7_Reset(void)
{
    /* TODO: 实现重置逻辑 */
}

TaskState_t Task7_GetState(void)
{
    return TASK_STATE_IDLE;
}

bool Task7_IsSuccess(void)
{
    return false;
}
