#pragma once

#include "PowerModels.h"

#include <QObject>

class DataStore final : public QObject {
    Q_OBJECT
public:
    explicit DataStore(QObject *parent = nullptr);

    const QVector<DeviceProfile> &devices() const { return m_devices; }
    const QVector<TriggerRule> &triggers() const { return m_triggers; }
    const QVector<CaptureSession> &sessions() const { return m_sessions; }
    const QVector<PdMessage> &messages() const { return m_messages; }

    void load();
    void save() const;
    void resetDemoData();

    void upsertDevice(const DeviceProfile &record);
    void removeDevice(const QString &id);
    void upsertTrigger(const TriggerRule &record);
    void removeTrigger(const QString &id);
    void upsertSession(const CaptureSession &record);
    void removeSession(const QString &id);
    void upsertMessage(const PdMessage &record);
    void removeMessage(const QString &id);
    void clearMessages();

    QString storagePath() const;

signals:
    void changed();

private:
    QVector<DeviceProfile> m_devices;
    QVector<TriggerRule> m_triggers;
    QVector<CaptureSession> m_sessions;
    QVector<PdMessage> m_messages;
};
