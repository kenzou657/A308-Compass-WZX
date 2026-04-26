# UART0 串口接收发送设计方案

## 一、UART0 配置参数分析

### 1. 当前配置参数

| 参数 | 值 | 说明 |
|------|-----|------|
| **UART 实例** | UART0 | 使用 UART0 模块 |
| **波特率** | 9600 bps | 标准波特率，适合 OpenMV 通信 |
| **时钟频率** | 4 MHz | UART 工作时钟 |
| **过采样率** | 16X | 标准过采样，提高抗干扰能力 |
| **IBRD** | 26 | 整数波特率分频器 |
| **FBRD** | 3 | 小数波特率分频器 |
| **实际波特率** | 9598.08 bps | 误差 ≈ 0.02%（✓ 可接受） |
| **RX 引脚** | PA11 (IOMUX_PINCM22) | 接收引脚 |
| **TX 引脚** | PA10 (IOMUX_PINCM21) | 发送引脚 |
| **DMA 通道** | 通道 0 | 用于 RX DMA 传输 |
| **DMA 传输大小** | 8 字节 | **⚠️ 问题：过小** |
| **DMA 模式** | FULL_CH_REPEAT_SINGLE_TRANSFER_MODE | 单次传输重复模式 |
| **中断配置** | DMA_DONE_RX + RX_TIMEOUT_ERROR | 已启用 DMA 完成和超时中断 |

### 2. 配置合理性评估

#### ✓ 合理的地方
- **波特率选择**：9600 bps 是标准波特率，适合 OpenMV 摄像头通信
- **过采样率**：16X 提供良好的抗干扰能力
- **波特率精度**：误差 0.02% 远低于 5% 的容限，非常精确
- **中断配置**：同时启用 DMA 完成中断和超时中断，能捕获两种场景
- **DMA 触发**：使用外部触发（UART RX FIFO），自动触发

#### ⚠️ 需要改进的地方

**问题 1：DMA 传输大小过小（8 字节）**
- 当前设置：`DL_DMA_setTransferSize(DMA, DMA_UART0_RX_CHAN_ID, 8);`
- 问题：
  - 8 字节太小，无法容纳完整的数据帧
  - 假设帧格式：`[AA 55] [数据...] [0D 0A]`，最少需要 4 字节（帧头+帧尾）
  - 实际数据帧可能 20-64 字节，8 字节会频繁触发 DMA 完成中断
  - 导致中断频率过高，CPU 负担重
- **建议**：改为 64 字节或 128 字节（根据实际帧长度）

**问题 2：缺少 RX_IDLE 中断配置**
- 当前配置只有 `DMA_DONE_RX` 和 `RX_TIMEOUT_ERROR`
- 缺少 `RX_IDLE_INTERRUPT`（空闲中断）
- 空闲中断用于检测帧结束（总线空闲 > 1 字符时间）
- **建议**：在 SysConfig 中添加 `DL_UART_MAIN_INTERRUPT_RX_IDLE`

**问题 3：DMA 模式选择**
- 当前：`FULL_CH_REPEAT_SINGLE_TRANSFER_MODE`（单次传输重复模式）
- 这种模式在 DMA 完成后会自动重新启动，可能导致数据覆盖
- **建议**：改为 `FULL_CH_SINGLE_TRANSFER_MODE`（单次传输模式），由软件手动重启 DMA

---

## 二、串口接收方案设计

### 1. 接收流程图

```
┌─────────────────────────────────────────────────────────┐
│                    UART RX 数据到达                      │
└────────────────────┬────────────────────────────────────┘
                     │
        ┌────────────┴────────────┐
        │                         │
        ▼                         ▼
   DMA 缓冲满                 总线空闲 > 1 字符
   (8 字节)                   (空闲中断)
        │                         │
        └────────────┬────────────┘
                     │
                     ▼
        ┌─────────────────────────┐
        │  触发 UART0_IRQHandler  │
        └────────────┬────────────┘
                     │
                     ▼
        ┌─────────────────────────────────────┐
        │  设置 RX_DATA_READY 标志位          │
        │  记录 DMA 已接收字节数              │
        └────────────┬────────────────────────┘
                     │
                     ▼
        ┌─────────────────────────────────────┐
        │  主循环检测 RX_DATA_READY 标志      │
        └────────────┬────────────────────────┘
                     │
                     ▼
        ┌─────────────────────────────────────┐
        │  调用 uart_process_rx_data()        │
        │  - 匹配帧头 (AA 55)                 │
        │  - 匹配帧尾 (0D 0A)                 │
        │  - 提取有效数据                     │
        └────────────┬────────────────────────┘
                     │
                     ▼
        ┌─────────────────────────────────────┐
        │  清除标志，重启 DMA                 │
        └─────────────────────────────────────┘
```

