// SPDX-License-Identifier: GPL-3.0-or-later
#include "clipboardmerge.h"
#include "common/mimetypes.h"
namespace {
bool textOnly(const QVariantMap &data)
{
    if (!data.contains(mimeText) || data.value(mimeText).toByteArray().isEmpty()) return false;
    for (auto it = data.cbegin(); it != data.cend(); ++it)
        if ((it.key().startsWith(QLatin1String("image/")) || it.key() == mimeUriList || it.key().contains(QLatin1String("Concealed")) || it.key() == QLatin1String("application/x-copyq-hidden"))) return false;
    return true;
}
}
void ClipboardMerge::reset()
{
    m_first = m_second = -1; m_target.clear(); m_previous.clear(); m_current.clear();
}
void ClipboardMerge::copy(qint64 now, const QString &target, const QVariantMap &clipboard)
{
    if (!textOnly(clipboard) || target.isEmpty()) { reset(); return; }
    if (m_first >= 0 && m_second < 0 && target == m_target && now >= m_first && now-m_first <= 400) {
        m_second = now; return;
    }
    reset(); m_first = now; m_target = target; m_previous = clipboard;
}
void ClipboardMerge::observe(qint64 now, const QString &target, const QVariantMap &clipboard, bool ownWrite, bool allowed)
{
    if (ownWrite || !allowed || !textOnly(clipboard) || target != m_target || m_first < 0 || now < m_first || now-m_first > 800) { reset(); return; }
    if (clipboard.value(mimeText).toByteArray() != m_previous.value(mimeText).toByteArray()) m_current = clipboard;
}
QVariantMap ClipboardMerge::take(qint64 now, const QString &target, const QVariantMap &clipboard, const QString &separator)
{
    if (m_second < 0 || now < m_second || now-m_second > 400 || target != m_target || !textOnly(clipboard)
            || m_current.isEmpty() || clipboard.value(mimeText).toByteArray() != m_current.value(mimeText).toByteArray()) { reset(); return {}; }
    const auto joined = m_previous.value(mimeText).toByteArray() + separator.toUtf8() + m_current.value(mimeText).toByteArray();
    reset(); return {{mimeText, joined}};
}
