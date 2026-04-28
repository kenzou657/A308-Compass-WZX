/*
 * drv_gripper.h
 * 夹爪驱动头文件
 * 
 * 功能：
 * - 协调舵机、真空泵、电磁阀的硬件操作
 * - 提供原子操作接口
 * - 管理时序延迟
 */

#ifndef DRV_GRIPPER_H_
#define DRV_GRIPPER_H_

#include <stdint.h>
#include <stdbool.h>
#include "drv_servo.h"
#include "drv_vacuum_pump.h"

/* ============ 夹爪位置角度定义 ============ */

/** 吸取位置角度（舵机下降，吸嘴接近物体） */
#define GRIPPER_SUCTION_ANGLE       0

/** 放下位置角度（舵机下降，吸嘴接近地面） */
#define GRIPPER_RELEASE_ANGLE       0

/** 抬起位置角度（舵机上升，物体被吸起） */
#define GRIPPER_HOLD_ANGLE          180

/** 待命位置角度（舵机中位） */
#define GRIPPER_IDLE_ANGLE          90

/* ============ 时序延迟定义 ============ */

/** 舵机移动延迟（ms），等待舵机到位 */
#define GRIPPER_MOVE_DELAY          500

/** 真空泵启动延迟（ms），等待真空形成 */
#define GRIPPER_PUMP_DELAY          1000

/** 释放延迟（ms），等待物体释放 */
#define GRIPPER_RELEASE_DELAY       500

/** 电磁阀操作延迟（ms） */
#define GRIPPER_VALVE_DELAY         100

/* ============ 函数声明 ============ */

/**
 * @brief 初始化夹爪驱动
 * 
 * 调用底层驱动初始化，设置初始位置
 * 
 * @return 无
 */
void Gripper_Init(void);

/**
 * @brief 舵机移动到指定角度
 * 
 * @param angle 目标角度（0~180°）
 * @param delay_ms 移动后的延迟时间（ms）
 * 
 * @return 无
 * 
 * @note 该函数是阻塞式的，会等待延迟时间后返回
 */
void Gripper_MoveServo(uint16_t angle, uint16_t delay_ms);

/**
 * @brief 启动真空泵
 * 
 * @param delay_ms 启动后的延迟时间（ms），等待真空形成
 * 
 * @return 无
 * 
 * @note 该函数是阻塞式的
 */
void Gripper_StartPump(uint16_t delay_ms);

/**
 * @brief 停止真空泵
 * 
 * @param delay_ms 停止后的延迟时间（ms）
 * 
 * @return 无
 * 
 * @note 该函数是阻塞式的
 */
void Gripper_StopPump(uint16_t delay_ms);

/**
 * @brief 打开电磁阀（释放真空）
 * 
 * @param delay_ms 打开后的延迟时间（ms），等待物体释放
 * 
 * @return 无
 * 
 * @note 该函数是阻塞式的
 */
void Gripper_OpenValve(uint16_t delay_ms);

/**
 * @brief 关闭电磁阀
 * 
 * @param delay_ms 关闭后的延迟时间（ms）
 * 
 * @return 无
 * 
 * @note 该函数是阻塞式的
 */
void Gripper_CloseValve(uint16_t delay_ms);

/**
 * @brief 获取舵机当前角度
 * 
 * @return 当前舵机角度（0~180°）
 */
uint16_t Gripper_GetServoAngle(void);

/**
 * @brief 获取真空泵当前脉宽
 * 
 * @return 当前脉宽值
 */
uint16_t Gripper_GetPumpPulse(void);

/**
 * @brief 获取电磁阀当前脉宽
 * 
 * @return 当前脉宽值
 */
uint16_t Gripper_GetValvePulse(void);

#endif /* DRV_GRIPPER_H_ */
