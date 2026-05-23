#include "MockUsbTransport.h"

#include <QRandomGenerator>
#include <QtMath>

MockUsbTransport::MockUsbTransport(QObject *parent) : UsbTransport(parent) {
    m_timer.setInterval(250);
    connect(&m_timer, &QTimer::timeout, this, &MockUsbTransport::tick);
}

void MockUsbTransport::connectDevice(const QString &port) {
    m_connected = true;
    emit connectionChanged("Connected");
    emit transportLog(QString("USB transport ready on %1, protocol endpoint reserved.").arg(port));
}

void MockUsbTransport::disconnectDevice() {
    m_timer.stop();
    m_connected = false;
    m_sampling = false;
    emit connectionChanged("Disconnected");
    emit transportLog("USB transport disconnected.");
}

void MockUsbTransport::setSampling(bool enabled) {
    if (!m_connected) {
        emit transportLog("Sampling ignored: no USB device connected.");
        return;
    }
    m_sampling = enabled;
    enabled ? m_timer.start() : m_timer.stop();
    emit transportLog(enabled ? "Live sampling started." : "Live sampling paused.");
}

void MockUsbTransport::sendCommand(const QString &command, const QVariantMap &) {
    emit transportLog(QString("Queued command for future firmware endpoint: %1").arg(command));
}

void MockUsbTransport::tick() {
    auto snapshot = makeSnapshot();
    m_energyWh = snapshot.sample.energyWh;
    emit sampleReceived(snapshot);
    if (m_index % 5 == 0) {
        emit pdMessageReceived(makePdMessage());
    }
    ++m_index;
}

LiveSnapshot MockUsbTransport::makeSnapshot() const {
    const double t = m_index / 4.0;
    const double voltage = 20.05 + qSin(t * 0.32) * 0.16 + qSin(t * 1.7) * 0.025;
    const double current = 2.35 + qSin(t * 0.46 + 1.1) * 0.36 + qSin(t * 2.4) * 0.04;
    const double power = voltage * current;

    LiveSnapshot snapshot;
    snapshot.connectionState = m_connected ? "Connected" : "Disconnected";
    snapshot.sample.time = QDateTime::currentDateTime();
    snapshot.sample.voltage = voltage;
    snapshot.sample.current = qMax(0.05, current);
    snapshot.sample.power = power;
    snapshot.sample.temperature = 38.0 + qSin(t * 0.18) * 3.0;
    snapshot.sample.energyWh = m_energyWh + power * (m_timer.interval() / 1000.0) / 3600.0;
    snapshot.negotiated = power > 52.0 ? "20.0V / 3.0A PPS" : "20.0V / 2.5A";
    snapshot.warning = snapshot.sample.temperature > 41.0 ? "温度接近阈值" : "";
    snapshot.pdoEntries = {
        {"Fixed", 5.0, 3.0, 0.0, 0.0, 15.0, "USB default"},
        {"Fixed", 9.0, 3.0, 0.0, 0.0, 27.0, "PD"},
        {"Fixed", 15.0, 3.0, 0.0, 0.0, 45.0, "PD"},
        {"Fixed", 20.0, 3.25, 0.0, 0.0, 65.0, "PD"},
        {"APDO", 0.0, 3.0, 3.3, 21.0, 63.0, "PPS"}
    };
    return snapshot;
}

PdMessage MockUsbTransport::makePdMessage() const {
    static const QStringList types = {"Source_Capabilities", "Request", "Accept", "PS_RDY", "Vendor_Defined", "Discover_Identity"};
    static const QStringList dirs = {"SRC -> SNK", "SNK -> SRC", "Cable -> DFP"};
    const int i = m_index / 5;
    PdMessage message;
    message.id = QString("pd-%1").arg(QDateTime::currentMSecsSinceEpoch());
    message.time = QDateTime::currentDateTime();
    message.direction = dirs.at(i % dirs.size());
    message.type = types.at(i % types.size());
    message.sop = (i % 4 == 0) ? "SOP'" : "SOP";
    message.messageId = QString::number(i % 8);
    message.raw = QString("0x%1 0x%2 0x%3")
                      .arg(0x1280 + i, 4, 16, QLatin1Char('0'))
                      .arg(0x2C91 + i * 3, 4, 16, QLatin1Char('0'))
                      .arg(0x04B0 + i * 7, 4, 16, QLatin1Char('0')).toUpper();
    message.decoded = message.type == "Source_Capabilities"
        ? "5V3A, 9V3A, 15V3A, 20V3.25A, PPS 3.3-21V"
        : "PD state transition captured";
    return message;
}
