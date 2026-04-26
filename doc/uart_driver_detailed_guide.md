# UART 驱动详细讲解（环形缓冲 + 流式扫描版本）

## 目录
1. [整体架构](#整体架构)
2. [核心概念](#核心概念)
3. [函数详解](#函数详解)
4. [工作流程](#工作流程)
5. [关键设计](#关键设计)

---

## 整体架构

### 环形缓冲 + 流式扫描模型

```
┌─────────────────────────────────────────────────────────┐
│                    UART RX FIFO                         │
└────────────────────┬────────────────────────────────────┘
                     │ DMA 自动搬运 8 字节
                     ▼
        ┌────────────────────────────────────────┐
        │  环形缓冲（128 字节）                   │
        │  [0][1][2]...[126][127]                │
        │  ▲                                     │
        │  └─ write_pos（DMA 写入位置）          │
        │                                        │
        │  每 8 字节触发一次中断                  │
        └────────────────────────────────────────┘
                     │
                     ▼
        ┌────────────────────────────────────────┐
        │  流式扫描（在最近 16 字节中搜索）      │
        │  搜索范围：[write_pos, write_pos+8]    │
        │                                        │
        │  for (offset = 0; offset <= 8; offset++)
        │    if (buf[write_pos+offset] == 0xAA &&
        │        buf[write_pos+offset+1] == 0x55 &&
        │        buf[write_pos+offset+6] == 0x0D &&
        │        buf[write_pos+offset+7] == 0x0A)
        │      → 找到有效帧                      │
        └────────────────────────────────────────┘
                     │
                     ▼
        ┌────────────────────────────────────────┐
        │  复制到 frame_data                     │
        │  设置 frame_ready = 1                  │
        │  更新统计计数                          │
        └────────────────────────────────────────┘
                     │
                     ▼
        ┌────────────────────────────────────────┐
        │  更新 write_pos（环形）                │
        │  write_pos = (write_pos + 8) % 128    │
        └────────────────────────────────────────┘
                     │
                     ▼
        ┌────────────────────────────────────────┐
        │  重新配置 DMA                          │
        │  设置新的目标地址                      │
        │  重新启用 DMA 通道                     │
        └────────────────────────────────────────┘
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

### 1. 环形缓冲（Circular Buffer）

**为什么需要环形缓冲？**

固定缓冲的问题：
```
固定缓冲（8 字节）：
时间 t1: DMA 写入 buffer[0..7]
时间 t2: DMA 写入 buffer[0..7]（覆盖旧数据）
         如果 CPU 还没处理，数据丢失！
```

环形缓冲的优势：
```
环形缓冲（128 字节）：
时间 t1: DMA 写入 buffer[0..7]
时间 t2: DMA 写入 buffer[8..15]
时间 t3: DMA 写入 buffer[16..23]
...
时间 t17: DMA 写入 buffer[120..127]
时间 t18: DMA 写入 buffer[0..7]（此时 buffer[0..7] 已被 CPU 处理）
```

**实现方式**：
```c
uint8_t buffer[128];           // 环形缓冲
uint16_t write_pos;            // 写入位置（0-127）

// 环形索引计算
uint16_t next_pos = (write_pos + 8) % 128;
```

**优点**：
- 无需复制数据，直接在缓冲中处理
- 内存利用率高
- 自动覆盖已处理的旧数据

### 2. 流式扫描（Stream Scanning）

**为什么需要流式扫描？**

固定帧长的问题：
```
假设接收到的数据流：
... [上一帧尾] 0D 0A [下一帧头] AA 55 ...

如果 DMA 恰好在错误位置开始接收 8 字节：
接收到: [0D 0A AA 55 xx xx xx xx]
验证:   frame[0]=0x0D ≠ 0xAA → 帧无效
        后续所有帧都错位，永久失效！
```

流式扫描的解决方案：
```
接收到: [0D 0A AA 55 xx xx xx xx]
扫描:   offset=0: buf[0]=0x0D ≠ 0xAA
        offset=1: buf[1]=0x0A ≠ 0xAA
        offset=2: buf[2]=0xAA ✓, buf[3]=0x55 ✓
                  buf[8]=0x0D ✓, buf[9]=0x0A ✓
        → 找到有效帧！自动恢复同步
```

**实现方式**：
```c
// 在最近 16 字节范围内搜索帧头
for (int offset = 0; offset <= 8; offset++) {
    uint16_t frame_start = (write_pos + offset) % 128;
    
    // 验证帧头帧尾（使用环形索引）
    if (buffer[frame_start] == 0xAA &&
        buffer[(frame_start+1)%128] == 0x55 &&
        buffer[(frame_start+6)%128] == 0x0D &&
        buffer[(frame_start+7)%128] == 0x0A) {
        // 找到有效帧
    }
}
```

**优点**：
- 自动同步恢复
- 即使初始错位也能恢复
- 鲁棒性强

### 3. 环形索引计算

**为什么需要环形索引？**

在环形缓冲中，索引会超出数组范围，需要模运算回绕。

**示例**：
```
缓冲大小：128 字节
write_pos = 125

下一个 8 字节的位置：
next_pos = (125 + 8) % 128 = 133 % 128 = 5

所以 DMA 会写入 buffer[125..127] 和 buffer[0..4]
```

**帧跨越缓冲边界的情况**：
```
write_pos = 124
帧数据位置：buffer[124..131]

环形索引：
idx0 = 124
idx1 = (124+1) % 128 = 125
idx2 = (124+2) % 128 = 126
idx3 = (124+3) % 128 = 127
idx4 = (124+4) % 128 = 0      ← 回绕
idx5 = (124+5) % 128 = 1
idx6 = (124+6) % 128 = 2
idx7 = (124+7) % 128 = 3
```

---

## 函数详解

### 1. `uart_init()` - 初始化函数

**功能**：配置 DMA 和中断，准备接收第一帧

**逻辑流程**：

```
1. 初始化接收状态变量
   ├─ write_pos = 0           (从位置 0 开始)
   ├─ frame_ready = 0         (无帧就绪)
   ├─ frame_error = 0         (无错误)
   ├─ frame_count = 0         (帧计数)
   └─ error_count = 0         (错误计数)

2. 初始化发送状态变量
   ├─ tx_len = 0
   └─ tx_busy = 0

3. 配置 DMA 源地址
   └─ 指向 UART RX FIFO: UART_CAM_INST->RXDATA

4. 配置 DMA 目标地址
   └─ 指向环形缓冲起始: &buffer[0]

5. 配置 DMA 传输大小
   └─ 8 字节（UART_DMA_TRANSFER_SIZE）

6. 启用 DMA 通道
   └─ DL_DMA_enableChannel()

7. 等待 DMA 通道启用完成
   └─ while (DL_DMA_isChannelEnabled() == false)

8. 启用 UART0 中断
   └─ NVIC_EnableIRQ(UART_CAM_INST_INT_IRQN)
```

**关键代码**：

```c
// 配置 DMA 目标地址（环形缓冲起始位置）
DL_DMA_setDestAddr(DMA, DMA_UART0_RX_CHAN_ID, 
                   (uint32_t)&g_uart_rx.buffer[0]);

// 配置 DMA 传输大小（每次 8 字节）
DL_DMA_setTransferSize(DMA, DMA_UART0_RX_CHAN_ID, UART_DMA_TRANSFER_SIZE);
```

---

### 2. `uart_isr_handler()` - 中断处理函数

**功能**：处理 DMA 完成中断，流式扫描帧、验证、切换缓冲、重启 DMA

**触发条件**：DMA 完成 8 字节接收

**逻辑流程**：

```
DMA_DONE_RX 中断触发
        │
        ▼
┌─────────────────────────────────────┐
│ 1. 获取当前 DMA 写入位置             │
│    current_write_pos = write_pos    │
└─────────────────────────────────────┘
        │
        ▼
┌─────────────────────────────────────┐
│ 2. 流式扫描（offset = 0 to 8）      │
│    计算帧头可能的起始位置            │
│    frame_start = (write_pos + offset) % 128
└─────────────────────────────────────┘
        │
        ▼
┌─────────────────────────────────────┐
│ 3. 计算环形索引                     │
│    idx0 = frame_start               │
│    idx1 = (frame_start+1) % 128     │
│    idx6 = (frame_start+6) % 128     │
│    idx7 = (frame_start+7) % 128     │
└─────────────────────────────────────┘
        │
        ▼
┌─────────────────────────────────────┐
│ 4. 验证帧头帧尾                     │
│    if (buf[idx0]==0xAA &&           │
│        buf[idx1]==0x55 &&           │
│        buf[idx6]==0x0D &&           │
│        buf[idx7]==0x0A)             │
└─────────────────────────────────────┘
        │
    ┌───┴───┐
    │       │
   是       否
    │       │
    ▼       ▼
┌────┐  ┌──────────┐
│有效│  │继续扫描  │
│帧  │  │下一个    │
└────┘  │offset    │
    │   └──────────┘
    │       │
    └───┬───┘
        │
        ▼
┌─────────────────────────────────────┐
│ 5. 复制有效帧到 frame_data          │
│    for (i = 0; i < 8; i++)          │
│      frame_data[i] = buf[(start+i)%128]
│    frame_ready = 1                  │
│    frame_count++                    │
└─────────────────────────────────────┘
        │
        ▼
┌─────────────────────────────────────┐
│ 6. 更新写入位置（环形）             │
│    write_pos = (write_pos + 8) % 128│
└─────────────────────────────────────┘
        │
        ▼
┌─────────────────────────────────────┐
│ 7. 重新配置 DMA                     │
│    ├─ 设置新的目标地址              │
│    ├─ 设置传输大小                  │
│    └─ 启用 DMA 通道                 │
└─────────────────────────────────────┘
        │
        ▼
    返回中断处理
```

**关键代码**：

```c
// 流式扫描：在最近 16 字节范围内搜索帧头
for (int offset = 0; offset <= 8; offset++) {
    // 计算帧头可能的起始位置（环形索引）
    uint16_t frame_start = (current_write_pos + offset) % UART_RX_BUFFER_SIZE;
    
    // 计算帧内各字节的环形索引
    uint16_t idx0 = frame_start;
    uint16_t idx1 = (frame_start + 1) % UART_RX_BUFFER_SIZE;
    uint16_t idx6 = (frame_start + 6) % UART_RX_BUFFER_SIZE;
    uint16_t idx7 = (frame_start + 7) % UART_RX_BUFFER_SIZE;
    
    // 验证帧头帧尾
    if (g_uart_rx.buffer[idx0] == FRAME_HEADER_1 && 
        g_uart_rx.buffer[idx1] == FRAME_HEADER_2 &&
        g_uart_rx.buffer[idx6] == FRAME_TAIL_1 && 
        g_uart_rx.buffer[idx7] == FRAME_TAIL_2) {
        
        // 找到有效帧，复制到 frame_data
        for (int i = 0; i < UART_FRAME_SIZE; i++) {
            uint16_t idx = (frame_start + i) % UART_RX_BUFFER_SIZE;
            g_uart_rx.frame_data[i] = g_uart_rx.buffer[idx];
        }
        
        g_uart_rx.frame_ready = 1;
        g_uart_rx.frame_count++;
        frame_found = 1;
        break;
    }
}
```

**流式扫描的优势**：
- 搜索范围只有 16 字节（9 个可能的帧头位置）
- 计算量小，中断延迟低
- 自动处理帧边界跨越 DMA 传输边界的情况

---

### 3. `uart_get_frame()` - 获取帧数据（不含帧头帧尾）

**功能**：从 `frame_data` 中提取有效数据（去掉帧头帧尾）

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

---

### 6. `uart_send_data()` - 发送字节数组

**功能**：发送多个字节

**逻辑流程**：

```
for (i = 0; i < len; i++) {
    uart_send_byte(data[i]);  // 逐字节发送
}
```

---

### 7. `uart_send_frame()` - 发送数据帧（自动添加帧头帧尾）

**功能**：构建完整帧并发送

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

---

### 8. `uart_get_stats()` - 获取统计信息

**功能**：获取成功接收帧数和错误帧数

**逻辑流程**：

```
1. 如果 frame_count 指针非空
   └─ *frame_count = g_uart_rx.frame_count

2. 如果 error_count 指针非空
   └─ *error_count = g_uart_rx.error_count
```

**使用示例**：

```c
uint32_t success, errors;
uart_get_stats(&success, &errors);
printf("成功帧: %u, 错误帧: %u\n", success, errors);
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
DMA 自动搬运 8 字节到 buffer[write_pos..write_pos+7]
    │
    ▼
DMA_DONE_RX 中断触发
    │
    ▼
uart_isr_handler()
    ├─ 流式扫描（在 16 字节范围内搜索帧头）
    ├─ 验证帧头帧尾
    ├─ 复制到 frame_data
    ├─ 设置 frame_ready = 1
    ├─ 更新 write_pos（环形）
    ├─ 重新配置 DMA 目标地址
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

### 帧同步恢复示例

```
实际数据流（初始错位 1 字节）：
... [0D 0A] [AA 55 D0 D1 D2 D3 0D 0A] [AA 55 E0 E1 E2 E3 0D 0A] ...

DMA 接收 1（错位）：
buffer[0..7] = [0A AA 55 D0 D1 D2 D3 0D]
扫描：offset=0: buf[0]=0x0A ≠ 0xAA
      offset=1: buf[1]=0xAA ✓, buf[2]=0x55 ✓
                buf[7]=0x0D ✓, buf[8]=0x0A ✓
      → 找到有效帧！自动恢复同步

DMA 接收 2（已同步）：
buffer[8..15] = [0A AA 55 E0 E1 E2 E3 0D]
扫描：offset=0: buf[8]=0x0A ≠ 0xAA
      offset=1: buf[9]=0xAA ✓, buf[10]=0x55 ✓
                buf[15]=0x0D ✓, buf[16]=0x0A ✓
      → 继续正常接收
```

---

## 关键设计

### 1. 为什么使用 128 字节环形缓冲？

- **缓冲大小**：128 字节 = 16 个 DMA 传输（8 字节/次）
- **优势**：
  - 足够大，不会因为 CPU 处理延迟而丢失数据
  - 足够小，内存占用合理（MSPM0G3507 有 8KB SRAM）
  - 2 的幂次方，模运算高效

### 2. 为什么 DMA 每次传输 8 字节？

- **固定帧长**：8 字节（帧头 2 + 数据 4 + 帧尾 2）
- **优势**：
  - 每个 DMA 传输对应一个完整帧
  - 中断频率合理（不会过于频繁）
  - 便于流式扫描

### 3. 为什么流式扫描范围是 16 字节？

- **搜索范围**：`[write_pos, write_pos+8]`（9 个可能的帧头位置）
- **原理**：
  - 最坏情况：帧头跨越两个 DMA 传输的边界
  - 需要检查前一个 DMA 传输的最后 8 字节 + 当前 DMA 传输的 8 字节
  - 总共 16 字节

### 4. 为什么需要环形索引？

- **缓冲回绕**：当 `write_pos + offset` 超过 127 时，需要回绕到 0
- **环形索引**：`(write_pos + offset) % 128`
- **优势**：自动处理缓冲边界，无需特殊判断

### 5. 发送为什么不用 DMA？

- **发送数据量小**：通常 < 128 字节
- **发送频率低**：不是实时性关键路径
- **直接轮询更简单**：代码复杂度低

---

## 总结

| 功能 | 实现方式 | 特点 |
|------|--------|------|
| 接收 | DMA + 环形缓冲 | 自动、高效、无丢失 |
| 流式扫描 | 16 字节范围搜索 | 自动同步、鲁棒 |
| 帧验证 | 帧头帧尾检查 | 简单、有效 |
| 发送 | 轮询 + 逐字节 | 简单、可控 |
| 统计 | 计数器 | 便于调试 |

这个设计在保证可靠性和实时性的同时，代码简洁易懂，适合嵌入式系统使用。
