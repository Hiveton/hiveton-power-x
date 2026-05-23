#include "DashboardWindow.h"

#include "RecordDialog.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QFile>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QSplitter>
#include <QSpinBox>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTextEdit>
#include <QVBoxLayout>

namespace {
QFrame *panel(const QString &title = {}) {
    auto *frame = new QFrame();
    frame->setObjectName("panel");
    auto *layout = new QVBoxLayout(frame);
    layout->setContentsMargins(14, 12, 14, 12);
    layout->setSpacing(10);
    if (!title.isEmpty()) {
        auto *label = new QLabel(title);
        label->setObjectName("panelTitle");
        layout->addWidget(label);
    }
    return frame;
}

QVBoxLayout *panelLayout(QWidget *widget) {
    return qobject_cast<QVBoxLayout *>(widget->layout());
}

QTableWidgetItem *item(const QString &text, const QString &id = {}) {
    auto *tableItem = new QTableWidgetItem(text);
    if (!id.isEmpty()) tableItem->setData(Qt::UserRole, id);
    tableItem->setFlags(tableItem->flags() & ~Qt::ItemIsEditable);
    return tableItem;
}

QString yesNo(bool value, bool zh) {
    return zh ? (value ? "是" : "否") : (value ? "Yes" : "No");
}
}

DashboardWindow::DashboardWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("PowerX USB Meter Host");
    resize(1440, 900);
    setMinimumSize(1120, 720);
    applyTheme();
    setCentralWidget(buildRoot());

    connect(&m_transport, &MockUsbTransport::connectionChanged, this, [this](const QString &state) {
        m_connected = state == "Connected";
        if (m_connection) {
            m_connection->setText(m_connected ? text("status_live") : text("status_offline"));
        }
        updateActionState();
    });
    connect(&m_transport, &MockUsbTransport::sampleReceived, this, &DashboardWindow::onSample);
    connect(&m_transport, &MockUsbTransport::pdMessageReceived, this, &DashboardWindow::onPdMessage);
    connect(&m_transport, &MockUsbTransport::transportLog, this, &DashboardWindow::appendLog);
    connect(&m_store, &DataStore::changed, this, &DashboardWindow::refreshTables);

    m_store.load();
    refreshTables();
    appendLog(text("log_ready"));
}

