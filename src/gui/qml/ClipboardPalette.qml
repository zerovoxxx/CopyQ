// SPDX-License-Identifier: GPL-3.0-or-later
pragma ComponentBehavior: Bound
import QtQuick
import QtQml
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QClip

Rectangle {
    id: root
    required property ClipboardPaletteWindow controller
    readonly property var preview: controller.history.preview
    width: 800
    height: 520
    color: colors.window
    border.color: colors.mid
    radius: 12
    SystemPalette { id: colors }

    Connections {
        target: root.controller as QtObject
        function onOpened() {
            search.text = ""
            search.forceActiveFocus()
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12
        RowLayout {
            Layout.fillWidth: true
            Label { text: "QClip"; font.bold: true; color: colors.windowText }
            ComboBox {
                id: sources
                Layout.preferredWidth: 160
                model: root.controller.tabs
                currentIndex: root.controller.currentTabIndex
                enabled: !root.controller.busy
                onActivated: root.controller.changeSource(currentText)
            }
            TextField {
                id: search
                objectName: "palette_search"
                Layout.fillWidth: true
                placeholderText: qsTr("Search clipboard history…")
                enabled: !root.controller.busy
                selectByMouse: true
                onTextEdited: root.controller.history.query = text
                Accessible.name: qsTr("Search clipboard history")
            }
            ThemeButton { values: root.controller.theme; text: "×"; onClicked: root.controller.cancel(); Accessible.name: qsTr("Close") }
            ThemeButton { values: root.controller.theme; text: qsTr("Manage…"); objectName: "palette_manage"; onClicked: root.controller.showManagement() }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 16
            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 400
                ListView {
                    id: results
                    objectName: "palette_results"
                    anchors.fill: parent
                    model: root.controller.history
                    currentIndex: root.controller.history.selectedRow
                    clip: true
                    spacing: 2
                    boundsBehavior: Flickable.StopAtBounds
                    onCurrentIndexChanged: positionViewAtIndex(currentIndex, ListView.Contain)
                    ScrollBar.vertical: ScrollBar {}
                    delegate: Rectangle {
                        id: row
                        required property int index
                        required property string summary
                        Component.onCompleted: root.controller.history.requestDisplay(index)
                        onSummaryChanged: root.controller.history.requestDisplay(index)
                        Connections {
                            target: root.controller.history
                            function onDisplaysInvalidated() { root.controller.history.requestDisplay(row.index) }
                        }
                        required property string itemType
                        width: ListView.view.width
                        height: 52
                        radius: 6
                        color: index === root.controller.history.selectedRow ? colors.highlight : colors.base
                        Column {
                            anchors.fill: parent
                            anchors.margins: 8
                            Text {
                                width: parent.width
                                text: row.summary
                                elide: Text.ElideRight
                                color: row.index === root.controller.history.selectedRow ? colors.highlightedText : colors.text
                            }
                            Text {
                                text: row.itemType
                                font.pixelSize: 11
                                color: row.index === root.controller.history.selectedRow ? colors.highlightedText : colors.text
                                opacity: 0.7
                            }
                        }
                        MouseArea {
                            anchors.fill: parent
                            enabled: !root.controller.busy
                            onClicked: root.controller.history.selectRow(row.index)
                            onDoubleClicked: root.controller.activate()
                        }
                        Accessible.name: summary
                        Accessible.role: Accessible.ListItem
                    }
                }
                Label {
                    anchors.centerIn: parent
                    visible: root.controller.history.count === 0
                    text: root.controller.history.filtering ? qsTr("Searching…")
                        : root.controller.history.sourceCount === 0 ? qsTr("Clipboard history is empty") : qsTr("No matches")
                    color: colors.windowText
                }
            }
            Rectangle { Layout.fillHeight: true; Layout.preferredWidth: 1; color: colors.mid }
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 330
                Label { text: root.preview.type || qsTr("Preview"); color: colors.windowText }
                ClipboardItemPreview {
                    objectName: "palette_preview"
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    history: root.controller.history
                    Accessible.name: root.preview.text || qsTr("Item preview")
                }
                RowLayout {
                    visible: (root.preview.urls || []).length > 0
                    ThemeButton { values: root.controller.theme; text: qsTr("Open"); onClicked: root.controller.openUrl(root.preview.urls[0]) }
                    ThemeButton { values: root.controller.theme; text: qsTr("Preview file"); visible: !!root.preview.filePath; onClicked: root.controller.previewFile() }
                }
                RowLayout {
                    ThemeButton { values: root.controller.theme; text: qsTr("Edit…"); enabled: !!root.preview.editable && !root.controller.busy; onClicked: root.controller.editItem() }
                    ThemeButton { values: root.controller.theme; text: qsTr("Plugin settings…"); onClicked: root.controller.showPluginSettings() }
                }
            }
        }

        Label {
            Layout.fillWidth: true
            visible: root.controller.error.length > 0
            text: root.controller.error
            wrapMode: Text.Wrap
            color: colors.windowText
        }
        Label {
            text: Qt.platform.os === "osx"
                ? qsTr("⌘↵ Copy · ⇧↵ Plain text · ⌥↵ Paste")
                : qsTr("Ctrl+Enter Copy · Shift+Enter Plain text · Alt+Enter Paste")
            color: colors.windowText
            font.pixelSize: 11
        }
        RowLayout {
            Layout.fillWidth: true
            Label { text: qsTr("↑↓ Select · Esc Close"); color: colors.windowText; font.pixelSize: 11 }
            Item { Layout.fillWidth: true }
            ThemeButton { values: root.controller.theme;
                text: qsTr("Actions…")
                visible: root.controller.commands.length > 0
                onClicked: actions.open()
                Menu {
                    id: actions
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
            ThemeButton { values: root.controller.theme;
                text: qsTr("Copy")
                enabled: !root.controller.busy && !root.controller.history.filtering && root.controller.history.selectedRow >= 0
                onClicked: root.controller.activate(false)
            }
            ThemeButton { values: root.controller.theme;
                text: root.controller.busy ? qsTr("Working…") : root.controller.enterLabel + " ↵"
                enabled: !root.controller.busy && !root.controller.history.filtering && root.controller.history.selectedRow >= 0
                onClicked: root.controller.activate()
            }
        }
    }
}
