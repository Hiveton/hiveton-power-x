#include "DataStore.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

#include <algorithm>

namespace {
QString idOrNew(const QString &id, const QString &prefix) {
    return id.isEmpty() ? QString("%1-%2").arg(prefix, QString::number(QDateTime::currentMSecsSinceEpoch())) : id;
}

QJsonObject toJson(const DeviceProfile &d) {
    return {{"id", d.id}, {"name", d.name}, {"serial", d.serial}, {"firmware", d.firmware}, {"port", d.port}, {"mode", d.mode}, {"note", d.note}};
}

QJsonObject toJson(const TriggerRule &t) {
    return {{"id", t.id}, {"name", t.name}, {"metric", t.metric}, {"comparator", t.comparator}, {"threshold", t.threshold}, {"action", t.action}, {"enabled", t.enabled}};
}

QJsonObject toJson(const CaptureSession &s) {
    return {{"id", s.id}, {"name", s.name}, {"startedAt", s.startedAt.toString(Qt::ISODate)}, {"sampleRateHz", s.sampleRateHz}, {"exportPath", s.exportPath}, {"note", s.note}};
}

QJsonObject toJson(const PdMessage &m) {
    return {{"id", m.id}, {"time", m.time.toString(Qt::ISODateWithMs)}, {"direction", m.direction}, {"type", m.type}, {"sop", m.sop}, {"messageId", m.messageId}, {"raw", m.raw}, {"decoded", m.decoded}};
}

DeviceProfile deviceFromJson(const QJsonObject &o) {
    return {o["id"].toString(), o["name"].toString(), o["serial"].toString(), o["firmware"].toString(), o["port"].toString(), o["mode"].toString(), o["note"].toString()};
}

TriggerRule triggerFromJson(const QJsonObject &o) {
    return {o["id"].toString(), o["name"].toString(), o["metric"].toString(), o["comparator"].toString(), o["threshold"].toDouble(), o["action"].toString(), o["enabled"].toBool(true)};
}

CaptureSession sessionFromJson(const QJsonObject &o) {
    return {o["id"].toString(), o["name"].toString(), QDateTime::fromString(o["startedAt"].toString(), Qt::ISODate), o["sampleRateHz"].toInt(10), o["exportPath"].toString(), o["note"].toString()};
}

PdMessage messageFromJson(const QJsonObject &o) {
    return {o["id"].toString(), QDateTime::fromString(o["time"].toString(), Qt::ISODateWithMs), o["direction"].toString(), o["type"].toString(), o["sop"].toString(), o["messageId"].toString(), o["raw"].toString(), o["decoded"].toString()};
}

template <typename T, typename F>
void upsert(QVector<T> &records, T record, F idOf) {
    for (auto &item : records) {
        if (idOf(item) == idOf(record)) {
            item = record;
            return;
        }
    }
    records.push_back(record);
}
}

DataStore::DataStore(QObject *parent) : QObject(parent) {}

QString DataStore::storagePath() const {
    const auto base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(base);
    return base + "/powerx-host-data.json";
}

void DataStore::load() {
    QFile file(storagePath());
    if (!file.open(QIODevice::ReadOnly)) {
        resetDemoData();
        return;
    }
    const auto doc = QJsonDocument::fromJson(file.readAll());
    const auto root = doc.object();
    m_devices.clear();
    m_triggers.clear();
    m_sessions.clear();
    m_messages.clear();
    for (const auto &v : root["devices"].toArray()) m_devices.push_back(deviceFromJson(v.toObject()));
    for (const auto &v : root["triggers"].toArray()) m_triggers.push_back(triggerFromJson(v.toObject()));
    for (const auto &v : root["sessions"].toArray()) m_sessions.push_back(sessionFromJson(v.toObject()));
    for (const auto &v : root["messages"].toArray()) m_messages.push_back(messageFromJson(v.toObject()));
    if (m_devices.isEmpty()) resetDemoData();
    emit changed();
}

