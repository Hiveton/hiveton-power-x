# Hiveton Power X

`Hiveton Power X` 是一套基于 `CH32L103K8U6` 的 USB 电源检测仪固件工程，目标是实现一款类似 `Power-Z` 的便携式 USB/Type-C 电源测试设备。

当前仓库已经包含：

- `CH32L103K8U6 + FreeRTOS` 的主工程框架
- 面向 PX1 硬件的 `BSP / Service / UI / App` 分层代码
- 基于 INA226 的电压、电流、功率测量链路
- `PD` 监听与请求、`E-Marker` 摘要、`QC/AFC/FCP` 传统快充诱骗骨架
- `0.96" ST7735S` 彩屏的低内存 UI 渲染框架
- 原理图、原厂 `EVT` 例程、屏幕参考代码、设计规格和实施计划

项目当前仍处于 `bring-up + MVP 开发` 阶段，已经完成软件架构搭建、160x80 产品 UI、主要协议链路和命令行验证脚本，后续重点是实机下载、板级验证和协议兼容性收尾。

## 项目状态总览

| 模块 | 当前状态 | 说明 |
| --- | --- | --- |
| 测量框架 | 已接入 | 当前使用 INA226 `0x40`，R3 `5mR`，输出 VBUS/电流/功率 snapshot |
| 纹波 | MVP | 当前为趋势级估算，不是示波器级 |
| PD | 已接入 | 已有监听、固定 PDO/PPS APDO 请求、PS_RDY 状态、SOP' E-marker 请求 |
| QC/AFC/FCP | 部分接入 | QC2/QC3 请求和 DP/DM 采样已接入，AFC/FCP 保留枚举与扩展位 |
| E-Marker | 基础支持 | 解析线缆电流能力、速度等级和线缆类型摘要，并用于 PD 请求电流限流 |
| LCD/UI | 已接入 | `ST7735S + line buffer`，9 页 160x80 产品 UI 已成型 |
| 按键交互 | 已接入 | 3 键导航、action mode、诱骗页选档/确认、设置页已落地 |
| 文档 | 已建立 | 原理图、规格、计划、bring-up 模板都已入库 |

PD 高压请求不会跨连接自动继承：Type-C detach 或 PD SoftReset 后会回到默认 5V，用户需要在 UI 再次确认才会请求 9V/12V/15V/20V。
PDO 页 action mode 支持 20mV 步进微调 PPS 目标电压；进入编辑时会先同步当前 PD 目标/合约电压，确认后按该目标发 PD/PPS Request。
PD 高压请求被 Reject/Wait 或超时失败后也会恢复默认 5V 偏好，同时保留真正失败的目标给 UI 显示。
PD `PS_RDY` 后还会等待 INA226 实测 VBUS 到达目标范围，确认后才把请求状态标成 `READY`。
PD VBUS 校验阶段会把新的目标请求排队，等当前电压确认完成后再发下一次 Request，避免连续诱骗打断当前合约确认。
PD 合约活动时，QC 页确认不会驱动 DP/DM legacy 诱骗，目标电压会按 PD 请求处理，避免 PD/QC 同时争用。
QC 高压请求失败后也会释放 DP/DM 到 Hi-Z，短暂保留 `FAIL` 快照给 UI 刷新，随后发布 `NONE` 清屏，避免下一次接入电源时沿用旧的诱骗电平或旧失败态。

## 1. 项目目标

这个项目要做的是一套完整的 USB 检测仪固件，面向以下典型能力：

- 实时测量 `VBUS 电压 / 电流 / 功率`
- 提供 `趋势级纹波` 观察能力
- 监测 `Type-C / PD / QC / AFC / FCP`
- 支持主动诱骗，请求目标电压档位
- 读取线缆 `E-Marker` 关键摘要信息
- 在小尺寸彩屏上提供低 RAM 占用的 UI

项目第一阶段偏向 `MVP`，强调：

- 开机即用
- 数值可快速读取
- 协议状态清晰
- 框架可扩展
- 在 `CH32L103K8U6` 资源约束下尽量保持模块边界清楚

## 2. 硬件平台

主控与主要硬件条件如下：

- MCU：`CH32L103K8U6`
- Type-C/PD：使用芯片自带 `USBPD` 硬件
- 屏幕：`0.96 寸 LCD 彩屏`，驱动 `ST7735S`
- 交互：`3 个按键`
- 软件框架：基于你现有的 `FreeRTOS` 工程继续开发

