#pragma once

#include "PowerModels.h"

#include <QWidget>

class ChartWidget final : public QWidget {
    Q_OBJECT
public:
    explicit ChartWidget(QWidget *parent = nullptr);
    void appendSample(const SamplePoint &sample);
    void clear();
    void setVoltageVisible(bool visible);
    void setCurrentVisible(bool visible);
    void setPowerVisible(bool visible);
    void setTexts(const QString &title, const QString &subtitle, const QStringList &legend);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void drawSeries(QPainter &p, const QRectF &plot, const QVector<double> &values, QColor color, double maxValue, const QString &label);

    QVector<SamplePoint> m_samples;
    QString m_title = "Real-time Waveform";
    QString m_subtitle = "Voltage / Current / Power rolling capture";
    QStringList m_legend = {"Voltage", "Current", "Power"};
    bool m_showVoltage = true;
    bool m_showCurrent = true;
    bool m_showPower = true;
};
