// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick

QtObject {
    property var values: ({})
    readonly property color background: values.bg || "#f7f7f8"
    readonly property color foreground: values.fg || "#202124"
    readonly property color alternate: values.alt_bg || "#eceef1"
    readonly property color highlight: values.sel_bg || "#355edc"
    readonly property color highlightedText: values.sel_fg || "#ffffff"
    readonly property bool dark: background.r * 0.299 + background.g * 0.587 + background.b * 0.114 < 0.5
    readonly property bool customStyle: values.custom_style === true
    readonly property color muted: Qt.rgba(foreground.r, foreground.g, foreground.b, 0.58)
    readonly property color line: Qt.rgba(foreground.r, foreground.g, foreground.b, dark ? 0.12 : 0.09)
    readonly property color surface: dark ? "#30323a" : "#ffffff"
    readonly property color hover: Qt.rgba(foreground.r, foreground.g, foreground.b, 0.055)
    readonly property color selection: Qt.rgba(highlight.r, highlight.g, highlight.b, dark ? 0.24 : 0.12)
    readonly property color editorBackground: values.edit_bg || background
    readonly property color editorText: values.edit_fg || foreground
    readonly property color notesBackground: values.notes_bg || alternate
    readonly property color notesText: values.notes_fg || foreground
    readonly property font textFont: values.font || Qt.font({pixelSize: 14})
    readonly property color searchText: values.find_fg || foreground
    readonly property font searchFont: values.find_font || textFont
    readonly property font editorFont: values.edit_font || textFont
    readonly property bool showScrollbars: values.show_scrollbars !== false
    readonly property bool showNumber: values.show_number !== false
    readonly property int spacing: values.quick_spacing === undefined ? 8 : Math.max(0, Math.min(256, values.quick_spacing))
    readonly property int margin: values.quick_margin === undefined ? 12 : Math.max(0, Math.min(256, values.quick_margin))
    readonly property int rowHeight: values.quick_row_height === undefined ? 48 : Math.max(32, Math.min(256, values.quick_row_height))
    readonly property int radius: values.quick_radius === undefined ? 8 : Math.max(0, Math.min(256, values.quick_radius))
    readonly property int controlHeight: 32
}
