// SPDX-License-Identifier: GPL-3.0-or-later
#include "clipboardstyle.h"

#include <QApplication>
#include <QLineEdit>
#include <QLabel>
#include <QListView>
#include <QMainWindow>
#include <QMenu>
#include <QPainter>
#include <QPushButton>
#include <QQuickWindow>
#include <QStyleOptionButton>
#include <QStyleOptionFocusRect>
#include <QStyleOptionMenuItem>
#include <QStyleOptionTab>
#include <QStyleOptionToolButton>
#include <QStyleOptionViewItem>
#include <QToolBar>
#include <QToolButton>
#include <QTreeView>
#include <QTabBar>
#include <QtMath>

ClipboardStyle::ClipboardStyle(QQuickItem *parent) : QQuickPaintedItem(parent)
{
    m_timer.setSingleShot(true);
    m_timer.setInterval(0);
    connect(&m_timer, &QTimer::timeout, this, &ClipboardStyle::renderStyle);
    connect(this, &ClipboardStyle::styleChanged, this, [this] { m_timer.start(); });
    connect(this, &QQuickItem::enabledChanged, this, [this] { m_timer.start(); });
    connect(this, &QQuickItem::visibleChanged, this, [this] { if (isVisible()) m_timer.start(); });
}

ClipboardStyle::~ClipboardStyle() = default;

void ClipboardStyle::setTheme(const QVariantMap &value) { if (m_theme != value) { m_theme = value; m_root.reset(); emit styleChanged(); } }
void ClipboardStyle::setKind(Kind value) { if (m_kind != value) { m_kind = value; m_root.reset(); emit styleChanged(); } }
void ClipboardStyle::setText(const QString &value) { if (m_text != value) { m_text = value; emit styleChanged(); } }
void ClipboardStyle::setFont(const QFont &value) { if (m_font != value) { m_font = value; m_root.reset(); emit styleChanged(); } }
void ClipboardStyle::setHovered(bool value) { if (m_hovered != value) { m_hovered = value; emit styleChanged(); } }
void ClipboardStyle::setPressed(bool value) { if (m_pressed != value) { m_pressed = value; emit styleChanged(); } }
void ClipboardStyle::setFocused(bool value) { if (m_focused != value) { m_focused = value; emit styleChanged(); } }
void ClipboardStyle::setSelected(bool value) { if (m_selected != value) { m_selected = value; emit styleChanged(); } }
void ClipboardStyle::setAlternate(bool value) { if (m_alternate != value) { m_alternate = value; emit styleChanged(); } }
void ClipboardStyle::setRowNumber(const QString &value) { if (m_rowNumber != value) { m_rowNumber = value; emit styleChanged(); } }

void ClipboardStyle::geometryChange(const QRectF &next, const QRectF &previous)
{
    QQuickPaintedItem::geometryChange(next, previous);
    if (next.size() != previous.size()) m_timer.start();
}

