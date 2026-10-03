// SPDX-License-Identifier: GPL-3.0-or-later
#include "tests.h"
#include "test_utils.h"
#include "tests_common.h"
#include <QTemporaryDir>
#include <QFile>
#include <QProcess>
#include <QSaveFile>
#include <qscopeguard.h>
#include "platform/platformnativeinterface.h"
#include "platform/platformwindow.h"
#ifdef Q_OS_MACOS
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#endif

void CoreTests::snippetLifecycle()
{
    RUN("disable", "");
    RUN("snippets", "true\n");
    RUN("eval" << "callPlugin('itemtests', 'snippetCall', 'createCollection', 'Work') !== ''", "true\n");
    RUN("eval" << "callPlugin('itemtests', 'snippetCall', 'createSnippet') !== ''", "true\n");
    RUN("eval" << "callPlugin('itemtests', 'snippetCall', 'editSnippet', {title:'Signature',keyword:'sig',data:{'text/plain':str('Hello {isodate:yyyy-MM-dd}')}})", "true\n");
    RUN("eval" << "print(String(callPlugin('itemtests', 'snippetState').selected.title) + String.fromCharCode(10))", "Signature\n");
    RUN("eval" << "callPlugin('itemtests', 'snippetCall', 'useSnippet', false)", "true\n");
    WAIT_ON_OUTPUT("eval" << "str(clipboard()).indexOf('Hello ') === 0", "true\n");
    RUN("add" << "delete history", "");
    RUN("remove" << "0", "");
    RUN("eval" << "print(String(callPlugin('itemtests', 'snippetState').snippets.length) + String.fromCharCode(10))", "1\n");
    QTemporaryDir dir;
    const auto backup = dir.filePath(QStringLiteral("backup.cpq"));
    RUN("exportData" << backup, "");
    RUN("eval" << "callPlugin('itemtests', 'snippetCall', 'removeSnippet')", "true\n");
    RUN("eval" << "print(String(callPlugin('itemtests', 'snippetState').snippets.length) + String.fromCharCode(10))", "0\n");
    RUN("importData" << backup, "");
    RUN("eval" << "print(String(callPlugin('itemtests', 'snippetState').snippets.length) + String.fromCharCode(10))", "1\n");
    QVERIFY2(m_test->stopServer().isEmpty(), "Could not stop isolated server");
    QVERIFY2(m_test->startServer().isEmpty(), "Could not restart isolated server");
    RUN("snippets", "true\n");
    RUN("eval" << "print(String(callPlugin('itemtests', 'snippetState').snippets[0].title) + String.fromCharCode(10))", "Signature\n");
    RUN("write" << "text/plain" << "Saved history" << "text/html" << "<b>Saved history</b>" << "application/custom" << "raw payload", "");
    RUN("palette", "true\n");
    WAIT_ON_OUTPUT("eval" << "callPlugin('itemtests', 'paletteState').filtering", "false\n");
    RUN("eval" << "callPlugin('itemtests', 'paletteSaveSnippet')", "true\n");
    RUN("remove" << "0", "");
    RUN("eval" << "print(String(callPlugin('itemtests', 'snippetState').snippets.length) + String.fromCharCode(10))", "2\n");
    RUN("eval" << "str(callPlugin('itemtests', 'snippetState').selected.data['text/html'])", "<b>Saved history</b>\n");
    RUN("eval" << "str(callPlugin('itemtests', 'snippetState').selected.data['application/custom'])", "raw payload\n");
}

