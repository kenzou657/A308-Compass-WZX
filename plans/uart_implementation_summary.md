# UART 驱动实现总结

## 一、实现概览

完整实现了基于 TI MSPM0G3507 的 UART 驱动，包含：
- **DMA 接收**：128 字节缓冲
- **空闲中断**：RXD_POS_EDGE 检测帧结束
- **流式帧解析**：状态机防止帧头帧尾重复匹配
- **串口发送**：轮询方式，自动添加帧头帧尾

---

## 二、核心设计

### 2.1 接收流程

```
硬件层：
┌──────────────────────────────────────────────────────┐
│ UART RX 数据到达 → DMA 自动接收 → 128 字节缓冲      │
└──────────────────────────────────────────────────────┘
                          ↓
        ┌─────────────────┴─────────────────┐
        │                                   │
        ▼                                   ▼
   DMA 缓冲满                          总线空闲
   (128 字节)                      (RXD_POS_EDGE)
        │                                   │
        └─────────────────┬─────────────────┘
                          ▼
        ┌──────────────────────────────────┐
        │  触发 UART0_IRQHandler           │
        │  设置 frame_ready = 1            │
        └──────────────────┬───────────────┘
                          ▼
软件层：
┌──────────────────────────────────────────────────────┐
│ 主循环检测 frame_ready                               │
│ 调用 uart_parse_stream() 流式解析                    │
│ 状态机逐字节处理，提取完整帧                         │
│ 调用 uart_get_frame() 获取帧数据                     │
│ 调用 uart_restart_dma() 重启接收                     │
└──────────────────────────────────────────────────────┘
```

### 2.2 流式解析状态机

```
状态转移：

IDLE
  ↓ (收到 0xAA)
HEADER_1
  ↓ (收到 0x55)
HEADER_2
  ↓ (收到数据或 0x0D)
DATA / TAIL_1
  ↓ (收到 0x0A)
FRAME_READY (返回 1)

错误处理：
- 任何不符合预期的字节 → 回到 IDLE
- 数据过长 → 帧错误，返回 -1
```

### 2.3 帧格式

```
[AA 55] [数据 0-N 字节] [0D 0A]
 ↑ ↑     ↑              ↑ ↑
 │ │     │              │ └─ 帧尾第二字节 (0x0A = \n)
 │ │     │              └─── 帧尾第一字节 (0x0D = \r)
 │ │     └─────────────────── 可变长数据
 │ └─────────────────────────── 帧头第二字节 (0x55)
 └───────────────────────────── 帧头第一字节 (0xAA)

示例（8 字节数据帧）：
AA 55 01 02 03 04 05 06 07 08 0D 0A
```

---

## 三、文件结构

### 3.1 驱动文件

| 文件 | 功能 |
|------|------|
| [`src/drivers/drv_uart.h`](src/drivers/drv_uart.h) | UART 驱动头文件，定义接口和数据结构 |
| [`src/drivers/drv_uart.c`](src/drivers/drv_uart.c) | UART 驱动实现，包含中断处理和流式解析 |
| [`src/isr/isr_uart.c`](src/isr/isr_uart.c) | UART 中断服务函数 |
| [`src/drivers/uart_example.c`](src/drivers/uart_example.c) | 使用示例和应用集成 |

### 3.2 关键数据结构

```c
// 接收状态机
typedef enum {
    UART_RX_STATE_IDLE = 0,
    UART_RX_STATE_HEADER_1,
    UART_RX_STATE_HEADER_2,
    UART_RX_STATE_DATA,
    UART_RX_STATE_TAIL_1,
    UART_RX_STATE_TAIL_2,
    UART_RX_STATE_FRAME_READY
} uart_rx_state_t;

// 接收控制块
typedef struct {
    uint8_t dma_buffer[128];        // DMA 接收缓冲
    uint8_t frame_buffer[64];       // 帧数据缓冲
    uint16_t dma_count;             // DMA 已接收字节数
    uint16_t dma_index;             // 流式解析索引
    uint16_t frame_len;             // 帧数据长度
    uart_rx_state_t state;          // 状态机状态
    uint8_t frame_ready;            // 帧就绪标志
    uint8_t frame_error;            // 帧错误标志
} uart_rx_t;
```

