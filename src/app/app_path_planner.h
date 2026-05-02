/**
 * @file app_path_planner.h
 * @brief 路径规划模块 - 头文件
 * 
 * 功能描述：
 * - 定义路径点数据结构
 * - 提供路径规划执行接口
 * - 管理预定义路径表
 * 
 * 设计思想：
 * - 将复杂的多阶段运动分解为原子路径点
 * - 支持灵活的路径组合
 * - 支持动态路径修改
 */

#ifndef APP_PATH_PLANNER_H
#define APP_PATH_PLANNER_H

#include <stdint.h>
#include <stdbool.h>

/* ==================== 路径点定义 ==================== */

/**
 * @brief 路径点结构
 * 
 * 描述小车在一个时间段内的运动参数
 */
typedef struct {
    int16_t target_yaw;             /* 目标偏航角（°×100）
                                       范围：-36000 ~ +36000
                                       0°：直行
                                       +45°：左转45°
                                       -45°：右转45° */
    
    uint32_t duration_ms;           /* 运动时间（ms）
                                       范围：100 ~ 30000 */
    
    uint16_t base_pwm;              /* 基础 PWM 占空比（0-700）
                                       0：原地转弯
                                       200-400：正常速度 */
    
    uint8_t direction;              /* 运动方向
                                       0：前进（CHASSIS_DIR_FORWARD）
                                       1：后退（CHASSIS_DIR_BACKWARD） */
    
    uint32_t pause_ms;              /* 到达后停顿时间（ms）
                                       0：不停顿
                                       500：停0.5s */
} PathPoint_t;

/* ==================== 路径定义 ==================== */

/**
 * @brief 路径结构
 * 
 * 管理一条完整的路径执行
 */
typedef struct {
    const PathPoint_t *points;      /* 路径点数组指针 */
    uint16_t point_count;           /* 路径点总数 */
    uint16_t current_point;         /* 当前执行的路径点索引 */
    uint32_t point_start_time;      /* 当前路径点开始时间 */
    uint8_t state;                  /* 路径执行状态
                                       0: 运动中
                                       1: 停顿中
                                       2: 完成 */
} Path_t;

/* ==================== 路径规划函数 ==================== */

/**
 * @brief 初始化路径
 * 
 * @param path 路径结构指针
 * @param points 路径点数组指针
 * @param count 路径点数量
 * 
 * @note 必须在使用路径前调用
 */
void PathInit(Path_t *path, const PathPoint_t *points, uint16_t count);

/**
 * @brief 执行路径规划
 * 
 * @param path 路径结构指针
 * 
 * 功能：
 * - 根据当前路径点执行运动
 * - 检查路径点是否完成
 * - 移动到下一个路径点
 * 
 * @note 应在主循环中定期调用（建议 10ms 周期）
 */
void PathExecute(Path_t *path);

/**
 * @brief 检查路径是否完成
 * 
 * @param path 路径结构指针
 * @return true 表示路径已完成，false 表示未完成
 */
bool PathIsComplete(const Path_t *path);

/**
 * @brief 重置路径
 * 
 * @param path 路径结构指针
 * 
 * 功能：
 * - 重置路径点索引为 0
 * - 重置路径状态为 0（运动中）
 */
void PathReset(Path_t *path);

/* ==================== 预定义路径表 ==================== */

/**
 * @brief 预定义路径表声明
 * 
 * 这些路径表定义了从停车区到各目标区的标准路径
 * 可根据实际场地情况进行调整
 */

extern const PathPoint_t g_path_to_target_1[];  /* 目标区1：直行（0°） */
extern const PathPoint_t g_path_to_target_2[];  /* 目标区2：左转45° */
extern const PathPoint_t g_path_to_target_3[];  /* 目标区3：左转90° */
extern const PathPoint_t g_path_to_target_4[];  /* 目标区4：右转45° */
extern const PathPoint_t g_path_to_target_5[];  /* 目标区5：右转90° */

extern const PathPoint_t g_path_return_home[];  /* 返回停车区 */

#endif /* APP_PATH_PLANNER_H */
