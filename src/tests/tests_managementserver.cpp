// SPDX-License-Identifier: GPL-3.0-or-later
#include "tests.h"
#include "test_utils.h"
#include "tests_common.h"
#include "gui/menuitems.h"
#include "common/mimetypes.h"
#include <QBuffer>
#include <QImage>

void CoreTests::managementActions()
{
    RUN("disable", "");
    RUN("add" << "duplicate" << "duplicate" << "other", "");
    RUN("eval" << "copy('baseline'); selectItems(1)", "true\n");
    WAIT_FOR_CLIPBOARD("baseline");
    RUN("palette", "true\n");
    RUN("eval" << "callPlugin('itemtests','paletteManage')", "true\n");
    WAIT_ON_OUTPUT("eval" << "callPlugin('itemtests','managementState').filtering", "false\n");
    RUN("eval" << "selectItems(0,2)", "true\n");
    RUN("eval" << "callPlugin('itemtests','managementState').selectedCount", "2");
    RUN("eval" << QStringLiteral("callPlugin('itemtests','managementAction',%1)").arg(Actions::Edit_CopySelectedItems), "true\n");
    WAIT_FOR_CLIPBOARD("other\nduplicate");
    RUN("eval" << "selectItems(1)", "true\n");
    RUN("add" << "new", "");
    WAIT_ON_OUTPUT("eval" << "callPlugin('itemtests','managementState').filtering", "false\n");
    RUN("eval" << "selectedItems().join(',')", "2\n");
    RUN("eval" << QStringLiteral("callPlugin('itemtests','managementAction',%1)").arg(Actions::Item_Remove), "true\n");
    RUN("size", "3\n");
    WAIT_ON_OUTPUT("eval" << "callPlugin('itemtests','managementState').filtering", "false\n");
    RUN("eval" << "callPlugin('itemtests','managementState').selectedCount", "0");
    RUN("read" << "0" << "1" << "2", "new\nother\nduplicate");
    RUN("filter" << "other", "");
    WAIT_ON_OUTPUT("eval" << "callPlugin('itemtests','managementState').count", "1");
    RUN("eval" << "selectedItems().join(',')", "1\n");
    RUN("clipboard", "other\nduplicate");
    RUN("eval" << QStringLiteral("callPlugin('itemtests','managementAction',%1)").arg(Actions::File_ToggleClipboardStoring), "true\n");
    RUN("monitoring", "true\n");
    RUN("eval" << "callPlugin('itemtests','managementState').monitoring", "true\n");
    RUN("disable", "");
    RUN("filter" << "", "");
    RUN("eval" << "remove(0,1,2); add('c','a','b')", "");
    WAIT_ON_OUTPUT("eval" << "callPlugin('itemtests','managementState').filtering", "false\n");
    RUN("eval" << "selectItems(0,1,2)", "true\n");
    RUN("eval" << QStringLiteral("callPlugin('itemtests','managementAction',%1)").arg(Actions::Edit_SortSelectedItems), "true\n");
    RUN("read" << "0" << "1" << "2", "a\nb\nc");
    WAIT_ON_OUTPUT("eval" << "callPlugin('itemtests','managementState').filtering", "false\n");
    RUN("eval" << "selectItems(0,2)", "true\n");
    RUN("eval" << QStringLiteral("callPlugin('itemtests','managementAction',%1)").arg(Actions::Edit_ReverseSelectedItems), "true\n");
    RUN("read" << "0" << "1" << "2", "c\na\nb");
    WAIT_ON_OUTPUT("eval" << "callPlugin('itemtests','managementState').filtering", "false\n");
    RUN("eval" << QStringLiteral("callPlugin('itemtests','managementAction',%1)").arg(Actions::Item_MoveToTop), "true\n");
    RUN("read" << "0" << "1" << "2", "c\na\nb");
    WAIT_ON_OUTPUT("eval" << "callPlugin('itemtests','managementState').filtering", "false\n");
    RUN("eval" << QStringLiteral("callPlugin('itemtests','managementAction',%1)").arg(Actions::Item_MoveToBottom), "true\n");
    RUN("read" << "0" << "1" << "2", "b\nc\na");
    WAIT_ON_OUTPUT("eval" << "callPlugin('itemtests','managementState').filtering", "false\n");
    RUN("eval" << QStringLiteral("callPlugin('itemtests','managementAction',%1)").arg(Actions::Item_MoveUp), "true\n");
    RUN("read" << "0" << "1" << "2", "c\na\nb");
    WAIT_ON_OUTPUT("eval" << "callPlugin('itemtests','managementState').filtering", "false\n");
    RUN("eval" << QStringLiteral("callPlugin('itemtests','managementAction',%1)").arg(Actions::Item_MoveDown), "true\n");
    RUN("read" << "0" << "1" << "2", "b\nc\na");
    RUN("eval" << "copy('pasted')", "true\n");
    WAIT_FOR_CLIPBOARD("pasted");
    WAIT_ON_OUTPUT("eval" << "callPlugin('itemtests','managementState').filtering", "false\n");
    RUN("eval" << "selectItems(2)", "true\n");
    RUN("eval" << QStringLiteral("callPlugin('itemtests','managementAction',%1)").arg(Actions::Edit_PasteItems), "true\n");
    RUN("read" << "2", "pasted");
    RUN("config" << "move" << "true", "true\n");
    WAIT_ON_OUTPUT("eval" << "callPlugin('itemtests','managementState').filtering", "false\n");
    RUN("eval" << "selectItems(2)", "true\n");
    RUN("eval" << QStringLiteral("callPlugin('itemtests','managementAction',%1)").arg(Actions::Item_MoveToClipboard), "true\n");
    WAIT_FOR_CLIPBOARD("pasted");
    RUN("read" << "0", "pasted");
}

