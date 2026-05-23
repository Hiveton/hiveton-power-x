# PowerX Host Qt

PowerX Host Qt 是 Hiveton PowerX USB 功率计的上位机前端原型，使用 Qt Widgets 开发。

## 功能

- 深色工程仪表盘界面，按生成设计稿的布局还原。
- 实时电压、电流、功率、能量和温度卡片。
- 自绘实时曲线，支持电压/电流/功率可见性切换。
- PD 协议时间线、PDO/APDO、E-Marker、统计面板。
- 设备档案、触发规则、采集会话、PD 报文四类数据的增删改查。
- CSV 导出。
- 预留 `UsbTransport` 接口，当前默认使用 `MockUsbTransport` 产生模拟数据。

## 构建

```bash
cmake -S PowerXHostQt -B PowerXHostQt/build -DCMAKE_PREFIX_PATH=/opt/homebrew
cmake --build PowerXHostQt/build
```

## 运行

```bash
PowerXHostQt/build/PowerXHostQt.app/Contents/MacOS/PowerXHostQt
```

截图模式：

```bash
PowerXHostQt/build/PowerXHostQt.app/Contents/MacOS/PowerXHostQt --screenshot PowerXHostQt/artifacts/dashboard.png
```
