// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QDateTime>
#include <QLocale>
#include <QVariantMap>
#include <functional>

struct SnippetContext {
    QDateTime now;
    QLocale locale;
    QString clipboard;
    QStringList history;
    std::function<quint32()> random;
    std::function<QString()> uuid;
    std::function<QVariantMap(const QString &)> reference;
};
struct SnippetResult {
    QVariantMap data;
    QString error;
    int cursor = -1; // UTF-16 offset in the generated plain text.
    bool valid() const { return error.isEmpty(); }
};
SnippetResult renderSnippet(const QVariantMap &data, const SnippetContext &context, bool references = true);
QString matchSnippet(const QString &input, const QVariantList &snippets, const QVariantList &collections, bool anywhere);
int snippetGraphemeCount(const QString &text);