void CoreTests::managementTabs()
{
    RUN("disable", "");
    RUN("write" << "text/plain" << "original" << "text/html" << "<b>rich</b>" << "application/custom" << "opaque", "");
    RUN("palette", "true\n");
    RUN("eval" << "callPlugin('itemtests','paletteManage')", "true\n");
    WAIT_ON_OUTPUT("eval" << "callPlugin('itemtests','managementState').filtering", "false\n");
    RUN("eval" << "callPlugin('itemtests','managementTab','createTab','Group/Target')", "true\n");
    WAIT_ON_OUTPUT("eval" << "callPlugin('itemtests','managementState').tabName", "Group/Target\n");
    RUN("eval" << "callPlugin('itemtests','managementTab','renameTab','Group/Renamed')", "true\n");
    WAIT_ON_OUTPUT("eval" << "callPlugin('itemtests','managementState').tabName", "Group/Renamed\n");
    RUN("eval" << "callPlugin('itemtests','managementTab','saveTabProperties',{maxItemCount:25,storeItems:true,encryptedExpireSeconds:9})", "true\n");
    RUN("eval" << "callPlugin('itemtests','managementState').tabProperties.maxItemCount", "25");
    RUN("eval" << "callPlugin('itemtests','managementTab','moveTab',-1)", "true\n");
    RUN("eval" << "callPlugin('itemtests','managementState').tabs[0]", "Group/Renamed\n");
    RUN("eval" << "setCurrentTab('CLIPBOARD')", "");
    RUN("eval" << "callPlugin('itemtests','managementState').tabName", "CLIPBOARD\n");
    RUN("eval" << QStringLiteral("callPlugin('itemtests','managementAction',%1)").arg(Actions::Tabs_PreviousTab), "true\n");
    WAIT_ON_OUTPUT("eval" << "callPlugin('itemtests','managementState').tabName", "Group/Renamed\n");
    RUN("eval" << QStringLiteral("callPlugin('itemtests','managementAction',%1)").arg(Actions::Tabs_NextTab), "true\n");
    WAIT_ON_OUTPUT("eval" << "callPlugin('itemtests','managementState').tabName", "CLIPBOARD\n");
    WAIT_ON_OUTPUT("eval" << "callPlugin('itemtests','managementState').filtering", "false\n");
    RUN("eval" << "tab('CLIPBOARD'); selectItems(0); callPlugin('itemtests','managementTransfer','Group/Renamed',false)", "true\n");
    RUN("eval" << "callPlugin('itemtests','managementState').error", "\n");
    RUN("eval" << "tab('Group/Renamed'); read('application/custom',0)", "opaque");
    RUN("eval" << "tab('CLIPBOARD'); size()", "1\n");
    RUN("eval" << "tab('CLIPBOARD'); selectItems(0); callPlugin('itemtests','managementTransfer','Group/Renamed',true)", "true\n");
    RUN("eval" << "tab('CLIPBOARD'); size()", "0\n");
    RUN("eval" << "setCurrentTab('Group/Renamed')", "");
    WAIT_ON_OUTPUT("eval" << "callPlugin('itemtests','managementState').filtering", "false\n");
    RUN("eval" << "tab('Group/Renamed'); size()", "2\n");
    RUN("eval" << "tab('Group/Renamed'); read('text/html',0)", "<b>rich</b>");
    RUN("eval" << "unload('Group/Renamed').length", "0\n");
    RUN("eval" << "forceUnload('Group/Renamed')", "");
    WAIT_ON_OUTPUT("eval" << "callPlugin('itemtests','managementState').count", "0");
    RUN("eval" << "callPlugin('itemtests','managementState').selectedCount", "0");
    RUN("eval" << "selectedItems().length", "0\n");
    RUN("eval" << "callPlugin('itemtests','managementTab','changeSource','Group/Renamed')", "true\n");
    WAIT_ON_OUTPUT("eval" << "callPlugin('itemtests','managementState').count", "2");
    RUN("eval" << "callPlugin('itemtests','managementState').tabProperties.maxItemCount", "25");
    RUN("eval" << "tab('Group/Renamed'); read('application/custom',0)", "opaque");
    RUN("eval" << "callPlugin('itemtests','managementTab','removeTab')", "true\n");
    RUN("eval" << "callPlugin('itemtests','managementState').tabs.join(',')", "CLIPBOARD\n");
}