QString DashboardWindow::text(const QString &key) const {
    static const QMap<QString, QString> en = {
        {"app_title", "PowerX USB Meter"},
        {"brand", "PowerX\nUSB Meter"},
        {"dashboard", "Dashboard"},
        {"pd", "PD Analyzer"},
        {"pdo", "PDO / APDO"},
        {"emarker", "E-Marker"},
        {"triggers", "Triggers"},
        {"sessions", "Sessions"},
        {"settings", "Settings"},
        {"connect", "Connect"},
        {"disconnect", "Disconnect"},
        {"record", "Record"},
        {"stop", "Stop"},
        {"clear", "Clear"},
        {"refresh", "Refresh"},
        {"identify", "Read Identity"},
        {"apply", "Apply"},
        {"export", "Export CSV"},
        {"status_live", "Live / USB Ready"},
        {"status_offline", "Offline / Mock Ready"},
        {"reserved", "HID / CDC / Vendor USB endpoint reserved"},
        {"voltage", "Voltage"},
        {"current", "Current"},
        {"power", "Power"},
        {"energy", "Energy"},
        {"temp", "Temp"},
        {"waveform", "Real-time Waveform"},
        {"wave_sub", "Voltage / Current / Power rolling capture"},
        {"pd_card", "USB-C PD Analyzer"},
        {"emarker_card", "E-Marker"},
        {"layers", "Chart Layers"},
        {"log", "Transport Log"},
        {"records", "Records"},
        {"add", "Add"},
        {"edit", "Edit"},
        {"delete", "Delete"},
        {"reset", "Reset Demo"},
        {"devices", "Devices"},
        {"pd_messages", "PD Messages"},
        {"pdo_title", "Advertised Power Profiles"},
        {"settings_title", "Application Settings"},
        {"device_commands", "Device Commands"},
        {"language", "Language"},
        {"sampling", "Sampling"},
        {"sample_rate", "Sample Rate"},
        {"usb_endpoint", "USB Endpoint"},
        {"storage", "Storage"},
        {"recording_state", "Recording State"},
        {"connection_state", "Connection State"},
        {"mock_mode", "Mock transport is active until firmware USB protocol is connected."},
        {"log_ready", "PowerX Host initialized. USB transport layer is reserved and currently runs in mock mode."},
        {"capture_note", "Live capture"},
        {"csv_title", "Export PD Messages"},
        {"delete_confirm", "Delete selected record?"},
        {"no_selection", "Select a row first."},
        {"csv_done", "CSV exported"},
        {"csv_failed", "CSV export failed"},
        {"connected_log", "Device connected"},
        {"disconnected_log", "Device disconnected"},
        {"command_refresh_pdo", "Request PDO/APDO refresh"},
        {"command_identity", "Request cable identity"},
        {"headers_device", "ID|Name|Serial|Firmware|Port|Mode|Note"},
        {"headers_trigger", "ID|Name|Metric|Condition|Threshold|Action|Enabled"},
        {"headers_session", "ID|Name|Started|Rate|Export|Note"},
        {"headers_message", "ID|Time|Direction|Type|SOP|MsgID|Raw|Decoded"},
        {"headers_pdo", "Type|Voltage|Current|Min|Max|Power|Flags"},
        {"role", "Role"},
        {"cable", "Cable"},
        {"vendor", "Vendor"},
        {"product", "Product"},
        {"speed", "Speed"},
        {"negotiated", "Negotiated"},
        {"samples_live", "Live samples update the dashboard, PD packet table and profile pages."}
    };
    static const QMap<QString, QString> zh = {
        {"app_title", "PowerX USB 功率计"},
        {"brand", "PowerX\nUSB 功率计"},
        {"dashboard", "仪表盘"},
        {"pd", "PD 协议分析"},
        {"pdo", "PDO / APDO"},
        {"emarker", "E-Marker 线缆"},
        {"triggers", "触发规则"},
        {"sessions", "采集会话"},
        {"settings", "设置"},
        {"connect", "连接"},
        {"disconnect", "断开"},
        {"record", "录制"},
        {"stop", "停止"},
        {"clear", "清空"},
        {"refresh", "刷新"},
        {"identify", "读取身份"},
        {"apply", "应用"},
        {"export", "导出 CSV"},
        {"status_live", "实时 / USB 就绪"},
        {"status_offline", "离线 / 模拟就绪"},
        {"reserved", "已预留 HID / CDC / Vendor USB 通信接口"},
        {"voltage", "电压"},
        {"current", "电流"},
        {"power", "功率"},
        {"energy", "电能"},
        {"temp", "温度"},
        {"waveform", "实时波形"},
        {"wave_sub", "电压 / 电流 / 功率滚动采集"},
        {"pd_card", "USB-C PD 协议分析"},
        {"emarker_card", "E-Marker 线缆信息"},
        {"layers", "曲线图层"},
        {"log", "传输日志"},
        {"records", "数据记录"},
        {"add", "新增"},
        {"edit", "编辑"},
        {"delete", "删除"},
        {"reset", "重置演示数据"},
        {"devices", "设备"},
        {"pd_messages", "PD 报文"},
        {"pdo_title", "电源能力档位"},
        {"settings_title", "应用设置"},
        {"device_commands", "设备命令"},
        {"language", "语言"},
        {"sampling", "采样"},
        {"sample_rate", "采样率"},
        {"usb_endpoint", "USB 接口"},
        {"storage", "数据存储"},
        {"recording_state", "录制状态"},
        {"connection_state", "连接状态"},
        {"mock_mode", "当前使用模拟传输，固件 USB 协议接入后可直接替换。"},
        {"log_ready", "PowerX Host 已初始化，USB 通信层已预留，当前运行模拟模式。"},
        {"capture_note", "实时录制"},
        {"csv_title", "导出 PD 报文"},
        {"delete_confirm", "确认删除当前选中的记录？"},
        {"no_selection", "请先选择一行记录。"},
        {"csv_done", "CSV 已导出"},
        {"csv_failed", "CSV 导出失败"},
        {"connected_log", "设备已连接"},
        {"disconnected_log", "设备已断开"},
        {"command_refresh_pdo", "请求刷新 PDO/APDO"},
        {"command_identity", "请求读取线缆身份"},
        {"headers_device", "ID|名称|序列号|固件|端口|模式|备注"},
        {"headers_trigger", "ID|名称|字段|条件|阈值|动作|启用"},
        {"headers_session", "ID|名称|开始时间|频率|导出|备注"},
        {"headers_message", "ID|时间|方向|类型|SOP|MsgID|原始数据|解析"},
        {"headers_pdo", "类型|电压|电流|最小|最大|功率|标记"},
        {"role", "角色"},
        {"cable", "线缆"},
        {"vendor", "厂商"},
        {"product", "产品"},
        {"speed", "速率"},
        {"negotiated", "协商档位"},
        {"samples_live", "实时采样会同步刷新仪表盘、PD 报文表和档位页面。"}
    };
    return (m_language == Language::Chinese ? zh : en).value(key, key);
}

QStringList DashboardWindow::texts(std::initializer_list<const char *> keys) const {
    QStringList values;
    for (const char *key : keys) values << text(QString::fromLatin1(key));
    return values;
}

QWidget *DashboardWindow::buildRoot() {
    auto *root = new QWidget();
    auto *layout = new QHBoxLayout(root);
    layout->setContentsMargins(14, 14, 14, 14);
    layout->setSpacing(14);
    layout->addWidget(buildSidebar(), 0);

    auto *main = new QWidget();
    auto *mainLayout = new QVBoxLayout(main);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(14);
    mainLayout->addWidget(buildTopBar(), 0);

    m_pageStack = new QStackedWidget();
    m_pageStack->addWidget(buildDashboardPage());
    m_pageStack->addWidget(buildPdPage());
    m_pageStack->addWidget(buildPdoPage());
    m_pageStack->addWidget(buildEmarkerPage());
    m_pageStack->addWidget(buildTriggerPage());
    m_pageStack->addWidget(buildSessionPage());
    m_pageStack->addWidget(buildSettingsPage());
    m_pageStack->setCurrentIndex(m_currentPage);
    mainLayout->addWidget(m_pageStack, 1);
    layout->addWidget(main, 1);
    updateLiveLabels();
    return root;
}

