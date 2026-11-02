#include "localusage.h"
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <QTest>
#include <QTimeZone>

class LocalUsageTest : public QObject {
    Q_OBJECT
private slots:
    void incrementalPartialRecordsAndBuckets() {
        QTemporaryDir home;
        QVERIFY(home.isValid());
        qputenv("CODEX_HOME", home.path().toUtf8());
        qputenv("CLAUDE_CONFIG_DIR", home.path().toUtf8());
        QVERIFY(QDir().mkpath(home.path() + "/sessions"));
        const qint64 now = QDateTime::currentSecsSinceEpoch();
        auto event = [now](const QString &id, int percent, qint64 at) {
            return QJsonDocument(QJsonObject{{"type", "event_msg"},
                {"timestamp", QDateTime::fromSecsSinceEpoch(at, QTimeZone::UTC).toString(Qt::ISODateWithMs)},
                {"payload", QJsonObject{{"type", "token_count"}, {"rate_limits", QJsonObject{
                    {"limit_id", id}, {"primary", QJsonObject{{"used_percent", percent},
                    {"window_minutes", 300}, {"resets_at", now + 3600}}}}}}}}).toJson(QJsonDocument::Compact) + '\n';
        };
        QFile file(home.path() + "/sessions/test.jsonl");
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("malformed\n");
        file.write(event("codex", 0, now - 10));
        const auto next = event("codex", 37, now);
        file.write(next.left(next.size() / 2)); file.flush();
        LocalUsage reader;
        auto snapshots = reader.read(now);
        QCOMPARE(snapshots.size(), 1);
        QCOMPARE(snapshots.first().rows.first().toMap().value("percent").toInt(), 0);
        file.write(next.mid(next.size() / 2));
        file.write(event("codex", 4, now - 5)); // Out-of-order record must not regress the latest.
        file.write(event("other", 52, now));
        file.flush();
        snapshots = reader.read(now);
        QCOMPARE(snapshots.size(), 2);
        for (const auto &snapshot : snapshots) {
            const auto row = snapshot.rows.first().toMap();
            QCOMPARE(row.value("percent").toInt(), row.value("id") == "codex_primary" ? 37 : 52);
        }
        QCOMPARE(reader.read(now).size(), 2); // Unchanged file retains the real observation time.
    }
    void missingNullAndExpiredClaudeWindows() {
        QTemporaryDir home;
        qputenv("CODEX_HOME", home.path().toUtf8());
        qputenv("CLAUDE_CONFIG_DIR", home.path().toUtf8());
        LocalUsage reader;
        const qint64 now = QDateTime::currentSecsSinceEpoch();
        QVERIFY(reader.read(now).isEmpty());
        QFile file(home.path() + "/usage-float.json");
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(QJsonDocument(QJsonObject{{"observed_at", now}, {"rate_limits", QJsonObject{
            {"five_hour", QJsonObject{{"used_percentage", QJsonValue::Null}, {"resets_at", now + 3600}}},
            {"seven_day", QJsonObject{{"used_percentage", 72}, {"resets_at", now - 10}}}}}}).toJson());
        file.close();
        QVERIFY(reader.read(now).isEmpty());
    }
};
QTEST_GUILESS_MAIN(LocalUsageTest)
#include "localusage_test.moc"
