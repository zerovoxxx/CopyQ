// SPDX-License-Identifier: GPL-3.0-or-later
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QClip

Pane {
    id: root
    required property ClipboardManagementWindow controller
    readonly property var history: controller.history
    readonly property var preview: history.preview
    readonly property bool hasSelection: history.selectedCount > 0 && !history.filtering
    readonly property bool popupActive: tabDialog.visible || removeTab.visible || properties.visible
        || itemMenu.visible || globalMenu.visible || tabMenu.visible
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
    background: Rectangle { color: theme.background }
    Theme { id: theme; values: root.controller.theme }

    function actionName(id) { return controller.actions[id] ? controller.actions[id].name : "" }
    function action(id) { controller.triggerAction(id) }
    Connections {
        target: root.controller as QtObject
        function onOpened() { search.forceActiveFocus() }
        function onNewTabRequested() { tabName.text = ""; tabDialog.rename = false; tabDialog.open() }
        function onRenameTabRequested() { tabName.text = root.controller.tabName; tabDialog.rename = true; tabDialog.open() }
        function onRemoveTabRequested() { if (root.controller.tabs.length > 1) removeTab.open() }
        function onSearchRequested() { search.forceActiveFocus(); search.selectAll() }
        function onItemMenuRequested() { if (root.hasSelection) itemMenu.open() }
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
            Label { text: "QClip"; font.bold: true; font.pixelSize: 22 }
            Label { text: qsTr("Clipboard manager"); opacity: 0.7; visible: root.width >= 950 }
            Item { Layout.fillWidth: true }
            Button {
                text: root.controller.monitoring ? qsTr("Pause recording") : qsTr("Resume recording")
                onClicked: root.action(ClipboardManagementWindow.File_ToggleClipboardStoring)
            }
            Button {
                text: qsTr("Settings…")
                onClicked: root.action(ClipboardManagementWindow.File_Preferences)
            }
            Button {
                text: qsTr("More…")
                onClicked: globalMenu.open()
                Menu {
                    id: globalMenu
                    Repeater {
                        model: [ClipboardManagementWindow.File_Import, ClipboardManagementWindow.File_Export,
                            ClipboardManagementWindow.File_Commands, ClipboardManagementWindow.File_ShowClipboardContent,
                            ClipboardManagementWindow.File_ProcessManager, ClipboardManagementWindow.Help_ShowLog,
                            ClipboardManagementWindow.Help_About, ClipboardManagementWindow.Help_Help,
                            ClipboardManagementWindow.File_Exit]
                        MenuItem {
                            required property int modelData
                            text: root.actionName(modelData)
                            onTriggered: root.action(modelData)
                        }
                    }
                }
            }
        }
        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: theme.alternate }
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0
            ColumnLayout {
                Layout.preferredWidth: 195
                Layout.fillHeight: true
                Layout.margins: theme.spacing
                Label { text: qsTr("Collections"); font.bold: true }
                ListView {
                    id: tabs
                    objectName: "management_tabs"
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    model: root.controller.tabs
                    clip: true
                    spacing: 4
                    ScrollBar.vertical: ScrollBar {}
                    delegate: ItemDelegate {
                        id: tabRow
                        required property string modelData
                        width: ListView.view.width
                        text: modelData
                        highlighted: modelData === root.controller.tabName
                        onClicked: root.controller.changeSource(modelData)
                        Accessible.name: modelData
                        DropArea {
                            anchors.fill: parent
                            onDropped: function(drop) {
                                if (root.controller.dropItems(tabRow.modelData, 0, drop.proposedAction === Qt.MoveAction))
                                    drop.acceptProposedAction()
                            }
                        }
                    }
                }
                RowLayout {
                    Button { text: qsTr("New"); objectName: "management_new_tab"; onClicked: { tabName.text = ""; tabDialog.rename = false; tabDialog.open() } }
                    Button {
                        text: qsTr("More…")
                        onClicked: tabMenu.open()
                        Menu {
                            id: tabMenu
                            MenuItem { text: qsTr("Rename…"); onTriggered: { tabName.text = root.controller.tabName; tabDialog.rename = true; tabDialog.open() } }
                            MenuItem { text: qsTr("Properties…"); onTriggered: properties.open() }
                            MenuItem { text: qsTr("Change icon…"); onTriggered: root.action(ClipboardManagementWindow.Tabs_ChangeTabIcon) }
                            MenuItem { text: qsTr("Move up"); enabled: root.controller.currentTabIndex > 0; onTriggered: root.controller.moveTab(-1) }
                            MenuItem { text: qsTr("Move down"); enabled: root.controller.currentTabIndex < root.controller.tabs.length - 1; onTriggered: root.controller.moveTab(1) }
                            MenuSeparator {}
                            MenuItem { text: qsTr("Remove collection…"); enabled: root.controller.tabs.length > 1; onTriggered: removeTab.open() }
                        }
                    }
                }
            }
            Rectangle { Layout.fillHeight: true; Layout.preferredWidth: 1; color: theme.alternate }
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.margins: theme.margin
                spacing: theme.spacing
                RowLayout {
                    Layout.fillWidth: true
                    TextField {
                        id: search
                        objectName: "management_search"
                        Layout.fillWidth: true
                        placeholderText: qsTr("Search this collection…")
                        text: root.history.query
                        onTextEdited: root.history.query = text
                        onAccepted: root.action(ClipboardManagementWindow.Item_MoveToClipboard)
                        selectByMouse: true
                        Accessible.name: qsTr("Search collection")
                    }
                    Button { text: qsTr("New item…"); onClicked: root.action(ClipboardManagementWindow.File_New) }
                }
                RowLayout {
                    Button { text: qsTr("Select all"); enabled: !root.history.filtering; onClicked: { root.history.selectAll(); results.forceActiveFocus() } }
                    Button { text: qsTr("Copy"); objectName: "management_copy"; enabled: root.hasSelection; onClicked: root.action(ClipboardManagementWindow.Edit_CopySelectedItems) }
                    Button { text: qsTr("Edit…"); enabled: root.hasSelection; onClicked: root.action(ClipboardManagementWindow.Item_Edit) }
                    Button { text: qsTr("Delete"); objectName: "management_delete"; enabled: root.hasSelection; onClicked: root.action(ClipboardManagementWindow.Item_Remove) }
                    Button {
                        text: qsTr("Actions…")
                        enabled: root.hasSelection
                        onClicked: itemMenu.open()
                        Menu {
                            id: itemMenu
                            Repeater {
                                model: [ClipboardManagementWindow.Item_MoveToClipboard, ClipboardManagementWindow.Edit_SortSelectedItems, ClipboardManagementWindow.Edit_ReverseSelectedItems,
                                    ClipboardManagementWindow.Edit_PasteItems, ClipboardManagementWindow.Item_ShowContent,
                                    ClipboardManagementWindow.Item_EditNotes, ClipboardManagementWindow.Item_EditWithEditor,
                                    ClipboardManagementWindow.Item_Action, ClipboardManagementWindow.Item_MoveUp,
                                    ClipboardManagementWindow.Item_MoveDown, ClipboardManagementWindow.Item_MoveToTop,
                                    ClipboardManagementWindow.Item_MoveToBottom]
                                MenuItem { required property int modelData; text: root.actionName(modelData); onTriggered: root.action(modelData) }
                            }
                            MenuSeparator {}
                            Repeater {
                                model: root.controller.commands
                                MenuItem {
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
                        spacing: 4
                        boundsBehavior: Flickable.StopAtBounds
                        ScrollBar.vertical: ScrollBar {}
                        delegate: Rectangle {
                            id: row
                            required property int index
                            required property string summary
                            required property string itemType
                            required property bool itemSelected
                            required property string notes
                            required property string tags
                            required property bool pinned
                            width: ListView.view.width
                            height: theme.rowHeight
                            radius: 6
                            color: itemSelected ? theme.highlight : index % 2 ? theme.alternate : theme.background
                            border.color: index === root.history.selectedRow ? theme.highlight : "transparent"
                            Column {
                                anchors.fill: parent
                                anchors.margins: 10
                                spacing: 4
                                Label { width: parent.width; text: row.summary; elide: Text.ElideRight; color: row.itemSelected ? theme.highlightedText : theme.foreground }
                                Label { width: parent.width; text: (row.pinned ? qsTr("Pinned · ") : "") + row.itemType + (row.tags ? " · " + row.tags : "") + (row.notes ? " · " + row.notes : ""); elide: Text.ElideRight; font.pixelSize: 11; opacity: 0.8; color: row.itemSelected ? theme.highlightedText : theme.foreground }
                            }
                            MouseArea {
                                anchors.fill: parent
                                acceptedButtons: Qt.LeftButton | Qt.RightButton
                                property point pressedPosition
                                onPressed: function(mouse) {
                                    pressedPosition = Qt.point(mouse.x, mouse.y)
                                    if (mouse.button === Qt.RightButton) {
                                        if (!root.history.isSelected(row.index)) root.history.selectRow(row.index)
                                        itemMenu.popup()
                                    } else {
                                        root.history.select(row.index, !!(mouse.modifiers & Qt.ShiftModifier), !!(mouse.modifiers & Qt.ControlModifier))
                                    }
                                    results.forceActiveFocus()
                                }
                                onPositionChanged: function(mouse) {
                                    if (pressed && (Math.abs(mouse.x - pressedPosition.x) + Math.abs(mouse.y - pressedPosition.y)) > 12)
                                        root.controller.startDrag(row.index)
                                }
                                onDoubleClicked: root.action(ClipboardManagementWindow.Item_MoveToClipboard)
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
                    Rectangle { Layout.fillHeight: true; Layout.preferredWidth: 1; color: theme.alternate; visible: root.width >= 1000 }
                    ColumnLayout {
                        Layout.fillHeight: true
                        Layout.preferredWidth: 270
                        visible: root.width >= 1000
                        Label { text: qsTr("Preview"); font.bold: true }
                        Image {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            visible: root.preview.type === "Image"
                            source: visible ? root.preview.image : ""
                            fillMode: Image.PreserveAspectFit
                            cache: false
                        }
                        ScrollView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            visible: root.preview.type !== "Image"
                            TextArea {
                                objectName: "management_preview"
                                readOnly: true
                                selectByMouse: true
                                wrapMode: TextEdit.Wrap
                                textFormat: root.preview.html ? TextEdit.RichText : TextEdit.PlainText
                                text: root.preview.html || root.preview.text || (root.preview.urls || []).join("\n")
                                font: theme.editorFont
                                palette.base: theme.editorBackground
                                palette.text: theme.editorText
                            }
                        }
                        Label { Layout.fillWidth: true; visible: !!root.preview.notes; text: root.preview.notes || ""; wrapMode: Text.Wrap; color: theme.notesText }
                        Label { visible: !!root.preview.tags; text: qsTr("Tags: ") + (root.preview.tags || "") }
                        Label { visible: !!root.preview.pinned; text: qsTr("Pinned item") }
                    }
                }
                RowLayout {
                    visible: root.hasSelection && root.controller.tabs.length > 1
                    Label { text: qsTr("To collection") }
                    ComboBox { id: targetTab; Layout.fillWidth: true; model: root.controller.tabs }
                    Button { text: qsTr("Copy"); enabled: targetTab.currentText !== root.controller.tabName; onClicked: root.controller.transferItems(targetTab.currentText, false) }
                    Button { text: qsTr("Move"); enabled: targetTab.currentText !== root.controller.tabName; onClicked: root.controller.transferItems(targetTab.currentText, true) }
                }
            }
        }
        Label { Layout.fillWidth: true; Layout.margins: theme.spacing; visible: root.controller.error.length > 0; text: root.controller.error; wrapMode: Text.Wrap }
        Label { Layout.margins: theme.spacing; text: qsTr("%1 of %2 items · %3 selected").arg(root.history.count).arg(root.history.sourceCount).arg(root.history.selectedCount) + (root.controller.monitoring ? "" : qsTr(" · Recording paused")); opacity: 0.7 }
    }

    Dialog {
        id: tabDialog
        property bool rename: false
        property string sourceTab
        anchors.centerIn: parent
        title: rename ? qsTr("Rename collection") : qsTr("New collection")
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel
        onOpened: { sourceTab = root.controller.tabName; tabName.forceActiveFocus(); tabName.selectAll() }
        onAccepted: rename ? root.controller.renameTab(tabName.text, sourceTab) : root.controller.createTab(tabName.text)
        TextField { id: tabName; objectName: "management_tab_name"; width: 320; placeholderText: qsTr("Collection name; / creates a group"); Accessible.name: qsTr("Collection name"); onAccepted: tabDialog.accept() }
    }
    Dialog {
        id: removeTab
        property string sourceTab
        width: Math.min(420, root.width - 40)
        anchors.centerIn: parent
        title: qsTr("Remove collection?")
        modal: true
        standardButtons: Dialog.Yes | Dialog.No
        onOpened: sourceTab = root.controller.tabName
        onAccepted: root.controller.removeTab(sourceTab)
        Label { text: qsTr("All items in %1 will be deleted.").arg(removeTab.sourceTab); wrapMode: Text.Wrap }
    }
    Dialog {
        id: properties
        objectName: "management_properties"
        property string sourceTab
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
            Label { text: qsTr("Capacity (0 uses the global limit)") }
            SpinBox { id: capacity; objectName: "management_capacity"; from: 0; to: 100000; editable: true }
            Label { text: qsTr("Save items to disk") }
            CheckBox { id: store; objectName: "management_store" }
            Label { text: qsTr("Password expires after seconds (0 uses global)") }
            SpinBox { id: encryptedExpiry; objectName: "management_encrypted_expiry"; from: 0; to: 999999; editable: true }
        }
    }
}
