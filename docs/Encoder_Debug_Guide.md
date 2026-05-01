# 编码器模块调试指南 - 寄存器观察方法

## 问题现象
Motor_Proc 执行频率过快，不是预期的 1 秒周期。

## 🔍 调试步骤

### 第一步：验证 SysTick 计时器工作状态

#### 1.1 检查 SysTick 寄存器
在 Keil 调试器中，打开 **View → Peripherals → Core Peripherals → SysTick**

观察以下寄存器：
```
SysTick_CTRL (0xE000E010)
  - Bit 0 (ENABLE): 应该为 1（SysTick 已启用）
  - Bit 1 (TICKINT): 应该为 1（SysTick 中断已启用）
  - Bit 2 (CLKSOURCE): 应该为 1（使用处理器时钟）

SysTick_LOAD (0xE000E014)
  - 应该为 79999（80MHz / 1000 - 1 = 79999，产生 1ms 中断）

SysTick_VAL (0xE000E018)
  - 当前计数值，应该在 0-79999 之间循环
  - 每次读取时值会变化（除非在断点处停止）
```

#### 1.2 检查 SysTick 中断是否在执行
在 `SysTick_Handler()` 中添加断点，观察：
- 中断是否被触发
- 触发频率是否为 1ms 一次
- `uwTick` 是否每次递增 1

### 第二步：验证 uwTick 的增长速度

#### 2.1 在调试器中监控 uwTick
在 Keil 调试器中：
1. 打开 **View → Watch Windows → Watch 1**
2. 添加变量：`uwTick`
3. 运行程序，观察 `uwTick` 的增长速度

**预期行为**：
- 每秒增加约 1000
- 如果 1 秒内增加 10000+，说明 SysTick 中断频率过高

#### 2.2 计算实际时间间隔
在 Motor_Proc 中添加调试代码：
```c
void Motor_Proc(void)
{
    static uint32_t last_uwTick = 0;
    static uint32_t call_count = 0;
    
    if ((uwTick - uwTick_Motor_Set_Point) < 1000) {
        return;
    }
    
    // 记录本次调用时的 uwTick
    uint32_t current_uwTick = uwTick;
    uint32_t time_delta = current_uwTick - last_uwTick;
    
    // 通过串口输出调试信息
    sprintf(g_print_buffer, "Motor_Proc called: count=%d, time_delta=%d ms\r\n", 
            call_count++, time_delta);
    uart_send_data((uint8_t *)g_print_buffer, strlen(g_print_buffer));
    
    last_uwTick = current_uwTick;
    uwTick_Motor_Set_Point = uwTick;
    
    // ... 原有代码 ...
}
```

### 第三步：检查 CLOCK 定时器是否干扰 SysTick

#### 3.1 查看 CLOCK 定时器配置
在 Keil 调试器中，打开 **View → Peripherals → TIMA0**

观察：
```
TIMA0_LOAD (周期寄存器)
  - 应该为 999（10ms 周期）

TIMA0_CTRL (控制寄存器)
  - Bit 0 (ENABLE): 应该为 1（定时器已启用）
  - Bit 1 (INTERRUPT): 应该为 1（中断已启用）

TIMA0_CNT (计数值)
  - 应该在 0-999 之间循环
```

#### 3.2 检查 CLOCK 中断频率
在 `TIMA0_IRQHandler()` 中添加调试代码：
```c
static uint32_t clock_isr_count = 0;

void TIMA0_IRQHandler(void)
{
    clock_isr_count++;
    
    switch (DL_TimerA_getPendingInterrupt(CLOCK_INST)) {
        case DL_TIMERA_IIDX_ZERO: {
            EncoderCalculateSpeed();
            break;
        }
        default:
            break;
    }
}
```

在调试器中监控 `clock_isr_count`，预期每秒增加 100 次（10ms 周期）。

### 第四步：检查 E1A/E2A 中断频率

#### 4.1 监控编码器中断次数
在 `isr_encoder.c` 中添加计数器：
```c
static uint32_t e1a_isr_count = 0;
static uint32_t e2a_isr_count = 0;

void TIMG0_IRQHandler(void)
{
    e1a_isr_count++;
    // ... 原有代码 ...
}

void TIMA1_IRQHandler(void)
{
    e2a_isr_count++;
    // ... 原有代码 ...
}
```

在调试器中监控这两个计数器，观察中断频率是否异常。

### 第五步：检查时间判断逻辑

#### 5.1 验证时间差计算
在 Motor_Proc 中添加详细调试：
```c
void Motor_Proc(void)
{
    uint32_t time_diff = uwTick - uwTick_Motor_Set_Point;
    
    // 通过串口输出时间差
    if (time_diff % 100 == 0) {  // 每 100ms 输出一次
        sprintf(g_print_buffer, "time_diff=%d, threshold=1000\r\n", time_diff);
        uart_send_data((uint8_t *)g_print_buffer, strlen(g_print_buffer));
    }
    
    if (time_diff < 1000) {
        return;
    }
    
    uwTick_Motor_Set_Point = uwTick;
    
    // ... 原有代码 ...
}
```

**预期行为**：
- `time_diff` 应该从 0 逐渐增加到 1000
- 当达到 1000 时，Motor_Proc 执行一次，然后 `time_diff` 重置为 0

### 第六步：检查中断优先级冲突

#### 6.1 查看中断优先级设置
在 Keil 调试器中，打开 **View → Peripherals → NVIC**

