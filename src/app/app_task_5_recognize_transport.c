/**
 * @file app_task_5_recognize_transport.c
 * @brief Task 5: 识别+搬运 - 实现文件
 */

#include "app_task_5_recognize_transport.h"
#include "../drivers/drv_chassis.h"
#include "../drivers/drv_buzzer.h"
#include "../utils/timer.h"

typedef struct {
    TaskState_t state;
    uint32_t start_time;
    uint32_t elapsed_time;
    uint32_t timeout_ms;
    
    uint8_t recognized_digit;
    uint8_t phase;                  /* 0: 识别, 1: 循迹到物品区, 2: 搬运, 3: 返回停车区 */
    uint32_t phase_start_time;
    
    uint32_t update_count;
} Task5_Context_t;

static Task5_Context_t g_task5_ctx;
extern volatile uint32_t uwTick;

void Task5_Init(void)
{
    /* TODO: 实现初始化逻辑
     * 
     * 步骤：
     * 1. 初始化任务状态
     * 2. 设置超时时间（根据赛题要求）
     * 3. 初始化阶段为 0（识别）
     * 4. 启动摄像头识别
     * 5. 蜂鸣器提示
     */
}

void Task5_Run(void)
{
    /* TODO: 实现主循环逻辑
     * 
     * 步骤：
     * 1. 检查任务状态
     * 2. 计算已运行时间
     * 3. 超时检查
     * 4. 根据阶段执行不同的逻辑：
     *    - 阶段0：识别数字
     *    - 阶段1：循迹到物品区
     *    - 阶段2：搬运物品
     *    - 阶段3：返回停车区
     */
}

void Task5_Stop(void)
{
    /* TODO: 实现停止逻辑 */
}

void Task5_Reset(void)
{
    /* TODO: 实现重置逻辑 */
}

TaskState_t Task5_GetState(void)
{
    return TASK_STATE_IDLE;
}

bool Task5_IsSuccess(void)
{
    return false;
}
