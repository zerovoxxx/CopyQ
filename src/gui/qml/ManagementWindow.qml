// SPDX-License-Identifier: GPL-3.0-or-later
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QClip

Pane {
    id: root
    objectName: "management_root"
    required property ClipboardManagementWindow controller
    readonly property var history: controller.history
    readonly property var preview: history.preview
    readonly property var options: controller.options
    readonly property int pageRows: Math.max(1, Math.floor(results.height / (theme.rowHeight + results.spacing)))
    readonly property bool hasSelection: history.selectedCount > 0 && !history.filtering
    readonly property bool popupActive: tabDialog.visible || removeTab.visible || properties.visible
        || itemMenu.visible || globalMenu.visible || tabMenu.visible || iconDialog.visible || clearHistory.visible
    width: 1100
    height: 740
    padding: 0
    font: theme.textFont
    palette.window: theme.background
    palette.windowText: theme.foreground
    palette.base: theme.background
    palette.text: theme.foreground
    palette.button: theme.alternate
    palette.buttonText: theme.foreground
    palette.highlight: theme.highlight
    palette.highlightedText: theme.highlightedText
    background: GlassBackground { values: root.controller.theme; controller: root.controller }
    Theme { id: theme; values: root.controller.theme }

    function actionName(id) { return controller.actions[id] ? controller.actions[id].name : "" }
    function action(id) { controller.triggerAction(id) }
    function toolbarText(id, label) { return root.options.hideToolbarLabels ? (controller.actions[id].icon || label) : label }
    Connections {
        target: root.controller as QtObject
        function onOpened() { results.forceActiveFocus() }
        function onNewTabRequested() { tabName.text = ""; tabDialog.rename = false; tabDialog.open() }
        function onRenameTabRequested() { tabName.text = root.controller.selectedTabPath; tabDialog.rename = true; tabDialog.open() }
        function onRemoveTabRequested() { if (root.controller.tabs.length > 1) removeTab.open() }
        function onSearchRequested() { search.forceActiveFocus(); search.selectAll() }
        function onItemMenuRequested() { if (root.hasSelection) itemMenu.open() }
        function onIconRequested() { iconDialog.open() }
    }
    Connections {
        target: root.history as QtObject
        function onQueryChanged() { search.text = root.history.query }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        RowLayout {
            Layout.fillWidth: true
            Layout.margins: theme.margin
            spacing: theme.spacing
            Label { text: "QClip"; font.weight: Font.DemiBold; font.pixelSize: 18 }
            Label { text: qsTr("Clipboard manager"); color: theme.muted; visible: root.width >= 1050 }
            Item { Layout.fillWidth: true }
            ThemeButton { values: root.controller.theme;
                quiet: true; iconName: root.controller.monitoring ? "pause" : "play"
                text: root.controller.monitoring ? qsTr("Pause recording") : qsTr("Resume recording")
                onClicked: root.action(ClipboardManagementWindow.File_ToggleClipboardStoring)
            }
            ThemeButton { values: root.controller.theme;
                quiet: true; text: qsTr("Settings…")
                onClicked: root.action(ClipboardManagementWindow.File_Preferences)
            }
            ThemeButton { values: root.controller.theme;
                quiet: true; iconName: "more"; Accessible.name: qsTr("More options")
                onClicked: globalMenu.open()
                ThemeMenu { values: root.controller.theme;
                    id: globalMenu
                    objectName: "management_global_menu"
                    ThemeMenuItem { values: root.controller.theme; text: qsTr("Snippets…"); onTriggered: root.controller.showSnippets() }
                    ThemeMenuItem { values: root.controller.theme; text: qsTr("Save snippet"); enabled: root.hasSelection; onTriggered: root.controller.saveSnippet() }
                    ThemeMenuItem { values: root.controller.theme; text: qsTr("Clear history…"); enabled: root.controller.tabName === root.options.historyTab; onTriggered: clearHistory.open() }
                    ThemeMenuSeparator { values: root.controller.theme }
                    Repeater {
                        model: [ClipboardManagementWindow.File_Import, ClipboardManagementWindow.File_Export,
                            ClipboardManagementWindow.File_Commands, ClipboardManagementWindow.File_ShowClipboardContent,
                            ClipboardManagementWindow.File_ShowPreview,
                            ClipboardManagementWindow.File_ProcessManager, ClipboardManagementWindow.Help_ShowLog,
                            ClipboardManagementWindow.Help_About, ClipboardManagementWindow.Help_Help,
                            ClipboardManagementWindow.File_Exit]
                        ThemeMenuItem { values: root.controller.theme;
                            required property int modelData
                            text: root.actionName(modelData)
                            onTriggered: root.action(modelData)
                        }
                    }
                }
            }
        }
        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: theme.line }
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0
            ColumnLayout {
                Layout.preferredWidth: 180
                visible: !root.options.hideTabs
                Layout.fillHeight: true
                Layout.margins: theme.margin
                spacing: theme.spacing
                Label { text: qsTr("COLLECTIONS"); color: theme.muted; font.pixelSize: 10; font.letterSpacing: 1 }
                ListView {
                    id: tabs
                    objectName: "management_tabs"
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    model: root.controller.tabTree
                    clip: true
                    spacing: 2
                    ScrollBar.vertical: ThemeScrollBar { values: root.controller.theme; policy: theme.showScrollbars ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff }
                    delegate: ThemeDelegate { values: root.controller.theme;
                        id: tabRow
                        required property var modelData
                        width: ListView.view.width
                        implicitHeight: theme.customStyle ? Math.max(theme.controlHeight, contentItem.implicitHeight + 8) : theme.controlHeight
                        highlighted: modelData.path === root.controller.selectedTabPath
                        onClicked: root.controller.selectTabPath(modelData.path)
                        Accessible.name: modelData.path
                        background: Rectangle {
                            radius: theme.radius
                            color: tabRow.highlighted ? theme.selection : tabRow.hovered ? theme.hover : "transparent"
                            ClipboardStyle {
                                anchors.fill: parent; theme: root.controller.theme
                                kind: root.options.treeMode === false ? ClipboardStyle.Tab : ClipboardStyle.Collection
                                font: tabRow.font; selected: tabRow.highlighted; hovered: tabRow.hovered
                                visible: theme.customStyle
                            }
                        }
                        contentItem: RowLayout {
                            spacing: 4
                            Item { Layout.preferredWidth: tabRow.modelData.depth * 12 }
                            ThemeButton {
                                values: root.controller.theme; quiet: true
                                visible: tabRow.modelData.group
                                Layout.preferredWidth: 22
                                iconName: tabRow.modelData.expanded ? "chevron-down" : "chevron-right"
                                enabled: tabRow.modelData.group
                                onClicked: root.controller.toggleGroup(tabRow.modelData.path)
                                Accessible.name: qsTr("Expand or collapse %1").arg(tabRow.modelData.path)
                            }
                            Label { text: tabRow.modelData.icon.length <= 2 ? tabRow.modelData.icon : ""; font: root.controller.iconFont; visible: text.length > 0 }
                            Image { source: tabRow.modelData.icon.length > 2 ? "file:" + tabRow.modelData.icon : ""; visible: tabRow.modelData.icon.length > 2; Layout.preferredWidth: 20; Layout.preferredHeight: 20; fillMode: Image.PreserveAspectFit }
                            ThemeIcon { name: "collection"; color: tabRow.highlighted ? theme.selectionText : theme.muted; visible: !theme.customStyle && tabRow.modelData.icon.length === 0 }
                            Label { text: tabRow.modelData.name; Layout.fillWidth: true; elide: Text.ElideRight; color: tabRow.highlighted ? theme.selectionText : theme.foreground; font.weight: tabRow.highlighted ? Font.DemiBold : Font.Normal; visible: !theme.customStyle }
                            Label { text: tabRow.modelData.count < 0 ? "?" : String(tabRow.modelData.count); color: theme.muted; font.pixelSize: 11; visible: !theme.customStyle && root.options.showTabCounts === true && tabRow.modelData.collection }
                            ClipboardStyle {
                                visible: theme.customStyle
                                theme: root.controller.theme
                                kind: root.options.treeMode === false ? ClipboardStyle.Tab : ClipboardStyle.Collection
                                text: tabRow.modelData.name
                                rowNumber: root.options.showTabCounts === true && tabRow.modelData.collection ? (tabRow.modelData.count < 0 ? "?" : String(tabRow.modelData.count)) : ""
                                font: tabRow.font
                                selected: theme.customStyle && tabRow.highlighted
                                hovered: theme.customStyle && tabRow.hovered
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                            }
                        }
                        MouseArea {
                            anchors.fill: parent
                            acceptedButtons: Qt.RightButton
                            onClicked: { root.controller.selectTabPath(tabRow.modelData.path); tabMenu.open() }
                        }
                        DropArea {
                            anchors.fill: parent
                            onDropped: function(drop) {
                                if (tabRow.modelData.collection && root.controller.dropItems(tabRow.modelData.path, 0, drop.proposedAction === Qt.MoveAction))
                                    drop.acceptProposedAction()
                            }
                        }
                    }
                }
                RowLayout {
                    ThemeButton { values: root.controller.theme; text: qsTr("New"); objectName: "management_new_tab"; onClicked: { tabName.text = ""; tabDialog.rename = false; tabDialog.open() } }
                    ThemeButton { values: root.controller.theme;
                        text: qsTr("More…")
                        onClicked: tabMenu.open()
                        ThemeMenu { values: root.controller.theme;
                            id: tabMenu
                            objectName: "management_collection_menu"
                            ThemeMenuItem { values: root.controller.theme; text: qsTr("Rename…"); onTriggered: { tabName.text = root.controller.selectedTabPath; tabDialog.rename = true; tabDialog.open() } }
                            ThemeMenuItem { values: root.controller.theme; text: qsTr("New inside group…"); visible: root.controller.selectedTabIsGroup; onTriggered: { tabName.text = root.controller.selectedTabPath + "/"; tabDialog.rename = false; tabDialog.open() } }
                            ThemeMenuItem { values: root.controller.theme; text: qsTr("Properties…"); enabled: !root.controller.selectedTabIsGroup; onTriggered: properties.open() }
                            ThemeMenuItem { values: root.controller.theme; text: qsTr("Change icon…"); onTriggered: root.action(ClipboardManagementWindow.Tabs_ChangeTabIcon) }
                            ThemeMenuItem { values: root.controller.theme; text: qsTr("Move up"); onTriggered: root.controller.selectedTabIsGroup ? root.controller.moveGroup(root.controller.selectedTabPath, -1) : root.controller.moveTab(-1) }
                            ThemeMenuItem { values: root.controller.theme; text: qsTr("Move down"); onTriggered: root.controller.selectedTabIsGroup ? root.controller.moveGroup(root.controller.selectedTabPath, 1) : root.controller.moveTab(1) }
                            ThemeMenuItem { values: root.controller.theme; text: qsTr("Sort collections"); onTriggered: root.controller.sortGroup(root.controller.selectedTabIsGroup ? root.controller.selectedTabPath : "") }
                            ThemeMenuSeparator { values: root.controller.theme }
                            ThemeMenuItem { values: root.controller.theme; text: qsTr("Remove collection…"); enabled: root.controller.tabs.length > 1; onTriggered: removeTab.open() }
                        }
                    }
                }
            }
            Rectangle { Layout.fillHeight: true; Layout.preferredWidth: 1; color: theme.line }
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.margins: theme.margin
                spacing: theme.spacing
                RowLayout {
                    Layout.fillWidth: true
                    spacing: theme.spacing
                    ThemeTextField { values: root.controller.theme;
                        id: search
                        objectName: "management_search"
                        KeyNavigation.tab: results
                        Layout.fillWidth: true
                        placeholderText: qsTr("Search this collection…")
                        iconName: "search"
                        text: root.history.query
                        onTextEdited: root.history.query = text
                        onAccepted: root.action(ClipboardManagementWindow.Item_MoveToClipboard)
                        selectByMouse: true
                        font: theme.searchFont
                        color: theme.searchText
                        Accessible.name: qsTr("Search collection")
                    }
                    ThemeButton { values: root.controller.theme; iconName: "plus"; text: qsTr("New item…"); onClicked: root.action(ClipboardManagementWindow.File_New) }
                }
                RowLayout {
                    visible: !root.options.hideToolbar
                    spacing: 6
                    ThemeButton { values: root.controller.theme; text: qsTr("Select all"); enabled: !root.history.filtering; onClicked: { root.history.selectAll(); results.forceActiveFocus() } }
                    ThemeButton {
                        id: copyButton; values: root.controller.theme; toolbar: true; iconName: "copy"
                        text: root.options.hideToolbarLabels ? "" : qsTr("Copy"); Accessible.name: qsTr("Copy")
                        objectName: "management_copy"; enabled: root.hasSelection
                        onClicked: root.action(ClipboardManagementWindow.Edit_CopySelectedItems)
                        ThemeToolTip { values: root.controller.theme; visible: copyButton.hovered; text: qsTr("Copy") }
                    }
                    ThemeButton {
                        id: editButton; values: root.controller.theme; toolbar: true; iconName: "edit"
                        text: root.options.hideToolbarLabels ? "" : qsTr("Edit…"); Accessible.name: qsTr("Edit…")
                        enabled: root.hasSelection; onClicked: root.action(ClipboardManagementWindow.Item_Edit)
                        ThemeToolTip { values: root.controller.theme; visible: editButton.hovered; text: qsTr("Edit…") }
                    }
                    ThemeButton {
                        id: deleteButton; values: root.controller.theme; toolbar: true; destructive: true; iconName: "trash"
                        text: root.options.hideToolbarLabels ? "" : qsTr("Delete"); Accessible.name: qsTr("Delete")
                        objectName: "management_delete"; enabled: root.hasSelection
                        onClicked: root.action(ClipboardManagementWindow.Item_Remove)
                        ThemeToolTip { values: root.controller.theme; visible: deleteButton.hovered; text: qsTr("Delete") }
                    }
                    ThemeButton { values: root.controller.theme;
                        text: qsTr("Actions…")
                        enabled: root.hasSelection
                        onClicked: itemMenu.open()
                        ThemeMenu { values: root.controller.theme;
                            id: itemMenu
                            objectName: "management_item_menu"
                            Repeater {
                                model: [ClipboardManagementWindow.Item_MoveToClipboard, ClipboardManagementWindow.Edit_SortSelectedItems, ClipboardManagementWindow.Edit_ReverseSelectedItems,
                                    ClipboardManagementWindow.Edit_PasteItems, ClipboardManagementWindow.Item_ShowContent,
                                    ClipboardManagementWindow.Item_EditNotes, ClipboardManagementWindow.Item_EditWithEditor,
                                    ClipboardManagementWindow.Item_Action, ClipboardManagementWindow.Item_MoveUp,
                                    ClipboardManagementWindow.Item_MoveDown, ClipboardManagementWindow.Item_MoveToTop,
                                    ClipboardManagementWindow.Item_MoveToBottom]
                                ThemeMenuItem { values: root.controller.theme; required property int modelData; text: root.actionName(modelData); onTriggered: root.action(modelData) }
                            }
                            ThemeMenuSeparator { values: root.controller.theme }
                            Repeater {
                                model: root.controller.commands
                                ThemeMenuItem { values: root.controller.theme;
                                    required property var modelData
                                    required property int index
                                    text: modelData.name + (modelData.shortcut ? "  " + modelData.shortcut : "")
                                    enabled: modelData.enabled
                                    onTriggered: root.controller.triggerCommand(index)
                                }
                            }
                        }
                    }
                }
                RowLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: theme.spacing
                    ListView {
                        id: results
                        objectName: "management_results"
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.preferredWidth: 430
                        model: root.history
                        currentIndex: root.history.selectedRow
                        onCurrentIndexChanged: positionViewAtIndex(currentIndex, ListView.Contain)
                        focus: true
                        clip: true
                        spacing: 2
                        boundsBehavior: Flickable.StopAtBounds
                        ScrollBar.vertical: ThemeScrollBar { values: root.controller.theme; policy: theme.showScrollbars ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff }
                        delegate: Rectangle {
                            id: row
                            required property int index
                            required property string summary
                            Component.onCompleted: root.history.requestDisplay(index)
                            onSummaryChanged: root.history.requestDisplay(index)
                            Connections {
                                target: root.history
                                function onDisplaysInvalidated() { root.history.requestDisplay(row.index) }
                            }
                            required property string itemType
                            required property bool itemSelected
                            required property string notes
                            required property string tags
                            required property bool pinned
                            required property int sourceRow
                            width: ListView.view.width
                            height: theme.rowHeight
                            radius: theme.radius
                            color: row.itemSelected ? theme.selection : rowMouse.containsMouse ? theme.hover : "transparent"
                            border.color: row.index === root.history.selectedRow && results.activeFocus ? theme.focusRing : "transparent"
                            ClipboardStyle {
                                visible: theme.customStyle
                                anchors.fill: parent
                                kind: ClipboardStyle.Item
                                theme: root.controller.theme
                                font: theme.textFont
                                text: row.summary + "\n" + (row.pinned ? qsTr("Pinned · ") : "") + row.itemType + (row.tags ? " · " + row.tags : "") + (row.notes ? " · " + row.notes : "")
                                selected: row.itemSelected
                                alternate: row.index % 2 !== 0
                                focused: row.index === root.history.selectedRow && results.activeFocus
                                hovered: rowMouse.containsMouse
                                rowNumber: theme.showNumber ? String(row.sourceRow + (root.options.rowIndexFromOne ? 1 : 0)) : ""
                            }
                            RowLayout {
                                anchors.fill: parent; anchors.margins: 8; spacing: 10
                                visible: !theme.customStyle
                                Rectangle {
                                    Layout.preferredWidth: 28; Layout.preferredHeight: 28; radius: 7
                                    color: row.itemSelected ? theme.selection : theme.hover
                                    ThemeIcon { anchors.centerIn: parent; name: row.itemType === "Image" ? "image" : row.itemType === "Files / links" ? "link" : "text"; color: row.itemSelected ? theme.selectionText : theme.muted }
                                }
                                ColumnLayout {
                                    Layout.fillWidth: true; spacing: 2
                                    Label { Layout.fillWidth: true; text: row.summary.split("\n")[0]; elide: Text.ElideRight; textFormat: Text.PlainText; color: theme.foreground }
                                    Label { Layout.fillWidth: true; text: (row.pinned ? qsTr("Pinned · ") : "") + row.itemType + (row.tags ? " · " + row.tags : "") + (row.notes ? " · " + row.notes : ""); elide: Text.ElideRight; textFormat: Text.PlainText; color: theme.muted; font.pixelSize: 11 }
                                }
                                Label { text: String(row.sourceRow + (root.options.rowIndexFromOne ? 1 : 0)); visible: theme.showNumber; color: theme.muted; font.pixelSize: 11; horizontalAlignment: Text.AlignRight; Layout.minimumWidth: 18 }
                            }
                            MouseArea {
                                id: rowMouse
                                hoverEnabled: true
                                anchors.fill: parent
                                acceptedButtons: Qt.LeftButton | Qt.RightButton
                                property point pressedPosition
                                onPressed: function(mouse) {
                                    pressedPosition = Qt.point(mouse.x, mouse.y)
                                    if (mouse.button === Qt.RightButton) {
                                        if (!root.history.isSelected(row.index)) root.history.selectRow(row.index)
                                        itemMenu.popup()
                                    } else {
                                        root.history.select(row.index, !!(mouse.modifiers & Qt.ShiftModifier), !!(mouse.modifiers & (Qt.ControlModifier | Qt.MetaModifier)))
                                        if (root.options.singleClick && mouse.modifiers === Qt.NoModifier)
                                            root.action(ClipboardManagementWindow.Item_MoveToClipboard)
                                    }
                                    results.forceActiveFocus()
                                }
                                onPositionChanged: function(mouse) {
                                    if (pressed && (Math.abs(mouse.x - pressedPosition.x) + Math.abs(mouse.y - pressedPosition.y)) > 12)
                                        root.controller.startDrag(row.index)
                                }
                                onDoubleClicked: if (!root.options.singleClick) root.action(ClipboardManagementWindow.Item_MoveToClipboard)
                            }
                            DropArea {
                                anchors.fill: parent
                                onDropped: function(drop) {
                                    if (root.controller.dropItems(root.controller.tabName, root.history.selectedRow < 0 ? 0 : row.index, drop.proposedAction === Qt.MoveAction))
                                        drop.acceptProposedAction()
                                }
                            }
                            Accessible.name: summary
                            Accessible.role: Accessible.ListItem
                            Accessible.selected: itemSelected
                        }
                        DropArea {
                            anchors.fill: parent
                            z: -1
                            onDropped: function(drop) {
                                if (root.controller.dropItems(root.controller.tabName, -1, drop.proposedAction === Qt.MoveAction))
                                    drop.acceptProposedAction()
                            }
                        }
                        Label {
                            anchors.centerIn: parent
                            visible: root.history.count === 0
                            text: root.history.filtering ? qsTr("Searching…") : root.history.sourceCount === 0 ? qsTr("This collection is empty") : qsTr("No matches")
                        }
                    }
                    Rectangle {
                        Layout.fillHeight: true
                        Layout.preferredWidth: 270
                        visible: root.width >= 1000 && root.options.preview !== false
                        radius: theme.radius + 2
                        color: theme.surface
                        border.color: theme.line
                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 12
                            spacing: theme.spacing
                            Label { text: qsTr("Preview"); color: theme.muted; font.pixelSize: 11; font.weight: Font.DemiBold }
                            ClipboardItemPreview {
                                objectName: "management_preview"
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                history: root.controller.history
                                theme: root.controller.theme
                                Accessible.name: root.preview.text || qsTr("Item preview")
                            }
                            Label { Layout.fillWidth: true; visible: !!root.preview.notes; text: root.preview.notes || ""; wrapMode: Text.Wrap; color: theme.notesText }
                            Label { visible: !!root.preview.tags; text: qsTr("Tags: ") + (root.preview.tags || "") }
                            Label { visible: !!root.preview.pinned; text: qsTr("Pinned item") }
                            Label { Layout.fillWidth: true; wrapMode: Text.Wrap; color: theme.muted; font.pixelSize: 11; text: root.preview.copiedAt >= 0 ? qsTr("Copied: ") + new Date(root.preview.copiedAt).toLocaleString() : qsTr("Copy time unknown") }
                            Label { Layout.fillWidth: true; wrapMode: Text.Wrap; color: theme.muted; font.pixelSize: 11; text: root.preview.application || qsTr("Source application unknown") }
                        }
                    }
                }
                RowLayout {
                    visible: root.hasSelection && root.controller.tabs.length > 1
                    Label { text: qsTr("To collection") }
                    ThemeComboBox { values: root.controller.theme; id: targetTab; Layout.fillWidth: true; model: root.controller.tabs }
                    ThemeButton { values: root.controller.theme; text: qsTr("Copy"); enabled: targetTab.currentText !== root.controller.tabName; onClicked: root.controller.transferItems(targetTab.currentText, false) }
                    ThemeButton { values: root.controller.theme; text: qsTr("Move"); enabled: targetTab.currentText !== root.controller.tabName; onClicked: root.controller.transferItems(targetTab.currentText, true) }
                }
            }
        }
        Label { Layout.fillWidth: true; Layout.margins: theme.spacing; visible: root.controller.error.length > 0; text: root.controller.error; wrapMode: Text.Wrap }
        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: theme.line }
        Label { Layout.margins: theme.margin; color: theme.muted; font.pixelSize: 11; text: qsTr("%1 of %2 items · %3 selected").arg(root.history.count).arg(root.history.sourceCount).arg(root.history.selectedCount) + (root.controller.monitoring ? "" : qsTr(" · Recording paused")) }
    }

    ThemeDialog { values: root.controller.theme;
        id: tabDialog
        objectName: "management_collection_dialog"
        property bool rename: false
        property string sourceTab
        property bool group: false
        property var members: []
        anchors.centerIn: parent
        title: rename ? qsTr("Rename collection") : qsTr("New collection")
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel
        onOpened: { sourceTab = root.controller.selectedTabPath; group = root.controller.selectedTabIsGroup; members = root.controller.collectionsInGroup(sourceTab); tabName.forceActiveFocus(); tabName.selectAll() }
        onAccepted: {
            if (!rename) root.controller.createTab(tabName.text)
            else if (group) root.controller.renameGroup(sourceTab, tabName.text, members)
            else root.controller.renameTab(tabName.text, sourceTab)
        }
        ThemeTextField { values: root.controller.theme; id: tabName; objectName: "management_tab_name"; width: 320; placeholderText: qsTr("Collection name; / creates a group"); Accessible.name: qsTr("Collection name"); onAccepted: tabDialog.accept() }
    }
    ThemeDialog { values: root.controller.theme;
        id: removeTab
        property string sourceTab
        property bool group: false
        property var members: []
        width: Math.min(420, root.width - 40)
        anchors.centerIn: parent
        title: qsTr("Remove collection?")
        modal: true
        standardButtons: Dialog.Yes | Dialog.No
        onOpened: { sourceTab = root.controller.selectedTabPath; group = root.controller.selectedTabIsGroup; members = root.controller.collectionsInGroup(sourceTab) }
        onAccepted: group ? root.controller.removeGroup(sourceTab, members) : root.controller.removeTab(sourceTab)
        Label { text: qsTr("All items in %1 will be deleted (%2 collections).").arg(removeTab.sourceTab).arg(removeTab.members.length); wrapMode: Text.Wrap }
    }
    ThemeDialog { values: root.controller.theme;
        id: properties
        objectName: "management_properties"
        property string sourceTab
        width: Math.min(540, root.width - 40)
        anchors.centerIn: parent
        title: qsTr("Collection properties")
        modal: true
        standardButtons: Dialog.Save | Dialog.Cancel
        onOpened: {
            sourceTab = root.controller.tabName
            capacity.value = root.controller.maxItemCount
            store.checked = root.controller.storeItems
            encryptedExpiry.value = root.controller.encryptedExpireSeconds
        }
        onAccepted: root.controller.saveTabProperties({ maxItemCount: capacity.value, storeItems: store.checked, encryptedExpireSeconds: encryptedExpiry.value }, sourceTab)
        GridLayout {
            columns: 2
            Label { text: qsTr("Capacity (0 uses the global limit)"); Layout.fillWidth: true; wrapMode: Text.Wrap }
            ThemeSpinBox { values: root.controller.theme; id: capacity; objectName: "management_capacity"; from: 0; to: 100000; editable: true }
            Label { text: qsTr("Save items to disk") }
            ThemeCheckBox { values: root.controller.theme; id: store; objectName: "management_store" }
            Label { text: qsTr("Password expires after seconds (0 uses global)"); Layout.fillWidth: true; wrapMode: Text.Wrap }
            ThemeSpinBox { values: root.controller.theme; id: encryptedExpiry; objectName: "management_encrypted_expiry"; from: 0; to: 999999; editable: true }
        }
    }
    ThemeDialog { values: root.controller.theme;
        id: clearHistory
        objectName: "management_clear_dialog"
        property string sourceTab
        anchors.centerIn: parent; modal: true
        title: qsTr("Clear ordinary history?"); standardButtons: Dialog.Yes | Dialog.No
        onOpened: sourceTab = root.controller.tabName
        onAccepted: if (sourceTab === root.controller.tabName) root.controller.clearHistory(Number(clearRange.currentValue))
        ColumnLayout {
            Label { text: qsTr("Pinned items are kept. Time ranges keep items with unknown or future copy times."); wrapMode: Text.Wrap; Layout.preferredWidth: 420 }
            ThemeComboBox { values: root.controller.theme;
                id: clearRange; Layout.fillWidth: true; textRole: "label"; valueRole: "minutes"
                model: [{label: qsTr("Last 5 minutes"), minutes: 5}, {label: qsTr("Last 15 minutes"), minutes: 15},
                    {label: qsTr("Last hour"), minutes: 60}, {label: qsTr("Last 24 hours"), minutes: 1440},
                    {label: qsTr("All ordinary history, including unknown times"), minutes: -1}]
            }
        }
    }
    ThemeDialog { values: root.controller.theme;
        id: iconDialog
        objectName: "management_icon_dialog"
        property string sourceTab
        anchors.centerIn: parent
        width: Math.min(580, root.width - 40)
        height: Math.min(500, root.height - 40)
        title: qsTr("Collection icon")
        modal: true
        standardButtons: Dialog.Save | Dialog.Cancel
        onOpened: { sourceTab = root.controller.selectedTabPath; iconValue.text = ""; iconSearch.text = "" }
        onAccepted: root.controller.saveIcon(iconValue.text, sourceTab)
        ColumnLayout {
            anchors.fill: parent
            ThemeTextField { values: root.controller.theme; id: iconSearch; Layout.fillWidth: true; placeholderText: qsTr("Search icons…") }
            GridView {
                Layout.fillWidth: true; Layout.fillHeight: true; clip: true
                cellWidth: 44; cellHeight: 44
                model: root.controller.icons(iconSearch.text)
                ScrollBar.vertical: ThemeScrollBar { values: root.controller.theme; policy: theme.showScrollbars ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff }
                delegate: ThemeButton {
                    id: iconButton
                    values: root.controller.theme; quiet: true
                    required property var modelData
                    width: 42; height: 42; text: modelData.icon || "×"; font: root.controller.iconFont
                    onClicked: iconValue.text = modelData.icon
                    ThemeToolTip { values: root.controller.theme; visible: iconButton.hovered; text: iconButton.modelData.name }
                    Accessible.name: modelData.name
                }
            }
            RowLayout {
                ThemeTextField { values: root.controller.theme; id: iconValue; Layout.fillWidth: true; placeholderText: qsTr("Icon or image file") }
                ThemeButton { values: root.controller.theme; text: qsTr("Browse…"); onClicked: { const path = root.controller.browseIcon(); if (path) iconValue.text = path } }
            }
        }
    }
}