### 2. 接收缓冲区设计

```c
// 定义接收缓冲区
#define UART_RX_BUFFER_SIZE  128      // DMA 缓冲区大小
#define UART_FRAME_MAX_SIZE  64       // 单帧最大数据长度

typedef struct {
    uint8_t dma_buffer[UART_RX_BUFFER_SIZE];    // DMA 接收缓冲
    uint8_t frame_buffer[UART_FRAME_MAX_SIZE];  // 帧数据缓冲
    uint16_t dma_count;                         // DMA 已接收字节数
    uint16_t frame_len;                         // 当前帧长度
    uint8_t frame_ready;                        // 帧就绪标志
} uart_rx_t;
```

### 3. 中断处理逻辑

```c
// UART0 中断处理函数
void UART0_IRQHandler(void)
{
    uint32_t status = DL_UART_Main_getPendingInterrupt(UART_CAM_INST);
    
    // DMA 接收完成中断
    if (status & DL_UART_MAIN_INTERRUPT_DMA_DONE_RX) {
        DL_UART_Main_clearInterruptStatus(UART_CAM_INST, 
                                          DL_UART_MAIN_INTERRUPT_DMA_DONE_RX);
        // 记录 DMA 已接收字节数
        uart_rx.dma_count = UART_RX_BUFFER_SIZE;
        uart_rx.frame_ready = 1;  // 设置帧就绪标志
    }
    
    // 空闲中断（总线空闲 > 1 字符时间）
    if (status & DL_UART_MAIN_INTERRUPT_RX_IDLE) {
        DL_UART_Main_clearInterruptStatus(UART_CAM_INST, 
                                          DL_UART_MAIN_INTERRUPT_RX_IDLE);
        // 获取 DMA 当前传输计数
        uart_rx.dma_count = UART_RX_BUFFER_SIZE - 
                           DL_DMA_getRemainingTransferSize(DMA, DMA_UART0_RX_CHAN_ID);
        uart_rx.frame_ready = 1;  // 设置帧就绪标志
    }
    
    // 超时错误中断
    if (status & DL_UART_MAIN_INTERRUPT_RX_TIMEOUT_ERROR) {
        DL_UART_Main_clearInterruptStatus(UART_CAM_INST, 
                                          DL_UART_MAIN_INTERRUPT_RX_TIMEOUT_ERROR);
        // 处理超时错误（可选：记录日志或重启 DMA）
    }
}
```

### 4. 帧头帧尾匹配逻辑

```c
// 帧格式：[AA 55] [数据...] [0D 0A]
#define FRAME_HEADER_1  0xAA
#define FRAME_HEADER_2  0x55
#define FRAME_TAIL_1    0x0D
#define FRAME_TAIL_2    0x0A

int uart_process_rx_data(void)
{
    uint16_t i, j;
    uint8_t *buf = uart_rx.dma_buffer;
    uint16_t len = uart_rx.dma_count;
    
    // 查找帧头
    for (i = 0; i < len - 3; i++) {
        if (buf[i] == FRAME_HEADER_1 && buf[i+1] == FRAME_HEADER_2) {
            // 找到帧头，继续查找帧尾
            for (j = i + 2; j < len - 1; j++) {
                if (buf[j] == FRAME_TAIL_1 && buf[j+1] == FRAME_TAIL_2) {
                    // 找到完整帧
                    uart_rx.frame_len = j - i - 2;  // 数据长度（不含帧头帧尾）
                    
                    // 复制数据到帧缓冲
                    if (uart_rx.frame_len <= UART_FRAME_MAX_SIZE) {
                        memcpy(uart_rx.frame_buffer, &buf[i+2], uart_rx.frame_len);
                        return 1;  // 帧有效
                    }
                    return -1;  // 帧过长
                }
            }
        }
    }
    
    return 0;  // 未找到完整帧
}
```

### 5. 主循环处理

```c
// 在主循环中调用
void main_loop(void)
{
    while (1) {
        if (uart_rx.frame_ready) {
            uart_rx.frame_ready = 0;
            
            // 处理接收到的数据
            int result = uart_process_rx_data();
            if (result == 1) {
                // 帧有效，处理数据
                uart_handle_frame(uart_rx.frame_buffer, uart_rx.frame_len);
            } else if (result == -1) {
                // 帧过长，错误处理
                uart_error_handler();
            }
            
            // 重启 DMA
            uart_restart_dma();
        }
    }
}
```

