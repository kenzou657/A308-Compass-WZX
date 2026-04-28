/*
 * 真空泵应用模块头文件
 * 
 * 功能：
 * - 实现完整的吸物/卸物流程
 * - 管理状态转移和时序
 * - 超时保护
 * - 根据任务产生的标志位驱动状态转移
 */

#ifndef APP_VACUUM_PUMP_H
#define APP_VACUUM_PUMP_H

#include <stdint.h>
#include <stdbool.h>

/* ==================== 状态枚举定义 ==================== */

typedef enum {
    VACUUM_STATE_IDLE = 0,              /* 待命 */
    VACUUM_STATE_SUCTION_CLOSE_VALVE,   /* 关闭电磁阀 */
    VACUUM_STATE_SUCTION_START_PUMP,    /* 启动真空泵 */
    VACUUM_STATE_SUCKING,               /* 吸物中 */
    VACUUM_STATE_RELEASE_STOP_PUMP,     /* 停止真空泵 */
    VACUUM_STATE_RELEASE_OPEN_VALVE,    /* 打开电磁阀 */
    VACUUM_STATE_RELEASING              /* 卸物中 */
} VacuumPumpState_t;

typedef enum {
    VACUUM_CMD_IDLE = 0,                /* 待命指令 */
    VACUUM_CMD_SUCTION,                 /* 吸物指令 */
    VACUUM_CMD_RELEASE                  /* 释放指令 */
} VacuumPumpCmd_t;

/* ==================== 时序参数定义 ==================== */

#define SUCTION_CLOSE_VALVE_TIME    100     /* 100ms */
#define SUCTION_START_PUMP_TIME     100     /* 100ms */
#define SUCTION_MAX_TIME            60000   /* 60秒 */
#define RELEASE_STOP_PUMP_TIME      100     /* 100ms */
#define RELEASE_OPEN_VALVE_TIME     2000    /* 2秒 */
#define RELEASE_MAX_TIME            10000   /* 10秒 */

/* ==================== 初始化函数 ==================== */

/*
 * 初始化真空泵应用模块
 * 
 * 功能：
 * - 初始化状态机
 * - 初始化时序计时器
 * - 调用驱动层初始化
 */
void VacuumPump_App_Init(void);

/* ==================== 更新函数 ==================== */

/*
 * 真空泵应用周期更新
 * 
 * 功能：
 * - 更新状态机
 * - 处理时序转移
 * - 检查超时
 * 
 * 说明：
 * - 应在主循环中周期调用（建议 10ms 或更快）
 */
void VacuumPump_App_Update(void);

/* ==================== 控制函数 ==================== */

/*
 * 设置真空泵控制指令
 * 
 * 参数：
 *   cmd - 控制指令 (VACUUM_CMD_IDLE/SUCTION/RELEASE)
 * 
 * 说明：
 * - 由任务调用，触发状态转移
 * - VACUUM_CMD_SUCTION: 从 IDLE 或 RELEASING 转移到吸物流程
 * - VACUUM_CMD_RELEASE: 从 SUCKING 转移到卸物流程
 * - VACUUM_CMD_IDLE: 停止所有操作，回到待命状态
 */
void VacuumPump_App_SetCommand(VacuumPumpCmd_t cmd);

/* ==================== 查询函数 ==================== */

/*
 * 获取真空泵当前状态
 * 
 * 返回：
 *   当前状态 (VacuumPumpState_t)
 * 
 * 说明：
 * - 任务可根据状态判断是否可以进行下一步操作
 * - 例如：VACUUM_STATE_SUCKING 表示物品已吸取
 */
VacuumPumpState_t VacuumPump_App_GetState(void);

#endif /* APP_VACUUM_PUMP_H */