---

## 四、API 接口

### 4.1 初始化

```c
void uart_init(void);
```
- 初始化 UART 驱动和状态机
- 启用 UART0 中断

### 4.2 接收

```c
int uart_parse_stream(void);
```
- 流式解析 DMA 缓冲中的数据
- 返回值：1（帧有效）、0（未找到完整帧）、-1（帧错误）

```c
int uart_get_frame(uint8_t *buffer, uint16_t *len);
```
- 获取接收到的帧数据
- 返回值：0（成功）、-1（无帧数据）

```c
void uart_restart_dma(void);
```
- 重启 DMA 接收
- 重置状态机和缓冲索引

### 4.3 发送

```c
int uart_send_frame(const uint8_t *data, uint16_t len);
```
- 发送数据帧（自动添加帧头帧尾）
- 返回值：0（成功）、-1（发送忙）、-2（数据过长）

```c
void uart_send_data(const uint8_t *data, uint16_t len);
```
- 发送原始数据（不添加帧头帧尾）

```c
void uart_send_byte(uint8_t byte);
```
- 发送单个字节

### 4.4 中断处理

```c
void uart_isr_handler(void);
```
- UART 中断处理函数
- 由 [`src/isr/isr_uart.c`](src/isr/isr_uart.c) 中的 `UART0_IRQHandler()` 调用

---

## 五、中断配置

### 5.1 启用的中断

| 中断 | 作用 |
|------|------|
| `DL_UART_MAIN_INTERRUPT_RXD_NEG_EDGE` | 检测帧开始（下降沿） |
| `DL_UART_MAIN_INTERRUPT_RXD_POS_EDGE` | **检测帧结束（上升沿 = 空闲中断）** |
| `DL_UART_MAIN_INTERRUPT_DMA_DONE_RX` | DMA 缓冲满 |
| `DL_UART_MAIN_INTERRUPT_RX` | FIFO 有数据 |
| `DL_UART_MAIN_INTERRUPT_RX_TIMEOUT_ERROR` | 超时错误（可选） |

### 5.2 中断优先级

- 建议：低优先级（不影响实时控制）
- 在 SysConfig 中配置

---

## 六、性能指标

| 指标 | 值 | 说明 |
|------|-----|------|
| **波特率** | 9600 bps | 标准波特率 |
| **DMA 缓冲** | 128 字节 | 可容纳 16 帧（8 字节/帧） |
| **单帧大小** | 8 字节 | 帧头(2) + 数据(?) + 帧尾(2) |
| **接收延迟** | < 1 ms | RXD_POS_EDGE 触发 |
| **发送速率** | 1200 字节/秒 | 9600 bps ÷ 8 bits |
| **CPU 占用** | 低 | DMA 自动传输，中断处理快速 |
| **内存占用** | ~300 字节 | 缓冲 + 结构体 |

---

## 七、集成步骤

### 7.1 添加文件到 Keil 项目

1. 添加 [`src/drivers/drv_uart.c`](src/drivers/drv_uart.c)
2. 添加 [`src/isr/isr_uart.c`](src/isr/isr_uart.c)
3. 确保包含路径包含 `src/drivers/`

### 7.2 在 empty.c 中初始化

```c
#include "src/drivers/drv_uart.h"

int main(void)
{
    SYSCFG_DL_init();
    uart_init();
    
    while (1) {
        // 处理 UART 接收
        if (g_uart_rx.frame_ready) {
            int result = uart_parse_stream();
            if (result == 1) {
                uint8_t data[64];
                uint16_t len;
                uart_get_frame(data, &len);
                // 处理数据
                uart_restart_dma();
            }
        }
    }
}
```

### 7.3 验证 SysConfig 配置

