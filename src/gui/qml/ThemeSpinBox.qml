// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls.Basic
import QClip

SpinBox {
    id: control
    required property var values
    Theme { id: theme; values: control.values }
    implicitWidth: 144
    implicitHeight: Math.max(theme.controlHeight, contentItem.implicitHeight + 10)
    leftPadding: 12; rightPadding: 40
    font: theme.textFont
    opacity: enabled ? 1 : 0.42
    contentItem: TextInput {
        text: control.textFromValue(control.value, control.locale)
        font: control.font; color: theme.foreground
        verticalAlignment: TextInput.AlignVCenter
        readOnly: !control.editable
        validator: control.validator
        inputMethodHints: Qt.ImhFormattedNumbersOnly
        selectByMouse: true
        selectionColor: theme.highlight; selectedTextColor: theme.highlightedText
    }
    up.indicator: Rectangle {
        x: control.width - width - 3; y: 3; width: 28; height: (control.height - 6) / 2
        radius: 3; color: control.up.pressed ? theme.pressed : control.up.hovered ? theme.hover : "transparent"
        ThemeIcon { anchors.centerIn: parent; width: 12; height: 12; name: "chevron-down"; rotation: 180; color: theme.muted; opacity: control.value < control.to ? 1 : 0.4 }
    }
    down.indicator: Rectangle {
        x: control.width - width - 3; y: control.height / 2; width: 28; height: (control.height - 6) / 2
        radius: 3; color: control.down.pressed ? theme.pressed : control.down.hovered ? theme.hover : "transparent"
        ThemeIcon { anchors.centerIn: parent; width: 12; height: 12; name: "chevron-down"; color: theme.muted; opacity: control.value > control.from ? 1 : 0.4 }
    }
    background: Rectangle {
        radius: theme.radius; color: theme.surface
        border.color: control.activeFocus ? theme.highlight : theme.line
        Rectangle { anchors.fill: parent; anchors.margins: -2; radius: parent.radius + 2; color: "transparent"; border.width: 2; border.color: theme.focusRing; visible: control.activeFocus }
    }
}