void CoreTests::managementGroups()
{
    RUN("config" << "tab_tree" << "true", "true\n");
    RUN("disable", "");
    RUN("palette", "true\n");
    RUN("eval" << "callPlugin('itemtests','paletteManage')", "true\n");
    RUN("eval" << "tab('Group/A'); add('a'); tab('Group/Nested/B'); add('b'); tab('Groupish'); add('outside'); setCurrentTab('CLIPBOARD')", "");
    WAIT_ON_OUTPUT("eval" << "callPlugin('itemtests','managementState').tabs.length", "4\n");
    RUN("eval" << "callPlugin('itemtests','managementTab','selectTabPath','Group')", "true\n");
    RUN("eval" << "callPlugin('itemtests','managementState').selectedTabIsGroup", "true\n");
    RUN("eval" << "callPlugin('itemtests','managementState').tabTree.map(x=>x.path).join(',')", "CLIPBOARD,Group,Group/A,Group/Nested,Group/Nested/B,Groupish\n");
    RUN("eval" << "callPlugin('itemtests','managementTab','moveGroup','Group',-1)", "true\n");
    RUN("eval" << "callPlugin('itemtests','managementState').tabs.join(',')", "Group/A,Group/Nested/B,CLIPBOARD,Groupish\n");
    RUN("eval" << "callPlugin('itemtests','managementTab','moveGroup','Group',1)", "true\n");
    RUN("eval" << "callPlugin('itemtests','managementState').tabs.join(',')", "CLIPBOARD,Group/A,Group/Nested/B,Groupish\n");
    RUN("eval" << "tab('Group/0'); add('zero'); callPlugin('itemtests','managementTab','sortGroup','Group')", "true\n");
    RUN("eval" << "callPlugin('itemtests','managementState').tabs.join(',')", "CLIPBOARD,Group/0,Group/A,Group/Nested/B,Groupish\n");
    RUN("eval" << "removeTab('Group/0')", "");
    RUN("eval" << "callPlugin('itemtests','managementTab','toggleGroup','Group')", "true\n");
    RUN("eval" << "callPlugin('itemtests','managementState').tabTree.map(x=>x.path).join(',')", "CLIPBOARD,Group,Groupish\n");
    RUN("eval" << "callPlugin('itemtests','managementTab','renameGroup','Group','Renamed',['Group/A','Group/Nested/B'])", "true\n");
    RUN("eval" << "tab('Renamed/A'); read(0)", "a");
    RUN("eval" << "tab('Renamed/Nested/B'); read(0)", "b");
    RUN("eval" << "tab('Groupish'); read(0)", "outside");
    RUN("eval" << "callPlugin('itemtests','managementTab','saveIcon','X','Renamed/A')", "true\n");
    RUN("eval" << "tabIcon('Renamed/A')", "X\n");
    RUN("eval" << "callPlugin('itemtests','managementTab','renameGroup','Renamed','Groupish',['Renamed/A','Renamed/Nested/B'])", "true\n");
    RUN("eval" << "tab('Groupish/A'); read(0)", "a");
    RUN("eval" << "callPlugin('itemtests','managementTab','renameGroup','Groupish','CLIPBOARD',['Groupish','Groupish/A','Groupish/Nested/B'])", "true\n");
    RUN("eval" << "callPlugin('itemtests','managementState').error.length>0", "true\n");
    RUN("eval" << "tab('Groupish'); read(0)", "outside");
    RUN("eval" << "callPlugin('itemtests','managementTab','removeGroup','Groupish',['Groupish/A','Groupish/Nested/B'])", "true\n");
    RUN("eval" << "callPlugin('itemtests','managementState').tabs.join(',')", "CLIPBOARD,Groupish\n");
    RUN("eval" << "callPlugin('itemtests','managementTab','removeGroup','Groupish',['Groupish/A'])", "true\n");
    RUN("eval" << "callPlugin('itemtests','managementState').error.length>0", "true\n");
    RUN("eval" << "tab('Groupish'); read(0)", "outside");
}

