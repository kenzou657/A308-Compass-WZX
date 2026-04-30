# DMA 串口接收 + 普通发送方案

## 一、项目现状分析

### 1.1 硬件配置（来自 ti_msp_dl_config.h）

| 模块 | 实例 | 波特率 | DMA 通道 | 用途 |
|------|------|--------|---------|------|
| UART0 | UART_CAM_INST | 9600 | DMA_UART0_RX_CHAN_ID (1) | 摄像头通信 |
| UART3 | UART_IMU_INST | 9600 | DMA_IMU_RX_CHAN_ID (0) | IMU 陀螺仪 |

### 1.2 GPIO 配置

**UART0（摄像头）**
- RX: PA11 (IOMUX_PINCM22)
- TX: PA10 (IOMUX_PINCM21)

**UART3（IMU）**
- RX: PB3 (IOMUX_PINCM16)
- TX: PB2 (IOMUX_PINCM15)

### 1.3 参考实现对比

| 特性 | 参考实现 | 说明 |
|------|---------|------|
| UART0 DMA 传输大小 | 12 字节 | 完整帧大小 |
| UART3 DMA 传输大小 | 11 字节 | 完整帧大小 |
| UART0 帧格式 | [AA 55] + 8字节 + [0D 0A] | 含校验和 |
| UART3 帧格式 | [55] + 帧类型 + 8字节 + 校验 | 含校验和 |
| 发送方式 | 普通轮询 | 逐字节发送 |

---

## 二、SysConfig 配置要点

### 2.1 DMA 模块配置

**DMA_UART0_RX_CHAN_ID (通道 1)**
```
- 触发源: DMA_UART0_RX_TRIG (UART0 RX)
- 源地址: UART0->RXDATA (自动配置)
- 目标地址: g_uart_rx.buffer[0] (驱动层配置)
- 传输大小: 12 字节 (每次 DMA 完成中断)
- 地址增量: 源固定，目标递增
- 中断: DMA_DONE_RX 中断启用
```

**DMA_IMU_RX_CHAN_ID (通道 0)**
```
- 触发源: DMA_UART3_RX_TRIG (UART3 RX)
- 源地址: UART3->RXDATA (自动配置)
- 目标地址: g_jy61p_rx.buffer[0] (驱动层配置)
- 传输大小: 11 字节 (每次 DMA 完成中断)
- 地址增量: 源固定，目标递增
- 中断: DMA_DONE_RX 中断启用
```

### 2.2 UART 模块配置

**UART0（摄像头）**
```
- 实例: UART0
- 波特率: 9600 bps
- 数据位: 8
- 停止位: 1
- 奇偶校验: 无
- 中断: DMA_DONE_RX 中断启用
- DMA 触发: 启用 RX DMA (触发源: DMA_UART0_RX_TRIG)
- GPIO: PA10 (TX), PA11 (RX)
```

**UART3（IMU）**
```
- 实例: UART3
- 波特率: 9600 bps
- 数据位: 8
- 停止位: 1
- 奇偶校验: 无
- 中断: DMA_DONE_RX 中断启用
- DMA 触发: 启用 RX DMA (触发源: DMA_UART3_RX_TRIG)
- GPIO: PB2 (TX), PB3 (RX)
```

---

## 三、UART0（摄像头）驱动设计

### 3.1 帧格式与校验

**帧格式**
```
[AA 55] [Mode] [ID] [Data_X(2)] [Data_Y(2)] [Reserved] [Checksum] [0D 0A]
 0   1    2      3     4-5        6-7         8          9         10  11
```

**校验和计算**
```c
uint8_t camera_uart_calc_checksum(const uint8_t *data)
{
    uint16_t sum = 0;
    // 累加前 9 个字节（从帧头到 Reserved）
    for (int i = 0; i < 9; i++) {
        sum += data[i];
    }
    // 取低 8 位
    return (uint8_t)(sum & 0xFF);
}
```

### 3.2 DMA 接收流程

```
1. DMA 每次接收 12 字节到环形缓冲
2. DMA_DONE_RX 中断触发
3. 流式扫描环形缓冲，查找帧头 [AA 55]
4. 验证帧尾 [0D 0A]
5. 计算校验和，与 buffer[9] 比较
6. 如果校验通过，复制到 frame_data，设置 frame_ready = 1
7. 更新 write_pos，重新配置 DMA
```

### 3.3 发送函数（普通轮询）

```c
void uart_send_byte(uint8_t byte)
{
    // 轮询等待 TXFIFO 非满
    while (DL_UART_Main_isBusy(UART_CAM_INST));
    DL_UART_Main_transmitData(UART_CAM_INST, byte);
}

void uart_send_data(const uint8_t *data, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++) {
        uart_send_byte(data[i]);
    }
}

int uart_send_frame(const uint8_t *data, uint16_t len)
{
    // 构建帧：[AA 55] [数据] [0D 0A]
    uart_send_byte(0xAA);
    uart_send_byte(0x55);
    uart_send_data(data, len);
    uart_send_byte(0x0D);
    uart_send_byte(0x0A);
    return 0;
}
```

