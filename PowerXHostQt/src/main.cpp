#include "DashboardWindow.h"
#include "DataStore.h"

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QPushButton>
#include <QTimer>

#include <iostream>

namespace {
int runSelfTest() {
    DataStore store;
    store.resetDemoData();
    const int initialDevices = store.devices().size();
    store.upsertDevice({"", "CRUD Device", "T-001", "9.9.9", "Test USB", "USB-C PD Analyzer", "created"});
    if (store.devices().size() != initialDevices + 1) return 2;
    auto edited = store.devices().last();
    edited.note = "edited";
    store.upsertDevice(edited);
    if (store.devices().last().note != "edited") return 3;
    store.removeDevice(edited.id);
    if (store.devices().size() != initialDevices) return 4;

    store.upsertTrigger({"", "CRUD Trigger", "Power", ">", 60.0, "Append event", true});
    const auto triggerId = store.triggers().last().id;
    store.removeTrigger(triggerId);
    store.upsertSession({"", "CRUD Session", QDateTime::currentDateTime(), 50, "test.csv", "created"});
    const auto sessionId = store.sessions().last().id;
    store.removeSession(sessionId);
    store.upsertMessage({"", QDateTime::currentDateTime(), "SRC -> SNK", "Source_Capabilities", "SOP", "1", "0x1234", "decoded"});
    const auto messageId = store.messages().last().id;
    store.removeMessage(messageId);

    DataStore reloaded;
    reloaded.load();
    if (reloaded.devices().isEmpty() || reloaded.triggers().isEmpty()) return 5;
    std::cout << "PowerXHostQt self-test passed. Data file: " << reloaded.storagePath().toStdString() << std::endl;
    return 0;
}
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("Hiveton");
    const QStringList args = QCoreApplication::arguments();
    QCoreApplication::setApplicationName(args.contains("--self-test") ? "PowerXHostQtSelfTest" : "PowerXHostQt");

    if (args.contains("--self-test")) {
        return runSelfTest();
    }

    DashboardWindow window;
    window.show();

    const int screenshotIndex = args.indexOf("--screenshot");
    const int screenshotDirIndex = args.indexOf("--screenshot-dir");
    if (screenshotIndex >= 0 && screenshotIndex + 1 < args.size()) {
        const QString path = args.at(screenshotIndex + 1);
        QDir().mkpath(QFileInfo(path).absolutePath());
        QTimer::singleShot(400, &window, [&window]() { window.findChild<QPushButton *>("primary"); });
        QTimer::singleShot(700, &window, [&window]() { QMetaObject::invokeMethod(&window, "connectDevice"); });
        QTimer::singleShot(2300, &window, [&window, path]() {
            window.saveScreenshot(path);
            qApp->quit();
        });
    } else if (screenshotDirIndex >= 0 && screenshotDirIndex + 1 < args.size()) {
        const QString dir = args.at(screenshotDirIndex + 1);
        QDir().mkpath(dir);
        const QStringList pages = {"dashboard", "pd", "pdo", "emarker", "triggers", "sessions", "settings"};
        QTimer::singleShot(500, &window, [&window]() { QMetaObject::invokeMethod(&window, "connectDevice"); });
        for (int i = 0; i < pages.size(); ++i) {
            QTimer::singleShot(1800 + i * 450, &window, [&window, dir, pages, i]() {
                QMetaObject::invokeMethod(&window, "switchPage", Q_ARG(int, i));
                window.saveScreenshot(QString("%1/en-%2.png").arg(dir, pages.at(i)));
            });
        }
        QTimer::singleShot(1800 + pages.size() * 450, &window, [&window]() {
            QMetaObject::invokeMethod(&window, "switchLanguage", Q_ARG(int, 1));
        });
        for (int i = 0; i < pages.size(); ++i) {
            QTimer::singleShot(2300 + pages.size() * 450 + i * 450, &window, [&window, dir, pages, i]() {
                QMetaObject::invokeMethod(&window, "switchPage", Q_ARG(int, i));
                window.saveScreenshot(QString("%1/zh-%2.png").arg(dir, pages.at(i)));
            });
        }
        QTimer::singleShot(2800 + pages.size() * 900, &window, []() { qApp->quit(); });
    }

    return app.exec();
}
