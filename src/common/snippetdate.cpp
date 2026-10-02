// SPDX-License-Identifier: GPL-3.0-or-later
#include "snippetdate.h"
#include <QTimeZone>
#include <qscopeguard.h>
#ifdef Q_OS_MACOS
#include "platform/mac/cfref.h"
#include <CoreFoundation/CoreFoundation.h>
#elif defined(Q_OS_WIN)
#include <icu.h>
#else
#include <unicode/udat.h>
#endif

QString formatSnippetDate(const QDateTime &date, const QLocale &locale,
                         const QString &type, const QString &format, bool *ok)
{
    const QStringList styles{QStringLiteral("full"), QStringLiteral("long"), QStringLiteral("medium"), QStringLiteral("short")};
    const int style = format.isEmpty() ? 2 : int(styles.indexOf(format));
    if (style < 0) {
        bool quoted = false;
        for (const auto character : format) {
            if (character == QLatin1Char('\'')) quoted = !quoted;
            else if (!quoted && character.isLetter() && character.unicode() < 128
                    && !QStringLiteral("GyYuUrQqMLwWdDFgEecabBhHKkjJmsSAzZOvVXx").contains(character)) { *ok = false; return {}; }
        }
        if (quoted) { *ok = false; return {}; }
    }
#ifdef Q_OS_MACOS
    const auto string = [](const QString &text) -> CFRef<CFStringRef> {
        return CFStringCreateWithCharacters(nullptr, reinterpret_cast<const UniChar*>(text.utf16()), text.size());
    };
    CFRef<CFLocaleRef> nativeLocale = locale == QLocale::system() ? CFLocaleCopyCurrent() : CFLocaleCreate(nullptr, string(locale.name()));
    const CFDateFormatterStyle nativeStyle[] = {kCFDateFormatterFullStyle, kCFDateFormatterLongStyle, kCFDateFormatterMediumStyle, kCFDateFormatterShortStyle};
    const auto timeStyle = type == QLatin1String("date") ? kCFDateFormatterNoStyle : nativeStyle[qMax(0, style)];
    const auto dateStyle = type == QLatin1String("time") ? kCFDateFormatterNoStyle : nativeStyle[qMax(0, style)];
    CFRef<CFDateFormatterRef> formatter = CFDateFormatterCreate(nullptr, nativeLocale, dateStyle, timeStyle);
    CFRef<CFTimeZoneRef> zone = CFTimeZoneCreateWithName(nullptr, string(QString::fromUtf8(date.timeZone().id())), false);
    if (!zone) zone = CFTimeZoneCreateWithTimeIntervalFromGMT(nullptr, date.offsetFromUtc());
    if (!formatter || !zone) { *ok = false; return {}; }
    CFDateFormatterSetProperty(formatter, kCFDateFormatterTimeZone, zone);
    if (style < 0) CFDateFormatterSetFormat(formatter, string(format));
    CFRef<CFStringRef> output = CFDateFormatterCreateStringWithAbsoluteTime(nullptr, formatter,
        date.toMSecsSinceEpoch() / 1000.0 - kCFAbsoluteTimeIntervalSince1970);
    if (!output) { *ok = false; return {}; }
    QString text(CFStringGetLength(output), QChar());
    CFStringGetCharacters(output, CFRangeMake(0, text.size()), reinterpret_cast<UniChar*>(text.data()));
    return text;
#else
    UErrorCode status = U_ZERO_ERROR;
    auto zone = QString::fromUtf8(date.timeZone().id());
    if (zone.startsWith(QLatin1String("UTC+")) || zone.startsWith(QLatin1String("UTC-"))) zone.replace(0, 3, QStringLiteral("GMT"));
    const auto localeName = locale.name().toUtf8();
    const auto nativeStyle = style < 0 ? UDAT_PATTERN : static_cast<UDateFormatStyle>(style);
    const auto timeStyle = style < 0 ? UDAT_PATTERN : type == QLatin1String("date") ? UDAT_NONE : nativeStyle;
    const auto dateStyle = style < 0 ? UDAT_PATTERN : type == QLatin1String("time") ? UDAT_NONE : nativeStyle;
    const auto formatter = udat_open(timeStyle, dateStyle, localeName.constData(),
        reinterpret_cast<const UChar*>(zone.utf16()), int(zone.size()),
        reinterpret_cast<const UChar*>(format.utf16()), int(format.size()), &status);
    const auto close = qScopeGuard([&] { if (formatter) udat_close(formatter); });
    if (U_FAILURE(status)) { *ok = false; return {}; }
    const auto instant = static_cast<UDate>(date.toMSecsSinceEpoch());
    const int size = udat_format(formatter, instant, nullptr, 0, nullptr, &status);
    if (status != U_BUFFER_OVERFLOW_ERROR && U_FAILURE(status)) { *ok = false; return {}; }
    status = U_ZERO_ERROR;
    QString output(size + 1, QChar());
    const int written = udat_format(formatter, instant, reinterpret_cast<UChar*>(output.data()), size + 1, nullptr, &status);
    if (U_FAILURE(status)) { *ok = false; return {}; }
    output.truncate(written);
    return output;
#endif
}