QWidget *DashboardWindow::buildSidebar() {
    auto *side = panel();
    side->setObjectName("sidebar");
    side->setFixedWidth(230);
    auto *layout = panelLayout(side);
    auto *brand = new QLabel(text("brand"));
    brand->setObjectName("brand");
    layout->addWidget(brand);
    m_modeList = new QListWidget();
    m_modeList->addItems(texts({"dashboard", "pd", "pdo", "emarker", "triggers", "sessions", "settings"}));
    m_modeList->setCurrentRow(m_currentPage);
    connect(m_modeList, &QListWidget::currentRowChanged, this, &DashboardWindow::switchPage);
    layout->addWidget(m_modeList, 1);
    auto *hint = new QLabel(text("reserved"));
    hint->setObjectName("muted");
    hint->setWordWrap(true);
    layout->addWidget(hint);
    return side;
}

QWidget *DashboardWindow::buildTopBar() {
    auto *bar = panel();
    auto *layout = new QHBoxLayout();
    panelLayout(bar)->addLayout(layout);
    m_title = new QLabel(text("app_title"));
    m_title->setObjectName("screenTitle");
    layout->addWidget(m_title);
    m_connection = new QLabel(m_connected ? text("status_live") : text("status_offline"));
    m_connection->setObjectName("statusPill");
    layout->addWidget(m_connection);
    layout->addStretch();
    m_languageCombo = new QComboBox();
    m_languageCombo->addItems({"English", "中文"});
    m_languageCombo->setCurrentIndex(m_language == Language::Chinese ? 1 : 0);
    connect(m_languageCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, &DashboardWindow::switchLanguage);
    layout->addWidget(m_languageCombo);
    m_connectButton = makeButton(text("connect"), "primary");
    layout->addWidget(m_connectButton);
    connect(m_connectButton, &QPushButton::clicked, this, &DashboardWindow::connectDevice);
    m_disconnectButton = makeButton(text("disconnect"));
    layout->addWidget(m_disconnectButton);
    connect(m_disconnectButton, &QPushButton::clicked, this, &DashboardWindow::disconnectDevice);
    m_recordButton = makeButton(m_recording ? text("stop") : text("record"), "accent");
    layout->addWidget(m_recordButton);
    connect(m_recordButton, &QPushButton::clicked, this, &DashboardWindow::toggleRecording);
    auto *exportButton = makeButton(text("export"));
    layout->addWidget(exportButton);
    connect(exportButton, &QPushButton::clicked, this, &DashboardWindow::exportCsv);
    updateActionState();
    return bar;
}

QWidget *DashboardWindow::buildDashboardPage() {
    auto *center = new QSplitter(Qt::Horizontal);
    auto *content = new QWidget();
    auto *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(14);
    contentLayout->addWidget(buildMetrics(), 0);
    m_chart = new ChartWidget();
    m_chart->setTexts(text("waveform"), text("wave_sub"), texts({"voltage", "current", "power"}));
    contentLayout->addWidget(m_chart, 1);

    auto *records = panel(text("records"));
    auto *recordsLayout = panelLayout(records);
    m_deviceTable = makeTable(text("headers_device").split('|'));
    wireTableInteractions(m_deviceTable, RecordKind::Device);
    recordsLayout->addWidget(buildRecordToolbar(RecordKind::Device, m_deviceTable, text("devices")));
    recordsLayout->addWidget(m_deviceTable, 1);
    contentLayout->addWidget(records, 1);
    center->addWidget(content);
    center->addWidget(buildInspector());
    center->setSizes({940, 330});
    return center;
}

QWidget *DashboardWindow::buildPdPage() {
    auto *page = panel(text("pd"));
    auto *layout = panelLayout(page);
    auto *summary = new QLabel(m_language == Language::Chinese
        ? "实时显示 USB-C PD 报文、方向、SOP、Message ID、原始十六进制与解析结果。"
        : "Live USB-C PD packet stream with direction, SOP, message ID, raw hex and decoded fields.");
    summary->setObjectName("bodyText");
    summary->setWordWrap(true);
    layout->addWidget(summary);
    m_messagePageTable = makeTable(text("headers_message").split('|'));
    wireTableInteractions(m_messagePageTable, RecordKind::Message);
    layout->addWidget(buildRecordToolbar(RecordKind::Message, m_messagePageTable, text("pd_messages")));
    auto *actions = new QWidget();
    auto *actionsLayout = new QHBoxLayout(actions);
    actionsLayout->setContentsMargins(0, 0, 0, 0);
    actionsLayout->addStretch();
    auto *identity = makeButton(text("identify"), "primary");
    auto *clear = makeButton(text("clear"));
    actionsLayout->addWidget(identity);
    actionsLayout->addWidget(clear);
    connect(identity, &QPushButton::clicked, this, &DashboardWindow::requestIdentity);
    connect(clear, &QPushButton::clicked, this, &DashboardWindow::clearMessages);
    layout->addWidget(actions);
    layout->addWidget(m_messagePageTable, 1);
    return page;
}

QWidget *DashboardWindow::buildPdoPage() {
    auto *page = panel(text("pdo_title"));
    auto *layout = panelLayout(page);
    m_pdoSummary = new QLabel();
    m_pdoSummary->setObjectName("bigLine");
    layout->addWidget(m_pdoSummary);
    m_pdoTable = makeTable(text("headers_pdo").split('|'));
    auto *refresh = makeButton(text("refresh"), "primary");
    connect(refresh, &QPushButton::clicked, this, &DashboardWindow::requestPdoRefresh);
    layout->addWidget(refresh, 0, Qt::AlignRight);
    layout->addWidget(m_pdoTable, 1);
    return page;
}