观察各中断的优先级：
```
E1A_INST_INT_IRQN (TIMG0)
E2A_INST_INT_IRQN (TIMA1)
CLOCK_INST_INT_IRQN (TIMA0)
SysTick_IRQn
```

**检查项**：
- 是否有中断优先级设置为 0（最高优先级）
- 是否有中断被禁用（ENABLE = 0）
- 是否有中断优先级倒序（应该是递增的）

### 第七步：使用逻辑分析仪观察 GPIO 输出

#### 7.1 在 Motor_Proc 中切换 GPIO
```c
void Motor_Proc(void)
{
    if ((uwTick - uwTick_Motor_Set_Point) < 1000) {
        return;
    }
    uwTick_Motor_Set_Point = uwTick;
    
    // 切换一个 GPIO 引脚（用于示波器/逻辑分析仪观察）
    DL_GPIO_togglePins(GPIOA, DL_GPIO_PIN_27);  // 切换 PA27（蜂鸣器引脚）
    
    // ... 原有代码 ...
}
```

然后用示波器或逻辑分析仪观察该引脚的频率，应该是 0.5Hz（每 2 秒切换一次）。

## 📊 调试检查清单

| 检查项 | 预期值 | 实际值 | 状态 |
|--------|--------|--------|------|
| SysTick_CTRL.ENABLE | 1 | | |
| SysTick_LOAD | 79999 | | |
| uwTick 增长速度 | 1000/s | | |
| CLOCK 中断频率 | 100/s | | |
| Motor_Proc 执行间隔 | 1000ms | | |
| time_diff 最大值 | ~1000 | | |

## 🔧 常见问题排查

### 问题 1：uwTick 增长过快
**可能原因**：
- SysTick_LOAD 值过小（应该是 79999）
- SysTick 中断被多次触发
- 时钟源配置错误

**排查方法**：
1. 检查 SysTick_LOAD 寄存器值
2. 在 SysTick_Handler 中添加断点，观察触发频率
3. 检查系统时钟频率是否为 80MHz

### 问题 2：Motor_Proc 执行频率过快
**可能原因**：
- `uwTick_Motor_Set_Point` 被意外修改
- 时间差计算溢出（uwTick 是 32 位无符号整数）
- 中断优先级导致 SysTick 被延迟

**排查方法**：
1. 在 Motor_Proc 中添加调试输出，观察 `time_diff` 值
2. 检查是否有其他代码修改 `uwTick_Motor_Set_Point`
3. 检查中断优先级设置

### 问题 3：CLOCK 中断频率异常
**可能原因**：
- CLOCK 定时器周期配置错误
- CLOCK 定时器被多次启动
- 中断处理函数中有死循环

**排查方法**：
1. 检查 TIMA0_LOAD 寄存器值（应该是 999）
2. 检查 EncoderStart() 是否被多次调用
3. 在 TIMA0_IRQHandler 中添加超时检测

## 💡 调试技巧

### 技巧 1：使用条件断点
在 Motor_Proc 中设置条件断点：
```
条件：(uwTick - uwTick_Motor_Set_Point) >= 1000
```
这样只有在满足执行条件时才会停止。

### 技巧 2：使用数据观察窗口
在 Keil 中打开 **View → Watch Windows**，添加以下变量：
- `uwTick`
- `uwTick_Motor_Set_Point`
- `uwTick - uwTick_Motor_Set_Point`
- `clock_isr_count`
- `e1a_isr_count`
- `e2a_isr_count`

### 技巧 3：使用 ITM 调试输出
如果支持 SWO（Serial Wire Output），可以使用 ITM 实时输出调试信息，而不需要 UART。

### 技巧 4：使用逻辑分析仪
连接逻辑分析仪到以下引脚：
- PA27（蜂鸣器，用于标记 Motor_Proc 执行）
- PA12（E1A 捕获）
- PA15（E2A 捕获）
- PB20（E1B GPIO）
- PB25（E2B GPIO）

观察这些信号的时序关系。

## 📝 调试记录模板

```
调试日期：____年____月____日
问题描述：Motor_Proc 执行频率过快

检查项目：
1. SysTick_LOAD = _______ (预期：79999)
2. uwTick 增长速度 = _______ /s (预期：1000)
3. CLOCK 中断频率 = _______ /s (预期：100)
4. Motor_Proc 执行间隔 = _______ ms (预期：1000)
5. time_diff 最大值 = _______ (预期：~1000)

根本原因：_________________________________

解决方案：_________________________________

验证结果：_________________________________
```

## 🎯 快速诊断流程

1. **第一步**：在调试器中监控 `uwTick`，观察增长速度
   - 如果 1 秒内增加 1000，说明 SysTick 正常
   - 如果增加过快或过慢，检查 SysTick_LOAD 和系统时钟

2. **第二步**：在 Motor_Proc 中添加调试输出，观察 `time_diff`
   - 如果 `time_diff` 总是小于 1000，说明时间判断逻辑有问题
   - 如果 `time_diff` 达到 1000 后立即重置，说明逻辑正常

3. **第三步**：检查是否有其他代码修改 `uwTick_Motor_Set_Point`
   - 搜索代码中所有对 `uwTick_Motor_Set_Point` 的赋值
   - 确保只在 Motor_Proc 中修改

4. **第四步**：检查中断优先级和启用状态
   - 在 NVIC 中查看各中断的优先级
   - 确保没有中断被意外禁用

