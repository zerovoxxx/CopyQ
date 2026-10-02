// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QDateTime>
#include <QLocale>
QString formatSnippetDate(const QDateTime &date, const QLocale &locale,
                         const QString &type, const QString &format, bool *ok);