void CoreTests::snippetExpansion()
{
#ifdef Q_OS_MACOS
    if (!AXIsProcessTrusted() || IsSecureEventInputEnabled()) QSKIP("Native Accessibility/secure-input gate unavailable.");
#endif
    RUN("disable", "");
    RUN("snippets", "true\n");
    RUN("eval" << "callPlugin('itemtests', 'snippetCall', 'createCollection', 'Work') !== ''", "true\n");
    RUN("eval" << "callPlugin('itemtests', 'snippetCall', 'createSnippet') !== ''", "true\n");
    RUN("eval" << "callPlugin('itemtests', 'snippetCall', 'editSnippet', {title:'Expansion',keyword:'sig',enabled:true,data:{'text/plain':str('Hi 中文{cursor} end')}})", "true\n");
    RUN("eval" << "callPlugin('itemtests', 'snippetCall', 'setSetting', 'autoExpand', true)", "true\n");
    QByteArray state;
    TEST(m_test->getClientOutput({"eval", "callPlugin('itemtests', 'snippetState').inputStatus"}, &state));
    if (!state.contains("Monitoring enabled")) QSKIP(qPrintable(QString::fromUtf8(state)));
    RUN("eval" << "callPlugin('itemtests', 'snippetCall', 'hide')", "true\n");
    QTemporaryDir dir;
    const auto path = dir.filePath(QStringLiteral("input.txt"));
    const auto suffix =
#ifdef Q_OS_WIN
        QStringLiteral(".exe");
#else
        QString();
#endif
    const auto helpers = QCoreApplication::applicationDirPath();
    QProcess receiver;
    receiver.start(helpers + QStringLiteral("/copyq-palette-tests") + suffix, {QStringLiteral("--input-target"), path});
    QVERIFY(receiver.waitForStarted());
    const auto cleanup = qScopeGuard([&] { receiver.terminate(); if (!receiver.waitForFinished(1000)) { receiver.kill(); receiver.waitForFinished(1000); } });
    const auto diagnostics = qScopeGuard([&] {
        if (QTest::currentTestFailed())
            qDebug().noquote() << m_test->readServerErrors(TestInterface::ReadAllStderr);
    });
    const auto read = [&] { QFile file(path); return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray(); };
    QTRY_COMPARE(read(), QByteArray("qclip"));
    const auto focused = [] { const auto target = platformNativeInterface()->getCurrentWindow(); return target && target->isActive() && target->getTitle().contains(QStringLiteral("QClip input target")); };
    QTRY_VERIFY(focused());
    RUN("copy" << "before expansion", "true\n");
    WAIT_FOR_CLIPBOARD("before expansion");
    QCOMPARE(QProcess::execute(helpers + QStringLiteral("/copyq-snippet-tests") + suffix, {QStringLiteral("--type-text"), QStringLiteral(" sig")}), 0);
    QTRY_COMPARE(read(), QStringLiteral("qclip Hi 中文 end").toUtf8());
    WAIT_FOR_CLIPBOARD("before expansion");
    QCOMPARE(QProcess::execute(helpers + QStringLiteral("/copyq-snippet-tests") + suffix, {QStringLiteral("--type-text"), QStringLiteral("X")}), 0);
    QTRY_COMPARE(read(), QStringLiteral("qclip Hi 中文X end").toUtf8());
    // A user's later copy must survive the delayed restore.
    RUN("copy" << "user clipboard", "true\n");
    QCOMPARE(QProcess::execute(helpers + QStringLiteral("/copyq-snippet-tests") + suffix, {QStringLiteral("--type-text"), QStringLiteral(" sig")}), 0);
    QTRY_COMPARE(read(), QStringLiteral("qclip Hi 中文X Hi 中文 end end").toUtf8());
    RUN("copy" << "new clipboard", "true\n");
    QTest::qWait(800);
    RUN("clipboard", "new clipboard");
    RUN("eval" << "callPlugin('itemtests', 'snippetCall', 'setSetting', 'autoExpand', false)", "true\n");
    RUN("eval" << "callPlugin('itemtests', 'snippetCall', 'setSetting', 'merge', true)", "true\n");
    QSaveFile selection(path + QStringLiteral(".control"));
    QVERIFY(selection.open(QIODevice::WriteOnly)); selection.write("select-first"); QVERIFY(selection.commit());
    QTest::qWait(100);
    const auto selected = read();
    QCOMPARE(QProcess::execute(helpers + QStringLiteral("/copyq-snippet-tests") + suffix, {QStringLiteral("--double-copy")}), 0);
    WAIT_FOR_CLIPBOARD(QByteArray("new clipboard\n") + selected);
    QTest::qWait(500);
    RUN("clipboard", QByteArray("new clipboard\n") + selected);
    RUN("eval" << "callPlugin('itemtests', 'snippetCall', 'setSetting', 'merge', false)", "true\n");
    RUN("eval" << "setCommands([{name:'Snippet command',input:'text/plain',cmd:'copyq: copy(\"command:\" + str(input()))'}])", "");
    RUN("eval" << "callPlugin('itemtests', 'snippetCall', 'editSnippet', {command:'Snippet command'})", "true\n");
    RUN("eval" << "callPlugin('itemtests', 'snippetCall', 'setSetting', 'autoExpand', true)", "true\n");
    QCOMPARE(QProcess::execute(helpers + QStringLiteral("/copyq-snippet-tests") + suffix, {QStringLiteral("--type-text"), QStringLiteral(" sig")}), 0);
    WAIT_FOR_CLIPBOARD(QStringLiteral("command:Hi 中文 end").toUtf8());
    QTRY_COMPARE(read(),QByteArray(" "));
    RUN("eval" << "callPlugin('itemtests', 'snippetCall', 'setSetting', 'autoExpand', false)", "true\n");
    // Explicit Snippet insertion keeps the public paste() override and external target.
    RUN("eval" << "setCommands([{isScript:true,cmd:'const originalPaste = global.paste; global.paste = function() { copy(\"SNIPPET_OVERRIDE\"); originalPaste(); }'}])", "");
    RUN("eval" << "callPlugin('itemtests', 'snippetCall', 'editSnippet', {command:'',data:{'text/plain':str('ordinary')}})", "true\n");
    RUN("snippets", "true\n");
    RUN("eval" << "callPlugin('itemtests', 'snippetCall', 'useSnippet', true)", "true\n");
    QTRY_COMPARE(read(),QByteArray(" SNIPPET_OVERRIDE"));
}
