// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls.Basic
import QClip

Button {
    id: control
    required property var values
    property bool toolbar: false
    implicitWidth: contentItem.implicitWidth
    implicitHeight: contentItem.implicitHeight
    padding: 0
    contentItem: ClipboardStyle {
        theme: control.values
        kind: control.toolbar ? ClipboardStyle.ToolbarButton : ClipboardStyle.Button
        text: control.text
        font: control.font
        hovered: control.hovered
        pressed: control.down
        focused: control.visualFocus
    }
    background: Item {}
    Accessible.name: text
}
