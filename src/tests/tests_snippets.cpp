// SPDX-License-Identifier: GPL-3.0-or-later
#include "app/app.h"
#include "common/common.h"
#include "common/config.h"
#include "common/contenttype.h"
#include <QSignalSpy>
#include "common/mimetypes.h"
#include "common/snippets.h"
#include "common/clipboardmerge.h"
#include "common/copyqimport.h"
#include "gui/clipboardsnippets.h"
#include "gui/menuitems.h"
#include "common/settings.h"
#include "item/itemeditorwidget.h"
#include "item/clipboardmodel.h"
#include <QQuickItem>
#include <QSettings>
#include "item/snippetstore.h"
#include "item/serialize.h"
#include "item/itemstore.h"
#include "platform/platformnativeinterface.h"
#include "tests/clipboardguard.h"
#include <QApplication>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>
#include <QTextDocument>
#include <QTextCursor>
#include <QTimeZone>
#include <QProcess>
#ifdef Q_OS_MACOS
#include "platform/mac/cfref.h"
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#elif defined(Q_OS_WIN)
#include <qt_windows.h>
#endif

int typeNativeText(const QString &text)
{
#ifdef Q_OS_MACOS
    if (!AXIsProcessTrusted() || IsSecureEventInputEnabled()) return 77;
    for (const auto character : text) {
        const UniChar value = character.unicode();
        CFRef<CGEventRef> down = CGEventCreateKeyboardEvent(nullptr, kVK_ANSI_A, true);
        CFRef<CGEventRef> up = CGEventCreateKeyboardEvent(nullptr, kVK_ANSI_A, false);
        CGEventKeyboardSetUnicodeString(down, 1, &value); CGEventKeyboardSetUnicodeString(up, 1, &value);
        CGEventSetFlags(down, 0); CGEventSetFlags(up, 0);
        CGEventPost(kCGHIDEventTap, down); CGEventPost(kCGHIDEventTap, up);
        QTest::qWait(30);
    }
    return 0;
#elif defined(Q_OS_WIN)
    for (const auto character : text) {
        const auto code = VkKeyScanW(character.unicode());
        if (code == -1) return 77;
        INPUT events[2]{};
        events[0].type = events[1].type = INPUT_KEYBOARD;
        events[0].ki.wVk = events[1].ki.wVk = WORD(code & 255);
        events[1].ki.dwFlags = KEYEVENTF_KEYUP;
        const bool shift = code & 0x100;
        if (shift) { INPUT modifier{}; modifier.type = INPUT_KEYBOARD; modifier.ki.wVk = VK_SHIFT; if (SendInput(1, &modifier, sizeof(INPUT)) != 1) return 1; }
        if (SendInput(2, events, sizeof(INPUT)) != 2) return 1;
        if (shift) { INPUT modifier{}; modifier.type = INPUT_KEYBOARD; modifier.ki.wVk = VK_SHIFT; modifier.ki.dwFlags = KEYEVENTF_KEYUP; SendInput(1, &modifier, sizeof(INPUT)); }
        QTest::qWait(30);
    }
    return 0;
#else
    return QProcess::execute(QStringLiteral("xdotool"), {QStringLiteral("type"), QStringLiteral("--delay"), QStringLiteral("30"), QStringLiteral("--"), text});
#endif
}

