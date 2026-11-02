#include "usage.h"
#include <QJsonArray>
#include <QTest>

class UsageTest : public QObject {
    Q_OBJECT
private slots:
    void claudeDoesNotInventZero() {
        const auto rows = Usage::claudeRows({{"five_hour", QJsonObject{{"utilization", 0}, {"resets_at", "2026-11-02T19:20:00.412+00:00"}}},
            {"seven_day", QJsonObject{{"utilization", QJsonValue::Null}}},
            {"seven_day_opus", QJsonValue::Null}});
        QCOMPARE(rows.size(), 1);
        QCOMPARE(rows[0].toMap().value("percent").toDouble(), 0.0);
        QVERIFY(rows[0].toMap().value("reset").toLongLong() > 0);
        QVERIFY(Usage::claudeRows({}).isEmpty());
    }
    void codexNamedBucketsAndFallback() {
        const QJsonObject window{{"usedPercent", 27}, {"windowDurationMins", 10080}, {"resetsAt", 1793778228.0}};
        const QJsonObject base{{"secondary", window}};
        const auto fallback = Usage::codexRows({{"rateLimits", base}});
        QCOMPARE(fallback.size(), 1);
        QCOMPARE(fallback[0].toMap().value("label").toString(), "Weekly");
        const auto named = Usage::codexRows({{"rateLimits", base}, {"rateLimitsByLimitId", QJsonObject{
            {"codex", base}, {"review", QJsonObject{{"limitName", "Review"}, {"primary", window}}}}}});
        QCOMPARE(named.size(), 2); // Don't double-count rateLimits + rateLimitsByLimitId.
        QCOMPARE(named[1].toMap().value("label").toString(), "Review · Weekly");
        QVERIFY(Usage::codexRows({}).isEmpty());
    }
    void scopedLimits() {
        const auto rows = Usage::claudeRows({{"limits", QJsonArray{QJsonObject{
            {"kind", "weekly_scoped"}, {"percent", 123}, {"scope", QJsonObject{
                {"model", QJsonObject{{"display_name", "Opus"}}}}}}}},
            {"seven_day_opus", QJsonObject{{"utilization", 90}}}});
        QCOMPARE(rows.size(), 1);
        QCOMPARE(rows[0].toMap().value("percent").toDouble(), 100.0);
    }
    void countdownDoesNotClaimResetSucceeded() {
        QCOMPARE(Usage::countdown(0, 100), "Reset unknown");
        QCOMPARE(Usage::countdown(100, 101), "Reset due · refresh pending");
        QCOMPARE(Usage::countdown(101, 100), "1m");
        QCOMPARE(Usage::countdown(3700, 100), "1h 0m");
        QCOMPARE(Usage::countdown(90100, 100), "1d 1h");
    }
};
QTEST_GUILESS_MAIN(UsageTest)
#include "usage_test.moc"
