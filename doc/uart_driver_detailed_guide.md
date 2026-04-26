# UART 驱动详细讲解

## 目录
1. [整体架构](#整体架构)
2. [核心概念](#核心概念)
3. [函数详解](#函数详解)
4. [工作流程](#工作流程)
5. [关键设计](#关键设计)

---

## 整体架构

### 双缓冲 DMA 接收模型

```
┌─────────────────────────────────────────────────────────┐
│                    UART RX FIFO                         │
└────────────────────┬────────────────────────────────────┘
                     │ DMA 自动搬运
                     ▼
        ┌────────────────────────┐
        │  缓冲区 0 (8 字节)      │  ◄─── DMA 写入
        │  [AA 55 xx xx xx xx 0D 0A]
        └────────────────────────┘
                     │
                     │ 帧完成中断
                     ▼
        ┌────────────────────────┐
        │  验证帧头帧尾          │
        │  复制到 frame_data     │
        │  设置 frame_ready      │
        └────────────────────────┘
                     │
                     │ 切换缓冲
                     ▼
        ┌────────────────────────┐
        │  缓冲区 1 (8 字节)      │  ◄─── DMA 写入
        │  [AA 55 yy yy yy yy 0D 0A]
        └────────────────────────┘
```

### 帧格式定义

```
字节位置:  0    1    2    3    4    5    6    7
内容:    [AA] [55] [D0] [D1] [D2] [D3] [0D] [0A]
         └─帧头─┘ └────数据 4 字节────┘ └─帧尾─┘
```

- **帧头**：`0xAA 0x55`（2 字节）
- **数据**：4 字节有效数据
- **帧尾**：`0x0D 0x0A`（2 字节，即 `\r\n`）
- **总长**：8 字节固定

---

## 核心概念

### 1. 双缓冲技术

**为什么需要双缓冲？**

在没有双缓冲的情况下，DMA 接收完一帧后，如果 CPU 还没来得及处理，新数据就会覆盖旧数据。

```
单缓冲问题：
时间 t1: DMA 接收完帧 1 → 中断 → CPU 开始处理
时间 t2: 新数据到达 → DMA 覆盖缓冲 → 帧 1 丢失！
```

**双缓冲解决方案：**

```
时间 t1: DMA 写入缓冲 0 → 中断 → CPU 处理缓冲 0
时间 t2: DMA 写入缓冲 1 → 中断 → CPU 处理缓冲 1
时间 t3: DMA 写入缓冲 0 → 中断 → CPU 处理缓冲 0
         （缓冲 1 的数据已被 CPU 读取，可安全覆盖）
```

**实现方式：**
```c
uint8_t buffer[2][UART_FRAME_SIZE];  // 两个 8 字节缓冲
uint8_t current_buffer;               // 当前 DMA 写入的缓冲索引（0 或 1）
```

### 2. DMA 工作模式

**SINGLE_TRANSFER_MODE（单次传输模式）**

- DMA 完成一次传输后停止
- 需要手动重新启动 DMA 以接收下一帧
- 优点：精确控制，不会自动覆盖数据

**vs REPEAT_SINGLE_TRANSFER_MODE（自动重复模式）**

- DMA 完成传输后自动重启
- 无需手动干预
- 缺点：如果 CPU 处理不及时，新数据会覆盖旧数据

**本设计选择 SINGLE_TRANSFER_MODE 的原因：**
- 固定帧长（8 字节），每帧完成后需要验证
- 需要在中断中切换缓冲区
- 防止帧边界混淆（上一帧尾部 + 下一帧头部）

### 3. 帧验证机制

**为什么需要帧验证？**

UART 是异步通信，可能出现：
- 数据传输错误（噪声干扰）
- 帧边界混淆（接收到的 8 字节不是完整的一帧）
- 同步丢失

**验证方法：**
```c
if (frame[0] == 0xAA &&           // 帧头第一字节
    frame[1] == 0x55 &&           // 帧头第二字节
    frame[6] == 0x0D &&           // 帧尾第一字节
    frame[7] == 0x0A) {           // 帧尾第二字节
    // 帧有效
}
```

**抗扰动设计：**

假设接收到的数据流为：
```
... [上一帧尾] 0D 0A [下一帧头] AA 55 ...
```

如果 DMA 恰好在错误的位置开始接收 8 字节：
```
接收到: [0D 0A AA 55 xx xx xx xx]
验证:   frame[0]=0x0D ≠ 0xAA → 帧无效 ✓
```

通过帧头帧尾验证，可以自动丢弃错误的帧，等待下一个正确的帧头。

---

## 函数详解

### 1. `uart_init()` - 初始化函数

**功能**：配置 DMA 和中断，准备接收第一帧

**逻辑流程**：

```
1. 初始化接收状态变量
   ├─ current_buffer = 0      (从缓冲 0 开始)
   ├─ frame_ready = 0         (无帧就绪)
   └─ frame_error = 0         (无错误)

2. 初始化发送状态变量
   ├─ tx_len = 0
   └─ tx_busy = 0

3. 配置 DMA 源地址
   └─ 指向 UART RX FIFO: UART_CAM_INST->RXDATA

4. 配置 DMA 目标地址
   └─ 指向缓冲 0: &g_uart_rx.buffer[0][0]

5. 配置 DMA 传输大小
   └─ 8 字节（UART_FRAME_SIZE）

6. 启用 DMA 通道
   └─ DL_DMA_enableChannel()

7. 等待 DMA 通道启用完成
   └─ while (DL_DMA_isChannelEnabled() == false)

8. 启用 UART0 中断
   └─ NVIC_EnableIRQ(UART_CAM_INST_INT_IRQN)
```

**关键代码解析**：

```c
// 配置 DMA 源地址（UART RX FIFO）
DL_DMA_setSrcAddr(DMA, DMA_UART0_RX_CHAN_ID, 
                  (uint32_t)(&UART_CAM_INST->RXDATA));
```
- `DMA`：DMA 控制器实例
- `DMA_UART0_RX_CHAN_ID`：UART0 RX 通道 ID（由 SysConfig 定义）
- `UART_CAM_INST->RXDATA`：UART 接收数据寄存器地址

```c
// 等待 DMA 通道启用
while (false == DL_DMA_isChannelEnabled(DMA, DMA_UART0_RX_CHAN_ID)) {
    ;
}
```
- 确保 DMA 真正启动，避免立即发送中断

---

### 2. `uart_isr_handler()` - 中断处理函数

**功能**：处理 DMA 完成中断，验证帧、切换缓冲、重启 DMA

**触发条件**：DMA 完成 8 字节接收

**逻辑流程**：

```
DMA_DONE_RX 中断触发
        │
        ▼
┌─────────────────────────────────────┐
│ 1. 获取当前缓冲的帧指针             │
│    frame = buffer[current_buffer]   │
└─────────────────────────────────────┘
        │
        ▼
┌─────────────────────────────────────┐
│ 2. 验证帧头帧尾                     │
│    if (frame[0]==0xAA &&            │
│        frame[1]==0x55 &&            │
│        frame[6]==0x0D &&            │
│        frame[7]==0x0A)              │
└─────────────────────────────────────┘
        │
    ┌───┴───┐
    │       │
   是       否
    │       │
    ▼       ▼
┌────┐  ┌──────────┐
│有效│  │设置错误  │
│帧  │  │标志      │
└────┘  └──────────┘
    │       │
    └───┬───┘
        │
        ▼
┌─────────────────────────────────────┐
│ 3. 复制有效帧到 frame_data          │
│    memcpy(frame_data, frame, 8)     │
│    frame_ready = 1                  │
└─────────────────────────────────────┘
        │
        ▼
┌─────────────────────────────────────┐
│ 4. 切换缓冲                         │
│    current_buffer = (current_buffer │
│                      + 1) % 2       │
└─────────────────────────────────────┘
        │
        ▼
┌─────────────────────────────────────┐
│ 5. 重新配置 DMA                     │
│    ├─ 设置新的目标地址              │
│    ├─ 设置传输大小                  │
│    └─ 启用 DMA 通道                 │
└─────────────────────────────────────┘
        │
        ▼
    返回中断处理
```

**关键代码解析**：

```c
// 获取当前接收完成的帧
uint8_t *frame = g_uart_rx.buffer[g_uart_rx.current_buffer];
```
- 指向刚刚接收完成的缓冲区

```c
// 验证帧头帧尾
if (frame[0] == FRAME_HEADER_1 && 
    frame[1] == FRAME_HEADER_2 &&
    frame[UART_FRAME_SIZE - 2] == FRAME_TAIL_1 && 
    frame[UART_FRAME_SIZE - 1] == FRAME_TAIL_2)
```
- `frame[UART_FRAME_SIZE - 2]` = `frame[6]`（帧尾第一字节）
- `frame[UART_FRAME_SIZE - 1]` = `frame[7]`（帧尾第二字节）

```c
// 切换到另一个缓冲
g_uart_rx.current_buffer = (g_uart_rx.current_buffer + 1) % 2;
```
- 如果 `current_buffer = 0`，则切换到 1
- 如果 `current_buffer = 1`，则切换到 0

```c
// 重新配置 DMA 接收下一帧
DL_DMA_setDestAddr(DMA, DMA_UART0_RX_CHAN_ID, 
                  (uint32_t)&g_uart_rx.buffer[g_uart_rx.current_buffer][0]);
DL_DMA_setTransferSize(DMA, DMA_UART0_RX_CHAN_ID, UART_FRAME_SIZE);
DL_DMA_enableChannel(DMA, DMA_UART0_RX_CHAN_ID);
```
- 将 DMA 目标地址指向另一个缓冲
- 重新设置传输大小
- 重新启用 DMA 通道（因为 SINGLE_TRANSFER_MODE 需要手动重启）

---

### 3. `uart_get_frame()` - 获取帧数据（不含帧头帧尾）

**功能**：从 `frame_data` 中提取有效数据（去掉帧头帧尾）

**参数**：
- `buffer`：输出缓冲，存储提取的数据
- `len`：输出长度指针

**返回值**：
- `0`：成功
- `-1`：无帧数据

**逻辑流程**：

```
1. 检查 frame_ready 标志
   ├─ 如果为 0（无帧）→ 返回 -1
   └─ 如果为 1（有帧）→ 继续

2. 计算数据长度
   └─ data_len = 8 - 2(帧头) - 2(帧尾) = 4 字节

3. 复制数据部分
   └─ memcpy(buffer, &frame_data[2], 4)
      （跳过前 2 字节帧头）

4. 清除 frame_ready 标志
   └─ frame_ready = 0

5. 返回 0（成功）
```

**使用示例**：

```c
uint8_t data[4];
uint16_t len;

if (uart_get_frame(data, &len) == 0) {
    // 成功获取 4 字节数据
    printf("接收到数据: %02X %02X %02X %02X\n", 
           data[0], data[1], data[2], data[3]);
}
```

---

### 4. `uart_get_full_frame()` - 获取完整帧（含帧头帧尾）

**功能**：获取完整的 8 字节帧

**逻辑流程**：

```
1. 检查 frame_ready 标志
   ├─ 如果为 0 → 返回 -1
   └─ 如果为 1 → 继续

2. 复制完整帧
   └─ memcpy(buffer, frame_data, 8)

3. 设置输出长度
   └─ *len = 8

4. 清除 frame_ready 标志
   └─ frame_ready = 0

5. 返回 0（成功）
```

---

### 5. `uart_send_byte()` - 发送单个字节

**功能**：通过 UART 发送一个字节

**逻辑流程**：

```
1. 等待 TX FIFO 非满
   └─ while (DL_UART_Main_isBusy(UART_CAM_INST))

2. 发送字节
   └─ DL_UART_Main_transmitData(UART_CAM_INST, byte)
```

**为什么要等待？**

UART TX FIFO 有限（通常 4-8 字节），如果 FIFO 满了，新数据会丢失。

---

### 6. `uart_send_data()` - 发送字节数组

**功能**：发送多个字节

**逻辑流程**：

```
for (i = 0; i < len; i++) {
    uart_send_byte(data[i]);  // 逐字节发送
}
```

**特点**：
- 简单的循环调用 `uart_send_byte()`
- 每个字节都会等待 TX FIFO 非满

---

### 7. `uart_send_frame()` - 发送数据帧（自动添加帧头帧尾）

**功能**：构建完整帧并发送

**参数**：
- `data`：数据指针
- `len`：数据长度

**返回值**：
- `0`：成功
- `-1`：发送忙
- `-2`：数据过长

**逻辑流程**：

```
1. 检查发送状态
   ├─ 如果 tx_busy = 1 → 返回 -1（发送忙）
   └─ 如果 tx_busy = 0 → 继续

2. 检查数据长度
   ├─ 如果 len + 4 > 128 → 返回 -2（数据过长）
   └─ 否则 → 继续

3. 构建帧
   ├─ tx_buffer[0] = 0xAA      (帧头)
   ├─ tx_buffer[1] = 0x55      (帧头)
   ├─ tx_buffer[2..len+1] = data  (数据)
   ├─ tx_buffer[len+2] = 0x0D  (帧尾)
   └─ tx_buffer[len+3] = 0x0A  (帧尾)

4. 设置发送状态
   ├─ tx_len = len + 4
   └─ tx_busy = 1

5. 发送所有字节
   └─ for (i = 0; i < total_len; i++)
        uart_send_byte(tx_buffer[i])

6. 清除发送忙标志
   └─ tx_busy = 0

7. 返回 0（成功）
```

**使用示例**：

```c
uint8_t data[4] = {0x11, 0x22, 0x33, 0x44};

if (uart_send_frame(data, 4) == 0) {
    // 发送成功
    // 实际发送的帧: AA 55 11 22 33 44 0D 0A
}
```

---

## 工作流程

### 接收流程

```
系统启动
    │
    ▼
uart_init()
    ├─ 初始化状态变量
    ├─ 配置 DMA 源地址 (UART RX FIFO)
    ├─ 配置 DMA 目标地址 (buffer[0])
    ├─ 配置 DMA 传输大小 (8 字节)
    ├─ 启用 DMA 通道
    └─ 启用 UART 中断
    │
    ▼
等待数据到达
    │
    ▼
UART RX FIFO 接收 8 字节
    │
    ▼
DMA 自动搬运 8 字节到 buffer[0]
    │
    ▼
DMA_DONE_RX 中断触发
    │
    ▼
uart_isr_handler()
    ├─ 验证帧头帧尾
    ├─ 复制到 frame_data
    ├─ 设置 frame_ready = 1
    ├─ 切换 current_buffer = 1
    ├─ 重新配置 DMA 目标地址 (buffer[1])
    └─ 重新启用 DMA
    │
    ▼
主程序检查 frame_ready
    │
    ▼
uart_get_frame() 或 uart_get_full_frame()
    ├─ 提取数据
    └─ 清除 frame_ready
    │
    ▼
处理数据
    │
    ▼
等待下一帧...
```

### 发送流程

```
应用程序调用 uart_send_frame(data, len)
    │
    ▼
检查 tx_busy 和数据长度
    │
    ▼
构建帧: [AA 55] [data] [0D 0A]
    │
    ▼
设置 tx_busy = 1
    │
    ▼
逐字节发送
    ├─ 等待 TX FIFO 非满
    └─ 发送字节
    │
    ▼
所有字节发送完成
    │
    ▼
设置 tx_busy = 0
    │
    ▼
返回 0（成功）
```

---

## 关键设计

### 1. 为什么使用 DMA？

**优点**：
- 自动搬运数据，CPU 无需干预
- 不占用 CPU 时间
- 适合高速通信

**缺点**：
- 需要配置 DMA 通道
- 中断处理更复杂

### 2. 为什么固定 8 字节？

- 帧头 2 字节 + 数据 4 字节 + 帧尾 2 字节 = 8 字节
- 固定长度便于 DMA 配置
- 便于帧验证

### 3. 为什么需要帧验证？

- UART 是异步通信，可能出现同步错误
- 帧头帧尾验证可以自动丢弃错误帧
- 提高通信可靠性

### 4. 为什么使用 SINGLE_TRANSFER_MODE？

- 需要在每帧完成后进行验证
- 需要切换缓冲区
- 防止自动覆盖未处理的数据

### 5. 发送为什么不用 DMA？

- 发送数据量小（通常 < 128 字节）
- 发送频率低
- 直接轮询发送更简单

---

## 总结

| 功能 | 实现方式 | 特点 |
|------|--------|------|
| 接收 | DMA + 双缓冲 | 自动、高效、可靠 |
| 帧验证 | 帧头帧尾检查 | 简单、有效 |
| 发送 | 轮询 + 逐字节 | 简单、可控 |
| 缓冲管理 | 双缓冲切换 | 防止数据丢失 |

这个设计在保证可靠性的同时，代码简洁易懂，适合嵌入式系统使用。
