// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls.Basic
import QClip

TextArea {
    id: control
    required property var values
    Theme { id: theme; values: control.values }
    padding: 8
    font: theme.editorFont
    color: theme.editorText
    placeholderTextColor: theme.muted
    selectionColor: theme.highlight
    selectedTextColor: theme.highlightedText
    selectByMouse: true
    background: Rectangle {
        radius: theme.radius
        color: theme.customStyle ? theme.editorBackground : theme.surface
        border.color: control.activeFocus ? theme.highlight : theme.line
    }
}