当前代码已经按原理图收口的关键板级映射集中在：

- [`bsp_board_config.h`](/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/FreeRTOS/Bsp/include/bsp_board_config.h)

其中包括：

- `LCD_SPI_DC / RST / CS / SCK / MOSI`
- `LCD_BL_PWM`
- `BTN_1 / BTN_2 / BTN_3`
- `CC1 / CC2`
- `USB_DP / USB_DM`
- `VBUS / CURRENT ADC`
- `CC1_EXT_RD_CTL`

## 3. 当前功能范围

### 已经落地的部分

- `FreeRTOS` 多任务骨架
- `INA226` 测量服务
- `PD` 快照模型与基础协商流程
- `legacy charge` 请求模型
- `ST7735S` 硬件 SPI / DMA 像素推送框架
- 主页、曲线、协议、触发、PDO、QC、CC、线缆、设置 9 页低内存渲染路径
- 主机侧单元测试
- 命令行 host 测试脚本和 `wchisp` 下载脚本

### 当前重点中的部分

- `QC/AFC/FCP` 真实电平时序细化
- `PD` 更完整的实机协商闭环
- 实机下载、页面验证和板级 bring-up

### 暂未完成或仍保守处理的部分

- 高可信绝对纹波测量
- 完整传统快充被动识别兼容性
- 上位机通信协议，目前 USB CDC 已删除以避免占用 PA11/PA12
- 量产级校准与出厂参数流程

## 4. 软件架构

工程按 `BSP -> Service -> UI -> App` 分层组织。

### 架构总览

```mermaid
flowchart TD
    A["ADC / USBPD / DPDM / Keys / LCD"] --> B["BSP"]
    B --> C["Service"]
    C --> D["Snapshot / State"]
    D --> E["App Tasks"]
    E --> F["UI"]
    F --> G["ST7735S LCD"]
```

其中每层关注点如下：

- `BSP`：只负责访问硬件能力
- `Service`：负责测量算法、协议状态机、请求模型
- `App`：在 FreeRTOS 任务中组织系统行为
- `UI`：只消费快照并渲染页面，不直接操控底层协议

### `Bsp`

职责是板级驱动和外设抽象，不直接承载产品策略。

主要文件：

- [`bsp_adc_dma.c`](/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/FreeRTOS/Bsp/bsp_adc_dma.c)
- [`bsp_usbpd_port.c`](/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/FreeRTOS/Bsp/bsp_usbpd_port.c)
- [`bsp_dpdm.c`](/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/FreeRTOS/Bsp/bsp_dpdm.c)
- [`bsp_lcd_st7735.c`](/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/FreeRTOS/Bsp/bsp_lcd_st7735.c)
- [`bsp_backlight.c`](/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/FreeRTOS/Bsp/bsp_backlight.c)
- [`bsp_keys.c`](/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/FreeRTOS/Bsp/bsp_keys.c)
- [`bsp_cc_ext_rd.c`](/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/FreeRTOS/Bsp/bsp_cc_ext_rd.c)

设计原则：

- 尽量只提供“硬件能力”
- 避免把产品逻辑写进驱动层
- 所有板级映射统一收口到 `bsp_board_config.h`

### `Service`

职责是协议、测量、快照和算法逻辑，是 UI 与驱动之间的业务层。

主要文件：

- [`service_measure.c`](/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/FreeRTOS/Service/service_measure.c)
- [`service_pd.c`](/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/FreeRTOS/Service/service_pd.c)
- [`service_legacy_charge.c`](/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/FreeRTOS/Service/service_legacy_charge.c)
- [`service_emark.c`](/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/FreeRTOS/Service/service_emark.c)
- [`service_protocol_snapshot.h`](/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/FreeRTOS/Service/include/service_protocol_snapshot.h)

设计原则：

- 使用轻量状态机
- 用快照结构给 UI 提供只读数据
- 尽量保留主机侧可测试性

### `UI`

职责是页面模型、低内存绘制和按键驱动下的页面行为。

主要文件：

- [`ui_model.c`](/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/FreeRTOS/UI/ui_model.c)
- [`ui_pages.c`](/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/FreeRTOS/UI/ui_pages.c)
- [`ui_renderer.c`](/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/FreeRTOS/UI/ui_renderer.c)
- [`ui_widgets.c`](/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/FreeRTOS/UI/ui_widgets.c)

UI 设计重点：