int doubleNativeCopy()
{
#ifdef Q_OS_MACOS
    if (!AXIsProcessTrusted() || IsSecureEventInputEnabled()) return 77;
    CFRef<CGEventRef> modifierDown = CGEventCreateKeyboardEvent(nullptr, kVK_Command, true);
    CGEventSetFlags(modifierDown, kCGEventFlagMaskCommand); CGEventPost(kCGHIDEventTap, modifierDown);
    for (int i = 0; i < 2; ++i) {
        CFRef<CGEventRef> down = CGEventCreateKeyboardEvent(nullptr, kVK_ANSI_C, true);
        CFRef<CGEventRef> up = CGEventCreateKeyboardEvent(nullptr, kVK_ANSI_C, false);
        CGEventSetFlags(down, kCGEventFlagMaskCommand); CGEventSetFlags(up, kCGEventFlagMaskCommand);
        CGEventPost(kCGHIDEventTap, down); CGEventPost(kCGHIDEventTap, up); QTest::qWait(80);
    }
    CFRef<CGEventRef> modifierUp = CGEventCreateKeyboardEvent(nullptr, kVK_Command, false);
    CGEventSetFlags(modifierUp, 0); CGEventPost(kCGHIDEventTap, modifierUp); return 0;
#elif defined(Q_OS_WIN)
    INPUT event{}; event.type = INPUT_KEYBOARD; event.ki.wVk = VK_CONTROL;
    if (SendInput(1, &event, sizeof(INPUT)) != 1) return 1;
    for (int i = 0; i < 2; ++i) {
        INPUT keys[2]{}; keys[0].type = keys[1].type = INPUT_KEYBOARD;
        keys[0].ki.wVk = keys[1].ki.wVk = 'C'; keys[1].ki.dwFlags = KEYEVENTF_KEYUP;
        SendInput(2, keys, sizeof(INPUT)); QTest::qWait(80);
    }
    event.ki.dwFlags = KEYEVENTF_KEYUP; return SendInput(1, &event, sizeof(INPUT)) == 1 ? 0 : 1;
#else
    return QProcess::execute(QStringLiteral("xdotool"), {QStringLiteral("keydown"), QStringLiteral("Control_L"), QStringLiteral("key"), QStringLiteral("c"), QStringLiteral("sleep"), QStringLiteral("0.08"), QStringLiteral("key"), QStringLiteral("c"), QStringLiteral("keyup"), QStringLiteral("Control_L")});
#endif
}

