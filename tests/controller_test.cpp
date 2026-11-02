#include "controller.h"
#include <QCoreApplication>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QTest>
#include <QTimeZone>
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
        else if (id == 3) {
            if (qEnvironmentVariable("USAGE_FLOAT_TEST_CASE") == "config-error")
                reply.insert("error", QJsonObject{{"message", "Config metadata unavailable"}});
            else reply.insert("result", QJsonObject{{"config", QJsonObject{{"model", "test-model"}}}});
        }
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
        qputenv("CODEX_HOME", data.path().toUtf8());
        // An explicit empty credential document also prevents macOS from
        // falling back to the real user Keychain while tests are running.
        QFile credentials(data.path() + "/.credentials.json");
        QVERIFY(credentials.open(QIODevice::WriteOnly));
        QCOMPARE(credentials.write("{}"), qint64(2));
        credentials.close();
        QCoreApplication::setOrganizationName("UsageFloatTests");
        QCoreApplication::setApplicationName("Isolated");
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, data.path());
    }
    void init() {
        QFile::remove(data.path() + "/usage-float.json");
        QDir(data.path() + "/sessions").removeRecursively();
        QSettings().clear();
        qunsetenv("USAGE_FLOAT_TEST_CASE");
    }
    void hybridDefaultMigratesExistingInterval() {
        QSettings().setValue("intervalMinutes", 3);
        Controller controller(true);
        QCOMPARE(controller.intervalMinutes(), 15);
        controller.setIntervalMinutes(20);
        Controller next(true);
        QCOMPARE(next.intervalMinutes(), 20);
    }
    void localUpdateAfterPollAndOlderDataCannotRegress() {
        Controller controller(false);
        controller.setCodexPath(QCoreApplication::applicationFilePath());
        QTRY_COMPARE_WITH_TIMEOUT(controller.codex().value("status").toString(), "ready", 4000);
        qputenv("USAGE_FLOAT_TEST_CASE", "rpc-error"); // Local checks must not poll.
        QTest::qWait(1100); // Local sample must be newer than the completed poll.
        const qint64 now = QDateTime::currentSecsSinceEpoch();
        QDir().mkpath(data.path() + "/sessions");
        QFile session(data.path() + "/sessions/test.jsonl");
        QVERIFY(session.open(QIODevice::WriteOnly));
        const QJsonObject event{{"timestamp", QDateTime::fromSecsSinceEpoch(now, QTimeZone::UTC).toString(Qt::ISODateWithMs)},
            {"type", "event_msg"}, {"payload", QJsonObject{{"type", "token_count"}, {"rate_limits", QJsonObject{
                {"limit_id", "codex"}, {"primary", QJsonObject{{"used_percent", 27}, {"window_minutes", 300}, {"resets_at", now + 3600}}}}}}}};
        session.write(QJsonDocument(event).toJson(QJsonDocument::Compact) + '\n'); session.close();
        // A separate reader validates the real local fixture without waiting
        // for the controller's minute-long file discovery interval.
        LocalUsage reader;
        QCOMPARE(reader.read(now).first().rows.first().toMap().value("percent").toInt(), 27);
        QFile claude(data.path() + "/usage-float.json");
        QVERIFY(claude.open(QIODevice::WriteOnly));
        claude.write(QJsonDocument(QJsonObject{{"observed_at", now}, {"rate_limits", QJsonObject{
            {"five_hour", QJsonObject{{"used_percentage", 38}, {"resets_at", now + 3600}}}}}}).toJson());
        claude.close();
        QTRY_COMPARE_WITH_TIMEOUT(controller.claude().value("status").toString(), "ready", 6000);
        QCOMPARE(controller.claude().value("rows").toList().first().toMap().value("percent").toInt(), 38);
        QCOMPARE(controller.claude().value("source").toString(), "Claude Code local status line");
        const auto observed = controller.claude().value("updated");
        QVERIFY(claude.open(QIODevice::WriteOnly | QIODevice::Truncate));
        claude.write(QJsonDocument(QJsonObject{{"observed_at", now - 30}, {"rate_limits", QJsonObject{
            {"five_hour", QJsonObject{{"used_percentage", 5}, {"resets_at", now + 3600}}}}}}).toJson());
        claude.close();
        QTest::qWait(5100);
        QCOMPARE(controller.codex().value("status").toString(), "ready");
        QCOMPARE(controller.claude().value("updated"), observed);
        QCOMPARE(controller.claude().value("rows").toList().first().toMap().value("percent").toInt(), 38);
    }
    void independentProvidersAndStaleReading() {
        Controller controller(false);
        controller.setCodexPath(QCoreApplication::applicationFilePath());
        QTRY_COMPARE_WITH_TIMEOUT(controller.codex().value("status").toString(), "ready", 4000);
        QCOMPARE(controller.claude().value("status").toString(), "error");
        QCOMPARE(controller.codex().value("rows").toList().first().toMap().value("percent").toInt(), 19);
        QCOMPARE(controller.codex().value("model").toString(), "test-model");
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
    void modelMetadataFailureDoesNotBlockUsage() {
        qputenv("USAGE_FLOAT_TEST_CASE", "config-error");
        Controller controller(false);
        controller.setCodexPath(QCoreApplication::applicationFilePath());
        QTRY_COMPARE_WITH_TIMEOUT(controller.codex().value("status").toString(), "ready", 4000);
        QCOMPARE(controller.codex().value("model").toString(), "Auto");
        QCOMPARE(controller.codex().value("rows").toList().first().toMap().value("percent").toInt(), 19);
    }
    void demoDoesNotReadAccounts() {
        Controller controller(true);
        QTRY_COMPARE(controller.claude().value("status").toString(), "ready");
        QCOMPARE(controller.codex().value("status").toString(), "ready");
        QVERIFY(!controller.refreshing());
    }
    void remainingModePreservesUsedAndUnknownData() {
        Controller controller(true);
        // Switching units does not turn an unavailable reading into 100% left.
        controller.setShowRemaining(true);
        QVERIFY(controller.claude().value("rows").toList().isEmpty());
        QTRY_COMPARE(controller.claude().value("status").toString(), "ready");
        auto row = controller.claude().value("rows").toList().first().toMap();
        QCOMPARE(row.value("percent").toDouble(), 32.0);
        QCOMPARE(row.value("displayPercent").toDouble(), 68.0);
        QCOMPARE(controller.codex().value("rows").toList().first().toMap().value("displayPercent").toDouble(), 84.0);
        controller.setShowRemaining(false);
        row = controller.claude().value("rows").toList().first().toMap();
        QCOMPARE(row.value("displayPercent").toDouble(), 32.0);
    }
};

int main(int argc, char **argv) {
    if (argc > 1 && QByteArray(argv[1]) == "app-server") return fakeCodex();
    QCoreApplication app(argc, argv);
    ControllerTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "controller_test.moc"