- 不使用全屏 framebuffer
- 采用 `line buffer + 分区刷新`
- 默认面向“快速看数值”
- 兼顾协议页和诱骗页

### `App`

职责是把测量、协议、UI 和系统任务拼起来，形成最终产品行为。

主要文件：

- [`app_controller.c`](/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/FreeRTOS/App/app_controller.c)
- [`app_tasks.c`](/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/FreeRTOS/App/app_tasks.c)
- [`app_trigger_control.c`](/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/FreeRTOS/App/app_trigger_control.c)

当前任务模型大致是：

- `measure_task`
- `protocol_task`
- `legacy_charge_task`
- `ui_task`

### 任务关系

```mermaid
flowchart LR
    M["measure_task"] --> S1["measure_snapshot"]
    P["protocol_task"] --> S2["protocol_snapshot"]
    L["legacy_charge_task"] --> S2
    U["ui_task"] --> S1
    U --> S2
    U --> LCD["LCD Render"]
```

## 5. 代码目录结构

仓库根目录的主要结构如下：

```text
HivetonPowerX/
├── README.md
├── PowerXCode/
│   ├── FreeRTOS/
│   │   ├── App/          # 应用层与任务组织
│   │   ├── Assets/       # 字体等静态资源
│   │   ├── Bsp/          # 板级驱动
│   │   ├── FreeRTOS/     # 内核源码
│   │   ├── Ld/           # 链接脚本
│   │   ├── Service/      # 测量/协议/快充业务层
│   │   ├── Startup/      # 启动文件
│   │   ├── Tests/        # 主机侧测试
│   │   ├── UI/           # 页面模型与渲染
│   │   ├── User/         # 工程入口、配置、中断
│   │   └── obj/          # MRS 工程生成的构建目录
│   └── SRC/              # CH32L103 外设库与底层源码
├── tools/
│   ├── run_host_tests.sh # 主机侧测试一键入口
│   ├── check_wch_isp_device.sh # WCH ISP USB 枚举预检
│   ├── flash_powerx.sh   # wchisp 探测与烧录入口
│   └── run_powerx_real_device_closure.sh # 实机下载与最终闭环入口
└── docs/
    ├── SCH_HivetonPX1_2026-04-03.pdf
    ├── EVT/              # 原厂参考例程
    ├── 0.96IPS.../       # 屏幕资料与参考代码
    ├── board-bringup-template.md
    └── superpowers/
        ├── specs/        # 设计规格
        └── plans/        # 实施计划
```

## 6. 关键数据流

系统的主要数据流可以概括成：

1. `BSP` 从 ADC、USBPD、DPDM、按键、LCD 等外设取数或发命令
2. `Service` 把原始输入变成测量结果、协议状态和诱骗请求
3. `App` 在任务里协调这些服务并发布共享快照
4. `UI` 消费快照并绘制到 ST7735S 屏幕

可以简单理解为：

```text
ADC/USBPD/DPDM/KEY
        ↓
       BSP
        ↓
    Service Layer
        ↓
   Snapshot / State
        ↓
        UI
        ↓
      LCD
```

其中最重要的几个共享模型是：

- `measure_snapshot_t`
- `protocol_snapshot_t`，包含 `kind/request_state`、固定 PDO、PPS APDO 范围、E-Marker、DP/DM 采样和 `cc_attached/cc_orientation`
- `legacy_charge_request_t`
- `ui_model_state_t`

## 7. 诱骗控制逻辑

当前诱骗页已经有一条最小闭环：

1. `BTN_1 / BTN_3` 在 `5V / 9V / 12V / 15V / 20V` 之间切换
2. `BTN_2` 依据当前协议类型下发目标请求
3. `PD` 目标电压走 `service_pd`
4. `QC/AFC/FCP` 目标电压走 `service_legacy_charge`

关键入口文件：

- [`app_trigger_control.c`](/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/FreeRTOS/App/app_trigger_control.c)
- [`ui_model.c`](/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/FreeRTOS/UI/ui_model.c)
- [`ui_renderer.c`](/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/FreeRTOS/UI/ui_renderer.c)

## 8. UI 与交互

当前 UI 主要围绕 3 键工作：

- `BTN_1`：翻页或在诱骗页中减档
- `BTN_2`：确认；长按进入/退出设置 action mode
- `BTN_3`：翻页或在诱骗页中加档

