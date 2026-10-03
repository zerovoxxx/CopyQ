// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls.Basic
import QClip

MenuItem {
    id: control
    required property var values
    implicitWidth: Math.max(200, contentItem.implicitWidth)
    implicitHeight: Math.max(32, contentItem.implicitHeight)
    padding: 0
    contentItem: ClipboardStyle {
        theme: control.values
        kind: ClipboardStyle.MenuItem
        text: control.text
        font: control.font
        hovered: control.highlighted || control.hovered
        pressed: control.down
        focused: control.visualFocus
    }
    background: Item {}
    Accessible.name: text
}
