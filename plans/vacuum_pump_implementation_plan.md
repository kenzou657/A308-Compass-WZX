# 真空泵吸取系统实现方案（修正版）

## 1. PWM 参数计算

### TIMG8 配置分析
```
时钟源: BUSCLK = 40MHz
分频比: 8
预分频: 249
有效时钟频率 = 40MHz / (8 × (249+1)) = 40MHz / 2000 = 20kHz

PWM周期 = 400 (计数值)
PWM频率 = 20kHz / 400 = 50Hz
PWM周期时间 = 1 / 50Hz = 20ms
```

### 脉宽值计算（舵机标准 50Hz 控制）
```
标准舵机控制：
- 周期: 20ms (50Hz)
- 脉宽范围: 0.5ms ~ 2.5ms

TIMG8 配置为 50Hz (20ms周期)，完全符合舵机标准！

脉宽值 = (脉宽时间 / 周期时间) × 周期计数值
- 0.5ms 脉宽: (0.5ms / 20ms) × 400 = 10
- 2.5ms 脉宽: (2.5ms / 20ms) × 400 = 50

实际应用脉宽值：
- 关闭 (0°): 脉宽值 = 10  (0.5ms, 2.5% 占空比)
- 打开 (180°): 脉宽值 = 50 (2.5ms, 12.5% 占空比)
```

## 2. 系统架构

```
┌─────────────────────────────────────────────────────────┐
│              任务层 (app_task_*.c)                       │
│  - 产生真空泵控制标志位                                  │
│  - vacuum_pump_cmd (IDLE/SUCTION/RELEASE)              │
└────────────┬──────────────────────────────────────────────┘
             │
┌────────────▼──────────────────────────────────────────────┐
│         真空泵应用层 (app_vacuum_pump.c)                  │
│  - 状态机管理 (IDLE/SUCKING/RELEASING)                   │
│  - 时序控制和超时保护                                     │
│  - 根据标志位驱动状态转移                                 │
└────────────┬──────────────────────────────────────────────┘
             │
┌────────────▼──────────────────────────────────────────────┐
│         真空泵驱动层 (drv_vacuum_pump.c)                  │
│  - PWM输出控制 (TIMG8)                                    │
│  - 真空泵和电磁阀的脉宽管理                               │
│  - 硬件抽象                                               │
└────────────┬──────────────────────────────────────────────┘
             │
┌────────────▼──────────────────────────────────────────────┐
│              硬件层 (SysConfig)                            │
│  - TIMG8-CH0 (PA26): 真空泵 PWM                           │
│  - TIMG8-CH1 (PA30): 电磁阀 PWM                           │
│  - 50Hz频率, 20ms周期                                     │
└─────────────────────────────────────────────────────────────┘
```

## 3. 状态机设计

```
┌──────────┐
│  IDLE    │ (待命状态)
└────┬─────┘
     │ vacuum_pump_cmd = SUCTION
     ▼
┌──────────────────────────────────────────┐
│  SUCTION_SEQUENCE (吸物流程)             │
│  ├─ SUCTION_CLOSE_VALVE (100ms)         │
│  │   设置电磁阀 PWM = 10 (关闭)         │
│  ├─ SUCTION_START_PUMP (100ms)          │
│  │   设置真空泵 PWM = 50 (打开)         │
│  └─ SUCKING (最长60秒)                   │
│      真空泵运行中，等待标志位变化        │
└────┬─────────────────────────────────────┘
     │ vacuum_pump_cmd = RELEASE
     ▼
┌──────────────────────────────────────────┐
│  RELEASE_SEQUENCE (卸物流程)             │
│  ├─ RELEASE_STOP_PUMP (100ms)           │
│  │   设置真空泵 PWM = 10 (关闭)         │
│  ├─ RELEASE_OPEN_VALVE (2秒)            │
│  │   设置电磁阀 PWM = 50 (打开)         │
│  └─ RELEASING (最长10秒)                 │
│      电磁阀开启中，等待标志位变化        │
└────┬─────────────────────────────────────┘
     │ vacuum_pump_cmd = IDLE
     └──────────────────────────────────────┘
```