按键事件语义：短按在松手时上报，长按达到阈值时上报且不再夹带短按。BTN2/PB9 与 BTN3/PA15 由 EXTI 捕获，BTN1/PB15 与 BTN3 共用 EXTI15，硬件限制下由 20 ms 扫描兜底。

当前页面顺序：

- `Main`：大数值主视图
- `Scope`：电压/电流/功率趋势和 min/max
- `Protocol`：协议状态与合同信息
- `Trigger`：目标电压预置与主动诱骗
- `PDO`：固定 PDO 列表、PPS 范围和选择状态
- `QC`：QC 目标、DP/DM 电压和请求状态
- `CC`：CC1/CC2 状态和方向
- `Cable`：E-marker 电流能力和线缆摘要
- `Settings`：亮度、旋转、TRIG 手动/自动

诱骗页目前已经支持 `5V / 9V / 12V / 15V / 20V` 预置选择，并能根据当前协议类型把请求分发到 `PD` 或 `legacy charge` 服务层。

## 9. 推荐阅读顺序

如果你刚接手这个项目，建议按下面顺序建立上下文：

1. 先看原理图和板级配置，确认硬件资源
2. 再看 `app_tasks.c`，理解系统任务是怎么拼起来的
3. 然后看 `service_measure.c / service_pd.c / service_legacy_charge.c`
4. 最后看 `ui_model.c / ui_renderer.c`，理解页面和交互

对应文件入口：

- [`SCH_HivetonPX1_2026-04-03.pdf`](/Users/hiveton/HivetonCode/HivetonPowerX/docs/SCH_HivetonPX1_2026-04-03.pdf)
- [`bsp_board_config.h`](/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/FreeRTOS/Bsp/include/bsp_board_config.h)
- [`app_tasks.c`](/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/FreeRTOS/App/app_tasks.c)
- [`service_measure.c`](/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/FreeRTOS/Service/service_measure.c)
- [`service_pd.c`](/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/FreeRTOS/Service/service_pd.c)
- [`service_legacy_charge.c`](/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/FreeRTOS/Service/service_legacy_charge.c)
- [`ui_renderer.c`](/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/FreeRTOS/UI/ui_renderer.c)

## 10. 参考资料与设计文档

仓库内的重要资料：

- 原理图：
  - [`SCH_HivetonPX1_2026-04-03.pdf`](/Users/hiveton/HivetonCode/HivetonPowerX/docs/SCH_HivetonPX1_2026-04-03.pdf)
- 板级 bring-up 模板：
  - [`board-bringup-template.md`](/Users/hiveton/HivetonCode/HivetonPowerX/docs/board-bringup-template.md)
- MVP 设计规格：
  - [`2026-04-04-hiveton-px1-mvp-design.md`](/Users/hiveton/HivetonCode/HivetonPowerX/docs/superpowers/specs/2026-04-04-hiveton-px1-mvp-design.md)
- MVP 实施计划：
  - [`2026-04-04-hiveton-px1-mvp.md`](/Users/hiveton/HivetonCode/HivetonPowerX/docs/superpowers/plans/2026-04-04-hiveton-px1-mvp.md)
- 原厂参考代码：
  - `docs/EVT`
- 屏幕参考代码与资料：
  - `docs/0.96IPS京东方焊接13pin-ST7735S技术资料`

## 11. 构建与开发说明

当前主工程是：

- `PowerXCode/FreeRTOS`

工程特点：

- 使用 `MRS` 工程文件
- `obj/` 目录中保存工程生成的构建脚本与输出
- 部分主机侧测试可以直接用本机 `cc` 编译运行

当前仓库更适合这样理解：

- `交叉编译与下载`：在本地 IDE / 工具链环境中完成
- `业务逻辑验证`：优先通过 `Tests/` 下的主机侧测试推进

当前已经存在的主机侧测试包括：

- `test_app_protocol_arbiter`
- `test_measure_service`
- `test_protocol_snapshot`
- `test_legacy_charge_and_emark`
- `test_ui_model`
- `test_ui_pages`
- `test_app_trigger_control`
- `test_app_ui_navigation`
- `test_bsp_adc_config`
- `test_bsp_keys_polarity`
- `test_bsp_lcd_rotation`
- `test_ui_scope_model`
- `test_ui_value_format`

一键运行：

```bash
tools/run_host_tests.sh
```

逐项目标覆盖审计：

```bash
tools/check_powerx_objective_coverage.sh --software-only artifacts/hardware-validation/latest.md
```

