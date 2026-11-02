#pragma once
#include <QDateTime>
#include <QJsonObject>
#include <QVariantList>

namespace Usage {
QVariantList claudeRows(const QJsonObject &response);
QVariantList codexRows(const QJsonObject &response);
QString countdown(qint64 resetSeconds, qint64 nowSeconds);
QString windowLabel(int minutes, const QString &fallback);
}
