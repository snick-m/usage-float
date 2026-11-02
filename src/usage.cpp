#include "usage.h"
#include <QJsonArray>
#include <QSet>
#include <cmath>

namespace {
void add(QVariantList &rows, const QString &id, const QString &label,
         const QJsonValue &percent, qint64 reset) {
    // Null/missing quota is unavailable, never a reassuring zero.
    if (!percent.isDouble() || !std::isfinite(percent.toDouble())) return;
    rows.append(QVariantMap{{"id", id}, {"label", label},
        {"percent", qBound(0.0, percent.toDouble(), 100.0)}, {"reset", reset}});
}
qint64 isoTime(const QJsonValue &value) {
    auto date = QDateTime::fromString(value.toString(), Qt::ISODateWithMs);
    if (!date.isValid()) date = QDateTime::fromString(value.toString(), Qt::ISODate);
    return date.isValid() ? date.toSecsSinceEpoch() : 0;
}
}

QVariantList Usage::claudeRows(const QJsonObject &data) {
    QVariantList rows;
    for (const auto &entry : {qMakePair(QString("five_hour"), QString("Session")),
                              qMakePair(QString("seven_day"), QString("Weekly"))}) {
        const auto window = data.value(entry.first).toObject();
        add(rows, entry.first, entry.second, window.value("utilization"), isoTime(window.value("resets_at")));
    }
    QSet<QString> scopedNames;
    for (const auto &value : data.value("limits").toArray()) {
        const auto limit = value.toObject();
        if (limit.value("kind").toString() != "weekly_scoped") continue;
        const auto model = limit.value("scope").toObject().value("model").toObject();
        const auto label = model.value("display_name").toString(model.value("id").toString("Model"));
        scopedNames.insert(label.toLower());
        add(rows, "scoped_" + label, label + " weekly", limit.value("percent"), isoTime(limit.value("resets_at")));
    }
    for (const QString &name : {QString("opus"), QString("sonnet"), QString("oauth_apps"), QString("cowork")}) {
        const auto window = data.value("seven_day_" + name).toObject();
        if (scopedNames.contains(name)) continue;
        QString label = name == "oauth_apps" ? "OAuth apps" : name;
        label[0] = label[0].toUpper();
        add(rows, "seven_day_" + name, label + " weekly", window.value("utilization"), isoTime(window.value("resets_at")));
    }
    return rows;
}

QString Usage::windowLabel(int minutes, const QString &fallback) {
    if (minutes == 300) return "Session";
    if (minutes == 10080) return "Weekly";
    if (minutes == 1440) return "Daily";
    if (minutes > 0 && minutes % 1440 == 0) return QString::number(minutes / 1440) + " days";
    if (minutes > 0 && minutes % 60 == 0) return QString::number(minutes / 60) + " hours";
    if (minutes > 0) return QString::number(minutes) + " minutes";
    return fallback;
}

QVariantList Usage::codexRows(const QJsonObject &data) {
    QVariantList rows;
    auto buckets = data.value("rateLimitsByLimitId").toObject();
    if (buckets.isEmpty() && data.value("rateLimits").isObject())
        buckets.insert("codex", data.value("rateLimits"));
    // Primary Codex allowance comes first; preserve additional named buckets.
    auto keys = buckets.keys();
    if (keys.removeOne("codex")) keys.prepend("codex");
    for (const auto &key : keys) {
        const auto bucket = buckets.value(key).toObject();
        const QString name = bucket.value("limitName").toString(key);
        for (const QString &slot : {QString("primary"), QString("secondary")}) {
            const auto window = bucket.value(slot).toObject();
            QString label = windowLabel(window.value("windowDurationMins").toInt(), slot == "primary" ? "Usage" : "Secondary");
            if (key != "codex") label = name + " · " + label;
            const double reset = window.value("resetsAt").toDouble();
            add(rows, key + "_" + slot, label, window.value("usedPercent"),
                std::isfinite(reset) && reset > 0 && reset < 1e12 ? qint64(reset) : 0);
        }
    }
    return rows;
}

QString Usage::countdown(qint64 reset, qint64 now) {
    if (reset <= 0) return "Reset unknown";
    const qint64 seconds = reset - now;
    if (seconds <= 0) return "Reset due · refresh pending";
    const qint64 minutes = (seconds + 59) / 60;
    if (minutes >= 1440) return QString("%1d %2h").arg(minutes / 1440).arg((minutes % 1440) / 60);
    if (minutes >= 60) return QString("%1h %2m").arg(minutes / 60).arg(minutes % 60);
    return QString("%1m").arg(minutes);
}