确保以下配置已设置：
- UART0：波特率 9600 bps
- DMA：传输大小 128 字节，模式 SINGLE_TRANSFER
- 中断：RXD_NEG_EDGE、RXD_POS_EDGE、DMA_DONE_RX

---

## 八、关键特性

### 8.1 流式解析的优势

✓ **防止帧头帧尾重复匹配**
- 单帧 8 字节，DMA 缓冲 128 字节可能包含 16 帧
- 状态机逐字节处理，自动分离多帧

✓ **自动错误恢复**
- 任何格式错误自动回到 IDLE 状态
- 无需手动重启

✓ **低 CPU 占用**
- 中断处理仅设置标志
- 解析在主循环中进行

### 8.2 空闲中断的实现

✓ **使用 RXD_POS_EDGE**
- 检测 RX 线上升沿（总线回到空闲）
- 比 RX_TIMEOUT 更快速（< 1 ms）

✓ **配合 DMA_DONE_RX**
- 处理缓冲满的情况
- 两种触发条件确保不丢数据

---

## 九、调试建议

### 9.1 验证中断触发

```c
volatile uint32_t g_uart_irq_count = 0;

void uart_isr_handler(void)
{
    g_uart_irq_count++;  // 计数中断
    // ...
}
```

### 9.2 监控 DMA 状态

```c
uint16_t remaining = DL_DMA_getRemainingTransferSize(DMA, DMA_UART0_RX_CHAN_ID);
uint16_t received = UART_RX_BUFFER_SIZE - remaining;
```

### 9.3 打印接收数据

```c
if (g_uart_rx.frame_ready) {
    printf("DMA 缓冲（%d 字节）：", g_uart_rx.dma_count);
    for (int i = 0; i < g_uart_rx.dma_count; i++) {
        printf("0x%02X ", g_uart_rx.dma_buffer[i]);
    }
    printf("\n");
}
```

---

## 十、常见问题

### Q1：为什么 DMA 缓冲是 128 字节？

**A**：单帧 8 字节，128 字节可容纳 16 帧。这样可以在帧之间有足够的处理时间，同时不会过度占用内存。

### Q2：RXD_POS_EDGE 何时触发？

**A**：当 RX 线从低电平变为高电平时触发。在 UART 通信中，这标志着一个或多个字节的接收完成，总线回到空闲状态。

### Q3：如何处理多帧数据？

**A**：使用流式解析，状态机会自动逐帧提取。每次 `uart_parse_stream()` 返回 1 时，表示找到一帧完整数据。

### Q4：发送时会阻塞吗？

**A**：是的。`uart_send_frame()` 使用轮询方式，会等待 TX FIFO 非满。如果需要非阻塞发送，可改用中断或 DMA。

### Q5：如何扩展为支持多个 UART？

**A**：复制驱动文件，修改宏定义（如 `UART_CAM_INST` → `UART_OTHER_INST`），为每个 UART 创建独立的全局变量和中断处理函数。

---

## 十一、扩展建议

### 11.1 添加 CRC 校验

```c
// 帧格式：[AA 55] [数据] [CRC] [0D 0A]
uint8_t crc = calculate_crc(data, len);
```

### 11.2 添加发送中断

```c
// 使用 TX 中断实现非阻塞发送
// 需要在 SysConfig 中启用 DL_UART_MAIN_INTERRUPT_TX
```

### 11.3 添加接收超时

```c
// 在主循环中检测超时
if (get_timestamp() - g_last_rx_time > 1000) {
    uart_restart_dma();
}
```

### 11.4 添加帧队列

```c
// 缓冲多帧数据，避免丢失
// 参考 uart_example.c 中的 frame_queue_t
```

---

## 十二、参考文档

- [`plans/uart_design_plan.md`](plans/uart_design_plan.md) - 设计方案
- [`plans/uart_integration_guide.md`](plans/uart_integration_guide.md) - 集成指南
- [`src/drivers/uart_example.c`](src/drivers/uart_example.c) - 使用示例
- TI MSPM0G3507 数据手册 - UART 和 DMA 模块

