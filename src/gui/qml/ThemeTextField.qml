// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls.Basic
import QClip

TextField {
    id: control
    required property var values
    Theme { id: theme; values: control.values }
    implicitHeight: Math.max(theme.controlHeight, contentHeight + 10)
    leftPadding: 10
    rightPadding: 10
    color: theme.foreground
    placeholderTextColor: theme.muted
    selectionColor: theme.highlight
    selectedTextColor: theme.highlightedText
    font: theme.textFont
    selectByMouse: true
    background: Item {
        Rectangle {
            anchors.fill: parent
            visible: !theme.customStyle
            radius: theme.radius
            color: theme.surface
            border.color: control.activeFocus ? theme.highlight : theme.line
            Behavior on border.color { ColorAnimation { duration: 120 } }
        }
        ClipboardStyle {
            anchors.fill: parent
            visible: theme.customStyle
            theme: control.values
            kind: ClipboardStyle.Search
            focused: control.activeFocus
            hovered: control.hovered
        }
    }
}
