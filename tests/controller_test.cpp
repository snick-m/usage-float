#include "controller.h"
#include <QCoreApplication>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QTest>
#include <iostream>

// The test executable doubles as a Codex protocol fixture. No real CLI or
// provider endpoint is used by these lifecycle tests.
static int fakeCodex() {
    std::string line;
    while (std::getline(std::cin, line)) {
        const auto request = QJsonDocument::fromJson(QByteArray::fromStdString(line)).object();
        const int id = request.value("id").toInt(-1);
        if (id < 0) continue;
        QJsonObject reply{{"id", id}};
        if (id == 1) reply.insert("result", QJsonObject{});
        else if (qEnvironmentVariable("USAGE_FLOAT_TEST_CASE") == "rpc-error")
            reply.insert("error", QJsonObject{{"code", -32000}, {"message", "SECRET_MUST_NOT_REACH_UI"}});
        else reply.insert("result", QJsonObject{{"rateLimits", QJsonObject{
            {"planType", "plus"}, {"primary", QJsonObject{{"usedPercent", 19}, {"windowDurationMins", 300}, {"resetsAt", 1893456000}}}}}});
        std::cout << QJsonDocument(reply).toJson(QJsonDocument::Compact).constData() << std::endl;
    }
    return 0;
}

class ControllerTest : public QObject {
    Q_OBJECT
    QTemporaryDir data;
private slots:
    void initTestCase() {
        QVERIFY(data.isValid());
        qputenv("CLAUDE_CONFIG_DIR", data.path().toUtf8());
        QCoreApplication::setOrganizationName("UsageFloatTests");
        QCoreApplication::setApplicationName("Isolated");
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, data.path());
    }
    void init() {
        QSettings().clear();
        qunsetenv("USAGE_FLOAT_TEST_CASE");
    }
    void independentProvidersAndStaleReading() {
        Controller controller(false);
        controller.setCodexPath(QCoreApplication::applicationFilePath());
        QTRY_COMPARE_WITH_TIMEOUT(controller.codex().value("status").toString(), "ready", 4000);
        QCOMPARE(controller.claude().value("status").toString(), "error");
        QCOMPARE(controller.codex().value("rows").toList().first().toMap().value("percent").toInt(), 19);
        QVERIFY(!controller.codex().value("stale").toBool());
        // Let the successful child termination be reaped before the next read.
        QTest::qWait(50);
        qputenv("USAGE_FLOAT_TEST_CASE", "rpc-error");
        controller.refresh();
        QTRY_COMPARE_WITH_TIMEOUT(controller.codex().value("status").toString(), "error", 4000);
        QVERIFY(controller.codex().value("stale").toBool());
        QCOMPARE(controller.codex().value("rows").toList().first().toMap().value("percent").toInt(), 19);
        QVERIFY(!controller.codex().value("message").toString().contains("SECRET"));
        QVERIFY(!controller.refreshing());
    }
    void missingExecutableSettles() {
        Controller controller(false);
        controller.setCodexPath(data.path() + "/nonexistent-codex");
        QTRY_COMPARE_WITH_TIMEOUT(controller.codex().value("status").toString(), "error", 3000);
        QVERIFY(!controller.codex().value("hasData").toBool());
        QVERIFY(!controller.refreshing());
    }
    void demoDoesNotReadAccounts() {
        Controller controller(true);
        QTRY_COMPARE(controller.claude().value("status").toString(), "ready");
        QCOMPARE(controller.codex().value("status").toString(), "ready");
        QVERIFY(!controller.refreshing());
    }
};

int main(int argc, char **argv) {
    if (argc > 1 && QByteArray(argv[1]) == "app-server") return fakeCodex();
    QCoreApplication app(argc, argv);
    ControllerTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "controller_test.moc"
