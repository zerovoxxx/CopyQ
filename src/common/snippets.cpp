// SPDX-License-Identifier: GPL-3.0-or-later
#include "snippets.h"
#include "snippetdate.h"
#include "common/mimetypes.h"
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QTextBoundaryFinder>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextBlock>
#include <QTextFragment>
#include <QUuid>
#include <limits>

namespace {
QString transform(QString text, const QString &modifier, bool *ok)
{
    if (modifier == QLatin1String("uppercase")) return text.toUpper();
    if (modifier == QLatin1String("lowercase")) return text.toLower();
    if (modifier == QLatin1String("trim")) return text.trimmed();
    if (modifier == QLatin1String("capitals") || modifier == QLatin1String("capitalcase")) {
        bool first = true;
        for (qsizetype i = 0; i < text.size(); ++i) {
            if (text[i].isLetter()) { text[i] = first ? text[i].toUpper() : text[i].toLower(); first = false; }
            else if (!text[i].isMark()) first = true;
        }
        return text;
    }
    if (modifier == QLatin1String("reverse")) {
        QString result;
        QTextBoundaryFinder finder(QTextBoundaryFinder::Grapheme, text);
        int start = 0;
        for (int end = finder.toNextBoundary(); end >= 0; end = finder.toNextBoundary()) {
            result.prepend(text.mid(start, end - start));
            start = end;
        }
        return result;
    }
    if (modifier == QLatin1String("stripdiacritics")) {
        text = text.normalized(QString::NormalizationForm_D);
        text.remove(QRegularExpression(QStringLiteral("\\p{M}")));
        return text.normalized(QString::NormalizationForm_C);
    }
    if (modifier == QLatin1String("stripnonalphanumeric")) {
        text.remove(QRegularExpression(QStringLiteral("[^\\p{L}\\p{N}\\p{M}]")));
        return text;
    }
    *ok = false;
    return {};
}

struct Replacement { int start; int length; QString text; QString html; int cursor; };
}

