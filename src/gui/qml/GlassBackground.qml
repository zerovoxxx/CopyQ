// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QClip

Rectangle {
    id: surface
    required property var values
    required property ClipboardWindow controller
    property int cornerRadius: 0
    Theme { id: theme; values: surface.values }
    Binding { target: surface.controller; property: "materialColor"; value: theme.background }
    color: controller.blurAvailable && !theme.customStyle
        ? Qt.rgba(theme.background.r, theme.background.g, theme.background.b, theme.dark ? 0.58 : 0.65)
        : theme.background
    radius: cornerRadius
    border.width: cornerRadius > 0 ? 1 : 0
    border.color: theme.line
    Rectangle {
        anchors.fill: parent
        radius: parent.radius
        color: "transparent"
        border.width: 1
        border.color: theme.dark ? "#0cffffff" : "#55ffffff"
    }
}
