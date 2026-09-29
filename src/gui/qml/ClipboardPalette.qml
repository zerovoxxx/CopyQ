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
            Button { text: "×"; onClicked: root.controller.cancel(); Accessible.name: qsTr("Close") }
            Button { text: qsTr("Manage…"); objectName: "palette_manage"; onClicked: root.controller.showManagement() }
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
                Image {
                    id: image
                    objectName: "palette_preview_image"
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    visible: root.preview.type === "Image"
                    source: visible ? root.preview.image : ""
                    fillMode: Image.PreserveAspectFit
                    cache: false
                }
                Label {
                    visible: image.visible && image.status === Image.Error
                    text: qsTr("Unable to load image preview")
                    color: colors.windowText
                }
                ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    visible: !image.visible
                    TextArea {
                        id: previewText
                        objectName: "palette_preview"
                        readOnly: true
                        selectByMouse: true
                        wrapMode: TextEdit.Wrap
                        textFormat: root.preview.html ? TextEdit.RichText : TextEdit.PlainText
                        text: root.preview.html || root.preview.text || (root.preview.urls || []).join("\n")
                        onLinkActivated: function(link) { root.controller.openUrl(link) }
                    }
                }
                RowLayout {
                    visible: (root.preview.urls || []).length > 0
                    Button { text: qsTr("Open"); onClicked: root.controller.openUrl(root.preview.urls[0]) }
                    Button { text: qsTr("Preview file"); visible: !!root.preview.filePath; onClicked: root.controller.previewFile() }
                }
                RowLayout {
                    Button { text: qsTr("Edit…"); enabled: !!root.preview.editable && !root.controller.busy; onClicked: root.controller.editItem() }
                    Button { text: qsTr("Plugin settings…"); onClicked: root.controller.showPluginSettings() }
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
            Button {
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
            Button {
                text: qsTr("Copy")
                enabled: !root.controller.busy && !root.controller.history.filtering && root.controller.history.selectedRow >= 0
                onClicked: root.controller.activate(false)
            }
            Button {
                text: root.controller.busy ? qsTr("Working…") : root.controller.enterLabel + " ↵"
                enabled: !root.controller.busy && !root.controller.history.filtering && root.controller.history.selectedRow >= 0
                onClicked: root.controller.activate()
            }
        }
    }
}
