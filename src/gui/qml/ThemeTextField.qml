// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls.Basic
import QClip

TextField {
    id: control
    required property var values
    Theme { id: theme; values: control.values }
    implicitHeight: Math.max(theme.controlHeight, contentHeight + 10)
    property string iconName: ""
    leftPadding: iconName.length > 0 ? 34 : 12
    rightPadding: 12
    topPadding: 5; bottomPadding: 5
    color: theme.foreground
    placeholderTextColor: theme.muted
    selectionColor: theme.highlight
    selectedTextColor: theme.highlightedText
    font: theme.textFont
    selectByMouse: true
    opacity: enabled ? 1 : 0.42
    ThemeIcon {
        x: 12; y: (control.height - height) / 2
        name: control.iconName; color: theme.muted; visible: name.length > 0
    }
    background: Item {
        Rectangle {
            anchors.fill: parent
            visible: !theme.customStyle
            radius: theme.radius
            color: theme.surface
            border.color: control.activeFocus ? theme.highlight : theme.line
            Rectangle {
                anchors.fill: parent; anchors.margins: -2; radius: parent.radius + 2
                color: "transparent"; border.width: 2; border.color: theme.focusRing
                visible: control.activeFocus
            }
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
