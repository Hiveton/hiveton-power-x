# PX1 USB PD / QC 实现整理

## 当前硬件连接

根据 `原理图.pdf`，PX1 这版硬件具备做 USB-C 表、PD 检测/诱骗、QC 检测/诱骗的基础能力。

关键网络：

| 功能 | 原理图网络 | MCU/器件 | 用途 |
| --- | --- | --- | --- |
| USB PD CC1 | `USB_CC1` | `PB6/CC1`，经 `CC_EN` 控制的双 NMOS 接入 | PD BMC 收发、CC 电平检测、方向判断 |
| USB PD CC2 | `USB_CC2` | `PB7/CC2`，经 `CC_EN` 控制的双 NMOS 接入 | PD BMC 收发、CC 电平检测、方向判断 |
| CC 开关使能 | `CC_EN` | `PB8`，默认下拉 `R77 10K` | 高电平接通 MCU CC 外设到 Type-C CC |
| CC1 外部 Rd | `CC1_EXT_RD_CTL` | `PB5` 控制 Q3/R91 路径 | 可控外部 Rd/按键复用路径，仅 CC1 |
| USB D+ | `USB_DP` | `PA12/UDP`，同时接 `PB0/ADC8` | QC/BC1.2 控制与采样，不能挂 USBFS 设备栈 |
| USB D- | `USB_DM` | `PA11/UDM`，同时接 `PB1/ADC9` | QC/BC1.2 控制与采样，不能挂 USBFS 设备栈 |
| 电压电流 | `VBUS_IN/OUT` | INA226 `0x40`，`R3 5mR` | 确认诱骗后的真实 VBUS/电流/功率 |
| 按键 | `BTN1/PB15`、`BTN2/PB9`、`BTN3/PA15` | BTN1/BTN3 低有效，BTN2 高有效 | BTN2/PB9 与 BTN3/PA15 使用 EXTI；BTN1/PB15 与 BTN3 共用 EXTI15，只能由 20ms 扫描兜底 |

结论：USB 串口必须删除，否则 PA11/PA12 会占用 DP/DM，影响 QC 和 USB 直通。当前固件已移除 USB CDC/debug 模块、USBFS 中断入口和构建引用，产品代码不再开启 USBFS 时钟或 USBFS 物理端口。

## PD 检测怎么做

PD 不需要 DP/DM，走 CC。

启动流程：

1. `PB8/CC_EN = 1`，接通 `USB_CC1/USB_CC2` 到 MCU USBPD 外设。
2. 配置 `PB6/PB7` 为 USBPD CC 输入，打开 CH32L103 的 `USBPD` 外设。
3. 两路 CC 都配置为 Sink Rd 模式，检测哪一路有源端 Rp：
   - CC1 有效则 `CC_SEL = 0`
   - CC2 有效则 `CC_SEL = 1`
4. 开启 BMC RX，等待 `Source_Capabilities`。
5. 解析 Source_Capabilities，得到固定 PDO 和 PPS APDO，页面显示为 PD/PPS。

当前代码基础：

| 模块 | 现状 |
| --- | --- |
| `bsp_usbpd_port.c` | 已做 CC_EN、方向检测、USBPD RX/TX、GoodCRC 等底层 |
| `service_pd.c` | 已能解析 Source_Capabilities，保存固定 PDO/PPS APDO，形成 PD snapshot |
| `app_tasks.c` | `PX1_ENABLE_PROTOCOL_TASK=1` 时会跑 PD task |

## PD 诱骗怎么做

诱骗就是在检测到 Source_Capabilities 后，选择一个 PDO，发送 PD `Request`。

建议业务状态机：

1. 默认只检测，不主动升压，避免上电就把 VBUS 拉到高压。
2. 用户在 UI 选择目标电压：5V / 9V / 12V / 15V / 20V。
3. 按确认后调用 `service_pd_set_preferred_voltage_mv(target_mv)`。
4. 下次收到 Source_Capabilities 时优先选择目标电压完全匹配的固定 PDO；没有精确固定档时，如果目标落在 PPS APDO 范围内，则发 PPS Request；否则退回到不超过目标电压的最佳固定 PDO。
5. 发送 `Request`，等待 `Accept`。
6. 等待 `PS_RDY` 后才认为诱骗成功。
7. 用 INA226 实测 VBUS，确认实际电压接近目标。
8. 超时或 Reject/Wait/SoftReset 时退回 5V/重新等待 Source_Capabilities。