void CoreTests::managementCommands()
{
    RUN("disable", "");
    RUN("add" << "one" << "two" << "three", "");
    RUN("eval" << "setCommands([{name:'Management selection',inMenu:true,cmd:'copyq: const result=selectedItems().join(\",\"); tab(\"Result\"); add(result)',matchCmd:'copyq: if(selectedItems().length !== 2) fail()'}])", "");
    RUN("palette", "true\n");
    RUN("eval" << "callPlugin('itemtests','paletteManage')", "true\n");
    WAIT_ON_OUTPUT("eval" << "callPlugin('itemtests','managementState').filtering", "false\n");
    RUN("eval" << "selectItems(0,2)", "true\n");
    WAIT_ON_OUTPUT("eval" << "callPlugin('itemtests','managementState').commands[0].enabled", "true\n");
    RUN("eval" << "callPlugin('itemtests','managementCommand',0)", "true\n");
    WAIT_ON_OUTPUT("eval" << "tab('Result'); read(0)", "0,2");
    RUN("eval" << "selectItems(1)", "true\n");
    WAIT_ON_OUTPUT("eval" << "callPlugin('itemtests','managementState').commands[0].enabled", "false\n");
    RUN("eval" << "setCommands([{display:true,cmd:'copyq: setData(mimeText,\"shown\")'}])", "");
    WAIT_ON_OUTPUT("eval" << "callPlugin('itemtests','managementState').filtering", "false\n");
    RUN("eval" << "selectItems(0)", "true\n");
    WAIT_ON_OUTPUT("eval" << "callPlugin('itemtests','managementState').preview.text", "shown\n");
    RUN("read" << "0", "three");
}

