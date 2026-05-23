# PX1 硬件验证报告模板

Firmware SHA256: c727a47c57545ba8a523f4bf1d2b698a3e175ced8ea09a2fe4c5c0d0a7d12e5e

## 必须逐项真机确认

- [x] flash-wchisp
- [ ] boot-backlight
- [ ] lcd-main-no-garble
- [ ] ui-all-9-pages
- [ ] buttons-browse-and-action
- [ ] pd-detect-source-cap
- [ ] pd-trigger-request-vbus
- [ ] qc-detect-dpdm
- [ ] qc-trigger-request-vbus
- [ ] cc-orientation
- [ ] cable-emarker
- [ ] settings-brightness-rotation-mode
- [ ] stability-no-freeze-5min

## 记录

- Flash evidence: artifacts/flash/latest-success.md
- 板子编号: PX1-
- 烧录时间: 2026-05-19 13:19:22 +0800
- 电源/PD 充电器型号: 品牌/型号，需支持 PD Source Cap
- QC 充电器型号: 品牌/型号，需支持 QC2/QC3
- PD 请求目标与 INA226 实测: Source_Cap PDO=5V/9V/15V/20V；目标 9V/12V/15V/20V；实测 VBUS=__V，I=__A，P=__W
- QC 请求目标与 INA226 实测: QC2/QC3 模式；目标 9V/12V/20V；DP=__V，DM=__V，VBUS=__V，I=__A
- 线缆/E-marker 结果: CC1/CC2 方向；E-marker 电流 __A，线缆类型/速度
- UI 页面确认: main/scope/protocol/trigger/pdo/qc/cc/cable/settings 9页逐页无花屏
- 按键确认: BTN1 上/下切页，BTN2 确认/action，BTN3 上/下切页
- 设置确认: brightness 亮度、rotation 旋转、TRIG AUTO/MAN 都已切换
- 5分钟稳定性: 连续运行 5 分钟，无卡死/无花屏/按键仍响应
- 证据文件: artifacts/hardware-validation/evidence/ui-pages.txt; artifacts/hardware-validation/evidence/pd-qc-trigger.txt; artifacts/hardware-validation/evidence/buttons-settings.txt; artifacts/hardware-validation/evidence/cc-cable-emarker.txt
- 异常现象: 如无异常填写 none

## 证据文件内容要求

- ui-pages.txt: 记录 main/scope/protocol/trigger/pdo/qc/cc/cable/settings 9 页逐页看到且为 160x80。
- pd-qc-trigger.txt: 记录 PD Source_Cap/PDO、请求目标电压、VBUS/电流实测、QC2/QC3 模式、DP/DM、QC VBUS/电流。
- buttons-settings.txt: 记录 BTN1/BTN2/BTN3 行为，以及 brightness/rotation/TRIG 设置切换。
- cc-cable-emarker.txt: 记录 CC1/CC2 方向、E-marker 检测结果、线缆电流/类型。
