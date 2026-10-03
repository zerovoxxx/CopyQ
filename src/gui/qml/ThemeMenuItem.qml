// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls.Basic
import QClip

MenuItem {
    id: control
    required property var values
    Theme { id: theme; values: control.values }
    font: theme.textFont
    implicitWidth: Math.max(200, contentItem.implicitWidth + leftPadding + rightPadding)
    implicitHeight: visible ? Math.max(theme.controlHeight, contentItem.implicitHeight + 10) : 0
    leftPadding: 10; rightPadding: 10; topPadding: 0; bottomPadding: 0
    opacity: enabled ? 1 : 0.42
    contentItem: Item {
        implicitWidth: theme.customStyle ? legacy.implicitWidth : label.implicitWidth
        implicitHeight: theme.customStyle ? legacy.implicitHeight : label.implicitHeight
        Text { id: label; anchors.fill: parent; text: control.text; textFormat: Text.PlainText; font: control.font; color: theme.foreground; verticalAlignment: Text.AlignVCenter; elide: Text.ElideRight; visible: !theme.customStyle }
        ClipboardStyle { id: legacy; anchors.fill: parent; visible: theme.customStyle; theme: control.values; kind: ClipboardStyle.MenuItem; text: control.text; font: control.font; hovered: control.highlighted || control.hovered; pressed: control.down; focused: control.visualFocus }
    }
    background: Rectangle { radius: theme.radius - 2; color: control.down ? theme.pressed : control.highlighted || control.hovered ? theme.selection : "transparent"; visible: !theme.customStyle }
    Accessible.name: text
}