QWidget *DashboardWindow::buildEmarkerPage() {
    auto *page = panel(text("emarker_card"));
    auto *layout = panelLayout(page);
    auto *grid = new QWidget();
    auto *form = new QFormLayout(grid);
    const auto em = m_lastSnapshot.emarker;
    form->addRow(keyValue(text("vendor"), em.vendor));
    form->addRow(keyValue(text("product"), em.product));
    form->addRow(keyValue(text("speed"), em.speed));
    form->addRow(keyValue(text("current"), em.current));
    form->addRow(keyValue(text("voltage"), em.voltage));
    form->addRow(keyValue("VDO", em.vdo));
    layout->addWidget(grid);
    auto *note = new QLabel(text("mock_mode"));
    note->setObjectName("bodyText");
    note->setWordWrap(true);
    layout->addWidget(note);
    auto *identity = makeButton(text("identify"), "primary");
    connect(identity, &QPushButton::clicked, this, &DashboardWindow::requestIdentity);
    layout->addWidget(identity, 0, Qt::AlignLeft);
    layout->addStretch();
    return page;
}

QWidget *DashboardWindow::buildTriggerPage() {
    auto *page = panel(text("triggers"));
    auto *layout = panelLayout(page);
    m_triggerPageTable = makeTable(text("headers_trigger").split('|'));
    wireTableInteractions(m_triggerPageTable, RecordKind::Trigger);
    layout->addWidget(buildRecordToolbar(RecordKind::Trigger, m_triggerPageTable, text("triggers")));
    layout->addWidget(m_triggerPageTable, 1);
    return page;
}

QWidget *DashboardWindow::buildSessionPage() {
    auto *page = panel(text("sessions"));
    auto *layout = panelLayout(page);
    m_sessionPageTable = makeTable(text("headers_session").split('|'));
    wireTableInteractions(m_sessionPageTable, RecordKind::Session);
    layout->addWidget(buildRecordToolbar(RecordKind::Session, m_sessionPageTable, text("sessions")));
    layout->addWidget(m_sessionPageTable, 1);
    return page;
}

QWidget *DashboardWindow::buildSettingsPage() {
    auto *page = panel(text("settings_title"));
    auto *layout = panelLayout(page);
    auto *formBox = panel();
    auto *form = new QFormLayout();
    panelLayout(formBox)->addLayout(form);
    auto *language = new QComboBox();
    language->addItems({"English", "中文"});
    language->setCurrentIndex(m_language == Language::Chinese ? 1 : 0);
    connect(language, qOverload<int>(&QComboBox::currentIndexChanged), this, &DashboardWindow::switchLanguage);
    form->addRow(text("language"), language);
    form->addRow(text("recording_state"), new QLabel(m_recording ? text("stop") : text("record")));
    form->addRow(text("connection_state"), new QLabel(m_connected ? text("status_live") : text("status_offline")));
    m_samplingRate = new QSpinBox();
    m_samplingRate->setRange(1, 1000);
    m_samplingRate->setValue(10);
    m_samplingRate->setSuffix(" Hz");
    connect(m_samplingRate, qOverload<int>(&QSpinBox::valueChanged), this, &DashboardWindow::applySamplingRate);
    form->addRow(text("sample_rate"), m_samplingRate);
    form->addRow(text("usb_endpoint"), new QLabel("HID / CDC / Vendor USB"));
    form->addRow(text("storage"), new QLabel(m_store.storagePath()));
    layout->addWidget(formBox);
    layout->addWidget(buildCommandPanel());
    auto *note = new QLabel(text("mock_mode"));
    note->setObjectName("bodyText");
    note->setWordWrap(true);
    layout->addWidget(note);
    layout->addStretch();
    return page;
}

QWidget *DashboardWindow::buildCommandPanel() {
    auto *box = panel(text("device_commands"));
    auto *layout = new QHBoxLayout();
    panelLayout(box)->addLayout(layout);
    auto *connectBtn = makeButton(text("connect"), "primary");
    auto *recordBtn = makeButton(m_recording ? text("stop") : text("record"), "accent");
    auto *pdoBtn = makeButton(text("refresh"));
    auto *identityBtn = makeButton(text("identify"));
    auto *clearBtn = makeButton(text("clear"));
    layout->addWidget(connectBtn);
    layout->addWidget(recordBtn);
    layout->addWidget(pdoBtn);
    layout->addWidget(identityBtn);
    layout->addWidget(clearBtn);
    layout->addStretch();
    connect(connectBtn, &QPushButton::clicked, this, &DashboardWindow::connectDevice);
    connect(recordBtn, &QPushButton::clicked, this, &DashboardWindow::toggleRecording);
    connect(pdoBtn, &QPushButton::clicked, this, &DashboardWindow::requestPdoRefresh);
    connect(identityBtn, &QPushButton::clicked, this, &DashboardWindow::requestIdentity);
    connect(clearBtn, &QPushButton::clicked, this, &DashboardWindow::clearMessages);
    return box;
}

