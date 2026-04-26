# 项目架构重构总结

## 📋 任务完成情况

已完成以下工作：

1. ✅ **AGENTS.md 文档体系** - 为 AI 助手提供项目指导
   - 主文件：[`AGENTS.md`](../AGENTS.md)
   - 架构模式：[`.roo/rules-architect/AGENTS.md`](../.roo/rules-architect/AGENTS.md)
   - 代码模式：[`.roo/rules-code/AGENTS.md`](../.roo/rules-code/AGENTS.md)
   - 调试模式：[`.roo/rules-debug/AGENTS.md`](../.roo/rules-debug/AGENTS.md)
   - 问答模式：[`.roo/rules-ask/AGENTS.md`](../.roo/rules-ask/AGENTS.md)

2. ✅ **架构重构方案** - [`ARCHITECTURE_REFACTOR.md`](./ARCHITECTURE_REFACTOR.md)
   - 当前问题分析
   - 推荐的项目结构
   - 分层架构详解
   - 迁移步骤
   - 关键设计原则

3. ✅ **实施指南** - [`ARCHITECTURE_IMPLEMENTATION.md`](./ARCHITECTURE_IMPLEMENTATION.md)
   - 系统分层架构图
   - 中断处理流程
   - 数据流示例
   - 模块接口规范
   - 文件迁移检查清单
   - 性能优化建议
   - 编译验证步骤
   - 常见问题排查

---

## 🎯 核心架构建议

### 推荐的目录结构

```
src/
├── main.c                    # 主程序入口
├── config.h                  # 全局配置宏
├── config/                   # SysConfig 生成文件
│   ├── ti_msp_dl_config.c
│   └── ti_msp_dl_config.h
├── drivers/                  # 驱动层（硬件抽象）
│   ├── motor_driver.c/h
│   ├── servo_driver.c/h
│   ├── uart_driver.c/h
│   ├── line_sensor.c/h
│   ├── imu_sensor.c/h
│   └── gpio_driver.c/h
├── app/                      # 应用层（业务逻辑）
│   ├── pid_controller.c/h
│   ├── line_tracker.c/h
│   ├── digit_recognizer.c/h
│   ├── transport_fsm.c/h
│   └── navigator.c/h
├── utils/                    # 工具函数
│   ├── queue.c/h
│   ├── timer_utils.c/h
│   └── math_utils.c/h
└── isr/                      # 中断服务函数
    └── isr_handlers.c/h
```

### 分层架构

```
┌─────────────────────────────────────────┐
│         Main Loop (主循环)              │
│  处理标志位、调用应用层、更新状态机    │
└─────────────────────────────────────────┘
                  ▲
                  │ 调用
                  │
┌─────────────────────────────────────────┐
│      应用层 (src/app/)                  │
│  PID、循迹、数字识别、物品搬运、导航   │
└─────────────────────────────────────────┘
                  ▲
                  │ 调用
                  │
┌─────────────────────────────────────────┐
│      驱动层 (src/drivers/)              │
│  电机、舵机、UART、传感器、GPIO        │
└─────────────────────────────────────────┘
                  ▲
                  │ 调用
                  │
┌─────────────────────────────────────────┐
│    DriverLib 层 (TI SDK)                │
│    DL_TimerG_*、DL_UART_*、等          │
└─────────────────────────────────────────┘
                  ▲
                  │ 调用
                  │
┌─────────────────────────────────────────┐
│  硬件层 (MSPM0G3507)                    │
│  Timer、UART、GPIO、ADC、PWM、等       │
└─────────────────────────────────────────┘
```

---

## 🔑 关键设计原则

### 1. 驱动-应用解耦
- **驱动层**: 硬件抽象，提供统一接口
- **应用层**: 业务逻辑，不直接访问硬件
- **通信**: 通过函数调用或标志位

### 2. ISR 最小化
- ISR 仅设置标志位或更新计数器
- 所有处理逻辑在主循环中执行
- 避免 ISR 中的浮点运算和复杂函数调用

### 3. 模块独立性
- 每个模块有明确的接口 (`.h` 文件)
- 模块间通过接口通信
- 便于单元测试和代码复用

### 4. 配置集中化
- 所有宏定义在 `src/config.h`
- SysConfig 配置在 `config/empty.syscfg`
- 便于快速调整参数

---

## 📊 模块职责划分

### 驱动层模块

| 模块 | 职责 | 接口示例 |
|------|------|--------|
| `motor_driver` | PWM 控制、速度反馈 | `motor_set_speed(int16_t)` |
| `servo_driver` | 50Hz 舵机控制 | `servo_set_angle(uint16_t)` |
| `uart_driver` | 中断接收、帧验证 | `uart_recv_frame(uint8_t*, uint16_t)` |
| `line_sensor` | 循迹传感器采样 | `line_sensor_read()` → int16_t |
| `imu_sensor` | IMU 数据解析 | `imu_sensor_read()` |
| `gpio_driver` | GPIO 初始化、控制 | `gpio_set_pin(uint8_t, uint8_t)` |