---

## 四、UART3（IMU）驱动设计

### 4.1 帧格式与校验

**帧格式**
```
[55] [Type] [Data_0] [Data_1] [Data_2] [Data_3] [Data_4] [Data_5] [Data_6] [Data_7] [Data_8] [Checksum]
 0    1       2        3        4        5        6        7        8        9       10       11
```

**帧类型**
- 0x51: 加速度帧
- 0x52: 角速度帧
- 0x53: 欧拉角帧

**校验和计算**
```c
static uint8_t jy61p_verify_checksum(const uint8_t *frame)
{
    uint8_t sum = 0;
    // 累加前 10 字节
    for (int i = 0; i < 10; i++) {
        sum += frame[i];
    }
    // 比较计算值与帧中的校验和
    return (sum == frame[10]);
}
```

### 4.2 数据解析与单位转换

**加速度帧（0x51）**
```c
// 原始值提取（小端序）
ax = (int16_t)((frame[3] << 8) | frame[2]);
ay = (int16_t)((frame[5] << 8) | frame[4]);
az = (int16_t)((frame[7] << 8) | frame[6]);
temp = (int16_t)((frame[9] << 8) | frame[8]);

// 单位转换
ax_g = (float)ax / 32768.0f * 16.0f;      // 转换为 g
ay_g = (float)ay / 32768.0f * 16.0f;
az_g = (float)az / 32768.0f * 16.0f;
temp_c = (float)temp / 100.0f;            // 转换为 °C
```

**角速度帧（0x52）**
```c
// 原始值提取（小端序）
wx = (int16_t)((frame[3] << 8) | frame[2]);
wy = (int16_t)((frame[5] << 8) | frame[4]);
wz = (int16_t)((frame[7] << 8) | frame[6]);
voltage = (uint16_t)((frame[9] << 8) | frame[8]);

// 单位转换
wx_dps = (float)wx / 32768.0f * 2000.0f;  // 转换为 °/s
wy_dps = (float)wy / 32768.0f * 2000.0f;
wz_dps = (float)wz / 32768.0f * 2000.0f;
voltage_v = (float)voltage / 100.0f;      // 转换为 V
```

**欧拉角帧（0x53）**
```c
// 原始值提取（小端序）
roll = (int16_t)((frame[3] << 8) | frame[2]);
pitch = (int16_t)((frame[5] << 8) | frame[4]);
yaw = (int16_t)((frame[7] << 8) | frame[6]);
version = (uint16_t)((frame[9] << 8) | frame[8]);

// 单位转换（关键：转换为正确的角度范围）
roll_deg = (float)roll / 32768.0f * 180.0f;    // 转换为 °（-180 ~ 180）
pitch_deg = (float)pitch / 32768.0f * 180.0f;
yaw_deg = (float)yaw / 32768.0f * 180.0f;
```

### 4.3 DMA 接收流程

```
1. DMA 每次接收 11 字节到环形缓冲
2. DMA_DONE_RX 中断触发
3. 流式扫描环形缓冲，查找帧头 [55]
4. 验证帧类型（0x51/0x52/0x53）
5. 验证校验和
6. 如果校验通过，解析数据到对应结构体
7. 设置对应的 _updated 标志
8. 更新 write_pos，重新配置 DMA
```

### 4.4 发送函数（普通轮询）

```c
void jy61p_send_byte(uint8_t byte)
{
    // 轮询等待 TXFIFO 非满
    while (DL_UART_Main_isBusy(UART_IMU_INST));
    DL_UART_Main_transmitData(UART_IMU_INST, byte);
}

int jy61p_send_command(const uint8_t *cmd, uint16_t len)
{
    // 发送命令（无帧头帧尾，直接发送）
    for (uint16_t i = 0; i < len; i++) {
        jy61p_send_byte(cmd[i]);
    }
    return 0;
}
```

---

## 五、中断服务函数（ISR 层）

### 5.1 UART0 ISR

**文件**: `src/isr/isr_uart.c`

```c
void UART0_IRQHandler(void)
{
    // 调用驱动层处理接收
    uart_isr_handler();
    
    // 调用应用层回调处理摄像头数据
    camera_uart_rx_callback();
}
```

### 5.2 UART3 ISR

**文件**: `src/isr/isr_jy61p.c`

```c
void UART3_IRQHandler(void)
{
    // 调用驱动层处理接收
    jy61p_isr_handler();
}
```

---

## 六、环形缓冲管理

### 6.1 缓冲大小设计

**UART0（摄像头）**
- 帧大小: 12 字节
- DMA 传输大小: 12 字节
- 环形缓冲大小: 144 字节 (12 × 12)
- 可容纳: 12 个完整帧

**UART3（IMU）**
- 帧大小: 11 字节
- DMA 传输大小: 11 字节
- 环形缓冲大小: 55 字节 (11 × 5)
- 可容纳: 5 个完整帧

### 6.2 流式扫描算法

