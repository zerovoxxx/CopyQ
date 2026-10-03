// SPDX-License-Identifier: GPL-3.0-or-later
pragma ComponentBehavior: Bound
import QtQuick
import QtQml
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QClip

Pane {
    id: root
    required property ClipboardPaletteWindow controller
    readonly property var preview: controller.history.preview
    readonly property bool canActivate: !controller.busy && !controller.history.filtering && controller.history.selectedRow >= 0
    width: 740
    height: 440
    padding: 0
    font: theme.textFont
    palette.windowText: theme.foreground
    palette.text: theme.foreground
    palette.highlight: theme.highlight
    palette.highlightedText: theme.highlightedText
    background: GlassBackground { values: root.controller.theme; controller: root.controller; cornerRadius: 12 }
    Theme { id: theme; values: root.controller.theme }

    Connections {
        target: root.controller as QtObject
        function onOpened() { search.text = ""; search.forceActiveFocus() }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: theme.margin
        spacing: theme.spacing
        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            Rectangle {
                Layout.preferredWidth: 24; Layout.preferredHeight: 24; radius: 7; color: theme.highlight
                Label { anchors.centerIn: parent; text: "Q"; color: "#ffffff"; font.bold: true; font.pixelSize: 15 }
            }
            Label { text: "QClip"; font.bold: true; font.pixelSize: 14; color: theme.foreground }
            Item { Layout.fillWidth: true }
            ThemeComboBox {
                id: sources
                values: root.controller.theme
                Layout.preferredWidth: Math.min(148, root.width / 4)
                model: root.controller.tabs
                currentIndex: root.controller.currentTabIndex
                enabled: !root.controller.busy
                onActivated: root.controller.changeSource(currentText)
                Accessible.name: qsTr("Collection")
            }
            ThemeButton {
                values: root.controller.theme; quiet: true; text: qsTr("Manage"); objectName: "palette_manage"
                onClicked: root.controller.showManagement()
            }
            ThemeButton {
                values: root.controller.theme; quiet: true; text: "•••"; Layout.preferredWidth: theme.controlHeight
                Accessible.name: qsTr("More options")
                onClicked: options.open()
                ThemeMenu {
                    id: options
                    values: root.controller.theme
                    ThemeMenuItem { values: root.controller.theme; text: qsTr("Snippets…"); onTriggered: root.controller.showSnippets() }
                    ThemeMenuItem { values: root.controller.theme; text: qsTr("Save snippet"); enabled: root.canActivate; onTriggered: root.controller.saveSnippet() }
                    ThemeMenuItem { values: root.controller.theme; text: qsTr("Plugin settings…"); onTriggered: root.controller.showPluginSettings() }
                }
            }
            ThemeButton { values: root.controller.theme; quiet: true; text: "×"; Layout.preferredWidth: theme.controlHeight; onClicked: root.controller.cancel(); Accessible.name: qsTr("Close") }
        }

        ThemeTextField {
            id: search
            objectName: "palette_search"
            values: root.controller.theme
            Layout.fillWidth: true
            implicitHeight: 36
            leftPadding: 34
            font.pixelSize: 14
            placeholderText: qsTr("Search clipboard history…")
            enabled: !root.controller.busy
            onTextEdited: root.controller.history.query = text
            Accessible.name: qsTr("Search clipboard history")
            Item {
                x: 12; y: (parent.height - 16) / 2; width: 16; height: 16
                Rectangle { width: 12; height: 12; radius: 6; color: "transparent"; border.width: 1.6; border.color: theme.muted }
                Rectangle { x: 11; y: 10; width: 7; height: 1.6; rotation: 45; color: theme.muted }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: theme.spacing
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 390
                spacing: 6
                RowLayout {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 16
                    Layout.leftMargin: 8; Layout.rightMargin: 8
                    Label { text: qsTr("HISTORY"); font.pixelSize: 10; font.letterSpacing: 1.1; color: theme.muted }
                    Item { Layout.fillWidth: true }
                    Label { text: String(root.controller.history.count); font.pixelSize: 11; color: theme.muted }
                }
                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
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
                        ScrollBar.vertical: ScrollBar { policy: theme.showScrollbars ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff }
                        delegate: Rectangle {
                            id: row
                            required property int index
                            required property string summary
                            required property string itemType
                            required property bool pinned
                            readonly property bool selected: index === root.controller.history.selectedRow
                            Component.onCompleted: root.controller.history.requestDisplay(index)
                            onSummaryChanged: root.controller.history.requestDisplay(index)
                            Connections {
                                target: root.controller.history
                                function onDisplaysInvalidated() { root.controller.history.requestDisplay(row.index) }
                            }
                            width: ListView.view.width
                            height: theme.rowHeight
                            radius: theme.radius
                            color: selected ? theme.selection : rowMouse.containsMouse ? theme.hover : "transparent"
                            border.color: selected ? Qt.rgba(theme.highlight.r, theme.highlight.g, theme.highlight.b, 0.25) : "transparent"
                            Behavior on color { ColorAnimation { duration: 110 } }
                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 8; anchors.rightMargin: 8
                                anchors.topMargin: 6; anchors.bottomMargin: 6
                                spacing: 10
                                Rectangle {
                                    Layout.preferredWidth: 28; Layout.preferredHeight: 28
                                    radius: 7
                                    color: row.selected ? Qt.rgba(theme.highlight.r, theme.highlight.g, theme.highlight.b, 0.13) : theme.hover
                                    Label {
                                        anchors.centerIn: parent
                                        text: row.itemType === "Image" ? "▧" : row.itemType === "Files / links" ? "↗" : "T"
                                        font.pixelSize: 14; font.bold: true
                                        color: row.selected ? theme.highlight : theme.muted
                                    }
                                }
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 2
                                    Label { Layout.fillWidth: true; text: row.summary; elide: Text.ElideRight; color: theme.foreground; textFormat: Text.PlainText }
                                    Label { Layout.fillWidth: true; text: (row.pinned ? qsTr("Pinned · ") : "") + row.itemType; elide: Text.ElideRight; color: theme.muted; font.pixelSize: 11 }
                                }
                                Label { visible: theme.showNumber; text: String(row.index + 1); color: theme.muted; font.pixelSize: 11 }
                            }
                            MouseArea {
                                id: rowMouse
                                hoverEnabled: true
                                anchors.fill: parent
                                enabled: !root.controller.busy
                                onClicked: root.controller.history.selectRow(row.index)
                                onDoubleClicked: root.controller.activate()
                            }
                            Accessible.name: summary
                            Accessible.role: Accessible.ListItem
                            Accessible.selected: selected
                        }
                    }
                    Column {
                        anchors.centerIn: parent
                        spacing: 10
                        width: parent.width - 24
                        visible: root.controller.history.count === 0
                        Label { anchors.horizontalCenter: parent.horizontalCenter; text: "⌕"; font.pixelSize: 32; color: theme.muted }
                        Label {
                            width: parent.width
                            horizontalAlignment: Text.AlignHCenter
                            wrapMode: Text.Wrap
                            text: root.controller.history.filtering ? qsTr("Searching…")
                                : root.controller.history.sourceCount === 0 ? qsTr("Clipboard history is empty") : qsTr("No matches")
                            color: theme.muted
                        }
                    }
                }
            }
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 284
                visible: root.width >= 640
                spacing: 6
                RowLayout {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 16
                    Layout.leftMargin: 12; Layout.rightMargin: 12
                    Label { text: qsTr("PREVIEW"); font.pixelSize: 10; font.letterSpacing: 1.1; color: theme.muted }
                    Item { Layout.fillWidth: true }
                    Label { text: root.preview.type || ""; font.pixelSize: 11; color: theme.muted }
                }
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    radius: theme.radius
                    color: theme.surface
                    border.color: theme.line
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 6
                        ClipboardItemPreview {
                            objectName: "palette_preview"
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            history: root.controller.history
                            theme: root.controller.theme
                            Accessible.name: root.preview.text || qsTr("Item preview")
                        }
                        Label {
                            Layout.fillWidth: true; visible: !!root.preview.application
                            text: root.preview.application || ""; elide: Text.ElideRight; color: theme.muted; font.pixelSize: 11
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 6
                            ThemeButton { values: root.controller.theme; quiet: true; text: qsTr("Open"); visible: (root.preview.urls || []).length > 0; onClicked: root.controller.openUrl(root.preview.urls[0]) }
                            ThemeButton { values: root.controller.theme; quiet: true; text: qsTr("Preview file"); visible: !!root.preview.filePath; onClicked: root.controller.previewFile() }
                            Item { Layout.fillWidth: true }
                            ThemeButton {
                                values: root.controller.theme; quiet: true; text: qsTr("Edit…")
                                enabled: !!root.preview.editable && !root.controller.busy
                                onClicked: root.controller.editItem()
                            }
                        }
                    }
                }
            }
        }
        Label {
            Layout.fillWidth: true
            visible: root.controller.error.length > 0
            text: root.controller.error; wrapMode: Text.Wrap; color: theme.foreground
        }
        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: theme.line }
        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            Label {
                Layout.fillWidth: true; elide: Text.ElideRight
                text: qsTr("↑↓ Select · Esc Close"); color: theme.muted; font.pixelSize: 11
                ToolTip.visible: shortcutHover.hovered
                ToolTip.text: Qt.platform.os === "osx" ? qsTr("⌘↵ Copy · ⇧↵ Plain text · ⌥↵ Paste") : qsTr("Ctrl+Enter Copy · Shift+Enter Plain text · Alt+Enter Paste")
                HoverHandler { id: shortcutHover }
            }
            ThemeButton {
                values: root.controller.theme; quiet: true; text: qsTr("Actions…")
                visible: root.controller.commands.length > 0
                onClicked: actions.open()
                ThemeMenu {
                    id: actions
                    values: root.controller.theme
                    Repeater {
                        model: root.controller.commands
                        ThemeMenuItem {
                            values: root.controller.theme
                            required property var modelData
                            required property int index
                            text: modelData.name + (modelData.shortcut ? "  " + modelData.shortcut : "")
                            enabled: modelData.enabled
                            onTriggered: root.controller.triggerCommand(index)
                        }
                    }
                }
            }
            ThemeButton { values: root.controller.theme; quiet: true; text: qsTr("Copy"); enabled: root.canActivate; onClicked: root.controller.activate(false) }
            ThemeButton {
                values: root.controller.theme; primary: true
                text: root.controller.busy ? qsTr("Working…") : root.controller.enterLabel + "  ↵"
                enabled: root.canActivate
                onClicked: root.controller.activate()
            }
        }
    }
}
