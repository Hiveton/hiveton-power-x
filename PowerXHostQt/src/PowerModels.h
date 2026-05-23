#pragma once

#include <QDateTime>
#include <QString>
#include <QVector>

struct SamplePoint {
    QDateTime time;
    double voltage = 0.0;
    double current = 0.0;
    double power = 0.0;
    double temperature = 0.0;
    double energyWh = 0.0;
};

struct DeviceProfile {
    QString id;
    QString name;
    QString serial;
    QString firmware;
    QString port;
    QString mode;
    QString note;
};

struct TriggerRule {
    QString id;
    QString name;
    QString metric;
    QString comparator;
    double threshold = 0.0;
    QString action;
    bool enabled = true;
};

struct CaptureSession {
    QString id;
    QString name;
    QDateTime startedAt;
    int sampleRateHz = 10;
    QString exportPath;
    QString note;
};

struct PdMessage {
    QString id;
    QDateTime time;
    QString direction;
    QString type;
    QString sop;
    QString messageId;
    QString raw;
    QString decoded;
};

struct PdoEntry {
    QString type;
    double voltage = 0.0;
    double current = 0.0;
    double minVoltage = 0.0;
    double maxVoltage = 0.0;
    double power = 0.0;
    QString flags;
};

struct EmarkerInfo {
    QString vendor = "Hiveton Lab Cable";
    QString product = "PX-C240";
    QString speed = "USB4 Gen3";
    QString current = "5A";
    QString voltage = "50V";
    QString vdo = "0x1AC2 0x0500 0x0321";
};

struct LiveSnapshot {
    SamplePoint sample;
    QString connectionState = "Disconnected";
    QString protocol = "USB-C PD 3.1";
    QString role = "Sink monitor";
    QString negotiated = "20.0V / 3.0A";
    QString cable = "E-Marker 5A";
    QString warning;
    QVector<PdoEntry> pdoEntries;
    EmarkerInfo emarker;
};
