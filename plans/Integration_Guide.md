# 任务文件集成指南

## 📋 已创建的文件清单

### 任务文件（16 个）

| 任务 | 头文件 | 实现文件 | 状态 |
|------|--------|---------|------|
| Task 1 | `app_task_1_line_tracking.h` | `app_task_1_line_tracking.c` | ✓ 已创建 |
| Task 2 | `app_task_2_dual_point.h` | `app_task_2_dual_point.c` | ✓ 已创建 |
| Task 3 | `app_task_3_round_trip.h` | `app_task_3_round_trip.c` | ✓ 已创建 |
| Task 4 | `app_task_4_digit_recognition.h` | `app_task_4_digit_recognition.c` | ✓ 已创建 |
| Task 5 | `app_task_5_recognize_transport.h` | `app_task_5_recognize_transport.c` | ✓ 已创建 |
| Task 6 | `app_task_6_auto_transport.h` | `app_task_6_auto_transport.c` | ✓ 已创建 |
| Task 7 | `app_task_7_batch_transport.h` | `app_task_7_batch_transport.c` | ✓ 已创建 |
| Task 8 | `app_task_8_creative.h` | `app_task_8_creative.c` | ✓ 已创建 |

### 路径规划模块（2 个）

| 模块 | 文件 | 状态 |
|------|------|------|
| 路径规划 | `app_path_planner.h` | ✓ 已创建 |
| 路径规划 | `app_path_planner.c` | ✓ 已创建 |

### 规划文档（4 个）

| 文档 | 文件 | 内容 |
|------|------|------|
| 总结 | `plans/SOLUTION_SUMMARY.md` | 方案总结 |
| 计划 | `plans/Task_Implementation_Plan.md` | 任务实现计划 |
| 架构 | `plans/System_Architecture_Design.md` | 系统架构设计 |
| 指南 | `plans/Implementation_Guide.md` | 实现指南 |

---

## 🔧 集成步骤

### 第一步：验证文件结构

确保所有文件都在正确的位置：

```
src/app/
├── app_task_1_line_tracking.h/c
├── app_task_2_dual_point.h/c
├── app_task_3_round_trip.h/c
├── app_task_4_digit_recognition.h/c
├── app_task_5_recognize_transport.h/c
├── app_task_6_auto_transport.h/c
├── app_task_7_batch_transport.h/c
├── app_task_8_creative.h/c
├── app_path_planner.h/c
├── app_task_manager.h/c          ✓ 已有
└── app_chassis_task.h/c          ✓ 已有
```

### 第二步：更新任务管理器

修改 `src/app/app_task_manager.c`，确保包含所有任务头文件：

```c
#include "app_task_1_line_tracking.h"
#include "app_task_2_dual_point.h"
#include "app_task_3_round_trip.h"
#include "app_task_4_digit_recognition.h"
#include "app_task_5_recognize_transport.h"
#include "app_task_6_auto_transport.h"
#include "app_task_7_batch_transport.h"
#include "app_task_8_creative.h"
```

**注意**：任务表已经在原文件中定义，只需确保函数指针正确指向各任务的函数。

### 第三步：在 Keil 项目中添加文件

1. 打开 Keil µVision
2. 在项目树中找到 `Source Group 1` 或相应的源文件组
3. 右键选择 "Add Files to Group"
4. 选择以下文件：
   - `app_task_1_line_tracking.c`
   - `app_task_2_dual_point.c`
   - `app_task_3_round_trip.c`
   - `app_task_4_digit_recognition.c`
   - `app_task_5_recognize_transport.c`
   - `app_task_6_auto_transport.c`
   - `app_task_7_batch_transport.c`
   - `app_task_8_creative.c`
   - `app_path_planner.c`

### 第四步：编译验证

1. 在 Keil 中执行 "Build" 或按 F7
2. 检查编译输出：
   - 应该没有错误（Error）
   - 可能有警告（Warning），但不影响功能
3. 如果有链接错误，检查：
   - 是否所有文件都被添加到项目中
   - 是否有重复定义的符号
   - 是否有未定义的外部符号

### 第五步：验证集成

编译成功后，验证以下内容：

```c
// 在 main.c 或初始化代码中
TaskManager_Init();           // 初始化任务管理器
TaskManager_StartTask();      // 启动任务
TaskManager_Update();         // 主循环中调用
```

---

## 📝 文件说明

### 任务文件模板说明

每个任务文件都遵循相同的模板结构：

#### 头文件（`.h`）
- 包含 6 个标准接口函数声明
- 包含任务特定的参数设置函数
- 包含详细的功能描述和赛题要求

#### 实现文件（`.c`）
- 定义任务上下文结构体
- 实现 6 个标准接口函数
- 包含详细的 TODO 注释，指导实现逻辑
- 包含内部辅助函数框架

### 路径规划模块说明

#### `app_path_planner.h`
- 定义 `PathPoint_t` 结构：描述单个路径点
- 定义 `Path_t` 结构：管理完整路径
- 声明 4 个核心函数：`PathInit()`, `PathExecute()`, `PathIsComplete()`, `PathReset()`
- 声明 6 个预定义路径表

