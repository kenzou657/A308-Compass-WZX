# 项目文件架构重构方案

## 当前状态分析

### 现有结构问题
1. **源代码混乱**: `empty.c` 和 `ti_msp_dl_config.c/h` 混在项目根目录
2. **驱动层缺失**: 没有独立的驱动模块（Motor、Servo、UART、Sensor）
3. **应用层缺失**: 没有业务逻辑层（PID、LineTracking、DigitRecognition）
4. **配置分散**: SysConfig 配置与源代码混合
5. **文档不完整**: 缺少详细的模块接口文档

---

## 推荐的项目架构

```
project-root/
├── .roo/                           # AI 助手规则
│   ├── rules-architect/
│   │   └── AGENTS.md
│   ├── rules-code/
│   │   └── AGENTS.md
│   ├── rules-debug/
│   │   └── AGENTS.md
│   └── rules-ask/
│       └── AGENTS.md
│
├── src/                            # 源代码根目录
│   ├── main.c                      # 主程序入口
│   ├── config.h                    # 项目全局配置宏
│   │
│   ├── config/                     # SysConfig 生成文件
│   │   ├── ti_msp_dl_config.c      # SysConfig 生成（勿手动编辑）
│   │   └── ti_msp_dl_config.h      # SysConfig 生成（勿手动编辑）
│   │
│   ├── drivers/                    # 驱动层（硬件抽象）
│   │   ├── motor_driver.h
│   │   ├── motor_driver.c          # PWM 控制、速度反馈
│   │   ├── servo_driver.h
│   │   ├── servo_driver.c          # 50Hz 舵机控制
│   │   ├── uart_driver.h
│   │   ├── uart_driver.c           # 中断/DMA 接收、帧验证
│   │   ├── line_sensor.h
│   │   ├── line_sensor.c           # 循迹传感器采样
│   │   ├── imu_sensor.h
│   │   ├── imu_sensor.c            # IMU 数据解析
│   │   ├── gpio_driver.h
│   │   └── gpio_driver.c           # GPIO 初始化、控制
│   │
│   ├── app/                        # 应用层（业务逻辑）
│   │   ├── pid_controller.h
│   │   ├── pid_controller.c        # PID 算法（位置式/增量式）
│   │   ├── line_tracker.h
│   │   ├── line_tracker.c          # 循迹控制逻辑
│   │   ├── digit_recognizer.h
│   │   ├── digit_recognizer.c      # OpenMV 数据解析、识别
│   │   ├── transport_fsm.h
│   │   ├── transport_fsm.c         # 物品搬运状态机
│   │   ├── navigator.h
│   │   └── navigator.c             # 路径规划、导航
│   │
│   ├── utils/                      # 工具函数
│   │   ├── queue.h
│   │   ├── queue.c                 # 环形队列（UART 缓冲）
│   │   ├── timer_utils.h
│   │   ├── timer_utils.c           # 定时器工具
│   │   ├── math_utils.h
│   │   └── math_utils.c            # 数学函数（整型缩放）
│   │
│   └── isr/                        # 中断服务函数
│       ├── isr_handlers.h
│       └── isr_handlers.c          # 所有 ISR 集中管理
│
├── keil/                           # Keil 项目文件
│   ├── empty_LP_MSPM0G3507_nortos_keil.uvprojx
│   ├── empty_LP_MSPM0G3507_nortos_keil.uvoptx
│   ├── mspm0g3507.sct             # 链接脚本
│   ├── startup_mspm0g350x_uvision.s
│   ├── .vscode/
│   └── Objects/                    # 编译输出（勿提交）
│
├── config/                         # 配置文件
│   └── empty.syscfg                # SysConfig 配置文件
│
├── doc/                            # 文档
│   ├── 2026 年四川省大学生电子设计竞赛 西南石油大学成都校区选拔赛.md
│   ├── ARCHITECTURE.md             # 架构设计文档
│   ├── API.md                      # 模块 API 文档
│   ├── BUILD.md                    # 构建指南
│   └── DEBUGGING.md                # 调试指南
│
├── tools/                          # 工具脚本
│   └── keil/
│       └── syscfg.bat              # SysConfig 生成脚本
│
├── .roorules                       # 项目角色定义
├── AGENTS.md                       # AI 助手指南
├── README.md                       # 项目说明
└── .gitignore                      # Git 忽略规则
```

