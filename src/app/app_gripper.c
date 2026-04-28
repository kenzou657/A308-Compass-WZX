/*
 * app_gripper.c
 * 夹爪应用模块实现
 * 
 * 功能：
 * - 实现吸取/放下的完整状态机
 * - 管理状态转移和时序
 * - 提供任务层接口
 */

#include "app_gripper.h"
#include "drv_gripper.h"

/* ============ 内部变量 ============ */

/** 夹爪当前状态 */
static GripperState_t g_gripper_state = GRIPPER_STATE_IDLE;

/** 夹爪当前命令 */
static GripperCmd_t g_gripper_cmd = GRIPPER_CMD_IDLE;

/** 状态计时器（ms） */
static uint32_t g_state_timer = 0;

/** 时序参数 */
static uint16_t g_move_delay = 500;      /* 舵机移动延迟 */
static uint16_t g_pump_delay = 1000;     /* 真空泵启动延迟 */
static uint16_t g_release_delay = 500;   /* 释放延迟 */

/* ============ 初始化函数 ============ */

void Gripper_App_Init(void)
{
    /* 调用驱动层初始化 */
    Gripper_Init();
    
    /* 初始化状态机 */
    g_gripper_state = GRIPPER_STATE_IDLE;
    g_gripper_cmd = GRIPPER_CMD_IDLE;
    g_state_timer = 0;
}

/* ============ 更新函数 ============ */

void Gripper_App_Update(void)
{
    /* 增加计时器（假设每次调用间隔10ms） */
    g_state_timer += 10;
    
    /* 状态机处理 */
    switch (g_gripper_state) {
        /* ========== 待命状态 ========== */
        case GRIPPER_STATE_IDLE:
            if (g_gripper_cmd == GRIPPER_CMD_SUCTION) {
                /* 开始吸取流程 */
                Gripper_MoveServo(GRIPPER_SUCTION_ANGLE, 0);
                g_gripper_state = GRIPPER_STATE_SUCTION_MOVE_DOWN;
                g_state_timer = 0;
            }
            break;
        
        /* ========== 吸取流程 ========== */
        case GRIPPER_STATE_SUCTION_MOVE_DOWN:
            if (g_state_timer >= g_move_delay) {
                /* 舵机已到位，启动泵 */
                Gripper_StartPump(0);
                g_gripper_state = GRIPPER_STATE_SUCTION_PUMP;
                g_state_timer = 0;
            }
            break;
        
        case GRIPPER_STATE_SUCTION_PUMP:
            if (g_state_timer >= g_pump_delay) {
                /* 真空已形成，抬起吸嘴 */
                Gripper_MoveServo(GRIPPER_HOLD_ANGLE, 0);
                g_gripper_state = GRIPPER_STATE_SUCTION_MOVE_UP;
                g_state_timer = 0;
            }
            break;
        
        case GRIPPER_STATE_SUCTION_MOVE_UP:
            if (g_state_timer >= g_move_delay) {
                /* 物体已吸取 */
                g_gripper_state = GRIPPER_STATE_HOLDING;
                g_gripper_cmd = GRIPPER_CMD_IDLE;
            }
            break;
        
        /* ========== 物体被吸取状态 ========== */
        case GRIPPER_STATE_HOLDING:
            if (g_gripper_cmd == GRIPPER_CMD_RELEASE) {
                /* 开始放下流程 */
                Gripper_MoveServo(GRIPPER_RELEASE_ANGLE, 0);
                g_gripper_state = GRIPPER_STATE_RELEASE_MOVE_DOWN;
                g_state_timer = 0;
            }
            break;
        
        /* ========== 放下流程 ========== */
        case GRIPPER_STATE_RELEASE_MOVE_DOWN:
            if (g_state_timer >= g_move_delay) {
                /* 吸嘴已降低，打开电磁阀 */
                Gripper_OpenValve(0);
                g_gripper_state = GRIPPER_STATE_RELEASE_OPEN_VALVE;
                g_state_timer = 0;
            }
            break;
        
        case GRIPPER_STATE_RELEASE_OPEN_VALVE:
            if (g_state_timer >= g_release_delay) {
                /* 真空已释放，停止泵 */
                Gripper_StopPump(0);
                g_gripper_state = GRIPPER_STATE_RELEASE_STOP_PUMP;
                g_state_timer = 0;
            }
            break;
        
        case GRIPPER_STATE_RELEASE_STOP_PUMP:
            if (g_state_timer >= 100) {  /* 100ms延迟 */
                /* 泵已停止，关闭阀 */
                Gripper_CloseValve(0);
                g_gripper_state = GRIPPER_STATE_RELEASE_CLOSE_VALVE;
                g_state_timer = 0;
            }
            break;
        
        case GRIPPER_STATE_RELEASE_CLOSE_VALVE:
            if (g_state_timer >= 100) {  /* 100ms延迟 */
                /* 阀已关闭，回到中位 */
                Gripper_MoveServo(GRIPPER_IDLE_ANGLE, 0);
                g_gripper_state = GRIPPER_STATE_RELEASE_MOVE_MID;
                g_state_timer = 0;
            }
            break;
        
        case GRIPPER_STATE_RELEASE_MOVE_MID:
            if (g_state_timer >= g_move_delay) {
                /* 放下完成 */
                g_gripper_state = GRIPPER_STATE_IDLE;
                g_gripper_cmd = GRIPPER_CMD_IDLE;
            }
            break;
        
        /* ========== 错误状态 ========== */
        case GRIPPER_STATE_ERROR:
            /* 错误状态，停止所有操作 */
            Gripper_StopPump(0);
            Gripper_CloseValve(0);
            Gripper_MoveServo(GRIPPER_IDLE_ANGLE, 0);
            break;
        
        default:
            g_gripper_state = GRIPPER_STATE_IDLE;
            break;
    }
}

/* ============ 控制函数 ============ */

void Gripper_App_SetCommand(GripperCmd_t cmd)
{
    g_gripper_cmd = cmd;
}

/* ============ 查询函数 ============ */

GripperState_t Gripper_App_GetState(void)
{
    return g_gripper_state;
}

bool Gripper_App_IsBusy(void)
{
    /* 判断是否在执行操作 */
    return (g_gripper_state != GRIPPER_STATE_IDLE && 
            g_gripper_state != GRIPPER_STATE_HOLDING &&
            g_gripper_state != GRIPPER_STATE_ERROR);
}

GripperCmd_t Gripper_App_GetCommand(void)
{
    return g_gripper_cmd;
}

void Gripper_App_SetTimings(uint16_t move_delay, uint16_t pump_delay, uint16_t release_delay)
{
    g_move_delay = move_delay;
    g_pump_delay = pump_delay;
    g_release_delay = release_delay;
}
