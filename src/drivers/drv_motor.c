#include "drv_motor.h"
#include "config.h"
#include "pid.h"
#include "drv_encoder.h"

/* ============ 全局变量 ============ */
// 记录当前电机状态
static struct {
    uint8_t direction_a;    // 电机A方向
    uint16_t pwm_a;         // 电机A PWM值
    uint8_t direction_b;    // 电机B方向
    uint16_t pwm_b;         // 电机B PWM值
} g_motor_state = {
    .direction_a = MOTOR_DIR_FORWARD,
    .pwm_a = 0,
    .direction_b = MOTOR_DIR_FORWARD,
    .pwm_b = 0
};

// PID 控制器实例
static PID_Increment_t g_pid_motor_a;
static PID_Increment_t g_pid_motor_b;

/* ============ 初始化函数 ============ */

/**
 * @brief 初始化电机驱动模块
 * @note PWM已在SysConfig中配置，此函数初始化电机状态
 */
void MotorInit(void)
{
    // 初始化电机状态为停止
    g_motor_state.direction_a = MOTOR_DIR_FORWARD;
    g_motor_state.pwm_a = PWM_MIN;
    g_motor_state.direction_b = MOTOR_DIR_FORWARD;
    g_motor_state.pwm_b = PWM_MIN;
    
    // 设置初始方向为正转
    MOTOR_A_DIR_FORWARD();
    MOTOR_B_DIR_FORWARD();
    
    // 设置初始PWM为0（停止）
    DL_TimerG_setCaptureCompareValue(PWM_MOTOR_INST, PWM_MIN, DL_TIMER_CC_1_INDEX);  // 电机A (PA4)
    DL_TimerG_setCaptureCompareValue(PWM_MOTOR_INST, PWM_MIN, DL_TIMER_CC_0_INDEX);  // 电机B (PA7)
}

/* ============ 底层驱动函数 ============ */

/**
 * @brief 设置单个电机的速度和方向
 * @param motor_id: 电机编号 (MOTOR_A 或 MOTOR_B)
 * @param direction: 旋转方向 (MOTOR_DIR_FORWARD 或 MOTOR_DIR_REVERSE)
 * @param pwm_value: PWM占空比 (0-1000)
 */
void MotorSet(uint8_t motor_id, uint8_t direction, uint16_t pwm_value)
{
    // 限制PWM值在有效范围内
    if (pwm_value > PWM_MAX) {
        pwm_value = PWM_MAX;
    }
    
    if (motor_id == MOTOR_A) {
        // 设置电机A方向
        if (direction == MOTOR_DIR_FORWARD) {
            MOTOR_A_DIR_FORWARD();
        } else {
            MOTOR_A_DIR_REVERSE();
        }
        
        // 更新电机A PWM（通道1，PA4）
        DL_TimerG_setCaptureCompareValue(PWM_MOTOR_INST, pwm_value, DL_TIMER_CC_1_INDEX);
        
        // 记录状态
        g_motor_state.direction_a = direction;
        g_motor_state.pwm_a = pwm_value;
        
    } else if (motor_id == MOTOR_B) {
        // 设置电机B方向
        if (direction == MOTOR_DIR_FORWARD) {
            MOTOR_B_DIR_FORWARD();
        } else {
            MOTOR_B_DIR_REVERSE();
        }
        
        // 更新电机B PWM（通道0，PA7）
        DL_TimerG_setCaptureCompareValue(PWM_MOTOR_INST, pwm_value, DL_TIMER_CC_0_INDEX);
        
        // 记录状态
        g_motor_state.direction_b = direction;
        g_motor_state.pwm_b = pwm_value;
    }
}

/**
 * @brief 设置电机A的速度和方向
 * @param direction: 旋转方向 (MOTOR_DIR_FORWARD 或 MOTOR_DIR_REVERSE)
 * @param pwm_value: PWM占空比 (0-1000)
 */
void MotorASet(uint8_t direction, uint16_t pwm_value)
{
    MotorSet(MOTOR_A, direction, pwm_value);
}

/**
 * @brief 设置电机B的速度和方向
 * @param direction: 旋转方向 (MOTOR_DIR_FORWARD 或 MOTOR_DIR_REVERSE)
 * @param pwm_value: PWM占空比 (0-1000)
 */
void MotorBSet(uint8_t direction, uint16_t pwm_value)
{
    MotorSet(MOTOR_B, direction, pwm_value);
}

/**
 * @brief 停止指定电机
 * @param motor_id: 电机编号 (MOTOR_A 或 MOTOR_B)
 */
void MotorStop(uint8_t motor_id)
{
    if (motor_id == MOTOR_A) {
        DL_TimerG_setCaptureCompareValue(PWM_MOTOR_INST, PWM_MIN, DL_TIMER_CC_1_INDEX);
        g_motor_state.pwm_a = PWM_MIN;
    } else if (motor_id == MOTOR_B) {
        DL_TimerG_setCaptureCompareValue(PWM_MOTOR_INST, PWM_MIN, DL_TIMER_CC_0_INDEX);
        g_motor_state.pwm_b = PWM_MIN;
    }
}