void ClipboardStyle::renderStyle()
{
    if (!isVisible()) return;
    if (!m_theme.value(QStringLiteral("custom_style"), true).toBool()) {
        m_snapshot = {};
        m_foreground = m_theme.value(QStringLiteral("fg")).value<QColor>();
        setImplicitWidth(qMax(60, QFontMetrics(m_font).horizontalAdvance(m_text) + 24));
        setImplicitHeight(m_kind == Item ? m_theme.value(QStringLiteral("quick_row_height"), 48).toInt() : 32);
        emit metricsChanged();
        update();
        return;
    }
    if (!m_root) {
        m_root = std::make_unique<QWidget>();
        m_root->setObjectName(QStringLiteral("qclip_style_scope"));
        auto root = new QMainWindow(m_root.get(), Qt::Widget);
        root->setWindowFlags(Qt::Widget);
        root->setAttribute(Qt::WA_DontShowOnScreen);
        root->setObjectName(QStringLiteral("MainWindow"));
        auto palette = QApplication::palette();
        for (const auto &role : QList<QPair<QPalette::ColorRole,QString>>{
                {QPalette::Base, QStringLiteral("bg")}, {QPalette::Text, QStringLiteral("fg")},
                {QPalette::AlternateBase, QStringLiteral("alt_bg")},
                {QPalette::Window, QStringLiteral("bg")}, {QPalette::WindowText, QStringLiteral("fg")},
                {QPalette::Button, QStringLiteral("alt_bg")}, {QPalette::ButtonText, QStringLiteral("fg")},
                {QPalette::Highlight, QStringLiteral("sel_bg")}, {QPalette::HighlightedText, QStringLiteral("sel_fg")}}) {
            const auto color = m_theme.value(role.second).value<QColor>();
            if (color.isValid()) palette.setColor(role.first, color);
        }
        root->setPalette(palette);
        root->setFont(m_font);
        root->setStyleSheet(QStringLiteral("QToolButton { color: %1; }").arg(palette.color(QPalette::ButtonText).name())
            + m_theme.value(QStringLiteral("main_css")).toString());
        QWidget *widget = nullptr;
        if (m_kind == Item) {
            auto view = new QListView(root);
            view->setObjectName(QStringLiteral("ClipboardBrowser"));
            view->setStyleSheet(m_theme.value(QStringLiteral("items_css")).toString());
            widget = view;
        } else if (m_kind == Collection) {
            widget = new QTreeView(root);
            widget->setObjectName(QStringLiteral("tab_tree"));
        } else if (m_kind == Tab) {
            widget = new QTabBar(root);
            widget->setObjectName(QStringLiteral("tab_bar"));
        } else if (m_kind == Menu || m_kind == MenuItem) {
            widget = new QMenu(root);
            widget->setWindowFlags(Qt::Widget);
            widget->setStyleSheet(m_theme.value(QStringLiteral("menu_css")).toString());
        } else if (m_kind == ToolbarButton) {
            auto toolbar = new QToolBar(root);
            root->addToolBar(toolbar);
            widget = new QToolButton(toolbar);
        } else if (m_kind == Search) {
            widget = new QLineEdit(root);
            widget->setObjectName(QStringLiteral("searchBar"));
        } else widget = new QPushButton(root);
        widget->setProperty("qclip_style_target", true);
        widget->setPalette(root->palette());
        widget->setFont(root->font());
        if (m_kind == Item) {
            auto label = new QLabel(widget);
            label->setObjectName(QStringLiteral("item"));
            label->setTextFormat(Qt::PlainText);
            label->setWordWrap(true);
            label->setAlignment(Qt::AlignTop | Qt::AlignLeft);
            label->setProperty("CopyQ_selected", false);
        }
        if (m_kind == Collection || m_kind == Tab) {
            auto counter = new QLabel(widget);
            counter->setObjectName(QStringLiteral("tab_item_counter"));
        }
        widget->ensurePolished();
    }
    QWidget *widget = nullptr;
    for (auto candidate : m_root->findChildren<QWidget *>())
        if (candidate->property("qclip_style_target").toBool()) { widget = candidate; break; }
    if (!widget) return;
    widget->setEnabled(isEnabled());
    widget->resize(qMax(1, qRound(width())), qMax(1, qRound(height())));
    QStyle::State state = QStyle::State_Active;
    if (isEnabled()) state |= QStyle::State_Enabled;
    if (m_hovered) state |= QStyle::State_MouseOver;
    if (m_pressed) state |= QStyle::State_Sunken; else state |= QStyle::State_Raised;
    if (m_focused) state |= QStyle::State_HasFocus;
    if (m_selected) state |= QStyle::State_Selected;
    const auto ratio = window() ? window()->devicePixelRatio() : 1.0;
    QImage snapshot(QSize(qMax(1, qCeil(width() * ratio)), qMax(1, qCeil(height() * ratio))), QImage::Format_ARGB32_Premultiplied);
    snapshot.setDevicePixelRatio(ratio);
    snapshot.fill(Qt::transparent);
    QPainter painter(&snapshot);
    painter.setFont(widget->font());
    QSize implicitSize;
    const auto drawCounter = [&] {
        if (m_rowNumber.isEmpty()) return;
        auto counter = widget->findChild<QLabel *>(QStringLiteral("tab_item_counter"));
        if (!counter) return;
        if (counter->property("CopyQ_selected").toBool() != m_selected) {
            counter->setProperty("CopyQ_selected", m_selected);
            counter->style()->unpolish(counter);
            counter->style()->polish(counter);
        }
        counter->setText(m_rowNumber);
        counter->ensurePolished();
        counter->resize(counter->sizeHint());
        counter->render(&painter, QPoint(qMax(0, widget->width() - counter->width() - 6),
            qMax(0, (widget->height() - counter->height()) / 2)));
    };
    if (m_kind == Item || m_kind == Collection) {
        QStyleOptionViewItem option;
        option.initFrom(widget);
        option.state = state;
        option.features = QStyleOptionViewItem::HasDisplay;
        if (m_kind == Item) option.features |= QStyleOptionViewItem::WrapText;
        option.textElideMode = Qt::ElideRight;
        if (m_alternate) option.features |= QStyleOptionViewItem::Alternate;
        option.text = m_kind == Item ? QString() : m_text;
        option.font = widget->font();
        option.fontMetrics = QFontMetrics(option.font);
        option.rect = widget->rect();
        widget->style()->drawControl(QStyle::CE_ItemViewItem, &option, &painter, widget);
        if (m_kind == Collection) {
            drawCounter();
        } else {
            const auto font = m_theme.value(QStringLiteral("num_font"), m_font).value<QFont>();
            const auto numberWidth = m_rowNumber.isEmpty() ? 0 : QFontMetrics(font).horizontalAdvance(m_rowNumber) + 12;
            auto label = widget->findChild<QLabel *>(QStringLiteral("item"));
            if (label->property("CopyQ_selected").toBool() != m_selected) {
                label->setProperty("CopyQ_selected", m_selected);
                label->style()->unpolish(label);
                label->style()->polish(label);
            }
            label->setText(m_text);
            label->resize(qMax(1, widget->width() - numberWidth - 12), qMax(1, widget->height() - 12));
            label->ensurePolished();
            label->render(&painter, QPoint(6, 6), QRegion(), QWidget::DrawChildren);
            if (!m_rowNumber.isEmpty()) {
                painter.setFont(font);
                painter.setPen(m_theme.value(m_selected ? QStringLiteral("sel_fg") : QStringLiteral("num_fg")).value<QColor>());
                painter.drawText(widget->rect().adjusted(4, 4, -6, -4), Qt::AlignRight | Qt::AlignTop, m_rowNumber);
            }
        }
        implicitSize = widget->style()->sizeFromContents(QStyle::CT_ItemViewItem, &option, QSize(180, 48), widget);
    } else if (m_kind == Tab) {
        QStyleOptionTab option;
        option.initFrom(widget);
        option.state = state;
        option.rect = widget->rect();
        option.text = m_text;
        option.shape = QTabBar::RoundedNorth;
        widget->style()->drawControl(QStyle::CE_TabBarTab, &option, &painter, widget);
        drawCounter();
        implicitSize = widget->style()->sizeFromContents(QStyle::CT_TabBarTab, &option,
            QSize(QFontMetrics(widget->font()).horizontalAdvance(m_text) + 16, 36), widget);
    } else if (m_kind == MenuItem) {
        QStyleOptionMenuItem option;
        option.initFrom(widget);
        option.state = state;
        if (m_hovered || m_pressed) option.state |= QStyle::State_Selected;
        option.rect = widget->rect();
        option.text = m_text;
        option.font = widget->font();
        option.fontMetrics = QFontMetrics(option.font);
        option.menuItemType = QStyleOptionMenuItem::Normal;
        option.checkType = QStyleOptionMenuItem::NotCheckable;
        widget->style()->drawControl(QStyle::CE_MenuItem, &option, &painter, widget);
        implicitSize = widget->style()->sizeFromContents(QStyle::CT_MenuItem, &option,
            QSize(option.fontMetrics.horizontalAdvance(m_text), option.fontMetrics.height()), widget);
    } else if (m_kind == Menu) {
        QStyleOption option;
        option.initFrom(widget);
        option.state = state;
        option.rect = widget->rect();
        widget->style()->drawPrimitive(QStyle::PE_PanelMenu, &option, &painter, widget);
        widget->style()->drawPrimitive(QStyle::PE_FrameMenu, &option, &painter, widget);
        implicitSize = QSize(180, 36);
    } else if (m_kind == ToolbarButton) {
        auto button = static_cast<QToolButton *>(widget);
        button->setText(m_text);
        button->setToolButtonStyle(Qt::ToolButtonTextOnly);
        button->setDown(m_pressed);
        button->setAttribute(Qt::WA_UnderMouse, m_hovered);
        QStyleOptionToolButton option;
        option.initFrom(widget);
        option.state = state;
        option.rect = widget->rect();
        option.text = m_text;
        option.font = widget->font();
        option.toolButtonStyle = Qt::ToolButtonTextOnly;
        option.features = QStyleOptionToolButton::None;
        option.subControls = QStyle::SC_ToolButton;
        widget->render(&painter, QPoint(), QRegion(), QWidget::DrawChildren);
        if (m_focused) {
            QStyleOptionFocusRect focus;
            focus.initFrom(widget);
            focus.state = state;
            focus.rect = widget->rect().adjusted(2, 2, -2, -2);
            widget->style()->drawPrimitive(QStyle::PE_FrameFocusRect, &focus, &painter, widget);
        }
        implicitSize = widget->style()->sizeFromContents(QStyle::CT_ToolButton, &option,
            QSize(QFontMetrics(widget->font()).horizontalAdvance(m_text), 24), widget);
    } else if (m_kind == Search) {
        QStyleOptionFrame option;
        option.initFrom(widget);
        option.state = state;
        option.rect = widget->rect();
        widget->style()->drawPrimitive(QStyle::PE_PanelLineEdit, &option, &painter, widget);
        implicitSize = QSize(180, 36);
    } else {
        QStyleOptionButton option;
        option.initFrom(widget);
        option.state = state;
        option.rect = widget->rect();
        option.text = m_text;
        widget->style()->drawControl(QStyle::CE_PushButton, &option, &painter, widget);
        implicitSize = widget->style()->sizeFromContents(QStyle::CT_PushButton, &option,
            QSize(QFontMetrics(widget->font()).horizontalAdvance(m_text), 24), widget);
    }
    painter.end();
    m_snapshot = snapshot;
    m_foreground = widget->palette().color(m_kind == Search ? QPalette::Text : QPalette::ButtonText);
    setImplicitWidth(qMax(implicitSize.width(), 60));
    setImplicitHeight(qMax(implicitSize.height(), 36));
    emit metricsChanged();
    update();
}