### IDE 构建

当前默认开发方式是使用 `MRS` 打开：

- [`PowerXCode/FreeRTOS/.project`](/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/FreeRTOS/.project)
- [`PowerXCode/FreeRTOS/.cproject`](/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/FreeRTOS/.cproject)

构建输出目录：

- `PowerXCode/FreeRTOS/obj`

主要构建产物名称：

- `FreeRTOS.elf`
- `FreeRTOS.hex`
- `FreeRTOS.lst`
- `FreeRTOS.siz`

### 命令行构建

当前工程的自动生成 `makefile` 位于：

- [`PowerXCode/FreeRTOS/obj/makefile`](/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/FreeRTOS/obj/makefile)

从当前仓库已知信息来看，命令行构建依赖以下工具链：

- `riscv-none-embed-gcc`
- `riscv-none-embed-objcopy`
- `riscv-none-embed-objdump`
- `riscv-none-embed-size`

在工具链已正确安装的前提下，常见构建入口是：

```bash
PATH="/Applications/MounRiver Studio 2.app/Contents/Resources/app/resources/darwin/components/WCH/Toolchain/RISC-V Embedded GCC/bin:$PATH" \
  make -C PowerXCode/FreeRTOS/obj -j10 all
```

清理命令：

```bash
cd PowerXCode/FreeRTOS/obj
make clean
```

### 当前下载/烧录说明

先确认板子已经以 WCH ISP USB 设备枚举：

```bash
tools/check_wch_isp_device.sh
```

如果需要先启动检查、再手动让板子进 ISP，可以用等待模式：

```bash
tools/check_wch_isp_device.sh --wait 30
```

预检会更新 `artifacts/flash/isp-preflight.md`。只有看到 `4348:55e0` 或 `1a86:55e0`，才说明板子已经进入 WCH ISP/BOOT 下载模式，可以继续烧录。PX1 手动进入 ISP 时按住 `BTN2/BOOT` 再重新插入 USB；原理图中 `BTN2 (ISP)` 连接 `PB9/BOOT0`，`PB2/BOOT1` 也在 MCU 上引出。

当前仓库已固化 `wchisp` 下载脚本：

```bash
tools/flash_powerx.sh
```

等待板子进入 ISP 后自动继续：

```bash
tools/flash_powerx.sh --wait 30
```

如果需要先开着命令再慢慢操作 `BTN2/BOOT` 和 USB-C，可以用无限等待模式；设备一枚举出来就会自动继续烧录：

```bash
tools/flash_powerx.sh --wait-forever
```

默认烧录：

- `PowerXCode/FreeRTOS/obj/FreeRTOS.bin`

脚本会先输出固件 SHA256，再执行 `wchisp info`。如果没有发现 `4348:55e0` 或 `1a86:55e0`，说明板子没有进入 WCH ISP/BOOT 下载模式，需要重新进入 ISP 后再执行。

每次执行下载脚本都会更新 `artifacts/flash/latest.md`，记录固件 SHA、`wchisp` 路径、USB 枚举、串口列表和本次下载尝试状态。只有烧录成功时，脚本才会额外更新 `artifacts/flash/latest-success.md`。硬件验收 gate 默认检查 `latest-success.md`，只有状态为 `flashed` 且固件 SHA 与当前 `FreeRTOS.bin` 一致时，`flash-wchisp` 才算通过。

硬件接入后一键闭环入口：

```bash
tools/run_powerx_real_device_closure.sh --wait 30
```

同样支持无限等待设备后继续完整闭环：

```bash
tools/run_powerx_real_device_closure.sh --wait-forever
```

这个脚本会串行执行 no-flash 软件 gate、逐项目标覆盖审计、WCH ISP 下载、硬件报告准备和最终目标审计。它不会伪造真机结论；如果 `artifacts/hardware-validation/latest.md` 还没有填入真实 PD/QC/按键/页面/稳定性结果，最终审计会继续失败。

完整目标验收还需要真机报告。先用当前固件 SHA 填写模板：

```bash
tools/prepare_powerx_hardware_validation_report.sh --force
```

完成亮屏、9 页 UI、按钮、PD/QC 检测诱骗、CC/线缆、设置和 5 分钟无卡死验证后，把报告里的对应项勾选为 `[x]`，并填写板号、烧录时间、PD/QC 电源型号、INA226 实测结果、线缆/E-marker、UI/按键/设置/稳定性记录，再执行：

