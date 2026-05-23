#pragma once

#include "PowerModels.h"

#include <QDialog>
#include <QMap>

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QFormLayout;
class QLineEdit;
class QSpinBox;

class RecordDialog final : public QDialog {
    Q_OBJECT
public:
    enum class Kind { Device, Trigger, Session, Message };

    explicit RecordDialog(Kind kind, bool chinese = false, QWidget *parent = nullptr);

    void setDevice(const DeviceProfile &record);
    DeviceProfile device() const;
    void setTrigger(const TriggerRule &record);
    TriggerRule trigger() const;
    void setSession(const CaptureSession &record);
    CaptureSession session() const;
    void setMessage(const PdMessage &record);
    PdMessage message() const;

private:
    QLineEdit *line(const QString &key, const QString &label, const QString &placeholder = {});
    QComboBox *combo(const QString &key, const QString &label, const QStringList &items);
    QDoubleSpinBox *doubleSpin(const QString &key, const QString &label, double min, double max, const QString &suffix = {});
    QSpinBox *spin(const QString &key, const QString &label, int min, int max, const QString &suffix = {});
    QCheckBox *check(const QString &key, const QString &label);

    Kind m_kind;
    bool m_chinese = false;
    QString m_id;
    QFormLayout *m_form = nullptr;
    QMap<QString, QWidget *> m_fields;
};