/**
 * @brief 停止所有电机
 */
void MotorStopAll(void)
{
    MotorStop(MOTOR_A);
    MotorStop(MOTOR_B);
}

/* ============ 高级控制函数 ============ */

/**
 * @brief 小车前进
 * @param speed: 速度等级 (0-1000)
 */
void Forward(uint16_t speed)
{
    // 限制速度范围
    if (speed > PWM_MAX) {
        speed = PWM_MAX;
    }
    
    // 两个电机都正转，速度相同
    MotorASet(MOTOR_DIR_FORWARD, speed);
    MotorBSet(MOTOR_DIR_FORWARD, speed);
}

/**
 * @brief 小车后退
 * @param speed: 速度等级 (0-1000)
 */
void Backward(uint16_t speed)
{
    // 限制速度范围
    if (speed > PWM_MAX) {
        speed = PWM_MAX;
    }
    
    // 两个电机都反转，速度相同
    MotorASet(MOTOR_DIR_REVERSE, speed);
    MotorBSet(MOTOR_DIR_REVERSE, speed);
}

/**
 * @brief 小车左转（差速转弯）
 * @param speed: 基础速度等级 (0-1000)
 * @note 左电机（电机A）速度降低，右电机（电机B）保持，实现左转
 */
void TurnLeft(uint16_t speed)
{
    // 限制速度范围
    if (speed > PWM_MAX) {
        speed = PWM_MAX;
    }
    
    // 左电机（A）速度降低到50%，右电机（B）保持全速
    uint16_t left_speed = speed / 2;
    
    MotorASet(MOTOR_DIR_FORWARD, left_speed);
    MotorBSet(MOTOR_DIR_FORWARD, speed);
}

/**
 * @brief 小车右转（差速转弯）
 * @param speed: 基础速度等级 (0-1000)
 * @note 右电机（电机B）速度降低，左电机（电机A）保持，实现右转
 */
void TurnRight(uint16_t speed)
{
    // 限制速度范围
    if (speed > PWM_MAX) {
        speed = PWM_MAX;
    }
    
    // 右电机（B）速度降低到50%，左电机（A）保持全速
    uint16_t right_speed = speed / 2;
    
    MotorASet(MOTOR_DIR_FORWARD, speed);
    MotorBSet(MOTOR_DIR_FORWARD, right_speed);
}

/**
 * @brief 小车停止
 */
void Halt(void)
{
    MotorStopAll();
}

/* ============ PID 闭环控制实现 ============ */

/**
 * @brief 初始化电机 PID 闭环控制
 */
void MotorPID_Init(void)
{
    // 初始化电机 A 的 PID 控制器
    // Kp=100(1.0), Ki=10(0.1), Kd=5(0.05)
    // 输出限幅：-700 ~ 700
    PID_Increment_Init(&g_pid_motor_a, MOTOR_PID_KP, MOTOR_PID_KI, MOTOR_PID_KD,
                       MOTOR_PID_OUTPUT_MAX, MOTOR_PID_OUTPUT_MIN);
    
    // 初始化电机 B 的 PID 控制器
    PID_Increment_Init(&g_pid_motor_b, MOTOR_PID_KP, MOTOR_PID_KI, MOTOR_PID_KD,
                       MOTOR_PID_OUTPUT_MAX, MOTOR_PID_OUTPUT_MIN);
}

/**
 * @brief 将速度值（mm/s）映射为 PWM 占空比
 *
 * 映射公式：pwm = (speed_mmps * 700) / 25600
 * 已知：25600 mm/s 对应占空比 700
 */
uint16_t Speed_To_PWM(int16_t speed_mmps)
{
    int32_t temp;
    uint16_t pwm;
    
    // 处理负速度（反转）
    if (speed_mmps < 0) {
        speed_mmps = -speed_mmps;
    }
    
    // 限制最大速度
    if (speed_mmps > MAX_SPEED_MMPS) {
        speed_mmps = MAX_SPEED_MMPS;
    }
    
    // 映射：pwm = (speed * 700) / 25600
    temp = (int32_t)speed_mmps * SPEED_TO_PWM_RATIO_NUM;
    pwm = (uint16_t)(temp / SPEED_TO_PWM_RATIO_DEN);
    
    // 限幅到 0-700
    if (pwm > MOTOR_PID_OUTPUT_MAX) {
        pwm = MOTOR_PID_OUTPUT_MAX;
    }
    
    return pwm;
}

/**
 * @brief 设置电机 A 的目标速度（mm/s），使用 PID 闭环控制
 */
