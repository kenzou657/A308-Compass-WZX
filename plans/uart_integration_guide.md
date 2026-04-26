# UART 驱动集成指南

## 一、文件结构

```
src/
├── drivers/
│   ├── drv_uart.h       # UART 驱动头文件
│   └── drv_uart.c       # UART 驱动实现
└── isr/
    └── isr_uart.c       # UART 中断处理
```

## 二、核心特性

### 1. DMA 接收 + 空闲中断
- **DMA 缓冲**：128 字节
- **触发条件**：
  - RXD_POS_EDGE（上升沿 = 空闲中断）
  - DMA_DONE_RX（缓冲满）

### 2. 流式帧解析
- **状态机**：7 个状态，防止帧头帧尾重复匹配
- **帧格式**：`[AA 55] [数据] [0D 0A]`
- **单帧大小**：8 字节（可配置）

### 3. 串口发送
- **方式**：轮询发送（无 DMA）
- **自动添加帧头帧尾**

---

## 三、集成步骤

### 步骤 1：在 empty.c 中初始化

```c
#include "ti_msp_dl_config.h"
#include "src/drivers/drv_uart.h"

int main(void)
{
    SYSCFG_DL_init();
    
    // 初始化 UART 驱动
    uart_init();
    
    // 主循环
    while (1) {
        // 检查是否有帧就绪
        if (g_uart_rx.frame_ready) {
            // 流式解析接收缓冲
            int result = uart_parse_stream();
            
            if (result == 1) {
                // 帧有效，获取数据
                uint8_t frame_data[64];
                uint16_t frame_len;
                uart_get_frame(frame_data, &frame_len);
                
                // 处理帧数据
                // TODO: 调用应用层处理函数
                
                // 重启 DMA
                uart_restart_dma();
            } else if (result == -1) {
                // 帧错误，重启 DMA
                uart_restart_dma();
            }
            // result == 0 表示未找到完整帧，继续等待
        }
    }
}
```

### 步骤 2：在 Keil 项目中添加文件

1. 添加 `src/drivers/drv_uart.c` 到项目
2. 添加 `src/isr/isr_uart.c` 到项目
3. 确保包含路径包含 `src/drivers/`

### 步骤 3：验证 SysConfig 配置

确保以下配置已在 SysConfig 中设置：

```
UART_CAM (UART0):
  - 波特率：9600 bps
  - 中断：
    ✓ DMA_DONE_RX
    ✓ RXD_NEG_EDGE
    ✓ RXD_POS_EDGE
    ✓ RX_TIMEOUT_ERROR（可选）

DMA_UART0_RX:
  - 传输大小：128 字节
  - 传输模式：FULL_CH_SINGLE_TRANSFER_MODE
  - 目标地址增量：启用
  - 源地址增量：禁用
```

---

## 四、API 使用示例

### 接收数据

```c
// 在主循环中
if (g_uart_rx.frame_ready) {
    int result = uart_parse_stream();
    
    if (result == 1) {
        // 帧有效
        uint8_t data[64];
        uint16_t len;
        uart_get_frame(data, &len);
        
        // 处理数据
        printf("收到帧，长度：%d\n", len);
        for (int i = 0; i < len; i++) {
            printf("0x%02X ", data[i]);
        }
        printf("\n");
        
        // 重启 DMA
        uart_restart_dma();
    }
}
```

### 发送数据

```c
// 发送帧（自动添加帧头帧尾）
uint8_t send_data[] = {0x01, 0x02, 0x03, 0x04};
uart_send_frame(send_data, 4);

// 或发送原始数据
uart_send_data((uint8_t *)"Hello", 5);

// 或发送单个字节
uart_send_byte(0xFF);
```

---

## 五、流式解析状态机详解

### 状态转移图

