#include "localusage.h"
#include "usage.h"
#include <QDateTime>
#include <QTimeZone>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <algorithm>

namespace {
constexpr qint64 tailBytes = 512 * 1024;
QString home(const char *variable, const QString &fallback) {
    return qEnvironmentVariable(variable, QDir::homePath() + fallback);
}
QJsonObject readObject(const QString &path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly) || file.size() > tailBytes) return {};
    return QJsonDocument::fromJson(file.readAll()).object();
}
QJsonObject codexWindow(const QJsonObject &window) {
    return {{"usedPercent", window.value("used_percent")},
            {"windowDurationMins", window.value("window_minutes")},
            {"resetsAt", window.value("resets_at")}};
}
}

QList<LocalSnapshot> LocalUsage::read(qint64 now) {
    QList<LocalSnapshot> result;
    const auto claude = readObject(home("CLAUDE_CONFIG_DIR", "/.claude") + "/usage-float.json");
    QJsonObject limits;
    for (const QString &key : {QString("five_hour"), QString("seven_day")}) {
        const auto window = claude.value("rate_limits").toObject().value(key).toObject();
        const qint64 reset = qint64(window.value("resets_at").toDouble());
        if (reset <= now) continue;
        limits.insert(key, QJsonObject{{"utilization", window.value("used_percentage")},
            {"resets_at", QDateTime::fromSecsSinceEpoch(reset, QTimeZone::UTC).toString(Qt::ISODate)}});
    }
    const auto claudeRows = Usage::claudeRows(limits);
    if (!claudeRows.isEmpty()) result.append({"claude", "Claude Code local status line", "", claudeRows,
                                            qint64(claude.value("observed_at").toDouble())});

    // Enumerate metadata once a minute; only tail the most recently modified
    // sessions, and retain offsets so unchanged transcripts are never reread.
    if (m_discovered == 0 || now - m_discovered >= 60) {
        QList<QFileInfo> files;
        QDirIterator it(home("CODEX_HOME", "/.codex") + "/sessions", {"*.jsonl"}, QDir::Files,
                        QDirIterator::Subdirectories);
        while (it.hasNext()) { it.next(); files.append(it.fileInfo()); }
        std::sort(files.begin(), files.end(), [](const QFileInfo &a, const QFileInfo &b) {
            return a.lastModified() > b.lastModified();
        });
        QHash<QString, Cursor> cursors;
        for (qsizetype i = 0; i < qMin(qsizetype(16), files.size()); ++i)
            cursors.insert(files[i].absoluteFilePath(), m_cursors.value(files[i].absoluteFilePath()));
        m_cursors = cursors;
        m_discovered = now;
    }
    for (auto it = m_cursors.begin(); it != m_cursors.end(); ++it) {
        QFile file(it.key());
        if (!file.open(QIODevice::ReadOnly)) continue;
        auto &offset = it.value().offset;
        if (offset == file.size()) continue;
        qint64 start = offset < 0 || offset > file.size() ? qMax(qint64(0), file.size() - tailBytes) : offset;
        start = qMax(start, file.size() - tailBytes);
        file.seek(start);
        if (start > 0 && start != offset) file.readLine(tailBytes); // partial first record
        while (!file.atEnd()) {
            const qint64 lineStart = file.pos();
            const auto line = file.readLine(tailBytes + 1);
            if (!line.endsWith('\n')) {
                // A writer can be midway through appending a record. Retry it.
                if (file.atEnd()) { file.seek(lineStart); break; }
                while (!file.atEnd() && !file.readLine(tailBytes).endsWith('\n')) {}
                continue;
            }
            const auto event = QJsonDocument::fromJson(line).object();
            const auto payload = event.value("payload").toObject();
            if (event.value("type") != "event_msg" || payload.value("type") != "token_count") continue;
            const auto rate = payload.value("rate_limits").toObject();
            const QString id = rate.value("limit_id").toString("codex");
            const qint64 observed = QDateTime::fromString(event.value("timestamp").toString(), Qt::ISODateWithMs).toSecsSinceEpoch();
            if (rate.isEmpty() || observed <= m_codex.value(id).observed || observed > now + 5) continue;
            QJsonObject bucket{{"limitName", rate.value("limit_name")}};
            for (const QString &slot : {QString("primary"), QString("secondary")}) {
                const auto window = rate.value(slot).toObject();
                if (window.value("resets_at").toDouble() > now) bucket.insert(slot, codexWindow(window));
            }
            const auto rows = Usage::codexRows({{"rateLimitsByLimitId", QJsonObject{{id, bucket}}}});
            if (!rows.isEmpty()) m_codex.insert(id, {"codex", "Codex local session log",
                                                   rate.value("plan_type").toString(), rows, observed});
        }
        offset = file.pos();
    }
    for (const auto &snapshot : m_codex) result.append(snapshot);
    return result;
}
