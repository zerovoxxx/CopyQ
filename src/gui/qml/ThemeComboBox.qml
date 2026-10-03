// SPDX-License-Identifier: GPL-3.0-or-later
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QClip

ComboBox {
    id: control
    required property var values
    Theme { id: theme; values: control.values }
    implicitHeight: Math.max(theme.controlHeight, implicitContentHeight + 8)
    implicitWidth: Math.max(100, implicitContentWidth + 40)
    font: theme.textFont
    leftPadding: 12; rightPadding: 36
    topPadding: 4; bottomPadding: 4
    opacity: enabled ? 1 : 0.42
    contentItem: Text {
        text: control.displayText; font: control.font; color: theme.foreground
        verticalAlignment: Text.AlignVCenter; elide: Text.ElideRight
    }
    indicator: ThemeIcon {
        objectName: "combo_indicator"
        x: control.mirrored ? 12 : control.width - width - 12
        y: (control.height - height) / 2
        width: theme.iconSize; height: width
        name: "chevron-down"; color: theme.muted
        rotation: control.popup.visible ? 180 : 0
        Behavior on rotation { NumberAnimation { duration: 120 } }
    }
    background: Rectangle {
        radius: theme.radius; color: control.down ? Qt.tint(theme.surface, theme.pressed) : control.hovered ? Qt.tint(theme.surface, theme.hover) : theme.surface
        border.color: control.visualFocus || control.popup.visible ? theme.highlight : theme.line
        Rectangle {
            anchors.fill: parent; anchors.margins: -2; radius: parent.radius + 2
            color: "transparent"; border.width: 2; border.color: theme.focusRing
            visible: control.visualFocus
        }
    }
    delegate: ThemeDelegate {
        required property int index
        required property var modelData
        values: control.values
        width: control.popup.availableWidth
        rightPadding: 30
        text: control.textRole ? String(modelData[control.textRole]) : String(modelData)
        highlighted: control.highlightedIndex === index
        ThemeIcon {
            anchors.right: parent.right; anchors.rightMargin: 8; anchors.verticalCenter: parent.verticalCenter
            name: "check"; color: theme.highlight
            visible: control.currentIndex === parent.index
        }
    }
    popup: Popup {
        y: control.height + 5
        width: control.width
        implicitHeight: Math.min(contentItem.implicitHeight + 12, 300)
        padding: 6
        background: Rectangle { radius: theme.radius + 2; color: theme.surface; border.color: theme.line }
        contentItem: ListView {
            clip: true; implicitHeight: contentHeight
            model: control.popup.visible ? control.delegateModel : null
            currentIndex: control.highlightedIndex
            spacing: 2
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ThemeScrollBar { values: control.values }
        }
    }
}