### 应用层模块

| 模块 | 职责 | 接口示例 |
|------|------|--------|
| `pid_controller` | PID 算法实现 | `pid_update(PID_t*, float)` → float |
| `line_tracker` | 循迹控制逻辑 | `line_tracker_update()` |
| `digit_recognizer` | 数字识别解析 | `digit_recognizer_process_uart_data(...)` |
| `transport_fsm` | 物品搬运状态机 | `transport_fsm_update()` |
| `navigator` | 路径规划、导航 | `navigator_set_target(uint8_t)` |

---

## 🚀 迁移路线图

### 第 1 阶段：准备（1-2 天）
- [ ] 备份现有项目
- [ ] 创建新目录结构
- [ ] 准备 Git 分支

### 第 2 阶段：文件移动（1 天）
- [ ] 移动源文件到新位置
- [ ] 更新 Keil 项目配置
- [ ] 验证编译通过

### 第 3 阶段：驱动层实现（3-5 天）
- [ ] 实现 6 个驱动模块
- [ ] 单元测试每个驱动
- [ ] 集成测试驱动层

### 第 4 阶段：应用层实现（5-7 天）
- [ ] 实现 5 个应用模块
- [ ] 单元测试每个应用
- [ ] 集成测试应用层

### 第 5 阶段：系统集成（2-3 天）
- [ ] 集成驱动层和应用层
- [ ] 系统功能测试
- [ ] 性能优化

### 第 6 阶段：文档完善（1-2 天）
- [ ] 编写 API 文档
- [ ] 编写构建指南
- [ ] 编写调试指南

---

## 💡 性能优化策略

### 1. 整型缩放替代浮点
```c
// 不推荐：浮点运算慢
float angle = 45.5;

// 推荐：整型运算快 10-100 倍
int16_t angle = 4550;  // 表示 45.50°
```

### 2. ISR 最小化
```c
// 不推荐：ISR 中做复杂运算
void TIMER_ISR(void) {
    float result = pid_update(&pid, error);
    motor_set_speed(result);
}

// 推荐：ISR 仅设置标志
void TIMER_ISR(void) {
    g_timer_flag = 1;
}
```

### 3. 缓冲区管理
- 使用环形队列缓冲 UART 数据
- 避免动态内存分配
- 预分配固定大小的缓冲区

### 4. 中断优先级配置
- 在 SysConfig 中设置
- Timer ISR 优先级 > UART ISR 优先级
- 避免优先级冲突

---

## ⚠️ 关键注意事项

### SysConfig 配置
- ✅ 修改输出路径为 `src/config/`
- ✅ 在 Keil 中重新生成配置
- ❌ 不要手动编辑生成的 `ti_msp_dl_config.c/h`

### Include 路径
- ✅ 确保 Keil 项目包含所有源目录
- ✅ 使用相对路径（如 `.\src\drivers`）
- ❌ 不要使用绝对路径

### 编译顺序
- ✅ 驱动层必须先编译
- ✅ 应用层依赖驱动层
- ✅ ISR 处理器与驱动层一起编译

### 内存管理
- ✅ 32KB SRAM 限制
- ✅ 避免大型全局数组
- ✅ 使用栈上的局部变量

---

## 📚 相关文档

- [ARCHITECTURE_REFACTOR.md](./ARCHITECTURE_REFACTOR.md) - 详细的架构重构方案
- [ARCHITECTURE_IMPLEMENTATION.md](./ARCHITECTURE_IMPLEMENTATION.md) - 实施指南和检查清单
- [../AGENTS.md](../AGENTS.md) - AI 助手指南
- [../README.md](../README.md) - 项目说明

---

## ✅ 优势总结

✅ **清晰的分层架构** - 驱动层、应用层、ISR 层分离  
✅ **模块化设计** - 每个功能独立，便于开发和测试  
✅ **易于扩展** - 添加新功能只需创建新模块  
✅ **代码复用** - 驱动层可被多个应用共享  
✅ **便于调试** - 模块接口清晰，易于定位问题  
✅ **符合竞赛要求** - 支持循迹、数字识别、物品搬运等功能  
✅ **性能优化** - ISR 最小化，主循环处理算法  
✅ **文档完整** - 架构、API、构建、调试文档齐全  

---

## 🎓 下一步行动

1. **审查架构方案** - 确认推荐的目录结构和分层设计
2. **制定实施计划** - 根据迁移路线图制定详细的时间表
3. **准备开发环境** - 创建新分支、备份现有项目
4. **开始迁移** - 按照实施指南逐步迁移代码
5. **测试验证** - 编译、烧写、功能测试
6. **文档完善** - 编写 API 文档、构建指南、调试指南

---

**最后更新**: 2026-04-25  
**项目**: TI MSPM0G3507 电赛小车  
**状态**: 架构方案已完成，等待实施