SnippetResult renderSnippet(const QVariantMap &data, const SnippetContext &context, bool references)
{
    SnippetResult result;
    result.data = data;
    QTextDocument document;
    const bool rich = data.contains(mimeHtml);
    if (rich) document.setHtml(QString::fromUtf8(data.value(mimeHtml).toByteArray()));
    else document.setPlainText(QString::fromUtf8(data.value(mimeText).toByteArray()));
    const auto original = document.toPlainText();
    const QRegularExpression tokens(QStringLiteral("\\{([^{}]*)\\}"));
    auto iterator = tokens.globalMatch(original);
    QList<Replacement> replacements;
    int cursorCount = 0;
    while (iterator.hasNext()) {
        const auto token = iterator.next();
        const auto expression = token.captured(1);
        QString output, html;
        int cursorOffset = -1;
        bool known = true, ok = true;
        const QRegularExpression dateExpression(QStringLiteral("^(iso)?(date|time|datetime)((?:\\s+[+-]\\d+[YMDdhms])*)(?::(.*))?$"));
        const auto dateMatch = dateExpression.match(expression);
        if (dateMatch.hasMatch()) {
            auto date = context.now;
            if (!date.isValid()) ok = false;
            const QRegularExpression arithmetic(QStringLiteral("([+-]\\d+)([YMDdhms])"));
            auto operations = arithmetic.globalMatch(dateMatch.captured(3));
            while (ok && operations.hasNext()) {
                const auto operation = operations.next();
                const int value = operation.captured(1).toInt(&ok);
                const auto unit = operation.captured(2);
                if (unit == QLatin1String("Y")) date = date.addYears(value);
                else if (unit == QLatin1String("M")) date = date.addMonths(value);
                else if (unit == QLatin1String("D") || unit == QLatin1String("d")) date = date.addDays(value);
                else date = date.addSecs(qint64(value) * (unit == QLatin1String("h") ? 3600 : unit == QLatin1String("m") ? 60 : 1));
                ok = ok && date.isValid();
            }
            const bool iso = !dateMatch.captured(1).isEmpty();
            if (iso && date.offsetFromUtc() == 0) date = date.toUTC();
            const auto type = dateMatch.captured(2);
            const auto format = dateMatch.captured(4);
            const auto locale = iso ? QLocale(QLocale::English, QLocale::UnitedStates) : context.locale;
            if (iso && format.isEmpty()) {
                output = type == QLatin1String("date") ? date.date().toString(Qt::ISODate)
                    : type == QLatin1String("time") ? date.time().toString(Qt::ISODate) : date.toString(Qt::ISODate);
            } else output = formatSnippetDate(date, locale, type, format, &ok);
        } else if (expression == QLatin1String("cursor")) {
            cursorOffset = 0;
            ok = ++cursorCount == 1;
        } else if (expression.startsWith(QLatin1String("snippet:"))) {
            if (!references) known = false;
            else {
                const auto body = context.reference ? context.reference(expression.mid(8)) : QVariantMap();
                if (body.isEmpty()) ok = false;
                else {
                    auto nested = renderSnippet(body, context, false);
                    ok = nested.valid();
                    output = QString::fromUtf8(nested.data.value(mimeText).toByteArray());
                    if (nested.cursor >= 0) { cursorOffset = nested.cursor; ok = ok && ++cursorCount == 1; }
                    if (rich && nested.data.contains(mimeHtml)) {
                        QTextDocument fragment;
                        fragment.setHtml(QString::fromUtf8(nested.data.value(mimeHtml).toByteArray()));
                        html = fragment.toHtml();
                    }
                }
            }
        } else if (expression == QLatin1String("clipboard") || expression.startsWith(QLatin1String("clipboard:")) || expression.startsWith(QLatin1String("clipboard."))) {
            const auto parts = expression.split(QLatin1Char('.'));
            const auto base = parts.first();
            if (base == QLatin1String("clipboard")) output = context.clipboard;
            else {
                const auto offset = base.mid(10).toInt(&ok);
                ok = ok && offset >= 0 && offset < context.history.size();
                if (ok) output = context.history.at(offset);
            }
            for (qsizetype i = 1; ok && i < parts.size(); ++i) output = transform(output, parts.at(i), &ok);
        } else if (expression.startsWith(QLatin1String("random:"))) {
            const auto variation = expression.mid(7);
            const auto random = context.random ? context.random() : QRandomGenerator::global()->generate();
            if (variation == QLatin1String("UUID")) output = context.uuid ? context.uuid() : QUuid::createUuid().toString(QUuid::WithoutBraces);
            else if (variation.contains(QLatin1String(".."))) {
                const auto bounds = variation.split(QLatin1String(".."));
                qint64 low = std::numeric_limits<qint32>::min(), high = std::numeric_limits<qint32>::max();
                if (bounds.size() != 2) ok = false;
                else {
                    if (!bounds.first().isEmpty()) low = bounds.first().toLongLong(&ok);
                    bool highOk = true;
                    if (!bounds.last().isEmpty()) high = bounds.last().toLongLong(&highOk);
                    ok = ok && highOk && low >= std::numeric_limits<qint32>::min() && high <= std::numeric_limits<qint32>::max() && low <= high;
                    if (ok) output = QString::number(low + qint64(quint64(random) % quint64(high-low+1)));
                }
            } else {
                const auto choices = variation.split(QLatin1Char(','));
                ok = !variation.isEmpty();
                if (ok) output = choices.at(random % quint32(choices.size()));
            }
        } else {
            known = false;
            for (const auto prefix : {"date", "time", "datetime", "isodate", "isotime", "isodatetime", "random", "cursor"})
                if (expression == QLatin1String(prefix) || expression.startsWith(QLatin1String(prefix) + QLatin1Char(':')) || expression.startsWith(QLatin1String(prefix) + QLatin1Char(' '))) { known = true; ok = false; }
        }
        if (!ok) { result.error = QObject::tr("Invalid placeholder: {%1}").arg(expression); result.data.clear(); return result; }
        if (known) replacements.append({int(token.capturedStart()), int(token.capturedLength()), output, html, cursorOffset});
    }
    QTextCursor position(&document);
    bool hasCursor = false;
    for (qsizetype i = replacements.size(); i-- > 0;) {
        const auto &replacement = replacements.at(i);
        QTextCursor cursor(&document);
        cursor.setPosition(replacement.start);
        cursor.setPosition(replacement.start + replacement.length, QTextCursor::KeepAnchor);
        if (!replacement.html.isEmpty()) {
            QTextDocument fragment;
            fragment.setHtml(replacement.html);
            cursor.removeSelectedText();
            bool first = true;
            for (auto block = fragment.begin(); block.isValid(); block = block.next()) {
                if (!first) cursor.insertBlock(block.blockFormat());
                first = false;
                for (auto item = block.begin(); !item.atEnd(); ++item) {
                    const auto part = item.fragment();
                    if (part.isValid()) cursor.insertText(part.text(), part.charFormat());
                }
            }
        }
        else cursor.insertText(replacement.text, cursor.charFormat());
        if (replacement.cursor >= 0) {
            position.setPosition(replacement.start + replacement.cursor);
            hasCursor = true;
        }
    }
    auto plain = document.toPlainText();
    if (hasCursor) {
        result.cursor = position.position();
        QTextBoundaryFinder boundary(QTextBoundaryFinder::Grapheme,plain);
        boundary.setPosition(result.cursor);
        if (!boundary.isAtBoundary()) {
            result.error=QObject::tr("Place the cursor between complete Unicode characters.");
            result.data.clear(); return result;
        }
    }
    if (data.contains(mimeText) || rich) result.data.insert(mimeText, plain.toUtf8());
    if (rich) result.data.insert(mimeHtml, document.toHtml().toUtf8());
    return result;
}