---

## 分层架构详解

### 1. 驱动层 (`src/drivers/`)
**职责**: 硬件抽象，提供统一接口

**Motor Driver** (`motor_driver.c/h`)
```c
// 接口示例
void motor_init(void);
void motor_set_speed(int16_t speed);  // -100 ~ 100
int16_t motor_get_speed(void);
void motor_stop(void);
```

**Servo Driver** (`servo_driver.c/h`)
```c
// 接口示例
void servo_init(void);
void servo_set_angle(uint16_t angle);  // 0 ~ 36000 (0° ~ 360°)
uint16_t servo_get_angle(void);
```

**UART Driver** (`uart_driver.c/h`)
```c
// 接口示例
void uart_init(void);
void uart_send_frame(uint8_t *data, uint16_t len);
uint16_t uart_recv_frame(uint8_t *buffer, uint16_t max_len);
```

**Line Sensor** (`line_sensor.c/h`)
```c
// 接口示例
void line_sensor_init(void);
int16_t line_sensor_read(void);  // -100 ~ 100 (偏离度)
```

### 2. 应用层 (`src/app/`)
**职责**: 业务逻辑，控制算法

**PID Controller** (`pid_controller.c/h`)
```c
typedef struct {
    float kp, ki, kd;
    float target, current;
    float integral, last_error;
} PID_t;

float pid_update(PID_t *pid, float error);
```

**Line Tracker** (`line_tracker.c/h`)
```c
void line_tracker_init(void);
void line_tracker_update(void);  // 在主循环中调用
int16_t line_tracker_get_motor_cmd(void);
```

**Digit Recognizer** (`digit_recognizer.c/h`)
```c
void digit_recognizer_init(void);
void digit_recognizer_process_uart_data(uint8_t *data, uint16_t len);
uint8_t digit_recognizer_get_result(void);  // 返回识别的数字 1-5
```

**Transport FSM** (`transport_fsm.c/h`)
```c
typedef enum {
    STATE_IDLE,
    STATE_NAVIGATE,
    STATE_PICKUP,
    STATE_RETURN,
    STATE_DROP
} transport_state_t;

void transport_fsm_init(void);
void transport_fsm_update(void);
void transport_fsm_set_target(uint8_t target_zone);
```

### 3. 中断处理 (`src/isr/`)
**职责**: 最小化 ISR，设置标志位

```c
// isr_handlers.c
volatile uint8_t g_timer_flag = 0;
volatile uint8_t g_uart_frame_ready = 0;
volatile uint8_t g_sensor_data_ready = 0;

void TIMER_ISR(void) {
    g_timer_flag = 1;  // 仅设置标志
}

void UART_ISR(void) {
    // 接收数据到缓冲区
    // 验证帧头/帧尾
    if (frame_valid) {
        g_uart_frame_ready = 1;
    }
}
```

### 4. 主程序 (`src/main.c`)
```c
int main(void) {
    SYSCFG_DL_init();
    
    // 初始化所有驱动
    motor_init();
    servo_init();
    uart_init();
    line_sensor_init();
    
    // 初始化应用层
    pid_controller_init();
    line_tracker_init();
    digit_recognizer_init();
    transport_fsm_init();
    
    while (1) {
        // 处理定时器标志
        if (g_timer_flag) {
            g_timer_flag = 0;
            
            // 读取传感器
            int16_t sensor_val = line_sensor_read();
            
            // 更新控制算法
            line_tracker_update();
            int16_t motor_cmd = line_tracker_get_motor_cmd();
            
            // 执行控制
            motor_set_speed(motor_cmd);
        }
        
        // 处理 UART 数据
        if (g_uart_frame_ready) {
            g_uart_frame_ready = 0;
            uint8_t buffer[64];
            uint16_t len = uart_recv_frame(buffer, sizeof(buffer));
            digit_recognizer_process_uart_data(buffer, len);
        }
        
        // 更新状态机
        transport_fsm_update();
    }
}
```

