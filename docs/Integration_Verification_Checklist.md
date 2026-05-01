# 小车底盘闭环控制 - 集成验证清单

## ✅ 文件清单

### 新增文件
- [x] [`src/config.h`](../src/config.h) - 已扩展陀螺仪 PID 参数
- [x] [`src/drivers/drv_chassis.h`](../src/drivers/drv_chassis.h) - 小车控制接口
- [x] [`src/drivers/drv_chassis.c`](../src/drivers/drv_chassis.c) - 小车控制实现
- [x] [`docs/Chassis_Control_Usage_Guide.md`](Chassis_Control_Usage_Guide.md) - 使用指南
- [x] [`plans/Gyro_Closed_Loop_Control_Plan.md`](../plans/Gyro_Closed_Loop_Control_Plan.md) - 设计方案

### 修改文件
- [x] [`empty.c`](../empty.c) - 已集成小车控制模块

---

## 🔧 Keil 项目配置

### 1. 添加源文件到 Keil 项目

在 Keil µVision 中，需要将新文件添加到项目：

1. 打开 `keil/empty_LP_MSPM0G3507_nortos_keil.uvprojx`
2. 在 Project 窗口中，右键点击 `Source Group 1` → `Add Existing Files to Group`
3. 添加以下文件：
   - `src/drivers/drv_chassis.c`

**注意**：头文件（`.h`）不需要添加到项目中，只需确保包含路径正确。

### 2. 验证包含路径

确保以下路径在 Keil 项目的 Include Paths 中：
- `.`（项目根目录）
- `src`
- `src/drivers`
- `src/utils`
- `src/app`
- `src/isr`

**检查方法**：
1. 右键项目 → `Options for Target`
2. `C/C++` 标签页
3. `Include Paths` 中应包含上述路径

---

## 📋 代码完整性检查

### 1. 依赖关系验证

```
drv_chassis.c 依赖：
├── drv_chassis.h ✅
├── drv_motor.h ✅
├── drv_jy61p.h ✅
├── pid.h ✅
├── config.h ✅
└── ti_msp_dl_config.h ✅

empty.c 依赖：
├── drv_chassis.h ✅
├── drv_motor.h ✅
├── drv_jy61p.h ✅
├── drv_uart.h ✅
└── 其他已有依赖 ✅
```

### 2. 函数调用链验证

```
main()
├── ChassisInit() ✅
└── while(1)
    └── Chassis_Control_Proc() ✅
        └── ChassisUpdate() ✅
            ├── jy61p_get_angle() ✅
            ├── PID_Position_Calculate() ✅
            └── MotorASet() / MotorBSet() ✅
```

### 3. 全局变量验证

```c
// drv_chassis.c
Chassis_t g_chassis; ✅

// empty.c
extern volatile uint32_t uwTick; ✅ (在 ti_msp_dl_config.c 或 timer.c 中定义)
```

---

## 🔍 编译前检查

### 1. 语法检查清单

- [x] 所有头文件都有 `#ifndef` 保护
- [x] 所有函数都有声明和实现
- [x] 所有结构体定义完整
- [x] 所有宏定义正确
- [x] 包含路径正确

### 2. 逻辑检查清单

- [x] PID 初始化参数正确（8 个参数）
- [x] PID 计算函数调用正确（3 个参数）
- [x] 电机控制函数调用正确
- [x] 陀螺仪数据读取正确
- [x] 时间计算逻辑正确
- [x] 死区判断逻辑正确
- [x] 差速控制逻辑正确

### 3. 数据类型检查

```c
// 确保数据类型匹配
int16_t target_yaw;           ✅ (°×100)
int16_t current_yaw;          ✅ (°×100)
int16_t yaw_error;            ✅ (°×100)
int16_t pid_output;           ✅ (差速值)
uint16_t motor_left_pwm;      ✅ (0-700)
uint16_t motor_right_pwm;     ✅ (0-700)
uint32_t motion_duration_ms;  ✅ (毫秒)
uint32_t elapsed_ms;          ✅ (毫秒)
```

---

## 🚀 编译步骤

### 方法 1：使用 Keil IDE

1. 打开 Keil µVision
2. 打开项目：`keil/empty_LP_MSPM0G3507_nortos_keil.uvprojx`
3. 添加 `src/drivers/drv_chassis.c` 到项目
4. 点击 `Project` → `Build Target` (或按 F7)
5. 检查 Build Output 窗口，确保无错误和警告

### 方法 2：使用命令行（如果配置了 UV4.exe）

```cmd
cd keil
UV4.exe -b empty_LP_MSPM0G3507_nortos_keil.uvprojx
```

---

## ⚠️ 可能的编译问题及解决方案

### 问题 1：找不到 `drv_chassis.h`