QWidget *DashboardWindow::buildMetrics() {
    auto *wrap = new QWidget();
    auto *layout = new QHBoxLayout(wrap);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);
    m_voltage = metricValue(text("voltage"), "V", "#3EA6FF");
    m_current = metricValue(text("current"), "A", "#35D6B2");
    m_power = metricValue(text("power"), "W", "#F6B84A");
    m_energy = metricValue(text("energy"), "Wh", "#B89CFF");
    m_temperature = metricValue(text("temp"), "C", "#FF7867");
    for (auto *label : {m_voltage, m_current, m_power, m_energy, m_temperature}) layout->addWidget(label->parentWidget(), 1);
    return wrap;
}

QWidget *DashboardWindow::buildInspector() {
    auto *right = new QWidget();
    right->setMinimumWidth(310);
    auto *layout = new QVBoxLayout(right);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(14);

    auto *pd = panel(text("pd_card"));
    m_protocol = new QLabel();
    m_protocol->setObjectName("bodyText");
    m_negotiated = new QLabel();
    m_negotiated->setObjectName("bigLine");
    panelLayout(pd)->addWidget(m_protocol);
    panelLayout(pd)->addWidget(m_negotiated);
    layout->addWidget(pd);

    auto *em = panel(text("emarker_card"));
    m_emarker = new QLabel();
    m_emarker->setObjectName("bodyText");
    panelLayout(em)->addWidget(m_emarker);
    layout->addWidget(em);

    auto *toggles = panel(text("layers"));
    const auto names = texts({"voltage", "current", "power"});
    for (int i = 0; i < names.size(); ++i) {
        auto *check = new QCheckBox(names.at(i));
        check->setChecked(true);
        panelLayout(toggles)->addWidget(check);
        if (i == 0) connect(check, &QCheckBox::toggled, this, [this](bool v) { if (m_chart) m_chart->setVoltageVisible(v); });
        if (i == 1) connect(check, &QCheckBox::toggled, this, [this](bool v) { if (m_chart) m_chart->setCurrentVisible(v); });
        if (i == 2) connect(check, &QCheckBox::toggled, this, [this](bool v) { if (m_chart) m_chart->setPowerVisible(v); });
    }
    layout->addWidget(toggles);

    auto *logPanel = panel(text("log"));
    m_log = new QTextEdit();
    m_log->setReadOnly(true);
    m_log->setMinimumHeight(160);
    panelLayout(logPanel)->addWidget(m_log);
    layout->addWidget(logPanel, 1);
    return right;
}

QWidget *DashboardWindow::buildRecordToolbar(RecordKind kind, QTableWidget *table, const QString &title) {
    auto *bar = new QWidget();
    auto *layout = new QHBoxLayout(bar);
    layout->setContentsMargins(0, 0, 0, 0);
    auto *label = new QLabel(title);
    label->setObjectName("panelTitle");
    layout->addWidget(label, 1);
    auto *add = makeButton(text("add"), "primary");
    auto *edit = makeButton(text("edit"));
    auto *del = makeButton(text("delete"));
    layout->addWidget(add);
    layout->addWidget(edit);
    layout->addWidget(del);
    connect(add, &QPushButton::clicked, this, [this, kind]() { performAdd(kind); });
    connect(edit, &QPushButton::clicked, this, [this, kind, table]() { performEdit(kind, table); });
    connect(del, &QPushButton::clicked, this, [this, kind, table]() { performDelete(kind, table); });
    if (kind == RecordKind::Device) {
        auto *reset = makeButton(text("reset"));
        layout->addWidget(reset);
        connect(reset, &QPushButton::clicked, this, &DashboardWindow::resetDemoData);
    }
    return bar;
}

QTableWidget *DashboardWindow::makeTable(const QStringList &headers) {
    auto *table = new QTableWidget();
    table->setColumnCount(headers.size());
    table->setHorizontalHeaderLabels(headers);
    table->horizontalHeader()->setStretchLastSection(true);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    table->verticalHeader()->hide();
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setAlternatingRowColors(true);
    table->setSortingEnabled(true);
    return table;
}

QPushButton *DashboardWindow::makeButton(const QString &textValue, const QString &objectName) {
    auto *button = new QPushButton(textValue);
    button->setObjectName(objectName);
    button->setMinimumHeight(34);
    return button;
}

QLabel *DashboardWindow::metricValue(const QString &title, const QString &, const QString &color) {
    auto *card = panel();
    auto *layout = panelLayout(card);
    auto *caption = new QLabel(title);
    caption->setObjectName("metricTitle");
    auto *value = new QLabel("--");
    value->setProperty("metricColor", color);
    value->setObjectName("metricValue");
    layout->addWidget(caption);
    layout->addWidget(value);
    return value;
}

QLabel *DashboardWindow::keyValue(const QString &key, const QString &value) {
    auto *label = new QLabel(QString("%1: %2").arg(key, value));
    label->setObjectName("bodyText");
    return label;
}