---

## 迁移步骤

### 第 1 步: 创建目录结构
```bash
mkdir -p src/{config,drivers,app,utils,isr}
mkdir -p config doc tools/keil
```

### 第 2 步: 移动文件
- `empty.c` → `src/main.c`
- `ti_msp_dl_config.c/h` → `src/config/`
- `empty.syscfg` → `config/`

### 第 3 步: 创建驱动模块
- 从 `empty.c` 中提取 GPIO/UART/Timer 初始化逻辑
- 创建 `motor_driver.c/h`, `servo_driver.c/h` 等

### 第 4 步: 创建应用模块
- 实现 PID 控制器
- 实现循迹算法
- 实现数字识别解析
- 实现物品搬运状态机

### 第 5 步: 更新 Keil 项目
- 修改 Keil 项目文件，指向新的源文件路径
- 更新包含路径 (Include Path)
- 重新编译验证

### 第 6 步: 更新 SysConfig
- 修改 `empty.syscfg` 中的输出路径
- 确保生成的 `ti_msp_dl_config.c/h` 输出到 `src/config/`

---

## 关键设计原则

### 1. 驱动-应用解耦
- 驱动层: 只负责硬件操作
- 应用层: 只负责算法逻辑
- 通信: 通过函数调用或标志位

### 2. ISR 最小化
- ISR 仅设置标志位或更新计数器
- 所有处理逻辑在主循环中执行
- 避免 ISR 中的浮点运算

### 3. 模块独立性
- 每个模块有明确的接口 (`.h` 文件)
- 模块间通过接口通信，不直接访问内部变量
- 便于单元测试和代码复用

### 4. 配置集中化
- 所有宏定义在 `src/config.h`
- SysConfig 配置在 `config/empty.syscfg`
- 便于快速调整参数

---

## 编译配置更新

### Keil 项目设置
1. **Include Paths**:
   ```
   .\src
   .\src\config
   .\src\drivers
   .\src\app
   .\src\utils
   .\src\isr
   ```

2. **Source Files**:
   ```
   src/main.c
   src/config/ti_msp_dl_config.c
   src/drivers/motor_driver.c
   src/drivers/servo_driver.c
   src/drivers/uart_driver.c
   src/drivers/line_sensor.c
   src/drivers/imu_sensor.c
   src/drivers/gpio_driver.c
   src/app/pid_controller.c
   src/app/line_tracker.c
   src/app/digit_recognizer.c
   src/app/transport_fsm.c
   src/app/navigator.c
   src/utils/queue.c
   src/utils/timer_utils.c
   src/utils/math_utils.c
   src/isr/isr_handlers.c
   keil/startup_mspm0g350x_uvision.s
   ```

3. **BeforeMake Hook**:
   ```
   cmd.exe /C "$P../../tools/keil/syscfg.bat '$P' ../../config/empty.syscfg"
   ```

---

## 文档补充

### 需要创建的文档

**ARCHITECTURE.md** - 系统架构详解
- 分层设计
- 模块交互图
- 数据流

**API.md** - 模块接口文档
- 每个驱动/应用模块的 API
- 参数说明
- 返回值说明

**BUILD.md** - 构建指南
- 环境配置
- 编译步骤
- 常见问题

**DEBUGGING.md** - 调试指南
- 断点设置
- 日志输出
- 性能分析

---

## 优势总结

✅ **清晰的分层架构** - 驱动层、应用层、ISR 层分离  
✅ **模块化设计** - 每个功能独立，便于开发和测试  
✅ **易于扩展** - 添加新功能只需创建新模块  
✅ **代码复用** - 驱动层可被多个应用共享  
✅ **便于调试** - 模块接口清晰，易于定位问题  
✅ **符合竞赛要求** - 支持循迹、数字识别、物品搬运等功能  
✅ **性能优化** - ISR 最小化，主循环处理算法  
✅ **文档完整** - 架构、API、构建、调试文档齐全
