// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls.Basic
import QClip

TextArea {
    id: control
    required property var values
    Theme { id: theme; values: control.values }
    padding: 12
    implicitHeight: Math.max(88, contentHeight + topPadding + bottomPadding)
    wrapMode: TextEdit.Wrap
    font: theme.editorFont
    color: theme.editorText
    placeholderTextColor: theme.muted
    selectionColor: theme.highlight
    selectedTextColor: theme.highlightedText
    selectByMouse: true
    opacity: enabled ? 1 : 0.42
    background: Rectangle {
        radius: theme.radius
        color: theme.customStyle ? theme.editorBackground : theme.surface
        border.color: control.activeFocus ? theme.highlight : theme.line
        Rectangle {
            anchors.fill: parent; anchors.margins: -2; radius: parent.radius + 2
            color: "transparent"; border.width: 2; border.color: theme.focusRing
            visible: control.activeFocus && !control.readOnly
        }
    }
}