#### `app_path_planner.c`
- 实现 4 个核心函数
- 定义 6 个预定义路径表（目标区1-5 + 返回停车区）
- 包含详细的 TODO 注释，指导实现逻辑

---

## 🔍 编译常见问题

### 问题 1：undefined reference to `TaskX_Init`

**原因**：任务文件未被添加到 Keil 项目中

**解决方案**：
1. 在 Keil 项目树中右键选择源文件组
2. 选择 "Add Files to Group"
3. 添加所有任务的 `.c` 文件

### 问题 2：multiple definition of `g_task_ctx`

**原因**：全局变量在头文件中定义，被多个文件包含

**解决方案**：
- 全局变量应该在 `.c` 文件中定义，在 `.h` 文件中声明为 `extern`
- 当前模板已经正确处理了这一点

### 问题 3：undefined reference to `PathExecute`

**原因**：路径规划模块文件未被添加到项目中

**解决方案**：
1. 确保 `app_path_planner.c` 被添加到 Keil 项目中
2. 在任务文件中包含 `app_path_planner.h`

### 问题 4：编译警告：implicit declaration of function

**原因**：函数声明缺失或头文件未被包含

**解决方案**：
1. 检查是否包含了必要的头文件
2. 检查函数是否在头文件中声明
3. 检查函数名是否拼写正确

---

## 📚 实现指南

### 实现顺序建议

1. **第一步**：实现路径规划模块
   - 实现 `PathInit()`, `PathExecute()`, `PathIsComplete()`, `PathReset()`
   - 定义预定义路径表

2. **第二步**：实现 Task 1（单点循迹）
   - 最简单的任务，用作参考实现
   - 实现所有 6 个标准接口

3. **第三步**：实现 Task 2-4（基本任务）
   - 基于 Task 1 的实现进行扩展
   - 逐步增加复杂度

4. **第四步**：实现 Task 5-8（发挥部分）
   - 需要额外的硬件支持（抓取装置）
   - 需要更复杂的算法

### 实现检查清单

对于每个任务，实现时需要检查：

- [ ] 所有 6 个标准接口都已实现
- [ ] 任务状态机逻辑正确
- [ ] 超时检查已实现
- [ ] 蜂鸣器提示已添加
- [ ] 底盘控制接口调用正确
- [ ] 编译无错误
- [ ] 编译无警告（或警告可接受）

---

## 🧪 测试建议

### 单元测试

对于每个任务，建议进行以下测试：

1. **初始化测试**
   ```c
   TaskX_Init();
   assert(TaskX_GetState() == TASK_STATE_RUNNING);
   ```

2. **状态转移测试**
   ```c
   TaskX_Init();
   for (int i = 0; i < 100; i++) {
       TaskX_Run();
   }
   assert(TaskX_GetState() == TASK_STATE_SUCCESS);
   ```

3. **超时测试**
   ```c
   TaskX_Init();
   // 模拟超时
   for (int i = 0; i < 20000; i++) {
       TaskX_Run();
   }
   assert(TaskX_GetState() == TASK_STATE_TIMEOUT);
   ```

### 集成测试

1. **任务切换测试**
   - 验证任务管理器能正确切换任务
   - 验证任务状态正确转移

2. **完整流程测试**
   - 从 Task 1 到 Task 8 依次执行
   - 验证每个任务都能正确完成

---

## 📖 参考资源

### 现有代码参考

- [`app_task_manager.h/c`](../src/app/app_task_manager.h) - 任务管理器框架
- [`app_chassis_task.h/c`](../src/app/app_chassis_task.h) - 原子任务示例
- [`drv_chassis.h/c`](../src/drivers/drv_chassis.h) - 底盘控制驱动

### 规划文档参考

- [`plans/SOLUTION_SUMMARY.md`](../plans/SOLUTION_SUMMARY.md) - 方案总结
- [`plans/Task_Implementation_Plan.md`](../plans/Task_Implementation_Plan.md) - 任务实现计划
- [`plans/System_Architecture_Design.md`](../plans/System_Architecture_Design.md) - 系统架构设计
- [`plans/Implementation_Guide.md`](../plans/Implementation_Guide.md) - 实现指南

---

## ✅ 集成完成检查清单

- [ ] 所有 18 个文件都已创建
- [ ] 所有文件都在正确的位置
- [ ] 任务管理器已更新，包含所有任务头文件
- [ ] 所有文件都已添加到 Keil 项目中
- [ ] 项目编译无错误
- [ ] 项目编译无警告（或警告可接受）
- [ ] 任务管理器能正确初始化
- [ ] 任务能正确启动和停止
- [ ] 单个任务能独立运行
- [ ] 任务切换功能正常

---

## 🚀 下一步行动

1. **验证文件创建** - 确认所有 18 个文件都已创建
2. **更新任务管理器** - 在 `app_task_manager.c` 中包含所有任务头文件
3. **添加到 Keil 项目** - 将所有 `.c` 文件添加到项目中
4. **编译验证** - 执行编译，检查是否有错误
5. **实现任务逻辑** - 按照 TODO 注释实现各任务的具体逻辑
6. **测试验证** - 进行单元测试和集成测试

---

**文档完成日期**：2026-05-01  
**版本**：1.0  
**状态**：✅ 已完成

