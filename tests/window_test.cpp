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
#include <QSignalSpy>
#include <QPointer>
#include <cmath>
#include <functional>

static QObject *findVisualObject(QObject *root, const QString &name) {
    QSet<QObject *> seen;
    std::function<QObject *(QObject *)> visit = [&](QObject *object) -> QObject * {
        if (!object || seen.contains(object)) return nullptr;
        seen.insert(object);
        if (object->objectName() == name) return object;
        for (auto *child : object->children())
            if (auto *found = visit(child)) return found;
        if (auto *item = qobject_cast<QQuickItem *>(object))
            for (auto *child : item->childItems())
                if (auto *found = visit(child)) return found;
        return nullptr;
    };
    return visit(root);
}

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
    void refreshKeepsUsageBarsAlive() {
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
        QList<QPointer<QObject>> bars;
        for (const auto &name : {"ClaudeUsageFill0", "ClaudeUsageFill1", "CodexUsageFill0", "CodexUsageFill1",
                                 "claudeUsageMeter0", "claudeUsageMeter1", "codexUsageMeter0", "codexUsageMeter1"}) {
            auto *bar = findVisualObject(window, QString::fromLatin1(name));
            QVERIFY2(bar, name);
            bars.append(bar);
        }
        auto *fill = qobject_cast<QQuickItem *>(bars.first().data());
        QVERIFY(fill);
        QTRY_VERIFY(fill->width() > 0);
        QTest::qWait(500); // Let the initial layout and width animation settle.
        QSignalSpy widthChanges(fill, &QQuickItem::widthChanged);
        controller.refresh();
        QCoreApplication::processEvents();
        for (const auto &bar : bars)
            QVERIFY2(bar, "Refresh recreated a usage bar, restarting its animation");
        QTest::qWait(500);
        QCOMPARE(widthChanges.count(), 0); // An unchanged reading must not animate.
        bool reachedZero = false;
        connect(fill, &QQuickItem::widthChanged, this, [&] { reachedZero |= fill->width() <= 0; });
        controller.setShowRemaining(true);
        QTRY_VERIFY(std::abs(fill->width() / fill->parentItem()->width() - 0.68) < 0.01);
        QVERIFY(!reachedZero);
        for (const auto &bar : bars) QVERIFY(bar);
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
        QVERIFY2(popup->flags().testFlag(Qt::WindowTransparentForInput),
                 "Tooltip surface must pass pointer input through to the hovered provider");
        QVERIFY(popup->flags().testFlag(Qt::WindowDoesNotAcceptFocus));
        QVERIFY(!window->flags().testFlag(Qt::WindowTransparentForInput));
        QImage image;
        QTRY_VERIFY(!(image = popup->grabWindow()).isNull());
        QVERIFY2(image.pixelColor(10, 10).lightnessF() < 0.5, "Provider tooltip rendered a light background");
        QSignalSpy destroyed(tooltip, &QObject::destroyed);
        // Clock ticks and provider updates must update bindings without replacing
        // the hovered provider delegate and closing its popup.
        for (int i = 0; i < 3; ++i) {
            controller.statesChanged();
            QCoreApplication::processEvents();
            QVERIFY2(destroyed.isEmpty(), "Usage update destroyed the open provider tooltip");
            QVERIFY(tooltip->property("visible").toBool());
            QVERIFY(popup->isVisible());
        }
        controller.setShowRemaining(true);
        QCoreApplication::processEvents();
        QVERIFY(destroyed.isEmpty());
        QVERIFY(tooltip->property("text").toString().contains("% remaining"));
        QVERIFY(tooltip->property("visible").toBool());
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
