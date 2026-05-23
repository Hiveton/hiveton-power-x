#pragma once

#include "PowerModels.h"

#include <QObject>
#include <QTimer>

class UsbTransport : public QObject {
    Q_OBJECT
public:
    explicit UsbTransport(QObject *parent = nullptr) : QObject(parent) {}
    virtual void connectDevice(const QString &port) = 0;
    virtual void disconnectDevice() = 0;
    virtual void setSampling(bool enabled) = 0;
    virtual void sendCommand(const QString &command, const QVariantMap &payload = {}) = 0;

signals:
    void connectionChanged(const QString &state);
    void sampleReceived(const LiveSnapshot &snapshot);
    void pdMessageReceived(const PdMessage &message);
    void transportLog(const QString &line);
};

class MockUsbTransport final : public UsbTransport {
    Q_OBJECT
public:
    explicit MockUsbTransport(QObject *parent = nullptr);
    void connectDevice(const QString &port) override;
    void disconnectDevice() override;
    void setSampling(bool enabled) override;
    void sendCommand(const QString &command, const QVariantMap &payload = {}) override;

private slots:
    void tick();

private:
    LiveSnapshot makeSnapshot() const;
    PdMessage makePdMessage() const;

    QTimer m_timer;
    bool m_connected = false;
    bool m_sampling = false;
    mutable int m_index = 0;
    double m_energyWh = 0.0;
};
