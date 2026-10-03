// SPDX-License-Identifier: GPL-3.0-or-later
#include "tests.h"
#include "test_utils.h"
#include "tests_common.h"
#include "platform/platformnativeinterface.h"
#include "platform/platformwindow.h"

#include <QFile>
#include <QProcess>
#include <QSaveFile>
#include <QTemporaryDir>
#include <qscopeguard.h>

#ifdef Q_OS_MACOS
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#endif

void CoreTests::paletteSearchAndCopy()
{
    RUN("disable", "");
    RUN("add" << "other" << "中文 history", "");
    RUN("eval" << "copy('baseline'); selectItems([1])", "true\n");
    WAIT_FOR_CLIPBOARD("baseline");
    RUN("palette", "true\n");
    WAIT_ON_OUTPUT("eval" << "callPlugin('itemtests', 'paletteState').filtering", "false\n");
    RUN("eval" << "callPlugin('itemtests', 'paletteInput', 'commit:中文')", "true\n");
    WAIT_ON_OUTPUT("eval" << "callPlugin('itemtests', 'paletteState').count", "1");
    RUN("clipboard", "baseline");
    RUN("eval" << "callPlugin('itemtests', 'paletteInput', 'preedit:zhong')", "true\n");
    RUN("eval" << "callPlugin('itemtests', 'paletteInput', 'RETURN')", "true\n");
    RUN("clipboard", "baseline");
    RUN("eval" << "callPlugin('itemtests', 'paletteInput', 'preedit:')", "true\n");
    RUN("eval" << "callPlugin('itemtests', 'paletteInput', 'CTRL+RETURN')", "true\n");
    WAIT_FOR_CLIPBOARD(QStringLiteral("中文 history").toUtf8());
    WAIT_ON_OUTPUT("eval" << "callPlugin('itemtests', 'paletteState').visible", "false\n");
    // The palette must never change the old management selection to communicate its item.
    TEST_SELECTED(QString(clipboardTabName) + " 1 1\n");
}

void CoreTests::paletteCommands()
{
    RUN("disable", "");
    RUN("add" << "selected", "");
    RUN("eval" << "setCommands([{name:'Palette Enter', cmd:'copyq: copy(\"override\")', inMenu:true, shortcuts:['RETURN']}])", "");
    RUN("palette", "true\n");
    WAIT_ON_OUTPUT("eval" << "callPlugin('itemtests', 'paletteState').enterLabel", "Palette Enter\n");
    RUN("eval" << "callPlugin('itemtests', 'paletteInput', 'RETURN')", "true\n");
    WAIT_FOR_CLIPBOARD("override");
    WAIT_ON_OUTPUT("eval" << "callPlugin('itemtests', 'paletteState').visible", "false\n");
}

void CoreTests::paletteEditor()
{
    RUN("disable", "");
    RUN("add" << "original", "");
    RUN("palette", "true\n");
    WAIT_ON_OUTPUT("eval" << "callPlugin('itemtests', 'paletteState').filtering", "false\n");
    RUN("eval" << "callPlugin('itemtests', 'paletteEdit')", "true\n");
    KEYS("focus:palette_editor_text" << "CTRL+A" << ":edited" << "CTRL+RETURN");
    RUN("read" << "0", "edited");
}

void CoreTests::paletteClipboardFailure()
{
    RUN("disable", "");
    RUN("add" << "selected", "");
    m_test->ignoreErrors(QRegularExpression(QStringLiteral("Command .*provideClipboard.*Exit code: 4")));
    RUN("eval" << "setCommands([{isScript:true,cmd:'global.provideClipboard = function() { sleep(100); throw new Error(\"Forced clipboard failure\"); }'}])", "");
    RUN("palette", "true\n");
    WAIT_ON_OUTPUT("eval" << "callPlugin('itemtests', 'paletteState').filtering", "false\n");
    RUN("eval" << "callPlugin('itemtests', 'paletteInput', 'CTRL+RETURN')", "true\n");
    WAIT_ON_OUTPUT("eval" << "!!callPlugin('itemtests', 'paletteState').error", "true\n");
    RUN("eval" << "callPlugin('itemtests', 'paletteState').busy", "false\n");
    RUN("eval" << "callPlugin('itemtests', 'paletteState').visible", "true\n");
    RUN("palette", "false\n");
}

