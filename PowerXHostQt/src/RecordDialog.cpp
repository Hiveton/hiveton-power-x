#include "RecordDialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QSpinBox>
#include <QVBoxLayout>

namespace {
QString rdText(bool zh, const QString &en, const QString &cn) {
    return zh ? cn : en;
}
}

RecordDialog::RecordDialog(Kind kind, bool chinese, QWidget *parent) : QDialog(parent), m_kind(kind), m_chinese(chinese) {
    setModal(true);
    setMinimumWidth(460);
    auto *layout = new QVBoxLayout(this);
    m_form = new QFormLayout();
    m_form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    layout->addLayout(m_form);

    switch (kind) {
    case Kind::Device:
        setWindowTitle(rdText(m_chinese, "Device Profile", "设备档案"));
        line("name", rdText(m_chinese, "Name", "设备名称"), "PowerX PX1");
        line("serial", rdText(m_chinese, "Serial", "序列号"), "PX1-0001");
        line("firmware", rdText(m_chinese, "Firmware", "固件版本"), "0.1.3");
        line("port", rdText(m_chinese, "Port", "端口"), "USB HID 1A86:55E0");
        combo("mode", rdText(m_chinese, "Mode", "模式"), {"USB-C PD Analyzer", "Load Monitor", "Cable E-Marker", "Firmware Update"});
        line("note", rdText(m_chinese, "Note", "备注"));
        break;
    case Kind::Trigger:
        setWindowTitle(rdText(m_chinese, "Trigger Rule", "触发规则"));
        line("name", rdText(m_chinese, "Rule name", "规则名称"));
        combo("metric", rdText(m_chinese, "Metric", "字段"), {"Voltage", "Current", "Power", "Temperature", "Energy"});
        combo("comparator", rdText(m_chinese, "Condition", "条件"), {">", ">=", "<", "<=", "=="});
        doubleSpin("threshold", rdText(m_chinese, "Threshold", "阈值"), -1000, 1000);
        combo("action", rdText(m_chinese, "Action", "动作"), {"Append event", "Pause capture + mark warning", "Snapshot CSV", "Send USB command"});
        check("enabled", rdText(m_chinese, "Enabled", "启用"));
        break;
    case Kind::Session:
        setWindowTitle(rdText(m_chinese, "Capture Session", "采集会话"));
        line("name", rdText(m_chinese, "Session name", "会话名称"));
        spin("sampleRateHz", rdText(m_chinese, "Sample rate", "采样率"), 1, 1000, " Hz");
        line("exportPath", rdText(m_chinese, "Export path", "导出路径"), "exports/session.csv");
        line("note", rdText(m_chinese, "Note", "备注"));
        break;
    case Kind::Message:
        setWindowTitle(rdText(m_chinese, "PD Message", "PD 报文"));
        combo("direction", rdText(m_chinese, "Direction", "方向"), {"SRC -> SNK", "SNK -> SRC", "Cable -> DFP", "DFP -> Cable"});
        combo("type", rdText(m_chinese, "Type", "类型"), {"Source_Capabilities", "Request", "Accept", "PS_RDY", "Vendor_Defined", "Discover_Identity", "Soft_Reset"});
        combo("sop", "SOP", {"SOP", "SOP'", "SOP''"});
        line("messageId", "Message ID");
        line("raw", rdText(m_chinese, "Raw data", "原始数据"));
        line("decoded", rdText(m_chinese, "Decoded", "解析"));
        break;
    }

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
}

QLineEdit *RecordDialog::line(const QString &key, const QString &label, const QString &placeholder) {
    auto *field = new QLineEdit();
    field->setPlaceholderText(placeholder);
    m_fields[key] = field;
    m_form->addRow(label, field);
    return field;
}

QComboBox *RecordDialog::combo(const QString &key, const QString &label, const QStringList &items) {
    auto *field = new QComboBox();
    field->addItems(items);
    field->setEditable(true);
    m_fields[key] = field;
    m_form->addRow(label, field);
    return field;
}

QDoubleSpinBox *RecordDialog::doubleSpin(const QString &key, const QString &label, double min, double max, const QString &suffix) {
    auto *field = new QDoubleSpinBox();
    field->setRange(min, max);
    field->setDecimals(3);
    field->setSuffix(suffix);
    m_fields[key] = field;
    m_form->addRow(label, field);
    return field;
}

QSpinBox *RecordDialog::spin(const QString &key, const QString &label, int min, int max, const QString &suffix) {
    auto *field = new QSpinBox();
    field->setRange(min, max);
    field->setSuffix(suffix);
    m_fields[key] = field;
    m_form->addRow(label, field);
    return field;
}

