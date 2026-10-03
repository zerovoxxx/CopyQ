// SPDX-License-Identifier: GPL-3.0-or-later
#include "clipboardwindow.h"
#include "platform/platformwindoweffects.h"

#include <QEvent>
#include <QGuiApplication>
#include <QPalette>
#include <QPlatformSurfaceEvent>
#include <QTimer>
#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

ClipboardWindow::ClipboardWindow()
    : m_materialColor(QGuiApplication::palette().color(QPalette::Window))
{
    auto surface = format();
    surface.setAlphaBufferSize(8);
    setFormat(surface);
    setColor(Qt::transparent);
    connect(this, &QWindow::visibleChanged, this, [this](bool visible) {
        if (visible) updateMaterial();
    });
    connect(qGuiApp, &QGuiApplication::paletteChanged, this, [this] {
        if (isVisible()) updateMaterial();
    });
}

void ClipboardWindow::setMaterialColor(const QColor &color)
{
    if (m_materialColor == color) return;
    m_materialColor = color;
    emit materialChanged();
    if (isVisible()) updateMaterial();
}

void ClipboardWindow::updateMaterial()
{
    const bool available = setWindowBlur(this, m_materialColor.lightnessF() < 0.5);
    if (m_blurAvailable != available) {
        m_blurAvailable = available;
        emit materialChanged();
    }
}

bool ClipboardWindow::event(QEvent *event)
{
    if (event->type() == QEvent::PlatformSurface) {
        const auto surface = static_cast<QPlatformSurfaceEvent *>(event);
        if (surface->surfaceEventType() == QPlatformSurfaceEvent::SurfaceCreated) {
            QTimer::singleShot(0, this, &ClipboardWindow::updateMaterial);
        } else if (m_blurAvailable) {
            m_blurAvailable = false;
            emit materialChanged();
        }
    }
    return QQuickView::event(event);
}

bool ClipboardWindow::nativeEvent(const QByteArray &type, void *message, qintptr *result)
{
#ifdef Q_OS_WIN
    const auto msg = static_cast<MSG *>(message);
    if (msg->message == WM_SETTINGCHANGE || msg->message == WM_THEMECHANGED
            || msg->message == WM_DWMCOMPOSITIONCHANGED)
        QTimer::singleShot(0, this, &ClipboardWindow::updateMaterial);
#endif
    return QQuickView::nativeEvent(type, message, result);
}
