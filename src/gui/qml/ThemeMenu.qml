// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick.Controls.Basic
import QClip

Menu {
    id: control
    required property var values
    padding: 6
    background: ClipboardStyle {
        theme: control.values
        kind: ClipboardStyle.Menu
        font: control.font
    }
}