---

## 三、串口发送方案设计

### 1. 发送缓冲区设计

```c
#define UART_TX_BUFFER_SIZE  128

typedef struct {
    uint8_t tx_buffer[UART_TX_BUFFER_SIZE];
    uint16_t tx_len;
    uint8_t tx_busy;  // 发送忙标志
} uart_tx_t;
```

### 2. 发送函数

```c
// 发送数据帧（自动添加帧头帧尾）
int uart_send_frame(const uint8_t *data, uint16_t len)
{
    uint16_t total_len;
    
    if (uart_tx.tx_busy) {
        return -1;  // 发送忙
    }
    
    if (len + 4 > UART_TX_BUFFER_SIZE) {
        return -2;  // 数据过长
    }
    
    // 构建帧：[AA 55] [数据] [0D 0A]
    uart_tx.tx_buffer[0] = FRAME_HEADER_1;
    uart_tx.tx_buffer[1] = FRAME_HEADER_2;
    memcpy(&uart_tx.tx_buffer[2], data, len);
    uart_tx.tx_buffer[len + 2] = FRAME_TAIL_1;
    uart_tx.tx_buffer[len + 3] = FRAME_TAIL_2;
    
    total_len = len + 4;
    uart_tx.tx_len = total_len;
    uart_tx.tx_busy = 1;
    
    // 启动发送
    uart_start_tx();
    
    return 0;
}

// 启动发送
void uart_start_tx(void)
{
    uint16_t i;
    for (i = 0; i < uart_tx.tx_len; i++) {
        DL_UART_Main_transmitData(UART_CAM_INST, uart_tx.tx_buffer[i]);
    }
    uart_tx.tx_busy = 0;
}
```

### 3. 简单发送（不使用中断）

由于 UART 发送通常较快，可以采用轮询方式：

```c
void uart_send_byte(uint8_t byte)
{
    // 等待 TX FIFO 非满
    while (DL_UART_Main_isBusy(UART_CAM_INST));
    DL_UART_Main_transmitData(UART_CAM_INST, byte);
}

void uart_send_string(const uint8_t *str, uint16_t len)
{
    uint16_t i;
    for (i = 0; i < len; i++) {
        uart_send_byte(str[i]);
    }
}
```

---

## 四、SysConfig 配置建议

### 需要修改的项目

1. **DMA 传输大小**
   - 当前：8 字节
   - 建议：64 或 128 字节
   - 原因：减少中断频率，提高效率

2. **DMA 传输模式**
   - 当前：`FULL_CH_REPEAT_SINGLE_TRANSFER_MODE`
   - 建议：`FULL_CH_SINGLE_TRANSFER_MODE`
   - 原因：防止数据自动覆盖，由软件控制重启

3. **UART 中断配置**
   - 添加：`DL_UART_MAIN_INTERRUPT_RX_IDLE`
   - 保留：`DL_UART_MAIN_INTERRUPT_DMA_DONE_RX`
   - 保留：`DL_UART_MAIN_INTERRUPT_RX_TIMEOUT_ERROR`

4. **UART 空闲中断超时时间**
   - 建议：配置为 1-2 字符时间（约 1-2 ms @ 9600 bps）
   - 用于检测帧结束

---

## 五、驱动层实现框架

### 文件结构

```
src/drivers/
├── drv_uart.h       // UART 驱动头文件
└── drv_uart.c       // UART 驱动实现
```

### 驱动接口

```c
// drv_uart.h
#ifndef DRV_UART_H
#define DRV_UART_H

#include <stdint.h>

// 初始化 UART
void uart_init(void);

// 发送数据帧
int uart_send_frame(const uint8_t *data, uint16_t len);

// 接收数据帧（非阻塞）
int uart_recv_frame(uint8_t *data, uint16_t *len);

// 中断处理函数
void uart_isr_handler(void);

#endif
```

---

## 六、总结

| 方面 | 设计方案 |
|------|---------|
| **接收方式** | DMA + 空闲中断 |
| **接收缓冲** | 128 字节 DMA 缓冲 + 64 字节帧缓冲 |
| **帧格式** | `[AA 55] [数据] [0D 0A]` |
| **帧匹配** | 软件查找帧头帧尾 |
| **发送方式** | 轮询方式（无中断） |
| **中断优先级** | 低（不影响实时控制） |
| **CPU 负担** | 低（DMA 自动传输，主循环处理） |

