// SPDX-License-Identifier: GPL-3.0-or-later
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QClip

Pane {
    id: root
    objectName: "snippets_root"
    required property ClipboardSnippetsWindow controller
    readonly property var selected: controller.selected
    readonly property var settings: controller.settings
    property string editingId: ""
    width: 1040
    height: 720
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
    function fillFields() {
        title.text = root.selected.title || ""
        keyword.text = root.selected.keyword || ""
        command.text = root.selected.command || ""
        expansion.checked = root.selected.enabled || false
        collection.currentIndex = -1
        for (let i = 0; i < root.controller.collections.length; ++i)
            if (root.controller.collections[i].id === root.selected.collection) collection.currentIndex = i
    }
    Connections {
        target: root.controller
        function onChanged() { if ((root.selected.id || "") !== root.editingId) { root.fillFields(); root.editingId = root.selected.id || "" } }
        function onSnippetSaved() { root.fillFields() }
    }
    Component.onCompleted: { root.fillFields(); root.editingId = root.selected.id || "" }
    ColumnLayout {
        anchors.fill: parent
        spacing: theme.spacing
        RowLayout {
            Layout.fillWidth: true
            Label { text: qsTr("Snippets"); font.bold: true; font.pixelSize: 22 }
            Item { Layout.fillWidth: true }
            ThemeButton { values: root.controller.theme; text: qsTr("Import collection…"); onClicked: root.controller.importFile() }
            ThemeButton { values: root.controller.theme; text: qsTr("Export collection…"); enabled: root.controller.collectionId !== ""; onClicked: root.controller.exportFile() }
            ThemeButton { values: root.controller.theme; text: qsTr("Import CopyQ profile…"); onClicked: root.controller.importCopyQ() }
        }
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            ColumnLayout {
                Layout.preferredWidth: 200
                Layout.fillHeight: true
                ThemeButton { values: root.controller.theme; text: qsTr("All snippets"); onClicked: root.controller.collectionId = "" }
                ListView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: root.controller.collections
                    delegate: ItemDelegate {
                        required property var modelData
                        width: ListView.view.width
                        text: modelData.name
                        highlighted: modelData.id === root.controller.collectionId
                        onClicked: root.controller.collectionId = modelData.id
                        onDoubleClicked: {
                            collectionDialog.identity = modelData.id
                            collectionName.text = modelData.name
                            prefix.text = modelData.prefix || ""
                            suffix.text = modelData.suffix || ""
                            groupEnabled.checked = modelData.enabled
                            collectionDialog.open()
                        }
                    }
                }
                ThemeButton { values: root.controller.theme; text: qsTr("New collection…"); onClicked: {
                    collectionDialog.identity = ""; collectionName.text = ""; prefix.text = ""; suffix.text = ""; groupEnabled.checked = true; collectionDialog.open()
                } }
                ThemeButton { values: root.controller.theme; text: qsTr("Delete collection…"); enabled: root.controller.collectionId !== ""; onClicked: removeGroup.open() }
            }
            ColumnLayout {
                Layout.preferredWidth: 270
                Layout.fillHeight: true
                TextField {
                    objectName: "snippet_search"
                    Layout.fillWidth: true
                    placeholderText: qsTr("Search title or keyword…")
                    onTextEdited: root.controller.query = text
                }
                ListView {
                    id: results
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: root.controller.snippets
                    delegate: ItemDelegate {
                        required property var modelData
                        required property int index
                        width: ListView.view.width
                        text: modelData.title + "\n" + modelData.resolvedKeyword + (modelData.conflict ? " ⚠" : "")
                        highlighted: modelData.id === root.selected.id
                        onClicked: { results.currentIndex = index; root.controller.select(modelData.id) }
                        onDoubleClicked: root.controller.useSnippet(true)
                    }
                    Keys.onReturnPressed: event => { if (currentIndex >= 0) { root.controller.select(root.controller.snippets[currentIndex].id); root.controller.useSnippet(true) } event.accepted = true }
                }
                ThemeButton { values: root.controller.theme; text: qsTr("New snippet"); enabled: root.controller.collectionId !== ""; onClicked: root.controller.createSnippet() }
            }
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                enabled: Boolean(root.selected.id)
                Label { text: qsTr("Title") }
                TextField { id: title; objectName: "snippet_title"; Layout.fillWidth: true }
                Label { text: qsTr("Keyword (case sensitive)") }
                TextField { id: keyword; objectName: "snippet_keyword"; Layout.fillWidth: true }
                Label { text: (root.selected.resolvedKeyword || "") + (root.selected.conflict ? " · " + root.selected.conflict : ""); wrapMode: Text.Wrap; Layout.fillWidth: true }
                ComboBox { id: collection; Layout.fillWidth: true; model: root.controller.collections; textRole: "name"; valueRole: "id" }
                CheckBox { id: expansion; text: qsTr("Allow automatic expansion") }
                TextField { id: command; Layout.fillWidth: true; placeholderText: qsTr("Optional CopyQ command name (runs instead of automatic paste)") }
                RowLayout {
                    ThemeButton { values: root.controller.theme; text: qsTr("Save details"); onClicked: root.controller.editSnippet({ title: title.text, keyword: keyword.text, command: command.text, enabled: expansion.checked, collection: collection.currentValue }) }
                    ThemeButton { values: root.controller.theme; text: qsTr("Edit body…"); onClicked: root.controller.editBody() }
                    ThemeButton { values: root.controller.theme; text: qsTr("Delete…"); onClicked: removeSnippet.open() }
                }
                Label { text: qsTr("Preview"); font.bold: true }
                ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    TextArea { text: root.controller.richPreview || root.controller.preview; textFormat: root.controller.richPreview ? TextEdit.RichText : TextEdit.PlainText; readOnly: true; wrapMode: TextEdit.Wrap; selectByMouse: true }
                }
                RowLayout {
                    ThemeButton { values: root.controller.theme; text: qsTr("Copy"); onClicked: root.controller.useSnippet(false) }
                    ThemeButton { values: root.controller.theme; text: qsTr("Paste to original window"); onClicked: root.controller.useSnippet(true) }
                    ThemeButton { values: root.controller.theme; text: qsTr("Run command"); enabled: Boolean(root.selected.command); onClicked: root.controller.runCommand() }
                }
            }
        }
        Label { Layout.fillWidth: true; text: root.controller.error; color: theme.foreground; wrapMode: Text.Wrap; visible: text !== "" }
        RowLayout {
            CheckBox { text: qsTr("Automatically expand"); checked: root.settings.autoExpand || false; onToggled: root.controller.setSetting("autoExpand", checked) }
            CheckBox { text: qsTr("Match anywhere"); checked: root.settings.anywhere || false; onToggled: root.controller.setSetting("anywhere", checked) }
            CheckBox { text: qsTr("Restore clipboard"); checked: root.settings.restoreClipboard !== false; onToggled: root.controller.setSetting("restoreClipboard", checked) }
            CheckBox { text: qsTr("Merge double copy"); checked: root.settings.merge || false; onToggled: root.controller.setSetting("merge", checked) }
            CheckBox { text: qsTr("Sound"); checked: root.settings.sound || false; onToggled: root.controller.setSetting("sound", checked) }
        }
        RowLayout {
            Label { text: qsTr("Merge separator") }
            ComboBox { model: [qsTr("Newline"), qsTr("Space"), qsTr("None")]; currentIndex: root.settings.separator === " " ? 1 : root.settings.separator === "" ? 2 : 0; onActivated: index => root.controller.setSetting("separator", index === 1 ? " " : index === 2 ? "" : "\n") }
            Label { text: qsTr("Excluded application IDs") }
            TextField { Layout.fillWidth: true; text: (root.settings.excludedApps || []).join(";"); placeholderText: qsTr("Application IDs separated by ;"); onEditingFinished: root.controller.setSetting("excludedApps", text.split(";").map(value => value.trim()).filter(value => value !== "")) }
        }
        Label { Layout.fillWidth: true; text: root.controller.inputStatus; wrapMode: Text.Wrap }
    }
    Dialog {
        id: collectionDialog
        property string identity: ""
        title: identity === "" ? qsTr("New collection") : qsTr("Edit collection")
        anchors.centerIn: parent
        modal: true
        standardButtons: Dialog.Save | Dialog.Cancel
        ColumnLayout {
            Label { text: qsTr("Name") }
            TextField { id: collectionName; Layout.fillWidth: true }
            Label { text: qsTr("Keyword prefix") }
            TextField { id: prefix; Layout.fillWidth: true }
            Label { text: qsTr("Keyword suffix") }
            TextField { id: suffix; Layout.fillWidth: true }
            CheckBox { id: groupEnabled; text: qsTr("Enable collection") }
        }
        onAccepted: {
            let identity = collectionDialog.identity
            if (identity === "") identity = root.controller.createCollection(collectionName.text)
            if (identity !== "") root.controller.editCollection(identity, { name: collectionName.text, prefix: prefix.text, suffix: suffix.text, enabled: groupEnabled.checked })
        }
    }
    Dialog {
        id: removeGroup
        anchors.centerIn: parent
        title: qsTr("Delete collection and its snippets?")
        modal: true
        standardButtons: Dialog.Yes | Dialog.No
        onAccepted: root.controller.removeCollection(root.controller.collectionId)
    }
    Dialog {
        id: removeSnippet
        anchors.centerIn: parent
        title: qsTr("Delete selected snippet?")
        modal: true
        standardButtons: Dialog.Yes | Dialog.No
        onAccepted: root.controller.removeSnippet()
    }
}