void DataStore::save() const {
    QJsonArray devices;
    QJsonArray triggers;
    QJsonArray sessions;
    QJsonArray messages;
    for (const auto &d : m_devices) devices.append(toJson(d));
    for (const auto &t : m_triggers) triggers.append(toJson(t));
    for (const auto &s : m_sessions) sessions.append(toJson(s));
    for (const auto &m : m_messages) messages.append(toJson(m));
    QFile file(storagePath());
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        file.write(QJsonDocument(QJsonObject{{"devices", devices}, {"triggers", triggers}, {"sessions", sessions}, {"messages", messages}}).toJson(QJsonDocument::Indented));
    }
}

void DataStore::resetDemoData() {
    m_devices = {
        {"dev-px1", "PowerX PX1 EVT", "PX1-260518-001", "0.1.3", "USB HID 1A86:55E0", "USB-C PD Analyzer", "当前默认模拟设备，真实 USB 接口已预留"},
        {"dev-lab", "Bench Supply Monitor", "LAB-240W", "0.1.0", "Mock Port", "Load Monitor", "用于多设备切换验证"}
    };
    m_triggers = {
        {"trg-temp", "温度保护", "Temperature", ">", 45.0, "Pause capture + mark warning", true},
        {"trg-power", "功率跌落", "Power", "<", 15.0, "Append event", true},
        {"trg-voltage", "过压标记", "Voltage", ">", 21.2, "Snapshot CSV", false}
    };
    m_sessions = {
        {"ses-demo", "PD 65W 充电记录", QDateTime::currentDateTime().addSecs(-900), 10, "exports/pd-65w.csv", "演示采集会话"}
    };
    m_messages = {
        {"msg-1", QDateTime::currentDateTime().addSecs(-10), "SRC -> SNK", "Source_Capabilities", "SOP", "0", "0x1280 0x2C91", "5V3A, 9V3A, 15V3A, 20V3.25A"},
        {"msg-2", QDateTime::currentDateTime().addSecs(-8), "SNK -> SRC", "Request", "SOP", "1", "0x1042 0x3301", "Request 20V 2.5A"}
    };
    save();
    emit changed();
}

void DataStore::upsertDevice(const DeviceProfile &record) {
    auto copy = record;
    copy.id = idOrNew(copy.id, "dev");
    upsert(m_devices, copy, [](const DeviceProfile &d) { return d.id; });
    save();
    emit changed();
}

void DataStore::removeDevice(const QString &id) {
    m_devices.erase(std::remove_if(m_devices.begin(), m_devices.end(), [&](const DeviceProfile &d) { return d.id == id; }), m_devices.end());
    save();
    emit changed();
}

void DataStore::upsertTrigger(const TriggerRule &record) {
    auto copy = record;
    copy.id = idOrNew(copy.id, "trg");
    upsert(m_triggers, copy, [](const TriggerRule &t) { return t.id; });
    save();
    emit changed();
}

void DataStore::removeTrigger(const QString &id) {
    m_triggers.erase(std::remove_if(m_triggers.begin(), m_triggers.end(), [&](const TriggerRule &t) { return t.id == id; }), m_triggers.end());
    save();
    emit changed();
}

void DataStore::upsertSession(const CaptureSession &record) {
    auto copy = record;
    copy.id = idOrNew(copy.id, "ses");
    if (!copy.startedAt.isValid()) copy.startedAt = QDateTime::currentDateTime();
    upsert(m_sessions, copy, [](const CaptureSession &s) { return s.id; });
    save();
    emit changed();
}

void DataStore::removeSession(const QString &id) {
    m_sessions.erase(std::remove_if(m_sessions.begin(), m_sessions.end(), [&](const CaptureSession &s) { return s.id == id; }), m_sessions.end());
    save();
    emit changed();
}

void DataStore::upsertMessage(const PdMessage &record) {
    auto copy = record;
    copy.id = idOrNew(copy.id, "msg");
    if (!copy.time.isValid()) copy.time = QDateTime::currentDateTime();
    upsert(m_messages, copy, [](const PdMessage &m) { return m.id; });
    save();
    emit changed();
}

void DataStore::removeMessage(const QString &id) {
    m_messages.erase(std::remove_if(m_messages.begin(), m_messages.end(), [&](const PdMessage &m) { return m.id == id; }), m_messages.end());
    save();
    emit changed();
}

void DataStore::clearMessages() {
    m_messages.clear();
    save();
    emit changed();
}