当前落地状态：

- 已支持 Fixed PDO 检测、缓存和 UI snapshot 暴露；PPS APDO 已缓存 `min/max/current`，目标电压落在 PPS 范围内时会生成 PPS Request。
- UI 诱骗页的确认键已接入 `service_pd_set_preferred_voltage_mv()`。
- PDO 页 action mode 已支持 20mV 步进的 PPS 目标电压微调；进入编辑时会先同步当前 PD 目标/合约电压，确认后直接按该目标发 PD/PPS Request。
- 已补 `service_pd_prepare_pending_request()`，所以先检测到 Source_Capabilities、后按确认也会立即发 PD Request。
- Snapshot 已包含请求状态、目标电压、选中 PDO 原始 Object Position、Fixed PDO 列表、PPS 范围，UI 可显示 `AVAIL/REQ/ACC/READY/FAIL`。
- `PS_RDY` 后不会立刻标记 `READY`，会等待 INA226 VBUS 实测进入目标范围；若超时不到位则显示 `FAIL` 并把内部高压偏好退回默认 5V。
- `PS_RDY` 后的 VBUS 校验阶段不会再发第二个 PD Request；用户新选的目标会排队，等当前 VBUS 确认完成后再请求，避免诱骗状态机被打断。
- RDO 生成会保留 Source_Capabilities 的原始 Object Position，不会因为 APDO 被跳过而把固定 PDO 位置压缩错。
- RDO 工作电流会按线缆能力限流：未读到 5A E-Marker 时最高请求 3A，避免无 E-Marker 线缆上报 5A 合约。
- Detach/SoftReset 后会清掉高压偏好并回到默认 5V，请求 9V/12V/15V/20V 必须由 UI 再次确认，避免换电源后自动继承上一次高压诱骗。
- PD 请求超时、Reject、Wait 会保留 `FAIL` 状态和真正失败的目标电压，不会被用户刚排队的新目标覆盖，便于现场判断失败阶段；内部高压偏好会恢复默认 5V，下一次 Source_Capabilities 不会继续自动请求失败高压。
- Contract ready 后会发 SOP' Discover Identity 请求，用于读取 E-marker 线缆电流能力、速度等级和线缆类型。

## QC 检测怎么做

QC 走 DP/DM，和 USBFS 设备栈冲突，所以固件不能保留 USB CDC/debug 代码路径。

原理图给了两类能力：

- `PA12/UDP`、`PA11/UDM` 可用 CH32 内部 BC/QC 辅助功能给 DP/DM 加偏置。
- `PB0/ADC8`、`PB1/ADC9` 接到同一 DP/DM 网络，可读回真实 DP/DM 电压。

建议检测流程：

1. 确保当前没有 PD 高压 contract；PD 优先级高于 QC。
2. DP/DM 先设为 Hi-Z。
3. 通过 ADC 读取 DP/DM 空闲电压，记录基础状态。
4. 做 BC1.2/DCP 探测：给一侧加 0.6V 偏置，读另一侧是否跟随，判断是否短接/充电口。
5. QC 不能只靠被动读电平判断，必须主动握手并用 INA226 看 VBUS 是否变化。
6. 若用户只是查看协议，主动探测后应立即退回 5V；若用户确认诱骗，则保持目标电压。

当前落地状态：

