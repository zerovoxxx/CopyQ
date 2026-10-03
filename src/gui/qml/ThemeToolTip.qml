// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls.Basic
import QClip

ToolTip {
    id: control
    required property var values
    Theme { id: theme; values: control.values }
    delay: 600
    padding: 8
    font: theme.textFont
    contentItem: Text { text: control.text; font: control.font; color: theme.foreground; wrapMode: Text.Wrap }
    background: Rectangle { radius: 6; color: theme.surface; border.color: theme.line }
}
