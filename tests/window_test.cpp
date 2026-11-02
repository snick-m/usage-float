#include "controller.h"
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickItem>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QTemporaryDir>
#include <QTest>
#include <QSet>
#include <cmath>
#include <functional>

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
            QTRY_COMPARE_WITH_TIMEOUT(window->height(), window->property("targetHeight").toInt(), 3000);
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
    void tooltipIsDarkAndNotClippedToCapsule() {
        QSettings().clear();
        Controller controller(true);
        controller.setWide(true);
        QQmlApplicationEngine engine;
        engine.rootContext()->setContextProperty("controller", &controller);
        engine.load(QUrl::fromLocalFile(QStringLiteral(USAGE_FLOAT_SOURCE_DIR "/qml/Main.qml")));
        QVERIFY(!engine.rootObjects().isEmpty());
        auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
        QVERIFY(window);
        controller.attachWindow(window);
        QTRY_COMPARE(controller.claude().value("status").toString(), "ready");
        QSet<QObject *> seen;
        QObject *tooltip = nullptr;
        // Repeater delegates may have visual parents distinct from QObject
        // ownership, so include both trees when locating the actual popup.
        std::function<void(QObject *)> visit = [&](QObject *object) {
            if (!object || seen.contains(object) || tooltip) return;
            seen.insert(object);
            if (object->objectName() == "providerToolTip") { tooltip = object; return; }
            for (auto *child : object->children()) visit(child);
            if (auto *item = qobject_cast<QQuickItem *>(object))
                for (auto *child : item->childItems()) visit(child);
        };
        visit(window); visit(window->contentItem());
        QVERIFY(tooltip);
        tooltip->setProperty("visible", true);
        QQuickWindow *popup = nullptr;
        QTRY_VERIFY(([&] {
            for (auto *candidate : QGuiApplication::topLevelWindows())
                if (candidate != window && candidate->isVisible() && candidate->height() > 44)
                    if ((popup = qobject_cast<QQuickWindow *>(candidate))) return true;
            return false;
        })());
        QVERIFY(popup->height() > window->height());
        QImage image;
        QTRY_VERIFY(!(image = popup->grabWindow()).isNull());
        QVERIFY2(image.pixelColor(10, 10).lightnessF() < 0.5, "Provider tooltip rendered a light background");
        tooltip->setProperty("visible", false);
    }
};

int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    QQuickStyle::setStyle("Basic");
    WindowTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "window_test.moc"