void CoreTests::paletteMimeAndDisplayCommands()
{
    RUN("disable", "");
    RUN("write" << "text/plain" << "plain" << "text/html" << "<b>rich</b>"
        << "text/uri-list" << "https://example.com\r\n" << "application/custom" << "opaque", "");
    RUN("eval" << "setCommands([{display:true,cmd:'copyq: setData(mimeText, \"shown\")'}])", "");
    RUN("palette", "true\n");
    WAIT_ON_OUTPUT("eval" << "callPlugin('itemtests', 'paletteState').preview.text", "shown\n");
    RUN("clipboard" << "application/custom", "");
    RUN("eval" << "callPlugin('itemtests', 'paletteInput', 'CTRL+RETURN')", "true\n");
    WAIT_FOR_CLIPBOARD("plain");
#ifdef Q_OS_WIN
    // Qt's Windows clipboard converter wraps HTML in CF_HTML fragment markers.
    WAIT_FOR_CLIPBOARD2("<!--StartFragment--><b>rich</b><!--EndFragment-->", QStringLiteral("text/html"));
#else
    WAIT_FOR_CLIPBOARD2("<b>rich</b>", QStringLiteral("text/html"));
#endif
    WAIT_FOR_CLIPBOARD2("opaque", QStringLiteral("application/custom"));
    RUN("read" << "text/plain" << "0", "plain");
    RUN("read" << "text/html" << "0", "<b>rich</b>");
    WAIT_ON_OUTPUT("eval" << "callPlugin('itemtests', 'paletteState').visible", "false\n");
    RUN("palette", "true\n");
    WAIT_ON_OUTPUT("eval" << "callPlugin('itemtests', 'paletteState').filtering", "false\n");
    RUN("eval" << "callPlugin('itemtests', 'paletteInput', 'CTRL+SHIFT+RETURN')", "true\n");
    WAIT_FOR_CLIPBOARD("plain");
    RUN("clipboard" << "text/html", "");
    RUN("clipboard" << "application/custom", "");
}

void CoreTests::palettePaste()
{
#ifdef Q_OS_MACOS
    if (!AXIsProcessTrusted() || IsSecureEventInputEnabled())
        QSKIP("Accessibility permission is unavailable or secure input is active; native paste remains unverified.");
#endif
    RUN("disable", "");
    RUN("add" << "PALETTE", "");
    QTemporaryDir dir;
    const auto path = dir.filePath(QStringLiteral("input.txt"));
    const auto helper = QCoreApplication::applicationDirPath() + QStringLiteral("/copyq-palette-tests")
#ifdef Q_OS_WIN
        + QStringLiteral(".exe")
#endif
        ;
    QProcess receiver;
    receiver.start(helper, {QStringLiteral("--input-target"), path});
    QVERIFY(receiver.waitForStarted());
    const auto cleanup = qScopeGuard([&]() {
        receiver.terminate();
        if (!receiver.waitForFinished(1000)) {
            receiver.kill();
            receiver.waitForFinished(1000);
        }
    });
    const auto read = [](const QString &fileName) {
        QFile file(fileName);
        return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
    };
    const auto focus = [&path](const QByteArray &command) {
        QSaveFile file(path + QStringLiteral(".control"));
        if (!file.open(QIODevice::WriteOnly))
            return false;
        file.write(command);
        return file.commit();
    };
    const auto focused = [](const QString &title) {
        const auto window = platformNativeInterface()->getCurrentWindow();
        return window && window->isActive() && window->getTitle().contains(title);
    };
    QTRY_COMPARE(read(path), QByteArray("qclip"));
    QTRY_VERIFY(focused(QStringLiteral("QClip input target")));
    RUN("palette", "true\n");
    WAIT_ON_OUTPUT("eval" << "callPlugin('itemtests', 'paletteState').filtering", "false\n");
    RUN("eval" << "callPlugin('itemtests', 'paletteInput', 'RETURN')", "true\n");
    QTRY_COMPARE(read(path), QByteArray("qclipPALETTE"));
    QCOMPARE(read(path + QStringLiteral(".other")), QByteArray("other"));
    WAIT_ON_OUTPUT("eval" << "callPlugin('itemtests', 'paletteState').visible", "false\n");

    // A paste() override still receives the palette's item and its captured external window.
    RUN("eval" << "setCommands([{isScript:true,cmd:'const originalPaste = global.paste; global.paste = function() { copy(\"OVERRIDE\"); originalPaste(); }'}])", "");
    QVERIFY(focus("first"));
    QTRY_VERIFY(focused(QStringLiteral("QClip input target")));
    RUN("palette", "true\n");
    WAIT_ON_OUTPUT("eval" << "callPlugin('itemtests', 'paletteState').filtering", "false\n");
    RUN("eval" << "callPlugin('itemtests', 'paletteInput', 'RETURN')", "true\n");
    QTRY_COMPARE(read(path), QByteArray("qclipPALETTEOVERRIDE"));
    QCOMPARE(read(path + QStringLiteral(".other")), QByteArray("other"));

    // Switching to another window of the same application cancels, without sending input.
    RUN("eval" << "setCommands([])", "");
    QVERIFY(focus("first"));
    QTRY_VERIFY(focused(QStringLiteral("QClip input target")));
    RUN("palette", "true\n");
    QVERIFY(focus("second"));
    QTRY_VERIFY(focused(QStringLiteral("QClip second target")));
    WAIT_ON_OUTPUT("eval" << "callPlugin('itemtests', 'paletteState').visible", "false\n");
    QCOMPARE(read(path + QStringLiteral(".other")), QByteArray("other"));
    RUN("palette", "true\n");
    QVERIFY(focus("close-second"));
    WAIT_ON_OUTPUT("eval" << "callPlugin('itemtests', 'paletteState').filtering", "false\n");
    RUN("eval" << "callPlugin('itemtests', 'paletteInput', 'RETURN')", "true\n");
    WAIT_ON_OUTPUT("eval" << "!!callPlugin('itemtests', 'paletteState').error", "true\n");
    QCOMPARE(read(path + QStringLiteral(".other")), QByteArray("other"));
    RUN("palette", "false\n");
}