QCheckBox *RecordDialog::check(const QString &key, const QString &label) {
    auto *field = new QCheckBox();
    m_fields[key] = field;
    m_form->addRow(label, field);
    return field;
}

void RecordDialog::setDevice(const DeviceProfile &record) {
    m_id = record.id;
    qobject_cast<QLineEdit *>(m_fields["name"])->setText(record.name);
    qobject_cast<QLineEdit *>(m_fields["serial"])->setText(record.serial);
    qobject_cast<QLineEdit *>(m_fields["firmware"])->setText(record.firmware);
    qobject_cast<QLineEdit *>(m_fields["port"])->setText(record.port);
    qobject_cast<QComboBox *>(m_fields["mode"])->setCurrentText(record.mode);
    qobject_cast<QLineEdit *>(m_fields["note"])->setText(record.note);
}

DeviceProfile RecordDialog::device() const {
    return {m_id,
            qobject_cast<QLineEdit *>(m_fields["name"])->text(),
            qobject_cast<QLineEdit *>(m_fields["serial"])->text(),
            qobject_cast<QLineEdit *>(m_fields["firmware"])->text(),
            qobject_cast<QLineEdit *>(m_fields["port"])->text(),
            qobject_cast<QComboBox *>(m_fields["mode"])->currentText(),
            qobject_cast<QLineEdit *>(m_fields["note"])->text()};
}

void RecordDialog::setTrigger(const TriggerRule &record) {
    m_id = record.id;
    qobject_cast<QLineEdit *>(m_fields["name"])->setText(record.name);
    qobject_cast<QComboBox *>(m_fields["metric"])->setCurrentText(record.metric);
    qobject_cast<QComboBox *>(m_fields["comparator"])->setCurrentText(record.comparator);
    qobject_cast<QDoubleSpinBox *>(m_fields["threshold"])->setValue(record.threshold);
    qobject_cast<QComboBox *>(m_fields["action"])->setCurrentText(record.action);
    qobject_cast<QCheckBox *>(m_fields["enabled"])->setChecked(record.enabled);
}

TriggerRule RecordDialog::trigger() const {
    return {m_id,
            qobject_cast<QLineEdit *>(m_fields["name"])->text(),
            qobject_cast<QComboBox *>(m_fields["metric"])->currentText(),
            qobject_cast<QComboBox *>(m_fields["comparator"])->currentText(),
            qobject_cast<QDoubleSpinBox *>(m_fields["threshold"])->value(),
            qobject_cast<QComboBox *>(m_fields["action"])->currentText(),
            qobject_cast<QCheckBox *>(m_fields["enabled"])->isChecked()};
}

void RecordDialog::setSession(const CaptureSession &record) {
    m_id = record.id;
    qobject_cast<QLineEdit *>(m_fields["name"])->setText(record.name);
    qobject_cast<QSpinBox *>(m_fields["sampleRateHz"])->setValue(record.sampleRateHz);
    qobject_cast<QLineEdit *>(m_fields["exportPath"])->setText(record.exportPath);
    qobject_cast<QLineEdit *>(m_fields["note"])->setText(record.note);
}

CaptureSession RecordDialog::session() const {
    return {m_id,
            qobject_cast<QLineEdit *>(m_fields["name"])->text(),
            QDateTime::currentDateTime(),
            qobject_cast<QSpinBox *>(m_fields["sampleRateHz"])->value(),
            qobject_cast<QLineEdit *>(m_fields["exportPath"])->text(),
            qobject_cast<QLineEdit *>(m_fields["note"])->text()};
}

void RecordDialog::setMessage(const PdMessage &record) {
    m_id = record.id;
    qobject_cast<QComboBox *>(m_fields["direction"])->setCurrentText(record.direction);
    qobject_cast<QComboBox *>(m_fields["type"])->setCurrentText(record.type);
    qobject_cast<QComboBox *>(m_fields["sop"])->setCurrentText(record.sop);
    qobject_cast<QLineEdit *>(m_fields["messageId"])->setText(record.messageId);
    qobject_cast<QLineEdit *>(m_fields["raw"])->setText(record.raw);
    qobject_cast<QLineEdit *>(m_fields["decoded"])->setText(record.decoded);
}

PdMessage RecordDialog::message() const {
    return {m_id,
            QDateTime::currentDateTime(),
            qobject_cast<QComboBox *>(m_fields["direction"])->currentText(),
            qobject_cast<QComboBox *>(m_fields["type"])->currentText(),
            qobject_cast<QComboBox *>(m_fields["sop"])->currentText(),
            qobject_cast<QLineEdit *>(m_fields["messageId"])->text(),
            qobject_cast<QLineEdit *>(m_fields["raw"])->text(),
            qobject_cast<QLineEdit *>(m_fields["decoded"])->text()};
}
