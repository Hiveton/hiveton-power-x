#pragma once

#include "ChartWidget.h"
#include "DataStore.h"
#include "MockUsbTransport.h"

#include <QMainWindow>

class QComboBox;
class QLabel;
class QListWidget;
class QPushButton;
class QSpinBox;
class QStackedWidget;
class QTableWidget;
class QTextEdit;

class DashboardWindow final : public QMainWindow {
    Q_OBJECT
public:
    explicit DashboardWindow(QWidget *parent = nullptr);
    bool saveScreenshot(const QString &path);

private slots:
    void connectDevice();
    void disconnectDevice();
    void toggleRecording();
    void onSample(const LiveSnapshot &snapshot);
    void onPdMessage(const PdMessage &message);
    void refreshTables();
    void addCurrentRecord();
    void editCurrentRecord();
    void deleteCurrentRecord();
    void exportCsv();
    void resetDemoData();
    void switchLanguage(int index);
    void switchPage(int index);
    void clearMessages();
    void requestIdentity();
    void requestPdoRefresh();
    void applySamplingRate(int rate);

private:
    enum class Language { English, Chinese };
    enum class RecordKind { Device, Trigger, Session, Message };

    QWidget *buildRoot();
    QWidget *buildSidebar();
    QWidget *buildTopBar();
    QWidget *buildDashboardPage();
    QWidget *buildPdPage();
    QWidget *buildPdoPage();
    QWidget *buildEmarkerPage();
    QWidget *buildTriggerPage();
    QWidget *buildSessionPage();
    QWidget *buildSettingsPage();
    QWidget *buildCommandPanel();
    QWidget *buildMetrics();
    QWidget *buildInspector();
    QWidget *buildRecordToolbar(RecordKind kind, QTableWidget *table, const QString &title);
    QTableWidget *makeTable(const QStringList &headers);
    QPushButton *makeButton(const QString &text, const QString &objectName = {});
    QLabel *metricValue(const QString &title, const QString &unit, const QString &color);
    QLabel *keyValue(const QString &key, const QString &value);
    QString text(const QString &key) const;
    QStringList texts(std::initializer_list<const char *> keys) const;
    void rebuildUi();
    void applyTheme();
    void setMetric(QLabel *label, double value, const QString &unit, int decimals);
    void fillDeviceTables();
    void fillTriggerTables();
    void fillSessionTables();
    void fillMessageTables();
    void fillPdoTable();
    void updateLiveLabels();
    void updateActionState();
    void wireTableInteractions(QTableWidget *table, RecordKind kind);
    void performAdd(RecordKind kind);
    void performEdit(RecordKind kind, QTableWidget *table);
    void performDelete(RecordKind kind, QTableWidget *table);
    QString selectedId(QTableWidget *table) const;
    void appendLog(const QString &line);

    DataStore m_store;
    MockUsbTransport m_transport;
    Language m_language = Language::English;
    LiveSnapshot m_lastSnapshot;
    int m_currentPage = 0;
    bool m_recording = false;
    bool m_connected = false;

    QStackedWidget *m_pageStack = nullptr;
    QListWidget *m_modeList = nullptr;
    QLabel *m_title = nullptr;
    QLabel *m_connection = nullptr;
    QLabel *m_voltage = nullptr;
    QLabel *m_current = nullptr;
    QLabel *m_power = nullptr;
    QLabel *m_energy = nullptr;
    QLabel *m_temperature = nullptr;
    QLabel *m_negotiated = nullptr;
    QLabel *m_protocol = nullptr;
    QLabel *m_emarker = nullptr;
    QLabel *m_pdoSummary = nullptr;
    QTextEdit *m_log = nullptr;
    ChartWidget *m_chart = nullptr;
    QPushButton *m_recordButton = nullptr;
    QPushButton *m_connectButton = nullptr;
    QPushButton *m_disconnectButton = nullptr;
    QComboBox *m_languageCombo = nullptr;
    QSpinBox *m_samplingRate = nullptr;

    QTableWidget *m_deviceTable = nullptr;
    QTableWidget *m_triggerTable = nullptr;
    QTableWidget *m_triggerPageTable = nullptr;
    QTableWidget *m_sessionTable = nullptr;
    QTableWidget *m_sessionPageTable = nullptr;
    QTableWidget *m_messageTable = nullptr;
    QTableWidget *m_messagePageTable = nullptr;
    QTableWidget *m_pdoTable = nullptr;
};
