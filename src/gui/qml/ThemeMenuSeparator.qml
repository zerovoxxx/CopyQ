// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls.Basic
import QClip

MenuSeparator {
    id: control
    required property var values
    Theme { id: theme; values: control.values }
    padding: 4
    implicitHeight: 9
    contentItem: Rectangle { implicitHeight: 1; color: theme.line }
}
