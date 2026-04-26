## 📊 JY61P UART 协议分析

### 三种数据帧格式

#### 1. 加速度帧 (0x51)
```
[0x55] [0x51] [AxL] [AxH] [AyL] [AyH] [AzL] [AzH] [TL] [TH] [SUM]
 11字节
```
- 加速度: X=((AxH<<8)|AxL)/32768*16g
- 温度: T=((TH<<8)|TL)/100°C
- 校验: SUM = 0x55+0x51+AxL+AxH+AyL+AyH+AzL+AzH+TL+TH

#### 2. 角速度帧 (0x52)
```
[0x55] [0x52] [WxL] [WxH] [WyL] [WyH] [WzL] [WzH] [VolL] [VolH] [SUM]
 11字节
```
- 角速度: X=((WxH<<8)|WxL)/32768*2000°/s
- 电压: V=((VolH<<8)|VolL)/100V
- 校验: SUM = 0x55+0x52+WxL+WxH+WyL+WyH+WzL+WzH+VolL+VolH

#### 3. 欧拉角帧 (0x53)
```
[0x55] [0x53] [RollL] [RollH] [PitchL] [PitchH] [YawL] [YawH] [VL] [VH] [SUM]
 11字节
```
- 欧拉角: Roll=((RollH<<8)|RollL)/32768*180°
- 版本号: V=(VH<<8)|VL
- 校验: SUM = 0x55+0x53+RollL+RollH+PitchL+PitchH+YawL+YawH+VL+VH