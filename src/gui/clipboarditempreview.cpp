// SPDX-License-Identifier: GPL-3.0-or-later
#include "clipboarditempreview.h"

#include "common/settings.h"
#include "common/contenttype.h"
#include "common/mimetypes.h"
#include "common/appconfig.h"
#include "gui/theme.h"
#include "item/itemfactory.h"
#include "item/itemwidget.h"

#include <QApplication>
#include <QKeyEvent>
#include <QLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QQuickWindow>
#include <QScrollArea>
#include <QWheelEvent>
#include <QtMath>

ClipboardItemPreview::ClipboardItemPreview(QQuickItem *parent)
    : QQuickPaintedItem(parent), m_scroll(std::make_unique<QScrollArea>())
{
    setAcceptedMouseButtons(Qt::AllButtons);
    setClip(true);
    m_scroll->setObjectName(QStringLiteral("plugin_preview"));
    m_scroll->setAttribute(Qt::WA_DontShowOnScreen);
    m_scroll->setFrameShape(QFrame::NoFrame);
    m_renderTimer.setSingleShot(true);
    m_renderTimer.setInterval(16);
    connect(&m_renderTimer, &QTimer::timeout, this, &ClipboardItemPreview::renderWidget);
    connect(this, &QQuickItem::visibleChanged, this, [this] {
        if (isVisible()) m_renderTimer.start();
        else m_renderTimer.stop();
    });
}

ClipboardItemPreview::~ClipboardItemPreview() = default;

void ClipboardItemPreview::setHistory(ClipboardPaletteModel *history)
{
    if (m_history == history) return;
    if (m_history) disconnect(m_history, nullptr, this, nullptr);
    m_history = history;
    if (history) {
        connect(history, &ClipboardPaletteModel::previewChanged, this, &ClipboardItemPreview::rebuild);
        connect(history, &QObject::destroyed, this, &ClipboardItemPreview::rebuild);
    }
    rebuild();
    emit historyChanged();
}

QWidget *ClipboardItemPreview::previewWidget() const
{
    return m_item ? m_item->widget() : nullptr;
}

void ClipboardItemPreview::setTheme(const QVariantMap &theme)
{
    if (m_theme == theme) return;
    m_theme = theme;
    rebuild();
    emit themeChanged();
}

void ClipboardItemPreview::rebuild()
{
    m_mouseTarget.clear();
    m_keyTarget.clear();
    delete m_scroll->takeWidget();
    m_item = nullptr;
    m_snapshot = {};
    if (m_history && m_history->selectedIndex().isValid() && m_history->itemFactory()) {
        Settings settings;
        Theme theme(settings);
        theme.decorateMainWindow(m_scroll.get());
        theme.decorateItemPreview(m_scroll.get());
        const auto data = m_history->selectedIndex().data(contentType::isHidden).toBool()
            ? QVariantMap{{mimeText, tr("Hidden item").toUtf8()}} : m_history->displayData();
        m_item = m_history->itemFactory()->createItem(data,
            m_scroll.get(), theme.isAntialiasingEnabled(), true, true);
        m_scroll->setWidget(m_item->widget());
        if (!m_theme.isEmpty() && !m_theme.value(QStringLiteral("custom_style")).toBool()) {
            // Clearing QSS restores its saved palette, so apply the current theme afterwards.
            m_scroll->setStyleSheet(QString());
            auto palette = m_scroll->palette();
            const auto foreground = m_theme.value(QStringLiteral("fg")).value<QColor>();
            const bool dark = m_theme.value(QStringLiteral("bg")).value<QColor>().lightnessF() < 0.5;
            const auto surface = dark ? QColor("#2e3036") : QColor(Qt::white);
            palette.setColor(QPalette::Window, surface);
            palette.setColor(QPalette::Base, surface);
            palette.setColor(QPalette::Text, foreground);
            palette.setColor(QPalette::WindowText, foreground);
            m_scroll->setPalette(palette);
            for (auto child : m_scroll->findChildren<QWidget*>())
                child->setPalette(palette);
            m_item->widget()->setPalette(palette);
            m_item->widget()->setStyleSheet(QStringLiteral("color: %1; background: transparent;").arg(foreground.name()));
        }
        m_scroll->show(); // WA_DontShowOnScreen: no native preview window or focus target.
        watchWidgets();
    }
    m_renderTimer.start();
    update();
}

void ClipboardItemPreview::watchWidgets()
{
    m_scroll->installEventFilter(this);
    for (auto widget : m_scroll->findChildren<QWidget *>())
        widget->installEventFilter(this);
}

