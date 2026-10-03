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
    background: GlassBackground { values: root.controller.theme; controller: root.controller }
    Theme { id: theme; values: root.controller.theme }
    Keys.onEscapePressed: root.controller.cancel()
    Keys.onReturnPressed: root.controller.apply(true)
    Keys.onEnterPressed: root.controller.apply(true)

    ColumnLayout {
        anchors.fill: parent; anchors.margins: theme.margin; spacing: theme.spacing
        Label { text: qsTr("Settings"); font.pixelSize: 18; font.weight: Font.DemiBold }
        Label { text: qsTr("General preferences, history and shortcuts"); color: theme.muted; font.pixelSize: 11; Layout.fillWidth: true }
        RowLayout {
            Layout.fillWidth: true; Layout.fillHeight: true
            spacing: theme.spacing
            Pane {
                Layout.preferredWidth: 170; Layout.fillHeight: true; padding: 8
                background: Rectangle { radius: theme.radius + 2; color: theme.hover }
                ColumnLayout {
                anchors.fill: parent; spacing: 2
                Repeater {
                    model: ["General", "History", "Layout", "Tray", "Notifications", "Plugins", "Advanced"]
                    ThemeDelegate { values: root.controller.theme;
                        required property string modelData
                        text: modelData; Layout.fillWidth: true; highlighted: root.section === modelData
                        onClicked: root.section = modelData
                    }
                }
                Item { Layout.fillHeight: true }
                Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: theme.line }
                ThemeDelegate { values: root.controller.theme; text: qsTr("Appearance…"); Layout.fillWidth: true; onClicked: root.controller.openPage("Appearance") }
                ThemeDelegate { values: root.controller.theme; text: qsTr("Shortcuts…"); Layout.fillWidth: true; onClicked: root.controller.openPage("Shortcuts") }
                ThemeDelegate { values: root.controller.theme; text: qsTr("Collections…"); Layout.fillWidth: true; onClicked: root.controller.openPage("Tabs") }
                }
            }
            ScrollView {
                objectName: "settings_fields_scroll"
                id: settingsFields
                Layout.fillWidth: true; Layout.fillHeight: true; clip: true
                ScrollBar.vertical: ThemeScrollBar { values: root.controller.theme }
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                background: Rectangle { radius: theme.radius + 2; color: theme.surface; border.color: theme.line }
                ColumnLayout {
                    width: settingsFields.availableWidth; spacing: 0
                    Label { text: root.section; font.weight: Font.DemiBold; Layout.margins: 12 }
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.leftMargin: 12; Layout.rightMargin: 12; Layout.bottomMargin: 8
                        visible: root.section === "General"
                        spacing: theme.spacing
                        Label { text: qsTr("Language") }
                        ThemeComboBox { values: root.controller.theme;
                            Layout.preferredWidth: 200
                            model: root.controller.languages; textRole: "name"; valueRole: "id"
                            onActivated: root.controller.setLanguage(currentValue)
                            Component.onCompleted: currentIndex = indexOfValue(root.controller.language)
                        }
                        Item { Layout.fillWidth: true }
                    }
                    RowLayout {
                        visible: root.section === "General"; Layout.fillWidth: true; spacing: theme.spacing
                        Layout.leftMargin: 12; Layout.rightMargin: 12; Layout.bottomMargin: 8
                        ThemeButton { values: root.controller.theme; text: qsTr("Encryption password…"); onClicked: root.controller.changeEncryptionPassword() }
                        Label { text: qsTr("Password changes apply immediately."); color: theme.muted; font.pixelSize: 11; wrapMode: Text.Wrap; Layout.fillWidth: true }
                    }
                    Repeater {
                        model: root.controller.fields
                        Pane {
                            id: field
                            required property var modelData
                            Layout.fillWidth: true
                            padding: 12
                            background: Item {}
                            visible: modelData.section === root.section
                            enabled: modelData.available
                            ColumnLayout {
                            anchors.fill: parent; spacing: 6
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: theme.spacing
                                Label { text: field.modelData.label; font.weight: Font.Medium; wrapMode: Text.Wrap; Layout.fillWidth: true }
                                ThemeCheckBox {
                                    values: root.controller.theme
                                    visible: field.modelData.kind === "bool"
                                    checked: !!field.modelData.value
                                    onClicked: root.controller.setValue(field.modelData.name, checked)
                                    Accessible.name: field.modelData.label
                                }
                            }
                            Label { text: field.modelData.description; wrapMode: Text.Wrap; color: theme.muted; font.pixelSize: 11; Layout.fillWidth: true; visible: text.length > 0 }
                            ThemeComboBox { values: root.controller.theme; Layout.fillWidth: true; visible: field.modelData.choices !== undefined; model: field.modelData.choices || []; currentIndex: Number(field.modelData.value); onActivated: root.controller.setValue(field.modelData.name, currentIndex) }
                            ThemeTextField { values: root.controller.theme;
                                visible: field.modelData.kind !== "bool" && field.modelData.kind !== "list" && field.modelData.choices === undefined
                                Layout.fillWidth: true; objectName: "settings_" + field.modelData.name
                                text: String(field.modelData.value)
                                onEditingFinished: if (text !== String(field.modelData.value)) root.controller.setValue(field.modelData.name, text)
                                Accessible.name: field.modelData.label
                            }
                            ThemeTextArea { values: root.controller.theme;
                                visible: field.modelData.kind === "list"; Layout.fillWidth: true
                                text: field.modelData.kind === "list" ? field.modelData.value.join("\n") : ""
                                onActiveFocusChanged: if (!activeFocus && visible) root.controller.setValue(field.modelData.name, text)
                                Accessible.name: field.modelData.label
                            }
                            Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; Layout.topMargin: 6; color: theme.line }
                            }
                        }
                    }
                    Repeater {
                        model: root.controller.plugins
                        RowLayout {
                            id: plugin
                            spacing: 6
                            required property var modelData
                            visible: root.section === "Plugins"; Layout.fillWidth: true
                            Layout.margins: 12
                            ThemeCheckBox { values: root.controller.theme; text: plugin.modelData.name; checked: plugin.modelData.enabled; onClicked: root.controller.setPluginEnabled(plugin.modelData.id, checked) }
                            Item { Layout.fillWidth: true }
                            ThemeButton { values: root.controller.theme; text: qsTr("Configure…"); onClicked: root.controller.openPage(plugin.modelData.id) }
                            ThemeButton { values: root.controller.theme; quiet: true; iconName: "arrow-up"; onClicked: root.controller.movePlugin(plugin.modelData.id, -1); Accessible.name: qsTr("Increase plugin priority") }
                            ThemeButton { values: root.controller.theme; quiet: true; iconName: "arrow-down"; onClicked: root.controller.movePlugin(plugin.modelData.id, 1); Accessible.name: qsTr("Decrease plugin priority") }
                        }
                    }
                }
            }
        }
        Label { text: root.controller.error; visible: text.length > 0; color: theme.danger; Layout.fillWidth: true; wrapMode: Text.Wrap }
        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: theme.line }
        RowLayout {
            Layout.fillWidth: true
            spacing: theme.spacing
            ThemeButton { values: root.controller.theme; quiet: true; text: qsTr("Restore defaults…"); onClicked: reset.open() }
            Item { Layout.fillWidth: true }
            ThemeButton { values: root.controller.theme; text: qsTr("Cancel"); onClicked: root.controller.cancel() }
            ThemeButton { values: root.controller.theme; text: qsTr("Apply"); onClicked: root.controller.apply(false) }
            ThemeButton { values: root.controller.theme; primary: true; text: qsTr("Save and close"); onClicked: root.controller.apply(true) }
        }
    }
    ThemeDialog { values: root.controller.theme;
        id: reset; anchors.centerIn: parent; title: qsTr("Restore option defaults?")
        modal: true; standardButtons: Dialog.Yes | Dialog.No
        onAccepted: root.controller.resetDefaults()
        Label { text: qsTr("The defaults are saved only when you apply your changes.") }
    }
}
