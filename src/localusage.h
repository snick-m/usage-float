#pragma once
#include <QHash>
#include <QJsonObject>
#include <QVariantList>

struct LocalSnapshot {
    QString provider;
    QString source;
    QString plan;
    QVariantList rows;
    qint64 observed = 0;
};

// Reads allowance snapshots only. Never infers plan percentages from tokens.
class LocalUsage {
public:
    QList<LocalSnapshot> read(qint64 now);
private:
    struct Cursor { qint64 offset = -1; };
    QHash<QString, Cursor> m_cursors;
    QHash<QString, LocalSnapshot> m_codex;
    qint64 m_discovered = 0;
};
