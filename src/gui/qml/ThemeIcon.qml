// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick

Canvas {
    id: icon
    property string name: ""
    property color color: "#626874"
    implicitWidth: 16
    implicitHeight: 16
    onNameChanged: requestPaint()
    onColorChanged: requestPaint()
    onWidthChanged: requestPaint()
    onHeightChanged: requestPaint()
    onPaint: {
        const ctx = getContext("2d")
        ctx.reset()
        ctx.scale(width / 24, height / 24)
        ctx.strokeStyle = color
        ctx.fillStyle = color
        ctx.lineWidth = 1.8
        ctx.lineCap = "round"
        ctx.lineJoin = "round"
        ctx.beginPath()
        switch (name) {
        case "chevron-down": ctx.moveTo(6, 9); ctx.lineTo(12, 15); ctx.lineTo(18, 9); break
        case "chevron-right": ctx.moveTo(9, 6); ctx.lineTo(15, 12); ctx.lineTo(9, 18); break
        case "close": ctx.moveTo(7, 7); ctx.lineTo(17, 17); ctx.moveTo(17, 7); ctx.lineTo(7, 17); break
        case "check": ctx.moveTo(5, 12); ctx.lineTo(10, 17); ctx.lineTo(19, 7); break
        case "search": ctx.arc(10.5, 10.5, 6.5, 0, 2 * Math.PI); ctx.moveTo(15.5, 15.5); ctx.lineTo(21, 21); break
        case "plus": ctx.moveTo(12, 5); ctx.lineTo(12, 19); ctx.moveTo(5, 12); ctx.lineTo(19, 12); break
        case "arrow-up": ctx.moveTo(6, 10); ctx.lineTo(12, 4); ctx.lineTo(18, 10); ctx.moveTo(12, 4); ctx.lineTo(12, 20); break
        case "arrow-down": ctx.moveTo(6, 14); ctx.lineTo(12, 20); ctx.lineTo(18, 14); ctx.moveTo(12, 4); ctx.lineTo(12, 20); break
        case "return": ctx.moveTo(19, 5); ctx.lineTo(19, 14); ctx.lineTo(5, 14); ctx.moveTo(10, 9); ctx.lineTo(5, 14); ctx.lineTo(10, 19); break
        case "copy": ctx.rect(9, 9, 11, 12); ctx.moveTo(15, 6); ctx.lineTo(15, 3); ctx.lineTo(4, 3); ctx.lineTo(4, 15); ctx.lineTo(6, 15); break
        case "edit": ctx.moveTo(4, 16); ctx.lineTo(16, 4); ctx.lineTo(20, 8); ctx.lineTo(8, 20); ctx.lineTo(3, 21); ctx.closePath(); ctx.moveTo(13, 7); ctx.lineTo(17, 11); break
        case "trash": ctx.moveTo(4, 6); ctx.lineTo(20, 6); ctx.moveTo(9, 6); ctx.lineTo(9, 3); ctx.lineTo(15, 3); ctx.lineTo(15, 6); ctx.moveTo(6, 6); ctx.lineTo(7, 21); ctx.lineTo(17, 21); ctx.lineTo(18, 6); ctx.moveTo(10, 10); ctx.lineTo(10, 17); ctx.moveTo(14, 10); ctx.lineTo(14, 17); break
        case "text": ctx.moveTo(5, 5); ctx.lineTo(19, 5); ctx.moveTo(12, 5); ctx.lineTo(12, 19); ctx.moveTo(8, 19); ctx.lineTo(16, 19); break
        case "image": ctx.rect(3, 4, 18, 16); ctx.moveTo(3, 17); ctx.lineTo(9, 11); ctx.lineTo(14, 16); ctx.lineTo(17, 13); ctx.lineTo(21, 17); ctx.moveTo(17, 8); ctx.arc(16, 8, 1, 0, 2 * Math.PI); break
        case "link": ctx.moveTo(8, 16); ctx.lineTo(18, 6); ctx.moveTo(10, 6); ctx.lineTo(18, 6); ctx.lineTo(18, 14); ctx.moveTo(6, 5); ctx.lineTo(3, 5); ctx.lineTo(3, 21); ctx.lineTo(19, 21); ctx.lineTo(19, 18); break
        case "collection": ctx.moveTo(3, 7); ctx.lineTo(3, 20); ctx.lineTo(21, 20); ctx.lineTo(21, 7); ctx.lineTo(12, 7); ctx.lineTo(10, 4); ctx.lineTo(3, 4); ctx.closePath(); break
        case "pause": ctx.moveTo(8, 5); ctx.lineTo(8, 19); ctx.moveTo(16, 5); ctx.lineTo(16, 19); break
        case "play": ctx.moveTo(7, 4); ctx.lineTo(20, 12); ctx.lineTo(7, 20); ctx.closePath(); break
        case "more":
            for (let x = 5; x <= 19; x += 7) { ctx.moveTo(x + 1.5, 12); ctx.arc(x, 12, 1.5, 0, 2 * Math.PI) }
            ctx.fill()
            return
        }
        ctx.stroke()
    }
}
