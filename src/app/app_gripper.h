/*
 * app_gripper.h
 * 夹爪应用模块头文件
 * 
 * 功能：
 * - 实现吸取/放下的完整状态机
 * - 管理状态转移和时序
 * - 提供任务层接口
 */

#ifndef APP_GRIPPER_H_
#define APP_GRIPPER_H_

#include <stdint.h>
#include <stdbool.h>

/* ============ 状态枚举定义 ============ */

/**
 * 夹爪状态枚举
 * 
 * 吸取流程：
 *   IDLE → SUCTION_MOVE_DOWN → SUCTION_PUMP → SUCTION_MOVE_UP → HOLDING
 * 
 * 放下流程：
 *   HOLDING → RELEASE_MOVE_DOWN → RELEASE_OPEN_VALVE → RELEASE_STOP_PUMP 
 *   → RELEASE_CLOSE_VALVE → RELEASE_MOVE_MID → IDLE
 */
typedef enum {
    GRIPPER_STATE_IDLE = 0,              /* 待命 */
    GRIPPER_STATE_SUCTION_MOVE_DOWN,     /* 吸取-舵机下降 */
    GRIPPER_STATE_SUCTION_PUMP,          /* 吸取-启动泵 */
    GRIPPER_STATE_SUCTION_MOVE_UP,       /* 吸取-舵机上升 */
    GRIPPER_STATE_HOLDING,               /* 物体被吸取 */
    GRIPPER_STATE_RELEASE_MOVE_DOWN,     /* 放下-舵机下降 */
    GRIPPER_STATE_RELEASE_OPEN_VALVE,    /* 放下-打开阀 */
    GRIPPER_STATE_RELEASE_STOP_PUMP,     /* 放下-停止泵 */
    GRIPPER_STATE_RELEASE_CLOSE_VALVE,   /* 放下-关闭阀 */
    GRIPPER_STATE_RELEASE_MOVE_MID,      /* 放下-回到中位 */
    GRIPPER_STATE_ERROR                  /* 错误 */
} GripperState_t;

/**
 * 夹爪命令枚举
 */
typedef enum {
    GRIPPER_CMD_IDLE = 0,                /* 待命指令 */
    GRIPPER_CMD_SUCTION,                 /* 吸取指令 */
    GRIPPER_CMD_RELEASE                  /* 放下指令 */
} GripperCmd_t;

/* ============ 初始化函数 ============ */

/**
 * @brief 初始化夹爪应用模块
 * 
 * 功能：
 * - 初始化状态机
 * - 初始化时序计时器
 * - 调用驱动层初始化
 * 
 * @return 无
 */
void Gripper_App_Init(void);

/* ============ 更新函数 ============ */

/**
 * @brief 夹爪应用周期更新
 * 
 * 功能：
 * - 更新状态机
 * - 处理状态转移
 * - 检查超时
 * 
 * 说明：
 * - 应在主循环中周期调用（建议 10ms 或更快）
 * - 非阻塞式，每次调用只处理一个状态转移
 * 
 * @return 无
 */
void Gripper_App_Update(void);

/* ============ 控制函数 ============ */

/**
 * @brief 设置夹爪控制指令
 * 
 * 参数：
 *   cmd - 控制指令 (GRIPPER_CMD_IDLE/SUCTION/RELEASE)
 * 
 * 说明：
 * - 由任务调用，触发状态转移
 * - GRIPPER_CMD_SUCTION: 从 IDLE 或 HOLDING 转移到吸取流程
 * - GRIPPER_CMD_RELEASE: 从 HOLDING 转移到放下流程
 * - GRIPPER_CMD_IDLE: 停止所有操作，回到待命状态
 * 
 * @param cmd 控制指令
 * @return 无
 */
void Gripper_App_SetCommand(GripperCmd_t cmd);

/* ============ 查询函数 ============ */

/**
 * @brief 获取夹爪当前状态
 * 
 * 返回：
 *   当前状态 (GripperState_t)
 * 
 * 说明：
 * - 任务可根据状态判断是否可以进行下一步操作
 * - 例如：GRIPPER_STATE_HOLDING 表示物品已吸取
 * 
 * @return 当前状态
 */
GripperState_t Gripper_App_GetState(void);

/**
 * @brief 判断夹爪是否忙碌
 * 
 * 返回：
 *   true - 夹爪正在执行操作（吸取或放下中）
 *   false - 夹爪空闲或已完成操作
 * 
 * 说明：
 * - 任务可用此函数判断是否可以进行下一步
 * - 例如：等待吸取完成后再移动小车
 * 
 * @return 忙碌状态
 */
bool Gripper_App_IsBusy(void);

/**
 * @brief 获取夹爪当前命令
 * 
 * @return 当前命令
 */
GripperCmd_t Gripper_App_GetCommand(void);

/**
 * @brief 设置时序参数
 * 
 * 参数：
 *   move_delay - 舵机移动延迟（ms）
 *   pump_delay - 真空泵启动延迟（ms）
 *   release_delay - 释放延迟（ms）
 * 
 * 说明：
 * - 用于调优时序参数
 * - 应在初始化后调用
 * 
 * @param move_delay 舵机移动延迟
 * @param pump_delay 真空泵启动延迟
 * @param release_delay 释放延迟
 * @return 无
 */
void Gripper_App_SetTimings(uint16_t move_delay, uint16_t pump_delay, uint16_t release_delay);

#endif /* APP_GRIPPER_H_ */
