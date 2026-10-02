// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QVariantMap>

class ClipboardMerge final
{
public:
    void reset();
    bool armed() const { return m_second >= 0; }
    void endGesture() { if (!armed()) reset(); }
    void copy(qint64 now, const QString &target, const QVariantMap &clipboard);
    void observe(qint64 now, const QString &target, const QVariantMap &clipboard, bool ownWrite, bool allowed);
    QVariantMap take(qint64 now, const QString &target, const QVariantMap &clipboard, const QString &separator);
private:
    qint64 m_first = -1;
    qint64 m_second = -1;
    QString m_target;
    QVariantMap m_previous;
    QVariantMap m_current;
};
