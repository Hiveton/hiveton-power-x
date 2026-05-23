#include "ChartWidget.h"

#include <QPainter>
#include <QPainterPath>
#include <QtMath>

ChartWidget::ChartWidget(QWidget *parent) : QWidget(parent) {
    setMinimumHeight(260);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void ChartWidget::appendSample(const SamplePoint &sample) {
    m_samples.push_back(sample);
    while (m_samples.size() > 240) m_samples.pop_front();
    update();
}

void ChartWidget::clear() {
    m_samples.clear();
    update();
}

void ChartWidget::setVoltageVisible(bool visible) { m_showVoltage = visible; update(); }
void ChartWidget::setCurrentVisible(bool visible) { m_showCurrent = visible; update(); }
void ChartWidget::setPowerVisible(bool visible) { m_showPower = visible; update(); }
void ChartWidget::setTexts(const QString &title, const QString &subtitle, const QStringList &legend) {
    m_title = title;
    m_subtitle = subtitle;
    m_legend = legend;
    update();
}

void ChartWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor("#0B111A"));

    QRectF card = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    p.setPen(QColor("#1F2A36"));
    p.setBrush(QColor("#101923"));
    p.drawRoundedRect(card, 10, 10);

    p.setPen(QColor("#E7EEF8"));
    QFont titleFont = font();
    titleFont.setPointSize(15);
    titleFont.setWeight(QFont::DemiBold);
    p.setFont(titleFont);
    p.drawText(QRectF(22, 16, width() - 44, 26), m_title);

    QFont labelFont = font();
    labelFont.setPointSize(10);
    p.setFont(labelFont);
    p.setPen(QColor("#7D8B9B"));
    p.drawText(QRectF(22, 42, width() - 44, 22), m_subtitle);

    QRectF plot(54, 78, width() - 86, height() - 112);
    p.setPen(QPen(QColor("#1B2B3A"), 1));
    for (int i = 0; i <= 8; ++i) {
        const double y = plot.top() + plot.height() * i / 8.0;
        p.drawLine(QPointF(plot.left(), y), QPointF(plot.right(), y));
    }
    for (int i = 0; i <= 12; ++i) {
        const double x = plot.left() + plot.width() * i / 12.0;
        p.drawLine(QPointF(x, plot.top()), QPointF(x, plot.bottom()));
    }
    p.setPen(QColor("#2B3B4D"));
    p.drawRect(plot);

    QVector<double> voltage;
    QVector<double> current;
    QVector<double> power;
    voltage.reserve(m_samples.size());
    current.reserve(m_samples.size());
    power.reserve(m_samples.size());
    for (const auto &sample : m_samples) {
        voltage.push_back(sample.voltage);
        current.push_back(sample.current);
        power.push_back(sample.power);
    }
    if (m_showVoltage) drawSeries(p, plot, voltage, QColor("#3EA6FF"), 24.0, "Voltage");
    if (m_showCurrent) drawSeries(p, plot, current, QColor("#35D6B2"), 6.0, "Current");
    if (m_showPower) drawSeries(p, plot, power, QColor("#F6B84A"), 80.0, "Power");

    p.setFont(labelFont);
    const QVector<QPair<QString, QColor>> legend = {
        {m_legend.value(0, "Voltage"), QColor("#3EA6FF")},
        {m_legend.value(1, "Current"), QColor("#35D6B2")},
        {m_legend.value(2, "Power"), QColor("#F6B84A")}
    };
    int x = 22;
    for (const auto &item : legend) {
        p.setBrush(item.second);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(QRectF(x, height() - 28, 10, 10), 3, 3);
        p.setPen(QColor("#B9C6D3"));
        p.drawText(QRectF(x + 16, height() - 32, 86, 20), item.first);
        x += 96;
    }
}

void ChartWidget::drawSeries(QPainter &p, const QRectF &plot, const QVector<double> &values, QColor color, double maxValue, const QString &) {
    if (values.size() < 2) return;
    QPainterPath path;
    const int n = values.size();
    for (int i = 0; i < n; ++i) {
        const double x = plot.left() + plot.width() * i / qMax(1, n - 1);
        const double clamped = qBound(0.0, values.at(i), maxValue);
        const double y = plot.bottom() - plot.height() * clamped / maxValue;
        i == 0 ? path.moveTo(x, y) : path.lineTo(x, y);
    }
    QColor glow = color;
    glow.setAlpha(55);
    p.setPen(QPen(glow, 7, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.drawPath(path);
    p.setPen(QPen(color, 2.2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.drawPath(path);
}
