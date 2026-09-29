// SPDX-License-Identifier: GPL-3.0-or-later
#include "historypolicy.h"
#include "common/mimetypes.h"
#include "common/contenttype.h"
#include <QAbstractItemModel>

qint64 HistoryPolicy::copiedAt(const QVariantMap &data)
{
    bool valid = false;
    const auto time = data.value(mimeHistoryTime).toByteArray().toLongLong(&valid);
    return valid && time >= 0 ? time : -1;
}

bool HistoryPolicy::isRecent(const QVariantMap &data, qint64 now, qint64 duration)
{
    const auto time = copiedAt(data);
    return time >= 0 && now >= time && duration >= 0 && now - time <= duration;
}

bool HistoryPolicy::isExpired(const QVariantMap &data, qint64 now, qint64 duration)
{
    const auto time = copiedAt(data);
    return time >= 0 && now >= time && duration > 0 && now - time > duration;
}

bool HistoryPolicy::accepts(const QVariantMap &data, const QStringList &types, const QStringList &ignoredApplications)
{
    if (data.value(mimeSecret).toByteArray() == "1")
        return false;
    const auto application = QString::fromUtf8(data.value(mimeSourceApplication).toByteArray());
    if (!application.isEmpty() && ignoredApplications.contains(application, Qt::CaseInsensitive))
        return false;
    QString type;
    if (data.contains(mimeUriList)) type = QStringLiteral("files");
    else {
        for (auto it = data.cbegin(); it != data.cend(); ++it) {
            if (it.key().startsWith(QLatin1String("image/"))) { type = QStringLiteral("image"); break; }
        }
        if (type.isEmpty()) type = data.contains(mimeText) || data.contains(mimeTextUtf8) || data.contains(mimeHtml)
            ? QStringLiteral("text") : QStringLiteral("other");
    }
    return types.contains(type);
}

QList<QPersistentModelIndex> HistoryPolicy::candidates(QAbstractItemModel *model, qint64 now, qint64 duration, Cleanup cleanup)
{
    QList<QPersistentModelIndex> result;
    if (!model) return result;
    for (int row = 0; row < model->rowCount(); ++row) {
        const auto index = model->index(row, 0);
        const auto data = index.data(contentType::data).toMap();
        if (data.contains(QLatin1String(COPYQ_MIME_PREFIX "item-pinned"))) continue;
        const bool match = cleanup == Cleanup::All
            || (cleanup == Cleanup::Recent ? isRecent(data, now, duration) : isExpired(data, now, duration));
        if (match) result.append(index);
    }
    return result;
}
