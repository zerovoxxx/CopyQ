// SPDX-License-Identifier: GPL-3.0-or-later
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QClip

Pane {
    id: root
    required property ClipboardSettingsWindow controller
    property string section: "General"
    width: 980; height: 720; padding: 0
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
    Keys.onEscapePressed: root.controller.cancel()
    Keys.onReturnPressed: root.controller.apply(true)
    Keys.onEnterPressed: root.controller.apply(true)

    ColumnLayout {
        anchors.fill: parent; anchors.margins: theme.margin; spacing: theme.spacing
        Label { text: qsTr("Settings"); font.pixelSize: 24; font.bold: true }
        Label { text: qsTr("Apply saves your changes. Cancel discards changes since the last Apply."); wrapMode: Text.Wrap; Layout.fillWidth: true }
        RowLayout {
            Layout.fillWidth: true; Layout.fillHeight: true
            ColumnLayout {
                Layout.preferredWidth: 165; Layout.fillHeight: true
                Repeater {
                    model: ["General", "History", "Layout", "Tray", "Notifications", "Plugins", "Advanced"]
                    ItemDelegate {
                        required property string modelData
                        text: modelData; Layout.fillWidth: true; highlighted: root.section === modelData
                        onClicked: root.section = modelData
                    }
                }
                Item { Layout.fillHeight: true }
                ThemeButton { values: root.controller.theme; text: qsTr("Appearance…"); Layout.fillWidth: true; onClicked: root.controller.openPage("Appearance") }
                ThemeButton { values: root.controller.theme; text: qsTr("Shortcuts…"); Layout.fillWidth: true; onClicked: root.controller.openPage("Shortcuts") }
                ThemeButton { values: root.controller.theme; text: qsTr("Collections…"); Layout.fillWidth: true; onClicked: root.controller.openPage("Tabs") }
            }
            Rectangle { Layout.fillHeight: true; Layout.preferredWidth: 1; color: theme.alternate }
            ScrollView {
                Layout.fillWidth: true; Layout.fillHeight: true; clip: true
                ColumnLayout {
                    width: parent.width; spacing: theme.spacing
                    RowLayout {
                        visible: root.section === "General"
                        Label { text: qsTr("Language") }
                        ComboBox {
                            model: root.controller.languages; textRole: "name"; valueRole: "id"
                            onActivated: root.controller.setLanguage(currentValue)
                            Component.onCompleted: currentIndex = indexOfValue(root.controller.language)
                        }
                        ThemeButton { values: root.controller.theme; text: qsTr("Encryption password…"); onClicked: root.controller.changeEncryptionPassword() }
                        Label { text: qsTr("Password changes apply immediately."); opacity: 0.7; wrapMode: Text.Wrap; Layout.fillWidth: true }
                    }
                    Repeater {
                        model: root.controller.fields
                        ColumnLayout {
                            id: field
                            required property var modelData
                            Layout.fillWidth: true
                            visible: modelData.section === root.section
                            enabled: modelData.available
                            Label { text: field.modelData.label; font.bold: true; wrapMode: Text.Wrap; Layout.fillWidth: true }
                            Label { text: field.modelData.description; wrapMode: Text.Wrap; opacity: 0.7; Layout.fillWidth: true; visible: text.length > 0 }
                            CheckBox { visible: field.modelData.kind === "bool"; checked: !!field.modelData.value; text: qsTr("Enabled"); onClicked: root.controller.setValue(field.modelData.name, checked) }
                            ComboBox { visible: field.modelData.choices !== undefined; model: field.modelData.choices || []; currentIndex: Number(field.modelData.value); onActivated: root.controller.setValue(field.modelData.name, currentIndex) }
                            TextField {
                                visible: field.modelData.kind !== "bool" && field.modelData.kind !== "list" && field.modelData.choices === undefined
                                Layout.fillWidth: true; objectName: "settings_" + field.modelData.name
                                text: String(field.modelData.value)
                                onEditingFinished: if (text !== String(field.modelData.value)) root.controller.setValue(field.modelData.name, text)
                                Accessible.name: field.modelData.label
                            }
                            TextArea {
                                visible: field.modelData.kind === "list"; Layout.fillWidth: true
                                text: field.modelData.kind === "list" ? field.modelData.value.join("\n") : ""
                                onActiveFocusChanged: if (!activeFocus && visible) root.controller.setValue(field.modelData.name, text)
                                Accessible.name: field.modelData.label
                            }
                            Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: theme.alternate }
                        }
                    }
                    Repeater {
                        model: root.controller.plugins
                        RowLayout {
                            id: plugin
                            required property var modelData
                            visible: root.section === "Plugins"; Layout.fillWidth: true
                            CheckBox { text: plugin.modelData.name; checked: plugin.modelData.enabled; onClicked: root.controller.setPluginEnabled(plugin.modelData.id, checked) }
                            Item { Layout.fillWidth: true }
                            ThemeButton { values: root.controller.theme; text: qsTr("Configure…"); onClicked: root.controller.openPage(plugin.modelData.id) }
                            ThemeButton { values: root.controller.theme; text: "↑"; onClicked: root.controller.movePlugin(plugin.modelData.id, -1); Accessible.name: qsTr("Increase plugin priority") }
                            ThemeButton { values: root.controller.theme; text: "↓"; onClicked: root.controller.movePlugin(plugin.modelData.id, 1); Accessible.name: qsTr("Decrease plugin priority") }
                        }
                    }
                }
            }
        }
        Label { text: root.controller.error; visible: text.length > 0; color: "#df7b6a"; Layout.fillWidth: true; wrapMode: Text.Wrap }
        RowLayout {
            Layout.fillWidth: true
            ThemeButton { values: root.controller.theme; text: qsTr("Restore defaults…"); onClicked: reset.open() }
            Item { Layout.fillWidth: true }
            ThemeButton { values: root.controller.theme; text: qsTr("Cancel"); onClicked: root.controller.cancel() }
            ThemeButton { values: root.controller.theme; text: qsTr("Apply"); onClicked: root.controller.apply(false) }
            ThemeButton { values: root.controller.theme; text: qsTr("Save and close"); onClicked: root.controller.apply(true) }
        }
    }
    Dialog {
        id: reset; anchors.centerIn: parent; title: qsTr("Restore option defaults?")
        modal: true; standardButtons: Dialog.Yes | Dialog.No
        onAccepted: root.controller.resetDefaults()
        Label { text: qsTr("The defaults are saved only when you apply your changes.") }
    }
}
