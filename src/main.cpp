#include "controller.h"
#ifdef Q_OS_LINUX
#include "panelservice.h"
#endif
#include <QApplication>
#include <QCommandLineParser>
#include <QIcon>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocalServer>
#include <QLocalSocket>
#include <QLockFile>
#include <QMenu>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QStandardPaths>
#include <QSystemTrayIcon>
#include <QDir>
#include <cstdio>

static QIcon appIcon() {
    QIcon icon;
    for (int size : {16, 24, 32, 48, 64, 128, 256, 512})
        icon.addFile(QString(":/icons/%1.png").arg(size), QSize(size, size));
    return icon;
}

int main(int argc, char **argv) {
#ifdef Q_OS_LINUX
    // Prefer the actual session protocol even on desktops where Qt's default
    // would choose XWayland. Honor explicit overrides (including headless QA).
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM") && !qEnvironmentVariableIsEmpty("WAYLAND_DISPLAY"))
        qputenv("QT_QPA_PLATFORM", "wayland");
#endif
    QApplication app(argc, argv);
    app.setOrganizationName("UsageFloat");
    app.setApplicationName("UsageFloat");
    app.setApplicationDisplayName("Usage Float");
    app.setApplicationVersion(USAGE_FLOAT_VERSION);
    app.setDesktopFileName("io.usagefloat.UsageFloat");
    app.setWindowIcon(appIcon());
    QQuickStyle::setStyle("Basic");
    QCommandLineParser parser;
    parser.setApplicationDescription("A small, local Claude Code and Codex allowance monitor.");
    parser.addHelpOption(); parser.addVersionOption();
    parser.addOption({"demo", "Show sample data without reading accounts (separate settings)."});
    parser.addOption({"check", "Read both providers once, print sanitized usage JSON, and exit."});
    parser.addOption({"screenshot", "Save a demo window PNG and exit (requires --demo).", "path"});
    parser.addOption({"compact", "Start in compact mode."});
    parser.addOption({"wide", "Start as a wide capsule with usage bars."});
    parser.addOption({"expanded", "Start in expanded mode."});
    parser.addOption({"settings", "Open the settings panel."});
    parser.process(app);
    if (parser.isSet("screenshot") && !parser.isSet("demo")) parser.showHelp(2);
    const bool demo = parser.isSet("demo");
    if (demo) app.setApplicationName("UsageFloatDemo");
    Controller controller(demo);
    if (parser.isSet("compact")) controller.setCompact(true);
    if (parser.isSet("wide")) { controller.setCompact(true); controller.setWide(true); }
    if (parser.isSet("expanded") || parser.isSet("settings")) controller.setCompact(false);
    if (parser.isSet("check")) {
        bool done = false;
        auto finish = [&] {
            if (done) return;
            done = true;
            const auto claude = controller.claude(), codex = controller.codex();
            const auto out = QJsonDocument(QJsonObject{{"claude", QJsonObject::fromVariantMap(claude)},
                                                       {"codex", QJsonObject::fromVariantMap(codex)}}).toJson();
            std::fwrite(out.constData(), 1, size_t(out.size()), stdout);
            app.exit(claude.value("status") == "ready" && codex.value("status") == "ready" ? 0 : 1);
        };
        QObject::connect(&controller, &Controller::statesChanged, &app, [&] {
            if (!controller.refreshing() && controller.claude().value("status") != "loading"
                && controller.codex().value("status") != "loading") finish();
        });
        QTimer::singleShot(30000, &app, finish);
        return app.exec();
    }
    // A second launch brings the existing widget forward instead of making duplicates.
    const QString runtime = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
    const QString socketPath = (runtime.isEmpty() ? QStandardPaths::writableLocation(QStandardPaths::TempLocation) : runtime)
        + "/usage-float-" + qEnvironmentVariable("USER", qEnvironmentVariable("USERNAME", "desktop")) + (demo ? "-demo" : "");
    QLockFile lock(socketPath + ".lock");
    lock.setStaleLockTime(0);
    if (!lock.tryLock(0)) {
        QLocalSocket socket; socket.connectToServer(socketPath);
        if (socket.waitForConnected(2000)) { socket.write("show\n"); socket.waitForBytesWritten(1000); return 0; }
        std::fprintf(stderr, "Usage Float is already running but could not be contacted.\n");
        return 1;
    }
    QLocalServer::removeServer(socketPath);
    QLocalServer server; server.setSocketOptions(QLocalServer::UserAccessOption);
    if (!server.listen(socketPath)) { std::fprintf(stderr, "Could not create local single-instance socket.\n"); return 1; }
    QObject::connect(&server, &QLocalServer::newConnection, &app, [&] {
        while (auto *socket = server.nextPendingConnection()) {
            controller.showWindow(); socket->disconnectFromServer(); socket->deleteLater();
        }
    });
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("controller", &controller);
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app, [] { QCoreApplication::exit(1); }, Qt::QueuedConnection);
    engine.loadFromModule("UsageFloat", "Main");
    if (engine.rootObjects().isEmpty()) return 1;
    auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
    if (!window) return 1;
    controller.attachWindow(window);
