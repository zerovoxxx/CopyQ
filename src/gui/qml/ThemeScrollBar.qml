// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls.Basic
import QClip

ScrollBar {
    id: control
    required property var values
    Theme { id: theme; values: control.values }
    padding: 2
    implicitWidth: 8
    implicitHeight: 8
    minimumSize: 0.08
    policy: theme.showScrollbars ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff
    contentItem: Rectangle {
        implicitWidth: 4; implicitHeight: 4; radius: 2
        visible: control.size < 1 && control.policy !== ScrollBar.AlwaysOff
        color: theme.foreground
        opacity: control.pressed ? 0.5 : control.hovered ? 0.35 : control.active ? 0.22 : 0.12
        Behavior on opacity { NumberAnimation { duration: 120 } }
    }
    background: Item {}
}
