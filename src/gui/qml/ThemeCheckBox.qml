// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls.Basic
import QClip

CheckBox {
    id: control
    required property var values
    Theme { id: theme; values: control.values }
    implicitHeight: Math.max(28, contentItem.implicitHeight + 4)
    leftPadding: 0
    rightPadding: 0
    spacing: text.length > 0 ? 8 : 0
    font: theme.textFont
    opacity: enabled ? 1 : 0.42
    indicator: Rectangle {
        x: control.leftPadding
        y: (control.height - height) / 2
        width: 16; height: 16; radius: 4
        color: control.checked ? theme.highlight : theme.surface
        border.color: control.checked || control.hovered ? theme.highlight : theme.muted
        ThemeIcon { anchors.fill: parent; anchors.margins: 2; name: "check"; visible: control.checked; color: theme.highlightedText }
        Rectangle {
            anchors.fill: parent; anchors.margins: -3; radius: 6; color: "transparent"
            border.width: 2; border.color: theme.focusRing; visible: control.visualFocus
        }
    }
    contentItem: Text {
        text: control.text; font: control.font; color: theme.foreground
        leftPadding: control.indicator.width + control.spacing
        verticalAlignment: Text.AlignVCenter
        wrapMode: Text.Wrap
        elide: Text.ElideRight
    }
}
