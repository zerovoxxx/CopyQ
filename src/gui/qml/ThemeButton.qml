// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls.Basic
import QClip

Button {
    id: control
    required property var values
    property bool toolbar: false
    property bool primary: false
    property bool quiet: toolbar
    property string iconName: ""
    property bool trailingIcon: false
    property bool selected: false
    property bool destructive: false
    Theme { id: theme; values: control.values }
    font: theme.textFont
    implicitWidth: Math.max(theme.controlHeight, contentItem.implicitWidth + leftPadding + rightPadding)
    implicitHeight: theme.customStyle ? Math.max(theme.controlHeight, legacy.implicitHeight) : Math.max(theme.controlHeight, label.implicitHeight + 10)
    leftPadding: theme.customStyle ? 0 : text.length === 0 ? 8 : 12
    rightPadding: leftPadding
    topPadding: 0
    bottomPadding: 0
    opacity: enabled ? 1 : 0.42
    contentItem: Item {
        implicitWidth: theme.customStyle ? legacy.implicitWidth : label.implicitWidth + (control.iconName.length > 0 ? theme.iconSize + (control.text.length > 0 ? 6 : 0) : 0)
        implicitHeight: theme.customStyle ? legacy.implicitHeight : label.implicitHeight
        Item {
            anchors.centerIn: parent
            width: Math.min(parent.width, label.implicitWidth + (control.iconName.length > 0 ? theme.iconSize + (control.text.length > 0 ? 6 : 0) : 0))
            height: parent.height
            z: 1
            visible: !theme.customStyle || control.text.length === 0
            Text {
                id: label
                anchors.verticalCenter: parent.verticalCenter
                x: control.iconName.length > 0 && !control.trailingIcon ? theme.iconSize + (control.text.length > 0 ? 6 : 0) : 0
                width: parent.width - (control.iconName.length > 0 ? theme.iconSize + (control.text.length > 0 ? 6 : 0) : 0)
                visible: !theme.customStyle
                text: control.text
                font: control.font
                color: control.primary ? theme.highlightedText : control.destructive ? theme.danger : control.selected ? theme.selectionText : theme.foreground
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }
            ThemeIcon {
                anchors.verticalCenter: parent.verticalCenter
                x: control.trailingIcon ? parent.width - width : 0
                width: theme.iconSize; height: width
                name: control.iconName
                color: control.primary ? theme.highlightedText : control.destructive ? theme.danger : control.selected ? theme.selectionText : theme.muted
                visible: name.length > 0
            }
        }
        ClipboardStyle {
            id: legacy
            anchors.fill: parent
            visible: theme.customStyle
            theme: control.values
            kind: control.toolbar ? ClipboardStyle.ToolbarButton : ClipboardStyle.Button
            text: control.text
            font: control.font
            hovered: control.hovered
            pressed: control.down
            focused: control.visualFocus
        }
    }
    background: Rectangle {
        visible: !theme.customStyle
        radius: theme.radius
        color: control.primary ? (control.down ? Qt.darker(theme.highlight, 1.12) : control.hovered ? Qt.lighter(theme.highlight, 1.06) : theme.highlight)
            : control.down ? theme.pressed : control.selected ? theme.selection : control.hovered ? theme.hover : control.quiet ? "transparent" : theme.surface
        border.color: control.quiet || control.primary || control.selected ? "transparent" : theme.line
        Rectangle {
            anchors.fill: parent; anchors.margins: -2; radius: parent.radius + 2
            color: "transparent"; border.width: 2; border.color: theme.focusRing
            visible: control.visualFocus
        }
        Behavior on color { ColorAnimation { duration: 110 } }
    }
    Accessible.name: text
}
