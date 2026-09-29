// SPDX-License-Identifier: GPL-3.0-or-later
#include "clipboardguard.h"
#include "common/common.h"

#include <QClipboard>
#include <QGuiApplication>

#ifdef Q_OS_MACOS
#include <AppKit/AppKit.h>
#endif

QVariantList backupClipboard()
{
#ifdef Q_OS_MACOS
    QVariantList result;
    @autoreleasepool {
        for (NSPasteboardItem *item in [[NSPasteboard generalPasteboard] pasteboardItems]) {
            QVariantMap data;
            for (NSString *type in [item types]) {
                NSData *bytes = [item dataForType:type];
                if (bytes)
                    data.insert(QString::fromNSString(type), QByteArray(
                        static_cast<const char*>([bytes bytes]), [bytes length]));
            }
            result.append(data);
        }
    }
    return result;
#else
    return {cloneData(QGuiApplication::clipboard()->mimeData())};
#endif
}

void restoreClipboard(const QVariantList &items)
{
#ifdef Q_OS_MACOS
    @autoreleasepool {
        NSMutableArray *objects = [NSMutableArray array];
        for (const auto &value : items) {
            NSPasteboardItem *item = [[NSPasteboardItem alloc] init];
            const auto data = value.toMap();
            for (auto it = data.cbegin(); it != data.cend(); ++it) {
                const auto bytes = it.value().toByteArray();
                [item setData:[NSData dataWithBytes:bytes.constData() length:bytes.size()]
                      forType:it.key().toNSString()];
            }
            [objects addObject:item];
            [item release];
        }
        NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
        [pasteboard clearContents];
        if ([objects count])
            [pasteboard writeObjects:objects];
    }
#else
    QGuiApplication::clipboard()->setMimeData(createMimeData(items.value(0).toMap()));
#endif
}