## 4. 模块设计详解

### 4.1 驱动层 (drv_vacuum_pump.h/c)

**职责**：
- 管理真空泵和电磁阀的 PWM 输出
- 提供脉宽设置接口
- 硬件初始化

**关键函数**：
```c
void VacuumPump_Init(void);                    // 初始化驱动
void VacuumPump_SetPulse(uint16_t pulse);      // 设置真空泵脉宽
void SolenoidValve_SetPulse(uint16_t pulse);   // 设置电磁阀脉宽
uint16_t VacuumPump_GetPulse(void);            // 获取真空泵脉宽
uint16_t SolenoidValve_GetPulse(void);         // 获取电磁阀脉宽
```

**宏定义**：
```c
// PWM 脉宽值 (周期计数值 = 400, 50Hz, 20ms周期)
#define VACUUM_PUMP_PULSE_MIN       10     // 0.5ms (最小脉宽)
#define VACUUM_PUMP_PULSE_MAX       50     // 2.5ms (最大脉宽)
#define VACUUM_PUMP_PULSE_CLOSE     10     // 关闭脉宽 (0.5ms)
#define VACUUM_PUMP_PULSE_OPEN      50     // 打开脉宽 (2.5ms)

#define SOLENOID_VALVE_PULSE_MIN    10     // 0.5ms (最小脉宽)
#define SOLENOID_VALVE_PULSE_MAX    50     // 2.5ms (最大脉宽)
#define SOLENOID_VALVE_PULSE_CLOSE  10     // 关闭脉宽 (0.5ms)
#define SOLENOID_VALVE_PULSE_OPEN   50     // 打开脉宽 (2.5ms)
```

### 4.2 应用层 (app_vacuum_pump.h/c)

**职责**：
- 实现完整的吸物/卸物流程
- 管理状态转移和时序
- 超时保护
- 根据任务产生的标志位驱动状态转移

**状态枚举**：
```c
typedef enum {
    VACUUM_STATE_IDLE = 0,              // 待命
    VACUUM_STATE_SUCTION_CLOSE_VALVE,   // 关闭电磁阀
    VACUUM_STATE_SUCTION_START_PUMP,    // 启动真空泵
    VACUUM_STATE_SUCKING,               // 吸物中
    VACUUM_STATE_RELEASE_STOP_PUMP,     // 停止真空泵
    VACUUM_STATE_RELEASE_OPEN_VALVE,    // 打开电磁阀
    VACUUM_STATE_RELEASING              // 卸物中
} VacuumPumpState_t;

typedef enum {
    VACUUM_CMD_IDLE = 0,                // 待命指令
    VACUUM_CMD_SUCTION,                 // 吸物指令
    VACUUM_CMD_RELEASE                  // 释放指令
} VacuumPumpCmd_t;
```

**关键函数**：
```c
void VacuumPump_App_Init(void);                    // 初始化应用
void VacuumPump_App_Update(void);                  // 周期更新（主循环调用）
void VacuumPump_App_SetCommand(VacuumPumpCmd_t cmd); // 设置控制指令
VacuumPumpState_t VacuumPump_App_GetState(void);   // 获取当前状态
```

**时序参数**：
```c
#define SUCTION_CLOSE_VALVE_TIME    100     // 100ms
#define SUCTION_START_PUMP_TIME     100     // 100ms
#define SUCTION_MAX_TIME            60000   // 60秒
#define RELEASE_STOP_PUMP_TIME      100     // 100ms
#define RELEASE_OPEN_VALVE_TIME     2000    // 2秒
#define RELEASE_MAX_TIME            10000   // 10秒
```

### 4.3 任务集成

**修改点**：
- 在各任务中（如 `app_task_5_recognize_transport.c`）设置真空泵控制标志位
- 调用 `VacuumPump_App_SetCommand(VACUUM_CMD_SUCTION)` 触发吸物
- 调用 `VacuumPump_App_SetCommand(VACUUM_CMD_RELEASE)` 触发释放

