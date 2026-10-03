// SPDX-License-Identifier: GPL-3.0-or-later
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QClip

Dialog {
    id: control
    required property var values
    Theme { id: theme; values: control.values }
    padding: 16
    font: theme.textFont
    background: Rectangle { radius: theme.radius + 4; color: theme.surface; border.color: theme.line }
    header: Label {
        text: control.title; visible: text.length > 0; font.bold: true
        color: theme.foreground; padding: 16; bottomPadding: 8; elide: Text.ElideRight
    }
    footer: DialogButtonBox {
        padding: 12
        spacing: 6
        standardButtons: control.standardButtons
        delegate: ThemeButton { values: control.values }
        onAccepted: control.accept()
        onRejected: control.reject()
    }
    Overlay.modal: Rectangle { color: "#55000000" }
}