void MotorA_SetSpeedPID(int16_t target_speed)
{
    int16_t feedback_speed;     // 反馈速度（编码器读取）
    int16_t pid_output;         // PID 输出（速度增量）
    int16_t actual_speed;       // 实际控制速度
    uint16_t pwm_value;         // PWM 占空比
    uint8_t direction;          // 电机方向
    
    // 1. 读取编码器反馈速度
    feedback_speed = EncoderGetSpeed(ENCODER_A);
    
    // 2. 计算 PID 输出（增量式 PID）
    pid_output = PID_Increment_Calculate(&g_pid_motor_a, target_speed, feedback_speed);
    
    // 3. 计算实际控制速度 = 目标速度 + PID 增量
    actual_speed = target_speed + pid_output;
    
    // 4. 限幅到最大速度范围
    if (actual_speed > MAX_SPEED_MMPS) {
        actual_speed = MAX_SPEED_MMPS;
    } else if (actual_speed < -MAX_SPEED_MMPS) {
        actual_speed = -MAX_SPEED_MMPS;
    }
    
    // 5. 确定方向
    if (actual_speed >= 0) {
        direction = MOTOR_DIR_FORWARD;
    } else {
        direction = MOTOR_DIR_REVERSE;
        actual_speed = -actual_speed;
    }
    
    // 6. 将速度映射为 PWM 占空比
    pwm_value = Speed_To_PWM(actual_speed);
    
    // 7. 设置电机
    MotorASet(direction, pwm_value);
}

/**
 * @brief 设置电机 B 的目标速度（mm/s），使用 PID 闭环控制
 */
void MotorB_SetSpeedPID(int16_t target_speed)
{
    int16_t feedback_speed;     // 反馈速度（编码器读取）
    int16_t pid_output;         // PID 输出（速度增量）
    int16_t actual_speed;       // 实际控制速度
    uint16_t pwm_value;         // PWM 占空比
    uint8_t direction;          // 电机方向
    
    // 1. 读取编码器反馈速度
    feedback_speed = EncoderGetSpeed(ENCODER_B);
    
    // 2. 计算 PID 输出（增量式 PID）
    pid_output = PID_Increment_Calculate(&g_pid_motor_b, target_speed, feedback_speed);
    
    // 3. 计算实际控制速度 = 目标速度 + PID 增量
    actual_speed = target_speed + pid_output;
    
    // 4. 限幅到最大速度范围
    if (actual_speed > MAX_SPEED_MMPS) {
        actual_speed = MAX_SPEED_MMPS;
    } else if (actual_speed < -MAX_SPEED_MMPS) {
        actual_speed = -MAX_SPEED_MMPS;
    }
    
    // 5. 确定方向
    if (actual_speed >= 0) {
        direction = MOTOR_DIR_FORWARD;
    } else {
        direction = MOTOR_DIR_REVERSE;
        actual_speed = -actual_speed;
    }
    
    // 6. 将速度映射为 PWM 占空比
    pwm_value = Speed_To_PWM(actual_speed);
    
    // 7. 设置电机
    MotorBSet(direction, pwm_value);
}

/**
 * @brief 设置两个电机的目标速度（mm/s），使用 PID 闭环控制
 */
void Motor_SetSpeedPID(int16_t target_speed_a, int16_t target_speed_b)
{
    MotorA_SetSpeedPID(target_speed_a);
    MotorB_SetSpeedPID(target_speed_b);
}

/* ============ 使用 PID 的高级控制函数 ============ */

/**
 * @brief 小车前进（PID 闭环控制）
 * @param speed: 目标速度（mm/s），范围 0 ~ 25600
 */
void ForwardPID(int16_t speed)
{
    // 限制速度范围
    if (speed > MAX_SPEED_MMPS) {
        speed = MAX_SPEED_MMPS;
    } else if (speed < 0) {
        speed = 0;
    }
    
    // 两个电机都设置相同的目标速度
    Motor_SetSpeedPID(speed, speed);
}

/**
 * @brief 小车后退（PID 闭环控制）
 * @param speed: 目标速度（mm/s），范围 0 ~ 25600
 */
void BackwardPID(int16_t speed)
{
    // 限制速度范围
    if (speed > MAX_SPEED_MMPS) {
        speed = MAX_SPEED_MMPS;
    } else if (speed < 0) {
        speed = 0;
    }
    
    // 两个电机都设置相反的目标速度（负值表示反转）
    Motor_SetSpeedPID(-speed, -speed);
}

/**
 * @brief 小车原地左转（PID 闭环控制）
 * @param speed: 目标速度（mm/s），范围 0 ~ 25600
 * @note 左电机反转，右电机正转，实现原地左转
 */
void TurnLeftPID(int16_t speed)
{
    // 限制速度范围
    if (speed > MAX_SPEED_MMPS) {
        speed = MAX_SPEED_MMPS;
    } else if (speed < 0) {
        speed = 0;
    }
    
    // 左电机（A）反转，右电机（B）正转
    Motor_SetSpeedPID(-speed, speed);
}

/**
 * @brief 小车原地右转（PID 闭环控制）
 * @param speed: 目标速度（mm/s），范围 0 ~ 25600
 * @note 右电机反转，左电机正转，实现原地右转
 */
void TurnRightPID(int16_t speed)
{
    // 限制速度范围
    if (speed > MAX_SPEED_MMPS) {
        speed = MAX_SPEED_MMPS;
    } else if (speed < 0) {
        speed = 0;
    }
    
    // 右电机（B）反转，左电机（A）正转
    Motor_SetSpeedPID(speed, -speed);
}