int snippetGraphemeCount(const QString &text)
{
    QTextBoundaryFinder finder(QTextBoundaryFinder::Grapheme, text);
    int count = 0;
    while (finder.toNextBoundary() >= 0) ++count;
    return count;
}

QString matchSnippet(const QString &input, const QVariantList &snippets, const QVariantList &collections, bool anywhere)
{
    QHash<QString, QVariantMap> groups;
    for (const auto &value : collections) groups.insert(value.toMap().value(QStringLiteral("id")).toString(), value.toMap());
    QHash<QString, int> counts;
    QString matched;
    qsizetype longest = 0;
    for (const auto &value : snippets) {
        const auto snippet = value.toMap();
        const auto group = groups.value(snippet.value(QStringLiteral("collection")).toString());
        const auto keyword = snippet.value(QStringLiteral("keyword")).toString();
        if (keyword.isEmpty()) continue;
        const auto resolved = group.value(QStringLiteral("prefix")).toString() + keyword + group.value(QStringLiteral("suffix")).toString();
        ++counts[resolved];
        if (!snippet.value(QStringLiteral("enabled")).toBool() || !group.value(QStringLiteral("enabled")).toBool() || !input.endsWith(resolved)) continue;
        const auto before = input.size() - resolved.size();
        if (!anywhere && before > 0 && (input.at(before-1).isLetterOrNumber() || input.at(before-1) == QLatin1Char('_'))) continue;
        if (resolved.size() > longest) { longest = resolved.size(); matched = snippet.value(QStringLiteral("id")).toString(); }
    }
    if (matched.isEmpty()) return {};
    for (const auto &value : snippets) {
        const auto snippet = value.toMap();
        if (snippet.value(QStringLiteral("id")).toString() != matched) continue;
        const auto group = groups.value(snippet.value(QStringLiteral("collection")).toString());
        const auto resolved = group.value(QStringLiteral("prefix")).toString() + snippet.value(QStringLiteral("keyword")).toString() + group.value(QStringLiteral("suffix")).toString();
        if (counts.value(resolved) != 1) return {};
    }
    return matched;
}