```
┌─────────────────────────────────────────────────────────┐
│                                                         │
│  IDLE ──(AA)──> HEADER_1 ──(55)──> HEADER_2            │
│   ▲                ▲                   │                │
│   │                └─(非AA)────────────┘                │
│   │                                    │                │
│   │                                    ▼                │
│   │                                  DATA               │
│   │                                    │                │
│   │                                    ▼                │
│   │                                  TAIL_1             │
│   │                                    │                │
│   │                                    ▼                │
│   └──────────(非0D)──────────────────TAIL_2             │
│                                        │                │
│                                        ▼                │
│                                   FRAME_READY           │
│                                   (返回 1)              │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

### 关键特性

1. **防止帧头重复匹配**
   - 在 HEADER_1 状态，如果收到 AA，保持在 HEADER_1
   - 只有收到 55 才转移到 HEADER_2

2. **灵活的数据接收**
   - 从 HEADER_2 直接可以进入 DATA 或 TAIL_1
   - 允许 0 字节的数据帧

3. **错误恢复**
   - 任何不符合预期的字节都回到 IDLE 状态
   - 自动重新同步

---

## 六、性能指标

| 指标 | 值 |
|------|-----|
| **DMA 缓冲大小** | 128 字节 |
| **单帧大小** | 8 字节（可配置） |
| **最大帧数** | 128 / 8 = 16 帧 |
| **接收延迟** | < 1 ms（RXD_POS_EDGE 触发） |
| **CPU 占用** | 低（DMA 自动传输） |
| **发送速率** | 9600 bps ≈ 1200 字节/秒 |

---

## 七、调试建议

### 1. 验证中断触发

```c
// 在中断处理中添加调试标志
volatile uint32_t g_uart_irq_count = 0;

void uart_isr_handler(void)
{
    g_uart_irq_count++;  // 计数中断次数
    // ... 其他处理
}
```

### 2. 监控 DMA 状态

```c
// 检查 DMA 剩余字节数
uint16_t remaining = DL_DMA_getRemainingTransferSize(DMA, DMA_UART0_RX_CHAN_ID);
uint16_t received = UART_RX_BUFFER_SIZE - remaining;
```

### 3. 打印接收数据

```c
// 在帧就绪时打印缓冲内容
if (g_uart_rx.frame_ready) {
    printf("DMA 缓冲内容（%d 字节）：\n", g_uart_rx.dma_count);
    for (int i = 0; i < g_uart_rx.dma_count; i++) {
        printf("0x%02X ", g_uart_rx.dma_buffer[i]);
    }
    printf("\n");
}
```

---

## 八、常见问题

### Q1：为什么使用流式解析而不是一次性匹配？

**A**：因为单帧只有 8 字节，DMA 缓冲 128 字节可能包含 16 帧数据。流式解析可以逐帧提取，防止帧头帧尾重复匹配。

### Q2：RXD_POS_EDGE 中断何时触发？

**A**：当 RX 线从低电平变为高电平时触发，即总线回到空闲状态。这标志着一个字节或多个字节的接收完成。

### Q3：如何处理帧错误？

**A**：当 `uart_parse_stream()` 返回 -1 时，表示帧错误。此时调用 `uart_restart_dma()` 重新开始接收。

### Q4：发送时是否会阻塞？

**A**：是的。`uart_send_frame()` 使用轮询方式，会等待 TX FIFO 非满。如果需要非阻塞发送，可以改用中断或 DMA。

---

## 九、扩展建议

### 1. 添加接收超时处理

```c
// 在主循环中添加超时检测
static uint32_t g_last_rx_time = 0;
if (g_uart_rx.frame_ready) {
    g_last_rx_time = get_timestamp();
} else if (get_timestamp() - g_last_rx_time > 1000) {
    // 1 秒无数据，重启 DMA
    uart_restart_dma();
}
```

### 2. 添加发送中断

```c
// 使用 TX 中断实现非阻塞发送
// 需要在 SysConfig 中启用 DL_UART_MAIN_INTERRUPT_TX
```

### 3. 添加 CRC 校验

```c
// 在帧数据后添加 CRC 字节
// 帧格式：[AA 55] [数据] [CRC] [0D 0A]
```