void CoreTests::managementEditor()
{
    RUN("disable", "");
    RUN("write" << "text/plain" << "plain" << "text/html" << "<b>rich</b>" << "application/custom" << "opaque", "");
    RUN("palette", "true\n");
    RUN("eval" << "callPlugin('itemtests','paletteManage')", "true\n");
    WAIT_ON_OUTPUT("eval" << "callPlugin('itemtests','managementState').filtering", "false\n");
    RUN("eval" << QStringLiteral("callPlugin('itemtests','managementAction',%1)").arg(Actions::Item_EditNotes), "true\n");
    KEYS("focus:palette_editor_text" << ":note" << "CTRL+RETURN");
    RUN("read" << "application/x-copyq-item-notes" << "0", "note");
    RUN("read" << "text/html" << "0", "<b>rich</b>");
    RUN("read" << "application/custom" << "0", "opaque");
    WAIT_ON_OUTPUT("eval" << "callPlugin('itemtests','managementState').filtering", "false\n");
    RUN("eval" << QStringLiteral("callPlugin('itemtests','managementAction',%1)").arg(Actions::Item_Edit), "true\n");
    KEYS("focus:palette_editor_text" << "CTRL+A" << ":edited" << "CTRL+RETURN");
    RUN("read" << "text/plain" << "0", "edited");
    RUN("read" << "application/custom" << "0", "opaque");
    RUN("read" << "application/x-copyq-item-notes" << "0", "note");
    WAIT_ON_OUTPUT("eval" << "callPlugin('itemtests','managementState').filtering", "false\n");
    RUN("eval" << QStringLiteral("callPlugin('itemtests','managementAction',%1)").arg(Actions::Item_Edit), "true\n");
    KEYS("focus:palette_editor_text" << "CTRL+A" << ":cancelled" << "ESC");
    RUN("read" << "0", "edited");
    RUN("eval" << QStringLiteral("callPlugin('itemtests','managementAction',%1)").arg(Actions::File_New), "true\n");
    KEYS("focus:palette_editor_text" << ":cancelled new item" << "ESC");
    RUN("size", "1\n");
    RUN("eval" << QStringLiteral("callPlugin('itemtests','managementAction',%1)").arg(Actions::File_New), "true\n");
    KEYS("focus:palette_editor_text" << ":new item" << "CTRL+RETURN");
    RUN("read" << "0", "new item");
    WAIT_ON_OUTPUT("eval" << "callPlugin('itemtests','managementState').filtering", "false\n");
    RUN("eval" << "selectItems(1)", "true\n");
    RUN("eval" << QStringLiteral("callPlugin('itemtests','managementAction',%1)").arg(Actions::Item_Edit), "true\n");
    KEYS("focus:palette_editor_text" << "CTRL+A" << ":orphaned edit");
    RUN("remove" << "1", "");
    KEYS("focus:palette_editor_text" << "CTRL+RETURN" << "ESC");
    RUN("read" << "0", "new item");
    RUN("size", "1\n");
    RUN("config" << "editor" << "", "\n");
    RUN("edit" << "0", "");
    KEYS("focus:palette_editor_text" << "CTRL+A" << ":cli edit" << "CTRL+Z" << "CTRL+SHIFT+Z" << "CTRL+F");
    KEYS("focus:item_editor_search" << ":cli" << "RETURN");
    KEYS("focus:palette_editor_text" << "CTRL+RETURN");
    RUN("read" << "0", "cli edit");
    RUN("edit" << "0", "");
    KEYS("focus:palette_editor_text" << "CTRL+A" << "CTRL+B" << "CTRL+I" << "CTRL+U" << "CTRL+RETURN");
    RUN("eval" << "const html=str(read('text/html',0)); /font-weight: *700/.test(html) && /font-style: *italic/.test(html) && /text-decoration: *underline/.test(html)", "true\n");
    RUN("read" << "0", "cli edit");
}

void CoreTests::managementHistory()
{
    RUN("disable", "");
    RUN("add" << "legacy", "");
    RUN("write" << "text/plain" << "pinned" << "application/x-copyq-item-pinned" << "1", "");
    RUN("eval" << "tab('Saved'); add('manual'); tab('CLIPBOARD')", "");
    RUN("show", "");
    WAIT_ON_OUTPUT("eval" << "callPlugin('itemtests','managementState').filtering", "false\n");
    RUN("eval" << "callPlugin('itemtests','managementTab','clearHistory',5)", "true\n");
    RUN("size", "2\n");
    RUN("eval" << "callPlugin('itemtests','managementTab','clearHistory',-1)", "true\n");
    RUN("size", "1\n");
    RUN("read" << "0", "pinned");
    RUN("eval" << "tab('Saved'); read(0)", "manual");
    RUN("eval" << "callPlugin('itemtests','managementTab','changeSource','Saved')", "true\n");
    WAIT_ON_OUTPUT("eval" << "callPlugin('itemtests','managementState').filtering", "false\n");
    RUN("eval" << "callPlugin('itemtests','managementTab','clearHistory',-1)", "true\n");
    RUN("eval" << "tab('Saved'); read(0)", "manual");
    RUN("eval" << "callPlugin('itemtests','managementState').error.length>0", "true\n");
}

