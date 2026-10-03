// SPDX-License-Identifier: GPL-3.0-or-later
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QClip

Dialog {
    id: control
    required property var values
    Theme { id: theme; values: control.values }
    padding: 20
    implicitWidth: Math.min(parent ? parent.width - 32 : 480, Math.max(360, implicitContentWidth + leftPadding + rightPadding))
    font: theme.textFont
    palette.window: theme.surface
    palette.windowText: theme.foreground
    palette.base: theme.surface
    palette.text: theme.foreground
    palette.button: theme.surface
    palette.buttonText: theme.foreground
    palette.highlight: theme.highlight
    palette.highlightedText: theme.highlightedText
    background: Rectangle { radius: theme.radius + 4; color: theme.surface; border.color: theme.line }
    header: Label {
        text: control.title; visible: text.length > 0; font.weight: Font.DemiBold
        color: theme.foreground; padding: 20; bottomPadding: 0; wrapMode: Text.Wrap
    }
    footer: DialogButtonBox {
        alignment: Qt.AlignRight
        padding: 16
        spacing: 8
        standardButtons: control.standardButtons
        background: Rectangle { color: theme.surface; radius: theme.radius + 4 }
        delegate: ThemeButton { values: control.values; primary: DialogButtonBox.buttonRole === DialogButtonBox.AcceptRole || DialogButtonBox.buttonRole === DialogButtonBox.YesRole }
        onAccepted: control.accept()
        onRejected: control.reject()
    }
    Overlay.modal: Rectangle { color: theme.dark ? "#66000000" : "#330f172a" }
}