- USB CDC/debug 代码已删除，PA11/PA12 只留给 DP/DM 直通、QC 控制和采样；产品代码不启用 USBFS 设备栈。
- `bsp_dpdm` 已从粗略 `DP/DM/BOTH` 扩展为每根线可配置 `Hi-Z/0V/0.6V/3.3V`。
- `PB0/ADC8`、`PB1/ADC9` 已接入 DP/DM 采样，snapshot 会记录 `dp_mv/dm_mv`。
- PD 合约活动时，QC 页面确认目标会走 PD 请求路径，不会再驱动 DP/DM legacy 诱骗，避免 PD/QC 同时争用。
- `service_legacy_charge_poll()` 已按 50ms 任务周期发布 `AVAILABLE/REQUESTING/READY/FAILED`。
- QC2 固定档位和 QC3 200mV step 都走统一 legacy service，请求成功与否以 INA226 VBUS 闭环判断。
- QC/QC3 高压请求超时失败后会释放 DP/DM 到 Hi-Z，清掉 active requested protocol，同时短暂保留 `FAIL` 快照给 UI 刷新；保持窗口结束后会发布 `NONE` 清屏，避免失败态或旧诱骗电平跨连接残留。
- 无主动请求时只能做保守的 DP/DM 电平观察，UI 显示为 `OTHER/AVAIL`，不再把被动 DP/DM 偏置直接判成 QC。
- 触发页的自动模式不会把 `OTHER` 当作 QC 直接驱动 DP/DM；未知协议只设置 PD 偏好，真正 QC 诱骗必须在 QC 专页确认，避免把普通 VBUS/被动偏置误触发成 QC。
- 真正 QC 是否升压以 INA226 读回 VBUS 是否接近目标作为准；确认成功后才显示 QC `READY`。
- 协议快照由 `app_protocol_arbiter` 仲裁：PD 优先；PD 空闲 `NONE` 不会擦掉真实 legacy/QC 状态，但 `cc_attached/cc_orientation` 已经有效时会压住被动 DP/DM 造成的 `OTHER`，避免 PD/Type-C 源被误显示成 OTHER。
- DP/DM 采样会临时打开比较器，采样后恢复原来的 BC 0.6V 源，避免破坏 QC2/QC3 保持电平。

## QC 诱骗怎么做

需要扩展 `bsp_dpdm`，不要只做 `DP/DM/BOTH` 三个模式，而是能设置每根线的目标电平：

| DP/DM 状态 | 建议实现方式 |
| --- | --- |
| Hi-Z | GPIO 输入浮空，关闭 BC source |
| 0V | GPIO 输出低或下拉 |
| 0.6V | CH32 内部 `UPD/UDM_BC_VSRC` |
| 3.3V | GPIO 输出高 |
| 采样 | PB0/PB1 ADC 读取实际 DP/DM |

QC2 常见固定电压用 DP/DM 电平组合触发；QC3 用 DP/DM 脉冲升降压。实际实现必须用 INA226 做闭环确认：

1. 设置 DP/DM 初始握手状态。
2. 等待协议规定的保持时间。
3. 设置目标电压对应的 DP/DM 状态或 QC3 脉冲。
4. 每 20-50ms 读取 INA226 VBUS。
5. VBUS 到达目标范围后显示成功。
6. 超时则释放 DP/DM 请求电平、标记失败，并等待用户重新确认。

## 实现优先级

已落地：

1. USB 串口代码已删除，避免 PA11/PA12 和 QC/USB 直通冲突。
2. PD Source_Capabilities 列表已进入 snapshot。
3. 按钮目标电压选择已接 PD 固定 PDO Request 和 PPS APDO Request，PDO 页支持 20mV 细调 PPS 目标，并按 E-Marker/默认 3A 线缆能力限制请求电流。
4. `bsp_dpdm` 已支持 DP/DM 线级状态控制和 PB0/PB1 ADC 采样。
5. QC2 5V/9V/12V/20V 固定电压组合已接入，使用 INA226 VBUS 做成功/失败判断。
6. QC3 200mV step 脉冲路径已接入；UI 选择非 QC2 固定档位时走 QC3。
7. 160x80 UI 已扩展为 9 个页面：主页、曲线、协议、触发、PDO、QC、CC、线缆、设置。
8. 设置页保留亮度、旋转、TRIG 手动/自动；USB CDC/debug 入口已从正式 UI 删除。
9. CC 附着状态已进入 snapshot 和 UI，CC 已连接但尚未形成 PD 合约时显示 Type-C/CC 状态，不再被 VBUS 兜底逻辑强制标成 OTHER。

后续未做：

- AFC/FCP 的真实波形握手，目前只保留业务枚举和基础 DP/DM 模式。

安全限制：PX1 的 VBUS_IN 到 VBUS_OUT 是通过 5mR 采样电阻直通的，诱骗升压会直接影响下游设备。做诱骗功能时 UI 必须要求用户确认，默认开机只检测不升压。