void CoreTests::managementHistoryCapture()
{
    RUN("disable", "");
    RUN("config" << "clipboard_history_types" << "text", "text\n");
    RUN("eval" << "config('clipboard_history_types') instanceof Array", "true\n");
    RUN("eval" << R"(
        setCommands([
            {isScript:true, cmd:'global.clipboardFormatsToSave = function() { return ["text/plain", "text/html", "application/custom", "image/png"] }'},
            {automatic:true, cmd:'copyq: tab("CaptureEvents"); add("automatic")'}
        ])
    )", "");
    RUN("enable", "");
    WAIT_ON_OUTPUT("clipboardFormatsToSave().join(',')", "text/plain,text/html,application/custom,image/png\n");
    const QVariantMap copied{{mimeText, QByteArray("external")}, {mimeHtml, QByteArray("<b>external</b>")},
        {QStringLiteral("application/custom"), QByteArray("opaque")}};
    TEST(m_test->setClipboard(copied));
    WAIT_ON_OUTPUT("read" << "0", "external");
#ifdef Q_OS_WIN
    RUN("read" << "text/html" << "0", "<!--StartFragment--><b>external</b><!--EndFragment-->");
#else
    RUN("read" << "text/html" << "0", "<b>external</b>");
#endif
    RUN("read" << "application/custom" << "0", "opaque");
    WAIT_ON_OUTPUT("eval" << "tab('CaptureEvents');size()", "1\n");
    QByteArray previousTime;
    QByteArray errors;
    QCOMPARE(m_test->run({QStringLiteral("read"), mimeHistoryTime, QStringLiteral("0")}, &previousTime, &errors), 0);
    QVERIFY2(errors.isEmpty(), errors.constData());
    QVERIFY(previousTime.toLongLong() > 0);
    TEST(m_test->setClipboard(copied));
    WAIT_ON_OUTPUT("eval" << QStringLiteral("Number(str(read('application/x-copyq-private-history-time',0)))>%1").arg(previousTime.toLongLong()), "true\n");
    RUN("size", "1\n");
    RUN("eval" << "tab('CaptureEvents');size()", "1\n");
    QImage image({2, 2}, QImage::Format_RGB32);
    image.fill(Qt::red);
    QByteArray png;
    QBuffer buffer(&png);
    QVERIFY(buffer.open(QIODevice::WriteOnly));
    QVERIFY(image.save(&buffer, "PNG"));
    TEST(m_test->setClipboard({{QStringLiteral("image/png"), png}}));
    WAIT_ON_OUTPUT("eval" << "clipboard('image/png').length > 0", "true\n");
    TEST(m_test->setClipboard(QByteArray("external two")));
    WAIT_ON_OUTPUT("read" << "0", "external two");
    RUN("size", "2\n");
    WAIT_ON_OUTPUT("eval" << "tab('CaptureEvents');size()", "2\n");
    auto secret = copied;
    secret.insert(mimeText, QByteArray("secret"));
    secret.insert(mimeSecret, QByteArray("1"));
    TEST(m_test->setClipboard(secret));
    WAIT_ON_OUTPUT("clipboard", "secret");
    RUN("disable", "");
    RUN("size", "2\n");
    RUN("eval" << "tab('CaptureEvents');size()", "2\n");
}

void CoreTests::managementDialogs()
{
    RUN("disable", "");
    RUN("add" << "inspect", "");
    RUN("show", "");
    WAIT_ON_OUTPUT("eval" << "callPlugin('itemtests','managementState').filtering", "false\n");
    for (const auto &entry : QList<QPair<int,QString>>{
            {Actions::Help_About, aboutDialogId}, {Actions::Help_ShowLog, logDialogId},
            {Actions::File_ProcessManager, actionHandlerDialogId},
            {Actions::Item_ShowContent, clipboardDialogId}, {Actions::Item_Action, actionDialogId}}) {
        RUN("eval" << QStringLiteral("callPlugin('itemtests','managementAction',%1)").arg(entry.first), "true\n");
        KEYS(entry.second << "ESCAPE");
        RUN("eval" << "callPlugin('itemtests','managementState').visible", "true\n");
    }
}
