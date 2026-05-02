/*
 * 任务管理器头文件
 * 
 * 功能：
 * - 任务枚举和状态管理
 * - 任务切换逻辑
 * - 任务执行调度
 */

#ifndef APP_TASK_MANAGER_H
#define APP_TASK_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

/* ==================== 任务ID定义 ==================== */

typedef enum {
    TASK_ID_1_LINE_TRACKING = 1,        /* Task 1：单点循迹 */
    TASK_ID_2_DUAL_POINT = 2,          /* Task 2：双点循迹 */
    TASK_ID_3_ROUND_TRIP = 3,          /* Task 3：双点往返 */
    TASK_ID_4_DIGIT_RECOGNITION = 4,   /* Task 4：数字识别 */
    TASK_ID_5_RECOGNIZE_TRANSPORT = 5, /* Task 5：识别+搬运 */
    TASK_ID_6_AUTO_TRANSPORT = 6,      /* Task 6：自主搬运 */
    TASK_ID_7_BATCH_TRANSPORT = 7,     /* Task 7：全自动搬运 */
    TASK_ID_8_CREATIVE = 8,            /* Task 8：创意任务 */
    TASK_ID_COUNT = 8,
} TaskID_t;

/* ==================== 任务状态定义 ==================== */

typedef enum {
    TASK_STATE_IDLE,           /* 空闲 */
    TASK_STATE_INIT,           /* 初始化 */
    TASK_STATE_RUNNING,        /* 运行中 */
    TASK_STATE_PAUSED,         /* 暂停 */
    TASK_STATE_SUCCESS,        /* 成功完成 */
    TASK_STATE_FAILED,         /* 失败 */
    TASK_STATE_TIMEOUT,        /* 超时 */
} TaskState_t;

/* ==================== 任务管理器接口 ==================== */

/*
 * 初始化任务管理器
 */
void TaskManager_Init(void);

/*
 * 启动当前选中的任务
 */
void TaskManager_StartTask(void);

/*
 * 停止当前任务
 */
void TaskManager_StopTask(void);

/*
 * 选择上一个任务
 */
void TaskManager_PrevTask(void);

/*
 * 选择下一个任务
 */
void TaskManager_NextTask(void);

/*
 * 获取当前任务状态
 */
TaskState_t TaskManager_GetTaskState(void);

/*
 * 获取当前任务ID
 */
TaskID_t TaskManager_GetCurrentTaskID(void);

/*
 * 获取当前任务名称
 */
const char *TaskManager_GetCurrentTaskName(void);

/*
 * 判断任务是否运行中
 */
bool TaskManager_IsRunning(void);

/*
 * 获取任务开始时间
 */
uint32_t TaskManager_GetStartTime(void);

/*
 * 获取任务运行时间(ms)
 */
uint32_t TaskManager_GetElapsedTime(void);

/*
 * 任务主循环（在main中调用）
 */
void TaskManager_Update(void);

#endif /* APP_TASK_MANAGER_H */