**伪代码**：
```c
// 在任务中
if (需要吸物) {
    VacuumPump_App_SetCommand(VACUUM_CMD_SUCTION);
}

if (需要释放) {
    VacuumPump_App_SetCommand(VACUUM_CMD_RELEASE);
}

// 查询状态
VacuumPumpState_t state = VacuumPump_App_GetState();
if (state == VACUUM_STATE_SUCKING) {
    // 物品已吸取，可以移动
}
```

### 4.4 主程序集成 (empty.c)

**初始化**：
```c
VacuumPump_Init();           // 初始化驱动
VacuumPump_App_Init();       // 初始化应用
```

**主循环**：
```c
while (1) {
    Key_Proc();
    TaskManager_Update();
    VacuumPump_App_Update();  // 添加此行（周期更新真空泵状态机）
    Camera_Proc();
    OLED_Proc();
}
```

## 5. 工作流程示例

### 场景：任务中吸取物品

1. **任务判断需要吸物**
   ```c
   VacuumPump_App_SetCommand(VACUUM_CMD_SUCTION);
   ```

2. **真空泵应用状态转移**
   - `IDLE` → `SUCTION_CLOSE_VALVE`
   - 设置电磁阀 PWM = 10 (关闭)
   - 等待 100ms

3. **自动转移** → `SUCTION_START_PUMP`
   - 设置真空泵 PWM = 50 (打开)
   - 等待 100ms

4. **自动转移** → `SUCKING`
   - 真空泵运行中
   - 监控超时（最长60秒）

5. **任务查询状态**
   ```c
   if (VacuumPump_App_GetState() == VACUUM_STATE_SUCKING) {
       // 物品已吸取，可以移动小车
   }
   ```

### 场景：任务中释放物品

1. **任务判断需要释放**
   ```c
   VacuumPump_App_SetCommand(VACUUM_CMD_RELEASE);
   ```

2. **真空泵应用状态转移**
   - `SUCKING` → `RELEASE_STOP_PUMP`
   - 设置真空泵 PWM = 10 (关闭)
   - 等待 100ms

3. **自动转移** → `RELEASE_OPEN_VALVE`
   - 设置电磁阀 PWM = 50 (打开)
   - 等待 2秒

4. **自动转移** → `RELEASING`
   - 电磁阀开启中
   - 监控超时（最长10秒）

5. **任务查询状态**
   ```c
   if (VacuumPump_App_GetState() == VACUUM_STATE_IDLE) {
       // 物品已释放，可以继续下一步
   }
   ```

## 6. 实现步骤

### Step 1: 创建驱动模块
- 文件：`src/drivers/drv_vacuum_pump.h` 和 `src/drivers/drv_vacuum_pump.c`
- 内容：PWM 初始化、脉宽设置函数

### Step 2: 创建应用模块
- 文件：`src/app/app_vacuum_pump.h` 和 `src/app/app_vacuum_pump.c`
- 内容：状态机、时序控制、超时保护

### Step 3: 修改主程序
- 文件：`empty.c`
- 修改：初始化真空泵模块、主循环中调用更新函数

### Step 4: 在任务中集成
- 文件：`src/app/app_task_*.c`
- 修改：调用 `VacuumPump_App_SetCommand()` 和查询状态

### Step 5: 验证 SysConfig
- 确保 TIMG8 已正确配置

## 7. 关键设计特点

✅ **模块化**：驱动层与应用层完全分离  
✅ **任务驱动**：由任务产生的标志位控制，而非按键  
✅ **可扩展**：易于添加其他控制逻辑  
✅ **安全**：超时保护防止设备损伤  
✅ **易调试**：状态机清晰，便于 UART 日志输出  
✅ **兼容性**：与现有任务管理系统无缝集成  

## 8. 调试建议

- 使用 UART 输出状态转移日志
- 使用逻辑分析仪观察 PWM 波形
- 验证各状态的时间间隔是否符合预期
- 测试超时保护是否正常工作
- 在 OLED 上显示当前真空泵状态
- 验证脉宽值 10 和 50 是否能正确控制真空泵和电磁阀