**错误信息**：
```
fatal error: drv_chassis.h: No such file or directory
```

**解决方案**：
1. 检查文件是否存在于 `src/drivers/` 目录
2. 检查 Keil 项目的 Include Paths 是否包含 `src/drivers`

### 问题 2：`uwTick` 未定义

**错误信息**：
```
undefined reference to `uwTick'
```

**解决方案**：
确保 `uwTick` 在 `ti_msp_dl_config.c` 或 `timer.c` 中定义：
```c
volatile uint32_t uwTick = 0;
```

### 问题 3：PID 函数未定义

**错误信息**：
```
undefined reference to `PID_Position_Init'
```

**解决方案**：
确保 `src/utils/pid.c` 已添加到 Keil 项目中。

### 问题 4：电机函数未定义

**错误信息**：
```
undefined reference to `MotorASet'
```

**解决方案**：
确保 `src/drivers/drv_motor.c` 已添加到 Keil 项目中。

---

## 🧪 运行时验证

### 1. 初始化验证

在 `main()` 函数中，确认以下初始化顺序：

```c
SYSCFG_DL_init();           // 1. 系统配置初始化
MotorInit();                // 2. 电机初始化
uart_init();                // 3. UART 初始化
jy61p_init();               // 4. 陀螺仪初始化
ChassisInit();              // 5. 小车控制初始化 ✅
__enable_irq();             // 6. 使能全局中断
```

### 2. 运行时检查

通过串口监测调试信息：

```
Chassis: Yaw=0, Err=5, PID=15, L=485, R=515
```

**正常状态**：
- `Yaw` 应该在目标值附近波动
- `Err` 应该在死区范围内（±50）
- `PID` 应该根据误差动态调整
- `L` 和 `R` 应该在合理范围内（0-700）

**异常状态**：
- `Yaw` 始终为 0 → 陀螺仪数据未更新
- `Err` 持续很大 → PID 参数不合适
- `PID` 始终为 0 → 死区过大或 PID 未工作
- `L` 或 `R` 为 0 → 电机控制异常

### 3. 功能测试

```c
// 测试 1：直线前进
ChassisSetMotion(0, 2000, 500, CHASSIS_DIR_FORWARD);
// 预期：小车直线前进 2 秒后自动停止

// 测试 2：转向
ChassisSetMotion(9000, 2000, 400, CHASSIS_DIR_FORWARD);
// 预期：小车转向到 90° 方向

// 测试 3：后退
ChassisSetMotion(0, 1500, 400, CHASSIS_DIR_BACKWARD);
// 预期：小车直线后退 1.5 秒后自动停止
```

---

## 📊 性能指标验证

| 指标 | 预期值 | 验证方法 |
|------|--------|---------|
| 控制周期 | 10ms | 测量 `Chassis_Control_Proc()` 调用间隔 |
| 角度精度 | ±0.5° | 观察稳态误差 |
| 时间精度 | ±10ms | 测量实际运行时间 |
| PWM 范围 | 0-700 | 监测 `motor_left_pwm` 和 `motor_right_pwm` |
| 差速范围 | ±300 | 监测 `pid_output` |

---

## ✅ 最终检查清单

### 编译阶段
- [ ] Keil 项目中已添加 `drv_chassis.c`
- [ ] 编译无错误
- [ ] 编译无警告
- [ ] 生成 `.axf` 和 `.hex` 文件

### 烧写阶段
- [ ] 成功烧写到 MSPM0G3507
- [ ] 程序正常启动
- [ ] LED 或蜂鸣器有响应（如果有）

### 运行阶段
- [ ] 陀螺仪校准完成
- [ ] 串口输出正常
- [ ] 小车响应 `ChassisSetMotion()` 命令
- [ ] 小车能保持目标偏航角
- [ ] 小车能在指定时间后自动停止
- [ ] PID 控制稳定，无明显抖动

---

## 🎯 下一步建议

1. **参数调优**：根据实际小车响应调整 PID 参数
2. **功能扩展**：添加速度闭环、路径规划等功能
3. **安全机制**：添加障碍物检测、紧急停止等功能
4. **性能优化**：优化控制算法，提高响应速度

---

## 📚 相关文档

- [使用指南](Chassis_Control_Usage_Guide.md)
- [设计方案](../plans/Gyro_Closed_Loop_Control_Plan.md)
- [API 参考](../src/drivers/drv_chassis.h)

---

## 🆘 技术支持

如遇到问题，请检查：
1. 硬件连接是否正确
2. 陀螺仪是否正常工作
3. 电机驱动是否正常
4. PID 参数是否合理
5. 串口调试信息

**调试技巧**：
- 使用 `ChassisGetState()` 获取实时状态
- 通过串口输出调试信息
- 逐步测试各个功能模块
- 从简单场景开始测试（如直线前进）