class SnippetTests final : public QObject
{
    Q_OBJECT
    SnippetContext context() const
    {
        SnippetContext ctx;
        ctx.now = QDateTime(QDate(2024, 1, 31), QTime(12, 34, 56), QTimeZone("UTC"));
        ctx.locale = QLocale(QLocale::English, QLocale::UnitedStates);
        ctx.clipboard = QString::fromUtf8("  é中👩‍💻  ");
        ctx.history = {QStringLiteral("new"), QStringLiteral("old")};
        ctx.random = [] { return quint32(4); };
        ctx.uuid = [] { return QStringLiteral("fixed-uuid"); };
        return ctx;
    }
    SnippetResult render(const QString &text) const { return renderSnippet({{mimeText, text.toUtf8()}}, context()); }
private slots:
    void storeRoundTrip()
    {
        QTemporaryDir dir;
        const auto path = dir.filePath(QStringLiteral("snippets.dat"));
        SnippetStore store(path);
        const auto group = store.createCollection(QStringLiteral("Work"));
        const auto id = store.createSnippet(group, {{mimeText, QByteArray("body")}, {mimeHtml, QByteArray("<b>body</b>")}, {QStringLiteral("custom/mime"), QByteArray("custom")}});
        QVERIFY(!id.isEmpty());
        QVERIFY(store.updateSnippet(id, {{QStringLiteral("keyword"), QStringLiteral("sig")}, {QStringLiteral("future"), 42}}));
        QVERIFY(store.updateCollection(group, {{QStringLiteral("prefix"), QStringLiteral("!")}, {QStringLiteral("name"), QStringLiteral("Renamed")}}));
        QCOMPARE(store.effectiveKeyword(store.snippet(id)), QStringLiteral("!sig"));
        QVERIFY(store.save());
        cleanDataFiles({});
        SnippetStore restarted(path);
        QVERIFY(restarted.load());
        QCOMPARE(restarted.document(), store.document());
        SnippetStore imported(dir.filePath(QStringLiteral("import.dat")));
        QVERIFY(imported.importCollection(store.exportCollection(group)));
        QCOMPARE(imported.snippet(id), store.snippet(id));
        const auto saved = imported.document();
        QVERIFY(!imported.importCollection(store.exportCollection(group)));
        QCOMPARE(imported.document(), saved);
        QVERIFY(!imported.importCollection(QByteArray("damaged")));
        QCOMPARE(imported.document(), saved);
        QVERIFY(imported.removeCollection(group));
        QVERIFY(imported.snippets().isEmpty());
    }
    void damagedStorage()
    {
        QTemporaryDir dir;
        const auto path = dir.filePath(QStringLiteral("snippets.dat"));
        SnippetStore store(path);
        auto doc = store.document();
        doc[QStringLiteral("version")] = 99;
        QVERIFY(!store.setDocument(doc));
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("damaged"); file.close();
        QVERIFY(!store.load());
        QVERIFY(!store.writable());
        QVERIFY(store.createCollection(QStringLiteral("Forbidden")).isEmpty());
        QVERIFY(!store.save());
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(file.readAll(), QByteArray("damaged"));
    }
    void encryptedStorage()
    {
#ifdef WITH_QCA_ENCRYPTION
        QVERIFY(Encryption::initialize());
        Encryption::EncryptionKey key;
        QVERIFY(key.generateRandomDEK());
        QTemporaryDir dir;
        SnippetStore store(dir.filePath(QStringLiteral("encrypted.dat")));
        const auto collection = store.createCollection(QStringLiteral("Private"));
        const auto id = store.createSnippet(collection, {{mimeText, QByteArray("SECRET-TEMPLATE")}});
        QVERIFY(store.save(key));
        QFile file(dir.filePath(QStringLiteral("encrypted.dat")));
        QVERIFY(file.open(QIODevice::ReadOnly));
        QVERIFY(!file.readAll().contains("SECRET-TEMPLATE"));
        file.close();
        SnippetStore reader(file.fileName());
        QVERIFY(reader.load(key));
        QCOMPARE(reader.snippet(id), store.snippet(id));
        Encryption::EncryptionKey replacement;
        QVERIFY(replacement.generateRandomDEK());
        QVERIFY(reader.save(replacement));
        SnippetStore locked(file.fileName());
        QVERIFY(!locked.load(key)); QVERIFY(!locked.writable());
        QVERIFY(locked.createCollection(QStringLiteral("Blocked")).isEmpty());
        QVERIFY(locked.load(replacement)); QCOMPARE(locked.snippet(id), store.snippet(id));
        QVERIFY(locked.save());
        SnippetStore plaintext(file.fileName()); QVERIFY(plaintext.load());
        QCOMPARE(plaintext.snippet(id), store.snippet(id));
#else
        QSKIP("QCA disabled in this build; encryption gate remains unverified.");
#endif
    }
    void storageLimit()
    {
        QTemporaryDir dir;
        SnippetStore store(dir.filePath(QStringLiteral("snippets.dat")));
        const auto group=store.createCollection(QStringLiteral("Large"));
        const auto id=store.createSnippet(group,{{mimeText,QByteArray("original")}});
        QVERIFY(store.save());
        QFile file(dir.filePath(QStringLiteral("snippets.dat"))); QVERIFY(file.open(QIODevice::ReadOnly));
        const auto before=file.readAll(); file.close();
        QVERIFY(store.updateSnippet(id,{{QStringLiteral("data"),QVariantMap{{mimeText,QByteArray(100'000'000,'x')}}}}));
        QVERIFY(!store.save()); QVERIFY(file.open(QIODevice::ReadOnly)); QCOMPARE(file.readAll(),before);
        SnippetStore reader(file.fileName()); QVERIFY(reader.load());
        QCOMPARE(reader.snippet(id).value(QStringLiteral("data")).toMap().value(mimeText).toByteArray(),QByteArray("original"));
    }
    void dynamicDates()
    {
        auto result = render(QStringLiteral("{isodate +1M:yyyy-MM-dd} {time -20m:HH:mm} {date:EEEE} {isodatetime}"));
        QVERIFY2(result.valid(), qPrintable(result.error));
        QCOMPARE(result.data.value(mimeText).toByteArray(), QByteArray("2024-02-29 12:14 Wednesday 2024-01-31T12:34:56Z"));
        QVERIFY(!render(QStringLiteral("{date:unsupported}")).valid());
        QVERIFY(!render(QStringLiteral("{date +no}")).valid());
        auto ctx = context();
        ctx.now = QDateTime(QDate(2024,3,9), QTime(12,0), QTimeZone("America/New_York"));
        result = renderSnippet({{mimeText, QByteArray("{isodatetime +1D}")}}, ctx);
        QVERIFY(result.valid());
        QCOMPARE(result.data.value(mimeText).toByteArray(), QByteArray("2024-03-10T12:00:00-04:00"));
        ctx.now = QDateTime(QDate(2024,1,31),QTime(12,34),QTimeZone(19800));
        result = renderSnippet({{mimeText,QByteArray("{time:HH:mm XXX}")}},ctx);
        QVERIFY2(result.valid(),qPrintable(result.error));
        QCOMPARE(result.data.value(mimeText).toByteArray(),QByteArray("12:34 +05:30"));
    }
    void dynamicClipboardAndRandom()
    {
        auto result = render(QStringLiteral("{clipboard.trim.stripdiacritics.reverse} {clipboard:1.uppercase} {random:1..5} {random:a,b,c} {random:UUID}"));
        QVERIFY2(result.valid(), qPrintable(result.error));
        QCOMPARE(QString::fromUtf8(result.data.value(mimeText).toByteArray()), QString::fromUtf8("👩‍💻中e OLD 5 b fixed-uuid"));
        QVERIFY(!render(QStringLiteral("{clipboard:2}")).valid());
        QVERIFY(!render(QStringLiteral("{random:5..1}")).valid());
        QVERIFY(!render(QStringLiteral("{clipboard.invalid}")).valid());
        QCOMPARE(render(QStringLiteral("{unknown} {var:workflow}")).data.value(mimeText).toByteArray(), QByteArray("{unknown} {var:workflow}"));
        QCOMPARE(snippetGraphemeCount(QString::fromUtf8("中👩‍💻é")), 3);
        QVERIFY(!render(QStringLiteral("e{cursor}\u0301")).valid());
        auto ctx = context(); ctx.clipboard = QStringLiteral("{date}");
        QCOMPARE(renderSnippet({{mimeText, QByteArray("{clipboard}")}}, ctx).data.value(mimeText).toByteArray(), QByteArray("{date}"));
    }
    void richTextAndReferences()
    {
        auto ctx = context();
        ctx.clipboard = QStringLiteral("<unsafe>&");
        ctx.reference = [](const QString &keyword) -> QVariantMap {
            return keyword == QLatin1String("!ref") ? QVariantMap{{mimeText, QByteArray("nested {snippet:!ref}")}, {mimeHtml, QByteArray("<i>nested {snippet:!ref}</i>")}} : QVariantMap();
        };
        auto result = renderSnippet({{mimeHtml, QByteArray("<p><b>Hello {clipboard}</b>{cursor} {snippet:!ref}</p>")}}, ctx);
        QVERIFY2(result.valid(), qPrintable(result.error));
        QCOMPARE(result.data.value(mimeText).toByteArray(), QByteArray("Hello <unsafe>& nested {snippet:!ref}"));
        QCOMPARE(result.cursor, 15);
        QTextDocument doc; doc.setHtml(QString::fromUtf8(result.data.value(mimeHtml).toByteArray()));
        QCOMPARE(doc.toPlainText().toUtf8(), result.data.value(mimeText).toByteArray());
        QTextCursor cursor(&doc); cursor.setPosition(7);
        QCOMPARE(cursor.charFormat().fontWeight(), int(QFont::Bold));
        cursor.setPosition(20);
        QVERIFY(cursor.charFormat().fontItalic());
        QVERIFY(!render(QStringLiteral("{cursor}{cursor}")).valid());
        QVERIFY(!renderSnippet({{mimeText, QByteArray("{snippet:missing}")}}, ctx).valid());
    }
    void copyMerging()
    {
        ClipboardMerge merge;
        const QVariantMap previous{{mimeText, QByteArray("previous")}};
        const QVariantMap current{{mimeText, QByteArray("current")}};
        merge.copy(0, QStringLiteral("target"), previous);
        merge.observe(20, QStringLiteral("target"), current, false, true);
        merge.copy(100, QStringLiteral("target"), current);
        merge.endGesture();
        merge.observe(110, QStringLiteral("target"), current, false, true);
        QCOMPARE(merge.take(150, QStringLiteral("target"), current, QStringLiteral("\n")).value(mimeText).toByteArray(), QByteArray("previous\ncurrent"));
        QVERIFY(merge.take(160, QStringLiteral("target"), current, QString()).isEmpty());
        merge.copy(1000, QStringLiteral("target"), previous);
        merge.observe(1020, QStringLiteral("target"), current, false, true);
        merge.copy(1500, QStringLiteral("target"), current);
        QVERIFY(merge.take(1520, QStringLiteral("target"), current, QString()).isEmpty());
        merge.copy(2000, QStringLiteral("target"), previous);
        merge.observe(2020, QStringLiteral("target"), current, true, true);
        merge.copy(2100, QStringLiteral("target"), current);
        QVERIFY(merge.take(2120, QStringLiteral("target"), current, QString()).isEmpty());
        merge.copy(3000, QStringLiteral("target"), previous);
        merge.observe(3020, QStringLiteral("other"), current, false, true);
        merge.copy(3100, QStringLiteral("target"), current);
        QVERIFY(merge.take(3120, QStringLiteral("target"), current, QString()).isEmpty());
        merge.copy(4000, QStringLiteral("target"), previous);
        merge.observe(4020, QStringLiteral("target"), {{mimeUriList, QByteArray("file:///tmp/test")}}, false, true);
        QVERIFY(!merge.armed());
        merge.copy(5000, QStringLiteral("target"), previous);
        merge.observe(5020, QStringLiteral("target"), previous, false, true);
        merge.copy(5100, QStringLiteral("target"), previous);
        QVERIFY(merge.take(5120, QStringLiteral("target"), previous, QString()).isEmpty());
    }
    void copyQMigration()
    {
        QTemporaryDir dir;
        const auto configPath = dir.filePath(QStringLiteral("copyq.ini"));
        QSettings config(configPath, QSettings::IniFormat);
        config.setValue(QStringLiteral("Options/tabs"), QStringList{QStringLiteral("History")});
        config.setValue(QStringLiteral("Options/maxitems"), 1000);
        config.setValue(QStringLiteral("Options/autostart"), true);
        config.setValue(QStringLiteral("Options/asset"), dir.filePath(QStringLiteral("scripts/tool.js")));
        config.sync();
        QVERIFY(QDir().mkpath(dir.filePath(QStringLiteral("scripts"))));
        QFile asset(dir.filePath(QStringLiteral("scripts/tool.js")));
        QVERIFY(asset.open(QIODevice::WriteOnly)); QCOMPARE(asset.write("print('asset')"), qint64(14)); asset.close();
        ClipboardModel model;
        model.insertItem({{mimeText, QByteArray("history")}, {QStringLiteral("custom/mime"), QByteArray("payload")}}, 0);
        QFile tab(dir.filePath(QStringLiteral("copyq_tab_SGlzdG9yeQ==.dat")));
        QVERIFY(tab.open(QIODevice::WriteOnly));
        QVERIFY(serializeData(model, &tab)); tab.close();
        QFile sourceConfig(configPath); QVERIFY(sourceConfig.open(QIODevice::ReadOnly));
        const auto before = sourceConfig.readAll(); sourceConfig.close();
        QByteArray archive; QString error;
        QVERIFY2(prepareCopyQImport(dir.path(), &archive, &error), qPrintable(error));
        QDataStream stream(archive); stream.setVersion(QDataStream::Qt_4_7);
        QByteArray header; QVariantMap metadata, history;
        stream >> header >> metadata >> history;
        QCOMPARE(header, QByteArray("CopyQ v4"));
        QCOMPARE(metadata.value(QStringLiteral("settings")).toMap().value(QStringLiteral("Options/maxitems")).toInt(), 1000);
        QVERIFY(!metadata.value(QStringLiteral("settings")).toMap().contains(QStringLiteral("Options/autostart")));
        QVERIFY(metadata.value(QStringLiteral("settings")).toMap().value(QStringLiteral("Options/asset")).toString().startsWith(QStringLiteral("qclip-import://")));
        const auto assets = metadata.value(QStringLiteral("files")).toMap();
        QTemporaryDir destination;
        QVERIFY2(saveCopyQImportFiles(assets,destination.path(),&error),qPrintable(error));
        QVariantMap reread;
        QVERIFY(readCopyQImportFiles(destination.path(),&reread,&error)); QCOMPARE(reread,assets);
        QVERIFY(!saveCopyQImportFiles({{QStringLiteral("../escape"),QByteArray("bad")}},destination.path(),&error));
#ifdef Q_OS_UNIX
        QFile linkSource(destination.filePath(QStringLiteral("outside")));
        QVERIFY(linkSource.open(QIODevice::WriteOnly)); linkSource.write("original"); linkSource.close();
        QVERIFY(QFile::link(linkSource.fileName(),destination.filePath(QStringLiteral("linked"))));
        QVERIFY(!saveCopyQImportFiles({{QStringLiteral("linked"),QByteArray("bad")}},destination.path(),&error));
        QVERIFY(linkSource.open(QIODevice::ReadOnly)); QCOMPARE(linkSource.readAll(),QByteArray("original"));
#endif
        ClipboardModel restored;
        QDataStream data(history.value(QStringLiteral("data")).toByteArray());
        QVERIFY(deserializeData(&restored, &data));
        QCOMPARE(restored.rowCount(), 1);
        QCOMPARE(restored.index(0,0).data(contentType::data), model.index(0,0).data(contentType::data));
        QVERIFY(sourceConfig.open(QIODevice::ReadOnly)); QCOMPARE(sourceConfig.readAll(), before);
        QVERIFY(QFile::remove(tab.fileName()));
        QVERIFY(!prepareCopyQImport(dir.path(), &archive, &error));
        QVERIFY(archive.isEmpty());
    }
    void copyQExternalMigration_data()
    {
        QTest::addColumn<bool>("encrypted");
        QTest::newRow("plain") << false;
        QTest::newRow("encrypted") << true;
    }
    void copyQExternalMigration()
    {
        QFETCH(bool, encrypted);
        Encryption::EncryptionKey key;
        if (encrypted) {
#ifdef WITH_QCA_ENCRYPTION
            QVERIFY(Encryption::initialize());
            QVERIFY(key.generateRandomDEK());
#else
            QSKIP("QCA encryption not enabled");
#endif
        }
        QTemporaryDir original;
        QSettings config(original.filePath(QStringLiteral("copyq.ini")), QSettings::IniFormat);
        config.setValue(QStringLiteral("Options/tabs"), QStringList{QStringLiteral("History")});
        config.setValue(QStringLiteral("Options/encrypt_tabs"), encrypted);
        config.sync();
        ClipboardModel model;
        model.insertItem({{mimeText, QByteArray("external-history")}, {QStringLiteral("custom/mime"), QByteArray("external-custom")}}, 0);
        QByteArray tabBytes;
        QDataStream tabStream(&tabBytes, QIODevice::WriteOnly);
        tabStream.setVersion(QDataStream::Qt_4_7);
        const auto previousPath = qApp->property("CopyQ_item_data_path");
        qApp->setProperty("CopyQ_item_data_path", original.filePath(QStringLiteral("data")));
        const bool saved = serializeData(model, &tabStream, 0, &key);
        qApp->setProperty("CopyQ_item_data_path", previousPath);
        QVERIFY(saved);
        QFile tab(original.filePath(QStringLiteral("copyq_tab_SGlzdG9yeQ==.dat")));
        QVERIFY(tab.open(QIODevice::WriteOnly)); QCOMPARE(tab.write(tabBytes), tabBytes.size()); tab.close();
        QVariantMap assets;
        QString error;
        QVERIFY2(readCopyQImportFiles(original.path(), &assets, &error), qPrintable(error));
        QTemporaryDir snapshot;
        QVERIFY2(saveCopyQImportFiles(assets, snapshot.path(), &error), qPrintable(error));
        QVERIFY(original.remove());
        QByteArray archive;
        QVERIFY2(prepareCopyQImport(snapshot.path(), &archive, &error, key), qPrintable(error));
        QDataStream stream(archive); stream.setVersion(QDataStream::Qt_4_7);
        QByteArray header; QVariantMap metadata, history;
        stream >> header >> metadata >> history;
        ClipboardModel restored;
        QDataStream data(history.value(QStringLiteral("data")).toByteArray());
        QVERIFY(deserializeData(&restored, &data));
        QCOMPARE(restored.index(0,0).data(contentType::data), model.index(0,0).data(contentType::data));
        QVariantMap after;
        QVERIFY(readCopyQImportFiles(snapshot.path(), &after, &error)); QCOMPARE(after, assets);
        for (auto it = assets.cbegin(); it != assets.cend(); ++it) {
            if (it.key().startsWith(QStringLiteral("data/"))) {
                const auto duplicate = QStringLiteral("duplicate/") + it.key();
                QVERIFY(saveCopyQImportFiles({{duplicate, it.value()}}, snapshot.path(), &error));
                QVERIFY(!prepareCopyQImport(snapshot.path(), &archive, &error, key));
                QVERIFY(archive.isEmpty());
                QVERIFY(QFile::remove(snapshot.filePath(duplicate)));
                QVERIFY(QFile::remove(snapshot.filePath(it.key())));
                break;
            }
        }
        QVERIFY(!prepareCopyQImport(snapshot.path(), &archive, &error, key));
        QVERIFY(archive.isEmpty());
    }
    void qmlSnippets()
    {
        QTemporaryDir dir;
        SnippetStore store(dir.filePath(QStringLiteral("snippets.dat")));
        auto shared = std::make_shared<ClipboardBrowserShared>();
        shared->menuItems = menuItems();
        Settings settings;
        settings.beginGroup(QStringLiteral("Shortcuts"));
        loadShortcuts(&shared->menuItems, settings);
        settings.endGroup();
        ClipboardSnippets window(&store, shared);
        auto ctx = context();
        window.setContext([ctx] { return ctx; });
        window.open({});
        QCOMPARE(window.status(), QQuickView::Ready);
        QVERIFY(window.rootObject());
        const auto collection = window.createCollection(QStringLiteral("Work"));
        QVERIFY(!collection.isEmpty());
        const auto id = window.createSnippet();
        QVERIFY(!id.isEmpty());
        QVERIFY(window.editSnippet({{QStringLiteral("title"), QStringLiteral("Signature")}, {QStringLiteral("keyword"), QStringLiteral("sig")}, {QStringLiteral("data"), QVariantMap{{mimeText, QByteArray("Hello {clipboard:1}")}}}}));
        QCOMPARE(window.preview(), QStringLiteral("Hello old"));
        auto field = window.rootObject()->findChild<QQuickItem*>(QStringLiteral("snippet_title"));
        QVERIFY(field);
        QTRY_COMPARE(field->property("text").toString(), QStringLiteral("Signature"));
        window.setQuery(QStringLiteral("SIG")); QCOMPARE(window.snippets().size(), 1);
        const auto artifacts = qEnvironmentVariable("COPYQ_TESTS_ARTIFACT_DIR");
        if (!artifacts.isEmpty()) {
            QVERIFY(QTest::qWaitForWindowExposed(&window));
            QTest::qWait(180);
            QVERIFY(window.grabWindow().save(artifacts + QStringLiteral("/snippets.png")));
            window.editBody();
            QWidget *bodyEditor = nullptr;
            for (auto widget : QApplication::topLevelWidgets())
                if (widget->isVisible() && widget->findChild<ItemEditorWidget*>()) bodyEditor = widget;
            QVERIFY(bodyEditor);
            QTest::qWait(180);
            QVERIFY(bodyEditor->grab().save(artifacts + QStringLiteral("/snippets-body-editor-light.png")));
            bodyEditor->close();
            auto dialog = window.rootObject()->findChild<QObject*>(QStringLiteral("snippet_collection_dialog"));
            QVERIFY(dialog);
            QVERIFY(QMetaObject::invokeMethod(dialog, "open"));
            QTest::qWait(180);
            QVERIFY(window.grabWindow().save(artifacts + QStringLiteral("/snippets-collection.png")));
            QVERIFY(QMetaObject::invokeMethod(dialog, "close"));
            window.resize(720, 480);
            QTest::qWait(180);
            QVERIFY(window.grabWindow().save(artifacts + QStringLiteral("/snippets-compact.png")));
            QSettings darkSettings(dir.filePath(QStringLiteral("dark.ini")), QSettings::IniFormat);
            darkSettings.setValue(QStringLiteral("bg"), QStringLiteral("#25272d"));
            darkSettings.setValue(QStringLiteral("fg"), QStringLiteral("#f0f1f5"));
            shared->theme.loadTheme(darkSettings);
            ClipboardSnippets dark(&store, shared);
            dark.setContext([ctx] { return ctx; });
            dark.open({});
            dark.select(id);
            QVERIFY(QTest::qWaitForWindowExposed(&dark));
            QTest::qWait(180);
            QVERIFY(dark.grabWindow().save(artifacts + QStringLiteral("/snippets-dark.png")));
            dark.editBody();
            bodyEditor = nullptr;
            for (auto widget : QApplication::topLevelWidgets())
                if (widget->isVisible() && widget->findChild<ItemEditorWidget*>()) bodyEditor = widget;
            QVERIFY(bodyEditor);
            QTest::qWait(180);
            QVERIFY(bodyEditor->grab().save(artifacts + QStringLiteral("/snippets-body-editor-dark.png")));
            bodyEditor->close();
            dark.hide();
        }
        window.setQuery(QStringLiteral("missing")); QVERIFY(window.snippets().isEmpty());
        window.setQuery(QString());
        QSignalSpy output(&window, &ClipboardSnippets::outputRequested);
        window.useSnippet(false); QCOMPARE(output.size(), 1);
        QCOMPARE(output.first().first().toMap().value(mimeText).toByteArray(), QByteArray("Hello old"));
        QVERIFY(window.removeSnippet()); QVERIFY(store.snippets().isEmpty());
        SnippetStore restarted(dir.filePath(QStringLiteral("snippets.dat"))); QVERIFY(restarted.load());
        QVERIFY(restarted.snippets().isEmpty());
    }
    void keywords()
    {
        SnippetStore store{QString()};
        const auto group = store.createCollection(QStringLiteral("Test"));
        QVERIFY(store.updateCollection(group, {{QStringLiteral("prefix"), QStringLiteral("!")}}));
        const auto id = store.createSnippet(group, {{mimeText, QByteArray("expanded")}});
        QVERIFY(store.updateSnippet(id, {{QStringLiteral("keyword"), QStringLiteral("sig")}, {QStringLiteral("enabled"), true}}));
        QCOMPARE(matchSnippet(QStringLiteral(" !sig"), store.snippets(), store.collections(), false), id);
        QVERIFY(matchSnippet(QStringLiteral("x!sig"), store.snippets(), store.collections(), false).isEmpty());
        QCOMPARE(matchSnippet(QStringLiteral("x!sig"), store.snippets(), store.collections(), true), id);
        QVERIFY(matchSnippet(QStringLiteral("!SIG"), store.snippets(), store.collections(), true).isEmpty());
        const auto duplicate = store.createSnippet(group, {{mimeText, QByteArray("other")}});
        QVERIFY(store.updateSnippet(duplicate, {{QStringLiteral("keyword"), QStringLiteral("sig")}}));
        QVERIFY(!store.keywordError(id).isEmpty());
        QVERIFY(matchSnippet(QStringLiteral("!sig"), store.snippets(), store.collections(), true).isEmpty());
    }
};

int main(int argc, char **argv)
{
    if (argc < 2 || qEnvironmentVariableIsEmpty("COPYQ_SETTINGS_PATH") || qEnvironmentVariableIsEmpty("COPYQ_ITEM_DATA_PATH") || qEnvironmentVariableIsEmpty("COPYQ_STATE_PATH")) return 2;
    std::unique_ptr<QApplication> app(platformNativeInterface()->createServerApplication(argc, argv));
    setSessionName(QStringLiteral("test"));
    initSession(app.get(), QStringLiteral("test"));
    if (argc == 3 && QString::fromLocal8Bit(argv[1]) == QLatin1String("--type-text")) return typeNativeText(QString::fromLocal8Bit(argv[2]));
    if (argc == 2 && QString::fromLocal8Bit(argv[1]) == QLatin1String("--double-copy")) return doubleNativeCopy();
    registerDataFileConverter();
    const auto clipboard = backupClipboard();
    SnippetTests tests;
    const int result = QTest::qExec(&tests, argc, argv);
    restoreClipboard(clipboard);
    return result;
}
#include "tests_snippets.moc"
