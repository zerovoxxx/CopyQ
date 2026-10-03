// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls.Basic
import QClip

Menu {
    id: control
    required property var values
    Theme { id: theme; values: control.values }
    font: theme.textFont
    padding: 6
    margins: 8
    implicitWidth: {
        let result = 220
        for (let i = 0; i < count; ++i) {
            const entry = itemAt(i)
            if (entry) result = Math.max(result, entry.implicitWidth + leftPadding + rightPadding)
        }
        return result
    }
    background: Rectangle {
        radius: theme.radius + 2; color: theme.surface; border.color: theme.line
        ClipboardStyle { anchors.fill: parent; visible: theme.customStyle; theme: control.values; kind: ClipboardStyle.Menu; font: control.font }
    }
}
