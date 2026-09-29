// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "gui/clipboardpalettemodel.h"

#include <QImage>
#include <QQuickPaintedItem>
#include <QTimer>
#include <memory>

class ItemWidget;
class QScrollArea;
class QWidget;

// Widgets stay on the GUI thread. The Quick render thread only paints a snapshot.
class ClipboardItemPreview : public QQuickPaintedItem
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(ClipboardPaletteModel *history READ history WRITE setHistory NOTIFY historyChanged)

public:
    explicit ClipboardItemPreview(QQuickItem *parent = nullptr);
    ~ClipboardItemPreview() override;
    ClipboardPaletteModel *history() const { return m_history; }
    void setHistory(ClipboardPaletteModel *history);
    void paint(QPainter *painter) override;
    QWidget *previewWidget() const;

signals:
    void historyChanged();

protected:
    bool eventFilter(QObject *object, QEvent *event) override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private:
    void rebuild();
    void renderWidget();
    void watchWidgets();
    void forwardMouse(QMouseEvent *event);
    QPointer<ClipboardPaletteModel> m_history;
    std::unique_ptr<QScrollArea> m_scroll;
    ItemWidget *m_item = nullptr;
    QPointer<QWidget> m_mouseTarget;
    QPointer<QWidget> m_keyTarget;
    QTimer m_renderTimer;
    QImage m_snapshot;
    bool m_rendering = false;
};