```bash
tools/check_powerx_hardware_validation.sh artifacts/hardware-validation/latest.md
```

如果需要把真机报告纳入完整 gate：

```bash
tools/run_powerx_goal_gate.sh --no-flash --hardware-report artifacts/hardware-validation/latest.md
```

最终目标完成前可以直接跑总审计入口：

```bash
tools/check_powerx_goal_completion.sh artifacts/hardware-validation/latest.md
```

这个脚本会串联 no-flash 软件 gate、9 页 UI 预览报告、`artifacts/flash/latest-success.md` 成功烧录证据和硬件验收报告；任一环节缺失都会失败。`artifacts/flash/latest.md` 仍会作为最近一次下载尝试记录显示在总审计报告里，便于排查后续 ISP 退出或 USB 枚举失败。
每次执行后会更新 `artifacts/goal/latest.md`，里面有软件、UI、下载和真机报告四项状态。

### 常见构建问题

1. `fatal error: xxx.h: No such file or directory`
这类错误通常不是源码本身缺失，而是 `MRS` 工程没有重新读取 `.cproject`，先 `Clean` 再完整重编一次。

2. `riscv-none-embed-gcc: command not found`
说明本机还没有安装 CH32L103 对应的 RISC-V 工具链，当前命令行构建无法继续。

3. `Tests/*.c` 参与了嵌入式工程编译
这些测试是主机侧测试，不应该作为板级固件对象参与最终链接。如果 IDE 又重新生成了错误规则，需要重新检查 `.cproject` 的 source path 和过滤设置。

4. `UI` 或 `Bsp` 新增头文件找不到
优先检查 `.cproject` 中是否已经包含：

- `App/include`
- `Bsp/include`
- `Service/include`
- `UI/include`
- `Assets`

5. 板上运行但数值不可信
优先检查：

- [`bsp_board_config.h`](/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/FreeRTOS/Bsp/include/bsp_board_config.h)
- [`board-bringup-template.md`](/Users/hiveton/HivetonCode/HivetonPowerX/docs/board-bringup-template.md)

重点确认 `VBUS 分压比`、`CURRENT 零点/增益`、`LCD/按键/背光` 以及 `DP/DM` 模拟链路。

如果后续继续完善仓库文档，建议优先再补：

- IDE 构建步骤
- 命令行构建步骤
- 烧录步骤
- 板级 bring-up 检查表

## 12. 当前限制

这个仓库当前有一些已知边界，阅读代码时建议先知道：

- `电压/电流/功率` 目前来自 INA226，仍需要实机校准确认绝对精度
- `纹波` 当前是趋势级，不是示波器级测量
- `legacy charge` 的 QC2/QC3 路径已接入，AFC/FCP 仍以可扩展框架和请求模型为主
- `E-Marker` 当前以摘要信息为主
- `obj/` 目录带有工程生成文件，后续是否继续纳入版本管理可以再收敛

## 13. 后续方向

接下来的主要工作会集中在：

- 板级真实参数补齐与校准
- `PD/QC/AFC/FCP` 实机调试
- 统计页与更多 UI 细节
- 协议快照合并策略完善
- 下载模式和实机验证流程固化

## 14. Bring-up 建议顺序

如果现在要继续把固件从“框架可跑”推进到“板上可用”，建议按这个顺序推进：

1. 先确认 `LCD / 背光 / 按键` 板级行为
2. 再确认 `VBUS / CURRENT` ADC 工程量换算
3. 接着验证 `PD attach / Source Cap / Request / PS_RDY`
4. 然后补 `QC/AFC/FCP` 的真实对端兼容性
5. 最后做 UI 细节、统计页和整体体验收尾

对应参考文档：

- [`board-bringup-template.md`](/Users/hiveton/HivetonCode/HivetonPowerX/docs/board-bringup-template.md)

---

如果你是第一次进入这个仓库，建议从这几个文件开始看：

- [`bsp_board_config.h`](/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/FreeRTOS/Bsp/include/bsp_board_config.h)
- [`app_tasks.c`](/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/FreeRTOS/App/app_tasks.c)
- [`service_measure.c`](/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/FreeRTOS/Service/service_measure.c)
- [`service_pd.c`](/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/FreeRTOS/Service/service_pd.c)
- [`ui_renderer.c`](/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/FreeRTOS/UI/ui_renderer.c)

这样能最快建立对这个项目“硬件、协议、任务、UI”四条主线的整体理解。