bool ClipboardItemPreview::eventFilter(QObject *, QEvent *event)
{
    switch (event->type()) {
    case QEvent::ChildAdded:
        QTimer::singleShot(0, this, &ClipboardItemPreview::watchWidgets);
        break;
    case QEvent::UpdateRequest:
    case QEvent::LayoutRequest:
    case QEvent::Resize:
    case QEvent::PaletteChange:
        if (!m_rendering && isVisible() && !m_renderTimer.isActive()) m_renderTimer.start();
        break;
    default: break;
    }
    return false;
}

void ClipboardItemPreview::geometryChange(const QRectF &next, const QRectF &previous)
{
    QQuickPaintedItem::geometryChange(next, previous);
    if (next.size() != previous.size()) m_renderTimer.start();
}

void ClipboardItemPreview::renderWidget()
{
    if (!m_item || width() < 1 || height() < 1 || !isVisible()) {
        m_snapshot = {};
        update();
        return;
    }
    m_rendering = true;
    m_scroll->resize(qRound(width()), qRound(height()));
    m_item->updateSize(QSize(AppConfig().option<Config::text_wrap>() ? m_scroll->viewport()->width() : 2048, 16384),
        m_scroll->viewport()->width());
    const auto ratio = window() ? window()->devicePixelRatio() : 1.0;
    QImage snapshot(QSize(qCeil(width() * ratio), qCeil(height() * ratio)), QImage::Format_ARGB32_Premultiplied);
    snapshot.setDevicePixelRatio(ratio);
    snapshot.fill(Qt::transparent);
    QPainter painter(&snapshot);
    m_scroll->render(&painter);
    painter.end();
    m_snapshot = snapshot;
    m_rendering = false;
    update();
}

void ClipboardItemPreview::paint(QPainter *painter)
{
    painter->drawImage(QPointF(), m_snapshot);
}

void ClipboardItemPreview::forwardMouse(QMouseEvent *event)
{
    auto target = m_mouseTarget.data();
    if (!target) target = m_scroll->childAt(event->position().toPoint());
    if (!target) target = m_scroll->viewport();
    if (event->type() == QEvent::MouseButtonPress) {
        m_mouseTarget = target;
        m_keyTarget = target;
        while (m_keyTarget && m_keyTarget->focusPolicy() == Qt::NoFocus && m_keyTarget != m_scroll.get())
            m_keyTarget = m_keyTarget->parentWidget();
        forceActiveFocus();
    }
    const auto position = target->mapFrom(m_scroll.get(), event->position().toPoint());
    QMouseEvent forwarded(event->type(), position, target->mapToGlobal(position),
        event->button(), event->buttons(), event->modifiers());
    QApplication::sendEvent(target, &forwarded);
    event->setAccepted(forwarded.isAccepted());
    if (event->type() == QEvent::MouseButtonRelease) m_mouseTarget.clear();
    m_renderTimer.start();
}

void ClipboardItemPreview::mousePressEvent(QMouseEvent *event) { forwardMouse(event); }
void ClipboardItemPreview::mouseMoveEvent(QMouseEvent *event) { forwardMouse(event); }
void ClipboardItemPreview::mouseReleaseEvent(QMouseEvent *event) { forwardMouse(event); }
void ClipboardItemPreview::mouseDoubleClickEvent(QMouseEvent *event) { forwardMouse(event); }

void ClipboardItemPreview::wheelEvent(QWheelEvent *event)
{
    auto target = m_scroll->childAt(event->position().toPoint());
    if (!target) target = m_scroll->viewport();
    const auto position = target->mapFrom(m_scroll.get(), event->position().toPoint());
    QWheelEvent forwarded(position, target->mapToGlobal(position), event->pixelDelta(), event->angleDelta(),
        event->buttons(), event->modifiers(), event->phase(), event->inverted());
    QApplication::sendEvent(target, &forwarded);
    event->setAccepted(forwarded.isAccepted());
    m_renderTimer.start();
}

void ClipboardItemPreview::keyPressEvent(QKeyEvent *event)
{
    // Tab and manager shortcuts stay with Quick; text selection/copy stays with the plugin.
    if (m_keyTarget && (event->matches(QKeySequence::Copy) || event->matches(QKeySequence::SelectAll)
            || event->key() == Qt::Key_Left || event->key() == Qt::Key_Right
            || event->key() == Qt::Key_Up || event->key() == Qt::Key_Down)) {
        QKeyEvent forwarded(event->type(), event->key(), event->modifiers(), event->text(), event->isAutoRepeat(), event->count());
        QApplication::sendEvent(m_keyTarget, &forwarded);
        event->setAccepted(forwarded.isAccepted());
        m_renderTimer.start();
    } else event->ignore();
}

void ClipboardItemPreview::keyReleaseEvent(QKeyEvent *event) { event->ignore(); }
