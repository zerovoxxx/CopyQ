// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QImage>
#include <QFont>
#include <QQuickPaintedItem>
#include <QTimer>
#include <QVariantMap>
#include <memory>

class QWidget;

// Evaluate existing QSS with Qt Widgets, including image/gradient/border/state rules.
class ClipboardStyle : public QQuickPaintedItem
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QVariantMap theme READ theme WRITE setTheme NOTIFY styleChanged)
    Q_PROPERTY(Kind kind READ kind WRITE setKind NOTIFY styleChanged)
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY styleChanged)
    Q_PROPERTY(QFont font READ font WRITE setFont NOTIFY styleChanged)
    Q_PROPERTY(bool hovered READ hovered WRITE setHovered NOTIFY styleChanged)
    Q_PROPERTY(bool pressed READ pressed WRITE setPressed NOTIFY styleChanged)
    Q_PROPERTY(bool focused READ focused WRITE setFocused NOTIFY styleChanged)
    Q_PROPERTY(bool selected READ selected WRITE setSelected NOTIFY styleChanged)
    Q_PROPERTY(bool alternate READ alternate WRITE setAlternate NOTIFY styleChanged)
    Q_PROPERTY(QColor foreground READ foreground NOTIFY metricsChanged)
    Q_PROPERTY(QString rowNumber READ rowNumber WRITE setRowNumber NOTIFY styleChanged)

public:
    enum Kind { Button, ToolbarButton, Search, Item, Collection, Menu, MenuItem, Tab };
    Q_ENUM(Kind)
    explicit ClipboardStyle(QQuickItem *parent = nullptr);
    ~ClipboardStyle() override;
    QVariantMap theme() const { return m_theme; }
    Kind kind() const { return m_kind; }
    QString text() const { return m_text; }
    QFont font() const { return m_font; }
    bool hovered() const { return m_hovered; }
    bool pressed() const { return m_pressed; }
    bool focused() const { return m_focused; }
    bool selected() const { return m_selected; }
    bool alternate() const { return m_alternate; }
    QColor foreground() const { return m_foreground; }
    QString rowNumber() const { return m_rowNumber; }
    void setTheme(const QVariantMap &value);
    void setKind(Kind value);
    void setText(const QString &value);
    void setFont(const QFont &value);
    void setHovered(bool value);
    void setPressed(bool value);
    void setFocused(bool value);
    void setSelected(bool value);
    void setAlternate(bool value);
    void setRowNumber(const QString &value);
    void paint(QPainter *painter) override;

signals:
    void styleChanged();
    void metricsChanged();

protected:
    void geometryChange(const QRectF &next, const QRectF &previous) override;

private:
    void renderStyle();
    QVariantMap m_theme;
    Kind m_kind = Button;
    QString m_text;
    QString m_rowNumber;
    QFont m_font;
    QColor m_foreground;
    bool m_hovered = false;
    bool m_pressed = false;
    bool m_focused = false;
    bool m_selected = false;
    bool m_alternate = false;
    std::unique_ptr<QWidget> m_root;
    QTimer m_timer;
    QImage m_snapshot;
};
