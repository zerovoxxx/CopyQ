// SPDX-License-Identifier: GPL-3.0-or-later
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QClip

Pane {
    id: root
    required property ClipboardCommandsWindow controller
    property string section: "General"
    property var selectedRows: []
    width: 1100; height: 760; padding: 0
    font: theme.textFont
    palette.window: theme.background; palette.windowText: theme.foreground
    palette.base: theme.background; palette.text: theme.foreground
    palette.button: theme.alternate; palette.buttonText: theme.foreground
    palette.highlight: theme.highlight; palette.highlightedText: theme.highlightedText
    background: GlassBackground { values: root.controller.theme; controller: root.controller }
    Theme { id: theme; values: root.controller.theme }
    Keys.onEscapePressed: root.controller.cancel()
    Component.onCompleted: selectedRows = controller.currentIndex < 0 ? [] : [controller.currentIndex]

    function select(row, toggle) {
        let rows = toggle ? selectedRows.slice() : []
        const position = rows.indexOf(row)
        if (position >= 0) rows.splice(position, 1)
        else rows.push(row)
        selectedRows = rows
        controller.select(row)
    }
    Connections {
        target: root.controller as QtObject
        function onFieldsChanged() {
            if (root.selectedRows.indexOf(root.controller.currentIndex) < 0)
                root.selectedRows = root.controller.currentIndex < 0 ? [] : [root.controller.currentIndex]
        }
    }

    ColumnLayout {
        anchors.fill: parent; anchors.margins: theme.margin; spacing: theme.spacing
        RowLayout {
            Layout.fillWidth: true
            Label { text: qsTr("Commands"); font.pixelSize: 18; font.weight: Font.DemiBold }
            Label { text: qsTr("Unsaved changes"); visible: root.controller.modified; color: theme.muted; font.pixelSize: 11 }
            Item { Layout.fillWidth: true }
        }
        RowLayout {
            Layout.fillWidth: true
            spacing: theme.spacing
            ThemeButton { values: root.controller.theme; iconName: "plus"; text: qsTr("New"); onClicked: root.controller.create() }
            ThemeComboBox { values: root.controller.theme; id: templates; model: root.controller.templates; Layout.preferredWidth: 220 }
            ThemeButton { values: root.controller.theme; text: qsTr("Add template"); enabled: templates.currentIndex >= 0; onClicked: root.controller.addTemplate(templates.currentIndex) }
            Item { Layout.fillWidth: true }
            ThemeButton { values: root.controller.theme; text: qsTr("Import…"); onClicked: root.controller.importFile() }
            ThemeButton { values: root.controller.theme; text: qsTr("Export…"); enabled: root.selectedRows.length > 0; onClicked: root.controller.exportFile(root.selectedRows) }
            ThemeButton { values: root.controller.theme; text: qsTr("Copy"); enabled: root.selectedRows.length > 0; onClicked: root.controller.copy(root.selectedRows) }
            ThemeButton { values: root.controller.theme; text: qsTr("Paste"); onClicked: root.controller.paste() }
        }
        RowLayout {
            Layout.fillWidth: true; Layout.fillHeight: true
            spacing: theme.spacing
            Pane {
                Layout.preferredWidth: 240; Layout.maximumWidth: 280; Layout.fillHeight: true; padding: 8
                background: Rectangle { radius: theme.radius + 2; color: theme.hover }
                ColumnLayout {
                anchors.fill: parent; spacing: theme.spacing
                ThemeTextField { values: root.controller.theme; id: search; iconName: "search"; Layout.fillWidth: true; placeholderText: qsTr("Filter commands") }
                ListView {
                    Layout.fillWidth: true; Layout.fillHeight: true; clip: true
                    model: root.controller.commands
                    delegate: ThemeDelegate { values: root.controller.theme;
                        id: commandRow
                        required property int index
                        required property var modelData
                        width: ListView.view.width
                        height: visible ? implicitHeight : 0
                        visible: search.text.length === 0 || modelData.name.toLowerCase().indexOf(search.text.toLowerCase()) >= 0
                        text: (modelData.enabled ? "" : qsTr("Disabled · ")) + (modelData.name || qsTr("Unnamed command"))
                        highlighted: root.selectedRows.indexOf(index) >= 0
                        MouseArea {
                            anchors.fill: parent
                            onClicked: mouse => root.select(commandRow.index, !!(mouse.modifiers & (Qt.ControlModifier | Qt.MetaModifier)))
                        }
                    }
                    ScrollBar.vertical: ThemeScrollBar { values: root.controller.theme; policy: theme.showScrollbars ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff }
                }
                RowLayout {
                    ThemeButton { values: root.controller.theme; quiet: true; iconName: "arrow-up"; enabled: root.controller.currentIndex > 0; onClicked: root.controller.move(root.controller.currentIndex, -1); Accessible.name: qsTr("Move command up") }
                    ThemeButton { values: root.controller.theme; quiet: true; iconName: "arrow-down"; enabled: root.controller.currentIndex >= 0; onClicked: root.controller.move(root.controller.currentIndex, 1); Accessible.name: qsTr("Move command down") }
                    ThemeButton { values: root.controller.theme; quiet: true; destructive: true; text: qsTr("Remove"); enabled: root.selectedRows.length > 0; onClicked: remove.open() }
                }
                }
            }
            ColumnLayout {
                Layout.fillWidth: true; Layout.fillHeight: true
                spacing: theme.spacing
                RowLayout {
                    spacing: 4
                    Repeater {
                        model: ["General", "Conditions", "Command", "Behavior", "Shortcuts"]
                        ThemeButton { values: root.controller.theme; required property string modelData; text: modelData; selected: root.section === modelData; quiet: true; onClicked: root.section = modelData }
                    }
                }
                ScrollView {
                    id: commandFields
                    objectName: "commands_fields_scroll"
                    Layout.fillWidth: true; Layout.fillHeight: true; clip: true
                    ScrollBar.vertical: ThemeScrollBar { values: root.controller.theme }
                    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                    background: Rectangle { radius: theme.radius + 2; color: theme.surface; border.color: theme.line }
                    ColumnLayout {
                        width: commandFields.availableWidth; spacing: 0
                        Repeater {
                            model: root.controller.fields
                            Pane {
                                id: field
                                required property var modelData
                                Layout.fillWidth: true; visible: modelData.section === root.section
                                padding: 12
                                background: Item {}
                                ColumnLayout {
                                anchors.fill: parent; spacing: 6
                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: theme.spacing
                                    Label { text: field.modelData.label; font.weight: Font.Medium; Layout.fillWidth: true; wrapMode: Text.Wrap }
                                    ThemeCheckBox {
                                        values: root.controller.theme
                                        visible: field.modelData.kind === "bool"
                                        checked: !!field.modelData.value
                                        onClicked: root.controller.setField(field.modelData.name, checked)
                                        Accessible.name: field.modelData.label
                                    }
                                }
                                ThemeTextField { values: root.controller.theme;
                                    visible: field.modelData.kind === "text" || field.modelData.kind === "regex"
                                    Layout.fillWidth: true; objectName: "command_" + field.modelData.name
                                    text: String(field.modelData.value)
                                    onEditingFinished: root.controller.setField(field.modelData.name, text)
                                    Accessible.name: field.modelData.label
                                }
                                ThemeTextArea { values: root.controller.theme;
                                    objectName: "command_" + field.modelData.name
                                    visible: field.modelData.kind === "code" || field.modelData.kind === "list"
                                    Layout.fillWidth: true; wrapMode: TextEdit.Wrap; font: theme.editorFont
                                    text: field.modelData.kind === "list" ? field.modelData.value.join("\n") : String(field.modelData.value)
                                    onActiveFocusChanged: if (!activeFocus && visible) root.controller.setField(field.modelData.name, text)
                                    Accessible.name: field.modelData.label
                                }
                                ThemeButton { values: root.controller.theme; visible: field.modelData.kind === "code"; text: qsTr("Open script editor…"); onClicked: root.controller.editCode(field.modelData.name) }
                                Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; Layout.topMargin: 6; color: theme.line }
                                }
                            }
                        }
                    }
                }
            }
        }
        Label { text: root.controller.error; visible: text.length > 0; color: theme.danger; wrapMode: Text.Wrap; Layout.fillWidth: true }
        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: theme.line }
        RowLayout {
            Layout.fillWidth: true
            spacing: theme.spacing
            Label { text: qsTr("Ctrl/⌘ click selects multiple commands."); color: theme.muted; font.pixelSize: 11 }
            Item { Layout.fillWidth: true }
            ThemeButton { values: root.controller.theme; text: qsTr("Cancel"); onClicked: root.controller.cancel() }
            ThemeButton { values: root.controller.theme; text: qsTr("Apply"); onClicked: root.controller.apply(false) }
            ThemeButton { values: root.controller.theme; primary: true; text: qsTr("Save and close"); onClicked: root.controller.apply(true) }
        }
    }
    ThemeDialog { values: root.controller.theme;
        id: remove; anchors.centerIn: parent; modal: true
        title: qsTr("Remove selected commands?"); standardButtons: Dialog.Yes | Dialog.No
        onAccepted: { root.controller.remove(root.selectedRows); root.selectedRows = root.controller.currentIndex < 0 ? [] : [root.controller.currentIndex] }
    }
}
