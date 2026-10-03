// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls.Basic
import QClip

Button {
    id: control
    required property var values
    property bool toolbar: false
    property bool primary: false
    property bool quiet: toolbar
    Theme { id: theme; values: control.values }
    font: theme.textFont
    implicitWidth: Math.max(theme.controlHeight, contentItem.implicitWidth + leftPadding + rightPadding)
    implicitHeight: theme.customStyle ? Math.max(theme.controlHeight, legacy.implicitHeight) : Math.max(theme.controlHeight, label.implicitHeight + 8)
    leftPadding: theme.customStyle ? 0 : 10
    rightPadding: leftPadding
    topPadding: 0
    bottomPadding: 0
    opacity: enabled ? 1 : 0.4
    contentItem: Item {
        implicitWidth: theme.customStyle ? legacy.implicitWidth : label.implicitWidth
        implicitHeight: theme.customStyle ? legacy.implicitHeight : label.implicitHeight
        Text {
            id: label
            anchors.fill: parent
            visible: !theme.customStyle
            text: control.text
            font: control.font
            color: control.primary ? theme.highlightedText : theme.foreground
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
        ClipboardStyle {
            id: legacy
            anchors.fill: parent
            visible: theme.customStyle
            theme: control.values
            kind: control.toolbar ? ClipboardStyle.ToolbarButton : ClipboardStyle.Button
            text: control.text
            font: control.font
            hovered: control.hovered
            pressed: control.down
            focused: control.visualFocus
        }
    }
    background: Rectangle {
        visible: !theme.customStyle
        radius: theme.radius
        color: control.primary ? (control.down ? Qt.darker(theme.highlight, 1.12) : theme.highlight)
            : control.down ? theme.selection : control.hovered ? theme.hover : control.quiet ? "transparent" : theme.surface
        border.color: control.visualFocus ? theme.highlight : control.quiet || control.primary ? "transparent" : theme.line
        Behavior on color { ColorAnimation { duration: 110 } }
    }
    Accessible.name: text
}