void DashboardWindow::applyTheme() {
    qApp->setStyleSheet(R"(
        QMainWindow, QWidget { background: #071018; color: #D7E2EF; font-size: 13px; }
        QLabel, QCheckBox { background: transparent; }
        QFrame#panel, QFrame#sidebar { background: #101923; border: 1px solid #1F2A36; border-radius: 10px; }
        QLabel#brand { font-size: 28px; font-weight: 700; color: #F0F6FF; line-height: 1.05; }
        QLabel#screenTitle { font-size: 24px; font-weight: 700; color: #F0F6FF; }
        QLabel#panelTitle { font-size: 13px; font-weight: 700; color: #F0F6FF; }
        QLabel#muted, QLabel#bodyText { color: #8392A5; }
        QLabel#bigLine { font-size: 19px; font-weight: 700; color: #35D6B2; }
        QLabel#statusPill { background: #0E2B3F; color: #6CCBFF; border: 1px solid #215170; border-radius: 14px; padding: 6px 12px; }
        QLabel#metricTitle { color: #8392A5; font-size: 12px; }
        QLabel#metricValue { font-size: 26px; font-weight: 800; color: white; }
        QListWidget { background: transparent; border: 0; outline: 0; }
        QListWidget::item { padding: 10px 12px; margin: 3px 0; border-radius: 8px; color: #9AA8B8; }
        QListWidget::item:selected { background: #12324A; color: #EAF7FF; border: 1px solid #245E86; }
        QPushButton { background: #172432; border: 1px solid #2A3B4C; border-radius: 8px; color: #DCE8F5; padding: 7px 12px; }
        QPushButton:hover { background: #203245; }
        QPushButton#primary { background: #126FB6; border-color: #2493E6; color: white; }
        QPushButton#accent { background: #A66E16; border-color: #F6B84A; color: white; }
        QTableWidget { background: #0B111A; alternate-background-color: #0F1822; border: 1px solid #1F2A36; gridline-color: #1F2A36; selection-background-color: #153C59; selection-color: white; }
        QHeaderView::section { background: #111D29; color: #91A2B5; border: 0; border-right: 1px solid #263443; padding: 8px; font-weight: 700; }
        QTextEdit, QLineEdit, QComboBox, QSpinBox, QDoubleSpinBox { background: #0B111A; color: #D7E2EF; border: 1px solid #263443; border-radius: 7px; padding: 6px; }
        QCheckBox { color: #B9C6D3; spacing: 8px; }
    )");
}

void DashboardWindow::rebuildUi() {
    QWidget *old = centralWidget();
    setCentralWidget(buildRoot());
    if (old) old->deleteLater();
    refreshTables();
}

void DashboardWindow::switchLanguage(int index) {
    const auto next = index == 1 ? Language::Chinese : Language::English;
    if (next == m_language) return;
    m_language = next;
    rebuildUi();
}

void DashboardWindow::switchPage(int index) {
    if (index < 0) return;
    m_currentPage = index;
    if (m_pageStack) m_pageStack->setCurrentIndex(index);
    if (m_modeList && m_modeList->currentRow() != index) m_modeList->setCurrentRow(index);
}

void DashboardWindow::connectDevice() {
    const auto port = m_store.devices().isEmpty() ? "Mock USB" : m_store.devices().first().port;
    m_transport.connectDevice(port);
    m_transport.setSampling(true);
    appendLog(text("connected_log"));
}

void DashboardWindow::disconnectDevice() {
    m_transport.disconnectDevice();
    m_recording = false;
    appendLog(text("disconnected_log"));
    updateActionState();
}

void DashboardWindow::toggleRecording() {
    if (!m_connected) {
        appendLog(text("no_selection"));
        return;
    }
    m_recording = !m_recording;
    if (m_recordButton) m_recordButton->setText(m_recording ? text("stop") : text("record"));
    m_transport.setSampling(m_recording);
    if (m_recording) {
        m_store.upsertSession({"", QString("Capture %1").arg(QDateTime::currentDateTime().toString("hh:mm:ss")), QDateTime::currentDateTime(), 10, "exports/live.csv", text("capture_note")});
    }
}

void DashboardWindow::onSample(const LiveSnapshot &snapshot) {
    m_lastSnapshot = snapshot;
    if (m_chart) m_chart->appendSample(snapshot.sample);
    updateLiveLabels();
}

void DashboardWindow::updateLiveLabels() {
    if (m_voltage) setMetric(m_voltage, m_lastSnapshot.sample.voltage, " V", 3);
    if (m_current) setMetric(m_current, m_lastSnapshot.sample.current, " A", 3);
    if (m_power) setMetric(m_power, m_lastSnapshot.sample.power, " W", 2);
    if (m_energy) setMetric(m_energy, m_lastSnapshot.sample.energyWh, " Wh", 4);
    if (m_temperature) setMetric(m_temperature, m_lastSnapshot.sample.temperature, " C", 1);
    if (m_protocol) {
        m_protocol->setText(QString("%1: %2\n%3: %4\n%5: %6")
            .arg(text("pd_card"), m_lastSnapshot.protocol, text("role"), m_lastSnapshot.role, text("cable"), m_lastSnapshot.cable));
    }
    if (m_negotiated) {
        m_negotiated->setText(QString("%1: %2").arg(text("negotiated"), m_lastSnapshot.negotiated));
    }
    if (m_emarker) {
        const auto &em = m_lastSnapshot.emarker;
        m_emarker->setText(QString("%1: %2\n%3: %4\n%5: %6 / %7\n%8: %9\nVDO: %10")
            .arg(text("vendor"), em.vendor, text("product"), em.product, text("current"), em.current, em.voltage, text("speed"), em.speed, em.vdo));
    }
    if (m_pdoSummary) {
        m_pdoSummary->setText(QString("%1 %2").arg(QString::number(m_lastSnapshot.pdoEntries.size()), text("pdo_title")));
    }
    fillPdoTable();
    if (!m_lastSnapshot.warning.isEmpty()) appendLog(m_lastSnapshot.warning);
}

void DashboardWindow::updateActionState() {
    if (m_connectButton) m_connectButton->setEnabled(!m_connected);
    if (m_disconnectButton) m_disconnectButton->setEnabled(m_connected);
    if (m_recordButton) {
        m_recordButton->setEnabled(m_connected);
        m_recordButton->setText(m_recording ? text("stop") : text("record"));
    }
}

void DashboardWindow::wireTableInteractions(QTableWidget *table, RecordKind kind) {
    if (!table) return;
    connect(table, &QTableWidget::cellDoubleClicked, this, [this, kind, table](int, int) {
        performEdit(kind, table);
    });
}

void DashboardWindow::onPdMessage(const PdMessage &message) {
    m_store.upsertMessage(message);
}

void DashboardWindow::setMetric(QLabel *label, double value, const QString &unit, int decimals) {
    label->setText(value <= 0.0 ? "--" : QString::number(value, 'f', decimals) + unit);
}

void DashboardWindow::refreshTables() {
    fillDeviceTables();
    fillTriggerTables();
    fillSessionTables();
    fillMessageTables();
    fillPdoTable();
}

void DashboardWindow::fillDeviceTables() {
    if (!m_deviceTable) return;
    m_deviceTable->setSortingEnabled(false);
    m_deviceTable->setRowCount(m_store.devices().size());
    int row = 0;
    for (const auto &d : m_store.devices()) {
        m_deviceTable->setItem(row, 0, item(d.id, d.id));
        m_deviceTable->setItem(row, 1, item(d.name));
        m_deviceTable->setItem(row, 2, item(d.serial));
        m_deviceTable->setItem(row, 3, item(d.firmware));
        m_deviceTable->setItem(row, 4, item(d.port));
        m_deviceTable->setItem(row, 5, item(d.mode));
        m_deviceTable->setItem(row, 6, item(d.note));
        ++row;
    }
    m_deviceTable->setSortingEnabled(true);
}

void DashboardWindow::fillTriggerTables() {
    const auto fill = [this](QTableWidget *table) {
        if (!table) return;
        table->setSortingEnabled(false);
        table->setRowCount(m_store.triggers().size());
        int row = 0;
        for (const auto &t : m_store.triggers()) {
            table->setItem(row, 0, item(t.id, t.id));
            table->setItem(row, 1, item(t.name));
            table->setItem(row, 2, item(t.metric));
            table->setItem(row, 3, item(t.comparator));
            table->setItem(row, 4, item(QString::number(t.threshold, 'f', 3)));
            table->setItem(row, 5, item(t.action));
            table->setItem(row, 6, item(yesNo(t.enabled, m_language == Language::Chinese)));
            ++row;
        }
        table->setSortingEnabled(true);
    };
    fill(m_triggerTable);
    fill(m_triggerPageTable);
}

void DashboardWindow::fillSessionTables() {
    const auto fill = [this](QTableWidget *table) {
        if (!table) return;
        table->setSortingEnabled(false);
        table->setRowCount(m_store.sessions().size());
        int row = 0;
        for (const auto &s : m_store.sessions()) {
            table->setItem(row, 0, item(s.id, s.id));
            table->setItem(row, 1, item(s.name));
            table->setItem(row, 2, item(s.startedAt.toString("yyyy-MM-dd hh:mm:ss")));
            table->setItem(row, 3, item(QString::number(s.sampleRateHz) + " Hz"));
            table->setItem(row, 4, item(s.exportPath));
            table->setItem(row, 5, item(s.note));
            ++row;
        }
        table->setSortingEnabled(true);
    };
    fill(m_sessionTable);
    fill(m_sessionPageTable);
}

void DashboardWindow::fillMessageTables() {
    const auto fill = [this](QTableWidget *table) {
        if (!table) return;
        table->setSortingEnabled(false);
        table->setRowCount(m_store.messages().size());
        int row = 0;
        for (const auto &m : m_store.messages()) {
            table->setItem(row, 0, item(m.id, m.id));
            table->setItem(row, 1, item(m.time.toString("hh:mm:ss.zzz")));
            table->setItem(row, 2, item(m.direction));
            table->setItem(row, 3, item(m.type));
            table->setItem(row, 4, item(m.sop));
            table->setItem(row, 5, item(m.messageId));
            table->setItem(row, 6, item(m.raw));
            table->setItem(row, 7, item(m.decoded));
            ++row;
        }
        table->setSortingEnabled(true);
    };
    fill(m_messageTable);
    fill(m_messagePageTable);
}

void DashboardWindow::fillPdoTable() {
    if (!m_pdoTable) return;
    m_pdoTable->setSortingEnabled(false);
    m_pdoTable->setRowCount(m_lastSnapshot.pdoEntries.size());
    int row = 0;
    for (const auto &p : m_lastSnapshot.pdoEntries) {
        m_pdoTable->setItem(row, 0, item(p.type));
        m_pdoTable->setItem(row, 1, item(p.voltage > 0 ? QString::number(p.voltage, 'f', 1) + " V" : "--"));
        m_pdoTable->setItem(row, 2, item(QString::number(p.current, 'f', 2) + " A"));
        m_pdoTable->setItem(row, 3, item(p.minVoltage > 0 ? QString::number(p.minVoltage, 'f', 1) + " V" : "--"));
        m_pdoTable->setItem(row, 4, item(p.maxVoltage > 0 ? QString::number(p.maxVoltage, 'f', 1) + " V" : "--"));
        m_pdoTable->setItem(row, 5, item(QString::number(p.power, 'f', 1) + " W"));
        m_pdoTable->setItem(row, 6, item(p.flags));
        ++row;
    }
    m_pdoTable->setSortingEnabled(true);
}

QString DashboardWindow::selectedId(QTableWidget *table) const {
    if (!table || !table->selectionModel()) return {};
    const auto rows = table->selectionModel()->selectedRows();
    if (rows.isEmpty() || !table->item(rows.first().row(), 0)) return {};
    return table->item(rows.first().row(), 0)->data(Qt::UserRole).toString();
}

void DashboardWindow::addCurrentRecord() {}
void DashboardWindow::editCurrentRecord() {}
void DashboardWindow::deleteCurrentRecord() {}

void DashboardWindow::performAdd(RecordKind kind) {
    RecordDialog dialog(static_cast<RecordDialog::Kind>(kind), m_language == Language::Chinese, this);
    if (dialog.exec() != QDialog::Accepted) return;
    if (kind == RecordKind::Device) m_store.upsertDevice(dialog.device());
    if (kind == RecordKind::Trigger) m_store.upsertTrigger(dialog.trigger());
    if (kind == RecordKind::Session) m_store.upsertSession(dialog.session());
    if (kind == RecordKind::Message) m_store.upsertMessage(dialog.message());
}

void DashboardWindow::performEdit(RecordKind kind, QTableWidget *table) {
    const QString id = selectedId(table);
    if (id.isEmpty()) {
        appendLog(text("no_selection"));
        return;
    }
    RecordDialog dialog(static_cast<RecordDialog::Kind>(kind), m_language == Language::Chinese, this);
    if (kind == RecordKind::Device) for (const auto &d : m_store.devices()) if (d.id == id) dialog.setDevice(d);
    if (kind == RecordKind::Trigger) for (const auto &t : m_store.triggers()) if (t.id == id) dialog.setTrigger(t);
    if (kind == RecordKind::Session) for (const auto &s : m_store.sessions()) if (s.id == id) dialog.setSession(s);
    if (kind == RecordKind::Message) for (const auto &m : m_store.messages()) if (m.id == id) dialog.setMessage(m);
    if (dialog.exec() != QDialog::Accepted) return;
    if (kind == RecordKind::Device) m_store.upsertDevice(dialog.device());
    if (kind == RecordKind::Trigger) m_store.upsertTrigger(dialog.trigger());
    if (kind == RecordKind::Session) m_store.upsertSession(dialog.session());
    if (kind == RecordKind::Message) m_store.upsertMessage(dialog.message());
}

void DashboardWindow::performDelete(RecordKind kind, QTableWidget *table) {
    const QString id = selectedId(table);
    if (id.isEmpty()) {
        appendLog(text("no_selection"));
        return;
    }
    if (QMessageBox::question(this, text("delete"), text("delete_confirm")) != QMessageBox::Yes) return;
    if (kind == RecordKind::Device) m_store.removeDevice(id);
    if (kind == RecordKind::Trigger) m_store.removeTrigger(id);
    if (kind == RecordKind::Session) m_store.removeSession(id);
    if (kind == RecordKind::Message) m_store.removeMessage(id);
}

void DashboardWindow::exportCsv() {
    const QString path = QFileDialog::getSaveFileName(this, text("csv_title"), "powerx-pd-messages.csv", "CSV Files (*.csv)");
    if (path.isEmpty()) return;
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        appendLog(text("csv_failed") + ": " + path);
        return;
    }
    file.write("time,direction,type,sop,message_id,raw,decoded\n");
    for (const auto &m : m_store.messages()) {
        const auto line = QString("\"%1\",\"%2\",\"%3\",\"%4\",\"%5\",\"%6\",\"%7\"\n")
                              .arg(m.time.toString(Qt::ISODateWithMs), m.direction, m.type, m.sop, m.messageId, m.raw, m.decoded);
        file.write(line.toUtf8());
    }
    appendLog(text("csv_done") + ": " + path);
}

void DashboardWindow::resetDemoData() {
    m_store.resetDemoData();
    if (m_chart) m_chart->clear();
}

void DashboardWindow::clearMessages() {
    m_store.clearMessages();
    appendLog(text("pd_messages") + " " + text("clear"));
}

void DashboardWindow::requestIdentity() {
    m_transport.sendCommand("read_identity");
    appendLog(text("command_identity"));
}

void DashboardWindow::requestPdoRefresh() {
    m_transport.sendCommand("refresh_pdo");
    appendLog(text("command_refresh_pdo"));
}

void DashboardWindow::applySamplingRate(int rate) {
    appendLog(QString("%1: %2 Hz").arg(text("sample_rate"), QString::number(rate)));
}

void DashboardWindow::appendLog(const QString &line) {
    if (m_log) {
        m_log->append(QString("[%1] %2").arg(QTime::currentTime().toString("hh:mm:ss"), line));
    }
}

bool DashboardWindow::saveScreenshot(const QString &path) {
    repaint();
    const QPixmap pixmap = grab();
    return pixmap.save(path);
}