```
当 DMA 完成一次传输时:
1. 获取当前写入位置 (write_pos)
2. 在 [write_pos, write_pos + DMA_SIZE] 范围内扫描帧头
3. 验证帧头、帧尾、校验和
4. 如果找到有效帧，复制到 frame_data
5. 更新 write_pos: (write_pos + DMA_SIZE) % BUFFER_SIZE
6. 重新配置 DMA 目标地址为新的 write_pos

优点:
- 自动恢复帧同步（即使帧边界跨越 DMA 传输边界）
- 无需额外的状态机
- 处理速度快 (O(DMA_SIZE) 复杂度)
```

---

## 七、实现步骤

### 第一阶段：驱动层实现
1. 完善 `src/drivers/drv_uart.c` 中的 DMA 初始化和中断处理
2. 完善 `src/drivers/drv_uart.h` 中的数据结构和函数声明
3. 实现 `uart_send_byte()` 和 `uart_send_data()`
4. 完善 `src/drivers/drv_jy61p.c` 中的 DMA 初始化和中断处理
5. 完善 `src/drivers/drv_jy61p.h` 中的数据结构和函数声明
6. 实现 `jy61p_send_command()`

### 第二阶段：应用层实现
1. 完善 `src/app/app_camera_uart.c` 中的校验和计算和帧解析
2. 完善 `src/app/app_camera_uart.h` 中的数据结构定义

### 第三阶段：ISR 层实现
1. 完善 `src/isr/isr_uart.c` 中的 UART0 中断处理
2. 完善 `src/isr/isr_jy61p.c` 中的 UART3 中断处理

### 第四阶段：主程序集成
1. 在 `empty.c` 中调用 `uart_init()` 和 `jy61p_init()`
2. 在主循环中调用 `uart_get_frame()` 和 `jy61p_get_acc/gyro/angle()`
3. 测试发送功能

### 第五阶段：测试验证
1. 单元测试：验证 DMA 接收、帧解析、发送
2. 集成测试：验证摄像头和 IMU 数据同时接收
3. 性能测试：验证帧丢失率、延迟

---

## 八、关键注意事项

### 8.1 UART0 校验和
- ✓ 校验范围：前 9 字节（帧头到 Reserved）
- ✓ 校验方式：累加和取低 8 位
- ✓ 校验位置：buffer[9]

### 8.2 UART3 校验和
- ✓ 校验范围：前 10 字节（帧头到 Data_7）
- ✓ 校验方式：简单累加
- ✓ 校验位置：buffer[10]

### 8.3 UART3 数据单位转换
- ✓ 加速度：原始值 / 32768 × 16 = g
- ✓ 角速度：原始值 / 32768 × 2000 = °/s
- ✓ 欧拉角：原始值 / 32768 × 180 = °（范围 -180 ~ 180）
- ✓ 温度：原始值 / 100 = °C
- ✓ 电压：原始值 / 100 = V

### 8.4 DMA 配置
- ✓ 源地址固定（UART RXDATA）
- ✓ 目标地址递增（环形缓冲）
- ✓ 每次传输大小固定（12 或 11 字节）
- ✓ 传输完成后触发中断

### 8.5 发送方式
- ✓ 使用普通轮询方式（不使用 DMA）
- ✓ 轮询等待 TXFIFO 非满
- ✓ 逐字节发送

### 8.6 环形缓冲
- ✓ 缓冲大小必须是 DMA 传输大小的整数倍
- ✓ 流式扫描自动处理帧边界跨越
- ✓ 无需手动管理读指针

### 8.7 性能优化
- ✓ 减少 ISR 中的浮点运算
- ✓ 数据解析在 ISR 中完成（时间敏感）
- ✓ 单位转换可在主循环中进行

---

## 九、文件清单

| 文件 | 修改内容 |
|------|---------|
| `src/drivers/drv_uart.c` | 完善 DMA 初始化、中断处理、发送函数 |
| `src/drivers/drv_uart.h` | 完善数据结构、函数声明 |
| `src/drivers/drv_jy61p.c` | 完善 DMA 初始化、中断处理、数据解析 |
| `src/drivers/drv_jy61p.h` | 完善数据结构、函数声明 |
| `src/app/app_camera_uart.c` | 实现校验和计算、帧解析 |
| `src/app/app_camera_uart.h` | 完善数据结构定义 |
| `src/isr/isr_uart.c` | 完善 UART0 中断处理 |
| `src/isr/isr_jy61p.c` | 完善 UART3 中断处理 |
| `empty.c` | 调用初始化函数 |

---

## 十、验证清单

- [ ] DMA 通道 0 和 1 已在 SysConfig 中配置
- [ ] UART0 和 UART3 的 DMA 触发已启用
- [ ] 中断优先级已设置
- [ ] 环形缓冲大小正确
- [ ] UART0 帧格式和校验正确
- [ ] UART3 帧格式和校验正确
- [ ] UART3 数据单位转换正确
- [ ] 流式扫描算法正确
- [ ] 发送函数轮询逻辑正确
- [ ] ISR 中无浮点运算
- [ ] 主循环中正确调用初始化函数
- [ ] 测试摄像头数据接收
- [ ] 测试 IMU 数据接收
- [ ] 测试发送功能
