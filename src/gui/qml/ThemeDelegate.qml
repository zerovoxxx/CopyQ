// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls.Basic
import QClip

ItemDelegate {
    id: control
    required property var values
    Theme { id: theme; values: control.values }
    implicitHeight: Math.max(theme.controlHeight, contentItem.implicitHeight + 8)
    leftPadding: 8
    rightPadding: 8
    font: theme.textFont
    contentItem: Text {
        text: control.text
        font: control.font
        color: theme.foreground
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
    background: Rectangle {
        radius: theme.radius
        color: control.highlighted ? theme.selection : control.hovered ? theme.hover : "transparent"
        border.color: control.visualFocus ? theme.highlight : "transparent"
        Behavior on color { ColorAnimation { duration: 110 } }
    }
    Accessible.name: text
}