void ClipboardStyle::paint(QPainter *painter)
{
    if (m_theme.value(QStringLiteral("custom_style"), true).toBool()) {
        painter->drawImage(QPointF(), m_snapshot);
        return;
    }
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setFont(m_font);
    const auto foreground = m_theme.value(QStringLiteral("fg"), QColor("#252832")).value<QColor>();
    const auto background = m_theme.value(QStringLiteral("bg"), QColor("#f4f5f7")).value<QColor>();
    const auto accent = m_theme.value(QStringLiteral("sel_bg"), QColor("#3975ed")).value<QColor>();
    const bool dark = background.lightnessF() < 0.5;
    auto line = foreground;
    line.setAlphaF(0.1);
    auto fill = foreground;
    fill.setAlphaF(m_pressed ? 0.09 : m_hovered ? 0.055 : 0.0);
    if (m_selected || (m_kind == MenuItem && m_hovered)) {
        fill = accent;
        fill.setAlphaF(dark ? 0.24 : 0.12);
    }
    if (m_kind == Menu || m_kind == Search || m_kind == Button)
        fill = dark ? QColor("#30323a") : QColor(Qt::white);
    painter->setPen(m_focused ? QPen(accent, 1) :
        (m_kind == Menu || m_kind == Search || m_kind == Button) ? QPen(line, 1) : QPen(Qt::NoPen));
    painter->setBrush(fill);
    painter->drawRoundedRect(boundingRect().adjusted(0.5, 0.5, -0.5, -0.5),
        m_theme.value(QStringLiteral("quick_radius"), 8).toInt(), m_theme.value(QStringLiteral("quick_radius"), 8).toInt());
    if (m_kind == Menu || m_kind == Search) return;
    painter->setPen(foreground);
    const auto rect = boundingRect().adjusted(8, 0, -8, 0);
    const QFontMetrics metrics(m_font);
    if (m_kind == Item) {
        auto numberColor = foreground;
        numberColor.setAlphaF(0.5);
        const int numberWidth = m_rowNumber.isEmpty() ? 0 : metrics.horizontalAdvance(m_rowNumber) + 18;
        const auto titleRect = rect.adjusted(0, 0, -numberWidth, 0);
        painter->drawText(QRectF(titleRect.x(), 4, titleRect.width(), (height() - 8) / 2),
            Qt::AlignVCenter, metrics.elidedText(m_text.section('\n', 0, 0), Qt::ElideRight, qRound(titleRect.width())));
        painter->setPen(numberColor);
        painter->drawText(QRectF(titleRect.x(), height() / 2, titleRect.width(), (height() - 8) / 2),
            Qt::AlignVCenter, metrics.elidedText(m_text.section('\n', 1), Qt::ElideRight, qRound(titleRect.width())));
        painter->drawText(rect, Qt::AlignRight | Qt::AlignVCenter, m_rowNumber);
    } else {
        painter->drawText(rect, Qt::AlignVCenter | (m_kind == Button || m_kind == ToolbarButton ? Qt::AlignHCenter : Qt::AlignLeft),
            metrics.elidedText(m_text, Qt::ElideRight, qRound(rect.width())));
        if (!m_rowNumber.isEmpty()) {
            painter->setPen(accent);
            painter->drawText(rect, Qt::AlignRight | Qt::AlignVCenter, m_rowNumber);
        }
    }
}