#ifdef Q_OS_LINUX
    PanelService panel(&controller);
    if (!panel.start(demo))
        std::fprintf(stderr, "Panel integration unavailable (session D-Bus connection failed).\n");
#endif
    if (parser.isSet("settings")) window->setProperty("settingsOpen", true);
    QSystemTrayIcon tray(appIcon());
    QMenu menu;
    const QString appDetails = QStringLiteral("Usage Float v%1").arg(app.applicationVersion());
    menu.addAction(appDetails)->setEnabled(false);
    menu.addSeparator();
    menu.addAction("Show Usage Float", &controller, &Controller::showWindow);
    menu.addAction("Refresh usage", &controller, &Controller::refresh);
    menu.addAction("Toggle compact view", &controller, [&] { controller.setCompact(!controller.compact()); controller.showWindow(); });
    auto *wideAction = menu.addAction("Wide usage bars");
    wideAction->setCheckable(true);
    wideAction->setChecked(controller.wide());
    QObject::connect(wideAction, &QAction::triggered, &controller, [&](bool checked) { controller.setWide(checked); controller.setCompact(true); });
    QObject::connect(&controller, &Controller::preferencesChanged, wideAction, [&] { wideAction->setChecked(controller.wide()); });
    auto *remainingAction = menu.addAction("Show remaining instead of used");
    remainingAction->setCheckable(true);
    remainingAction->setChecked(controller.showRemaining());
    QObject::connect(remainingAction, &QAction::triggered, &controller, &Controller::setShowRemaining);
    QObject::connect(&controller, &Controller::preferencesChanged, remainingAction, [&] { remainingAction->setChecked(controller.showRemaining()); });
    menu.addSeparator(); menu.addAction("Quit", &controller, &Controller::quit);
    tray.setToolTip(appDetails + QStringLiteral(" · Claude Code + Codex"));
    tray.setContextMenu(&menu);
    const bool hasTray = QSystemTrayIcon::isSystemTrayAvailable();
    window->setProperty("trayAvailable", hasTray);
    app.setQuitOnLastWindowClosed(!hasTray);
    QObject::connect(&tray, &QSystemTrayIcon::activated, &controller, [&](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) controller.showWindow();
    });
    if (hasTray) tray.show();
#ifdef Q_OS_LINUX
    QObject::connect(&panel, &PanelService::panelActiveChanged, &app, [&](bool active) {
        // The native taskbar widget replaces the duplicate tray icon while connected.
        tray.setVisible(hasTray && !active);
        window->setProperty("trayAvailable", hasTray || active);
        app.setQuitOnLastWindowClosed(!(hasTray || active));
        if (!active && !hasTray) controller.showWindow();
    });
#endif
    if (parser.isSet("screenshot")) {
        QTimer::singleShot(1500, &app, [&] { app.exit(window->grabWindow().save(parser.value("screenshot")) ? 0 : 1); });
    }
    return app.exec();
}
