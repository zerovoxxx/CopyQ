// SPDX-License-Identifier: GPL-3.0-or-later
#include "tests.h"
#include "test_utils.h"
#include "tests_common.h"
#include "gui/menuitems.h"

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
}
