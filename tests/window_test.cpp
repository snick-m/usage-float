#include "controller.h"
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickItem>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QTemporaryDir>
#include <QTest>
#include <cmath>

class WindowTest : public QObject {
    Q_OBJECT
    QTemporaryDir settings;
private slots:
    void initTestCase() {
        QVERIFY(settings.isValid());
        QCoreApplication::setOrganizationName("UsageFloatTests");
        QCoreApplication::setApplicationName("WindowTransitions");
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settings.path());
    }
    void repeatedExpandAndCollapseFitsContents() {
        QSettings().clear();
        Controller controller(true);
        QQmlApplicationEngine engine;
        engine.rootContext()->setContextProperty("controller", &controller);
        engine.load(QUrl::fromLocalFile(QStringLiteral(USAGE_FLOAT_SOURCE_DIR "/qml/Main.qml")));
        QVERIFY(!engine.rootObjects().isEmpty());
        auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
        QVERIFY(window);
        controller.attachWindow(window);
        auto *scroll = window->findChild<QQuickItem *>("detailsScroll");
        QVERIFY(scroll);
        QTRY_COMPARE(controller.claude().value("status").toString(), "ready");
        for (int i = 0; i < 8; ++i) {
            controller.setWide(i % 2);
            controller.setCompact(false);
            QTRY_COMPARE(window->height(), window->property("targetHeight").toInt());
            QVERIFY(window->height() > 280);
            const int content = int(std::ceil(scroll->property("contentHeight").toDouble()));
            const int available = window->property("availableHeight").toInt();
            if (content <= available) QVERIFY2(window->height() >= content, "Details require scrolling despite available screen space");
            controller.setCompact(true);
            QTRY_COMPARE(window->height(), 44);
            QTRY_COMPARE(window->width(), i % 2 ? 536 : 328);
        }
        controller.setCompact(false);
        window->setProperty("settingsOpen", true);
        QTRY_COMPARE(window->height(), window->property("targetHeight").toInt());
        window->setProperty("settingsOpen", false);
        QTRY_COMPARE(window->height(), window->property("targetHeight").toInt());
        controller.setCompact(true);
        QTRY_COMPARE(window->height(), 44);
    }
};

int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    QQuickStyle::setStyle("Basic");
    WindowTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "window_test.moc"
