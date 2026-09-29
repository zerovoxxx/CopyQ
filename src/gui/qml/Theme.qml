// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick

QtObject {
    property var values: ({})
    readonly property color background: values.bg || "#f7f7f8"
    readonly property color foreground: values.fg || "#202124"
    readonly property color alternate: values.alt_bg || "#eceef1"
    readonly property color highlight: values.sel_bg || "#355edc"
    readonly property color highlightedText: values.sel_fg || "#ffffff"
    readonly property color editorBackground: values.edit_bg || background
    readonly property color editorText: values.edit_fg || foreground
    readonly property color notesBackground: values.notes_bg || alternate
    readonly property color notesText: values.notes_fg || foreground
    readonly property font textFont: values.font || Qt.font({pixelSize: 14})
    readonly property font editorFont: values.edit_font || textFont
    readonly property int spacing: 12
    readonly property int margin: 20
    readonly property int rowHeight: 64
}
