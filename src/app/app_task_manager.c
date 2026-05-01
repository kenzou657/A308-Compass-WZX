/*
 * 任务管理器实现
 * 
 * 功能：
 * - 任务枚举和状态管理
 * - 任务切换逻辑
 * - 任务执行调度
 */

#include "app_task_manager.h"
#include "app_task_1_line_tracking.h"
#include "app_task_2_dual_point.h"
#include "app_task_3_round_trip.h"
#include "app_task_4_digit_recognition.h"
#include "app_task_5_recognize_transport.h"
#include "app_task_6_auto_transport.h"
#include "app_task_7_batch_transport.h"
#include "app_task_8_creative.h"

/* ==================== 任务结构定义 ==================== */

typedef struct {
    TaskID_t task_id;
    const char *task_name;
    
    /* 任务函数指针 */
    void (*init)(void);
    void (*run)(void);
    void (*stop)(void);
    void (*reset)(void);
    void (*get_state)(void);  /* 返回void，由调用者强制转换 */
    bool (*is_success)(void);
} Task_t;

/* ==================== 全局变量 ==================== */

/* 任务表 */
static const Task_t g_tasks[TASK_ID_COUNT] = {
    {
        .task_id = TASK_ID_1_LINE_TRACKING,
        .task_name = "Task 1: 单点循迹",
        .init = Task1_Init,
        .run = Task1_Run,
        .stop = Task1_Stop,
        .reset = Task1_Reset,
        .get_state = (void (*)(void))Task1_GetState,
        .is_success = Task1_IsSuccess,
    },
    {
        .task_id = TASK_ID_2_DUAL_POINT,
        .task_name = "Task 2: 双点循迹",
        .init = Task2_Init,
        .run = Task2_Run,
        .stop = Task2_Stop,
        .reset = Task2_Reset,
        .get_state = (void (*)(void))Task2_GetState,
        .is_success = Task2_IsSuccess,
    },
    {
        .task_id = TASK_ID_3_ROUND_TRIP,
        .task_name = "Task 3: 双点往返",
        .init = Task3_Init,
        .run = Task3_Run,
        .stop = Task3_Stop,
        .reset = Task3_Reset,
        .get_state = (void (*)(void))Task3_GetState,
        .is_success = Task3_IsSuccess,
    },
    {
        .task_id = TASK_ID_4_DIGIT_RECOGNITION,
        .task_name = "Task 4: 数字识别",
        .init = Task4_Init,
        .run = Task4_Run,
        .stop = Task4_Stop,
        .reset = Task4_Reset,
        .get_state = (void (*)(void))Task4_GetState,
        .is_success = Task4_IsSuccess,
    },
    {
        .task_id = TASK_ID_5_RECOGNIZE_TRANSPORT,
        .task_name = "Task 5: 识别+搬运",
        .init = Task5_Init,
        .run = Task5_Run,
        .stop = Task5_Stop,
        .reset = Task5_Reset,
        .get_state = (void (*)(void))Task5_GetState,
        .is_success = Task5_IsSuccess,
    },
    {
        .task_id = TASK_ID_6_AUTO_TRANSPORT,
        .task_name = "Task 6: 自主搬运",
        .init = Task6_Init,
        .run = Task6_Run,
        .stop = Task6_Stop,
        .reset = Task6_Reset,
        .get_state = (void (*)(void))Task6_GetState,
        .is_success = Task6_IsSuccess,
    },
    {
        .task_id = TASK_ID_7_BATCH_TRANSPORT,
        .task_name = "Task 7: 全自动搬运",
        .init = Task7_Init,
        .run = Task7_Run,
        .stop = Task7_Stop,
        .reset = Task7_Reset,
        .get_state = (void (*)(void))Task7_GetState,
        .is_success = Task7_IsSuccess,
    },
    {
        .task_id = TASK_ID_8_CREATIVE,
        .task_name = "Task 8: 创意任务",
        .init = Task8_Init,
        .run = Task8_Run,
        .stop = Task8_Stop,
        .reset = Task8_Reset,
        .get_state = (void (*)(void))Task8_GetState,
        .is_success = Task8_IsSuccess,
    },
};

/* 任务管理器状态 */
static TaskID_t g_current_task_id = TASK_ID_1_LINE_TRACKING;
static uint32_t g_task_start_time = 0;

/* ==================== 任务管理器实现 ==================== */

void TaskManager_Init(void)
{
    g_current_task_id = TASK_ID_1_LINE_TRACKING;
    g_task_start_time = 0;
}

void TaskManager_StartTask(void)
{
    /* 获取当前任务 */
    const Task_t *task = &g_tasks[g_current_task_id - 1];
    
    /* 重置任务状态 */
    task->reset();
    
    /* 初始化任务 */
    task->init();
    
    /* 记录开始时间 */
    g_task_start_time = 0;  /* TODO: 使用系统时间 */
}

void TaskManager_StopTask(void)
{
    /* 获取当前任务 */
    const Task_t *task = &g_tasks[g_current_task_id - 1];
    
    /* 停止任务 */
    task->stop();
}

void TaskManager_PrevTask(void)
{
    if (g_current_task_id > TASK_ID_1_LINE_TRACKING) {
        g_current_task_id--;
    } else {
        g_current_task_id = TASK_ID_8_CREATIVE;
    }
}

void TaskManager_NextTask(void)
{
    if (g_current_task_id < TASK_ID_8_CREATIVE) {
        g_current_task_id++;
    } else {
        g_current_task_id = TASK_ID_1_LINE_TRACKING;
    }
}

TaskState_t TaskManager_GetTaskState(void)
{
    /* 获取当前任务 */
    const Task_t *task = &g_tasks[g_current_task_id - 1];
    
    /* 调用get_state函数并强制转换 */
    typedef TaskState_t (*GetStateFunc)(void);
    GetStateFunc get_state = (GetStateFunc)task->get_state;
    return get_state();
}

TaskID_t TaskManager_GetCurrentTaskID(void)
{
    return g_current_task_id;
}

const char *TaskManager_GetCurrentTaskName(void)
{
    /* 获取当前任务 */
    const Task_t *task = &g_tasks[g_current_task_id - 1];
    
    return task->task_name;
}

bool TaskManager_IsRunning(void)
{
    TaskState_t state = TaskManager_GetTaskState();
    return state == TASK_STATE_RUNNING;
}

uint32_t TaskManager_GetStartTime(void)
{
    return g_task_start_time;
}

uint32_t TaskManager_GetElapsedTime(void)
{
    /* TODO: 使用系统时间计算 */
    return 0;
}

void TaskManager_Update(void)
{
    /* 获取当前任务 */
    const Task_t *task = &g_tasks[g_current_task_id - 1];
    
    /* 运行任务 */
    task->run();
}
