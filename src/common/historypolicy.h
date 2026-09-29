// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QVariantMap>
#include <QPersistentModelIndex>

namespace HistoryPolicy {
qint64 copiedAt(const QVariantMap &data);
bool isRecent(const QVariantMap &data, qint64 now, qint64 duration);
bool isExpired(const QVariantMap &data, qint64 now, qint64 duration);
bool accepts(const QVariantMap &data, const QStringList &types, const QStringList &ignoredApplications);
enum class Cleanup { Recent, Expired, All };
QList<QPersistentModelIndex> candidates(QAbstractItemModel *model, qint64 now, qint64 duration, Cleanup cleanup);
}
