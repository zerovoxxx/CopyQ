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
    leftPadding: 10; rightPadding: 28
    contentItem: Text {
        text: control.displayText; font: control.font; color: theme.foreground
        verticalAlignment: Text.AlignVCenter; elide: Text.ElideRight
    }
    indicator: Text {
        x: control.width - width - 12; y: (control.height - height) / 2
        text: "⌄"; color: theme.muted; font.pixelSize: 16
    }
    background: Rectangle {
        radius: theme.radius; color: control.hovered ? theme.hover : theme.surface
        border.color: control.visualFocus ? theme.highlight : theme.line
    }
    delegate: ThemeDelegate {
        required property int index
        required property var modelData
        values: control.values
        width: control.width
        text: control.textRole ? String(modelData[control.textRole]) : String(modelData)
        highlighted: control.highlightedIndex === index
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
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar {}
        }
    }
}
