// SPDX-License-Identifier: GPL-3.0-or-later
#include "clipboardmanagement.h"
#include "common/common.h"

#include <QAction>
#include <QDropEvent>
#include <QGuiApplication>
#include <QInputMethod>
#include <QKeyEvent>
#include <QMenu>
#include <QQmlEngine>
#include <QQuickItem>
#include <QScreen>

ClipboardManagement::ClipboardManagement(ItemFactory *factory)
    : m_model(factory, this)
{
    setObjectName(QStringLiteral("clipboard_management"));
    setTitle(tr("QClip — Clipboard manager"));
    setResizeMode(QQuickView::SizeRootObjectToView);
    setMinimumSize(QSize(800, 520));
    engine()->addImageProvider(QStringLiteral("palette"), new PaletteImageProvider(&m_model));
}

ClipboardManagement::~ClipboardManagement()
{
    // Destroy QML bindings before the model member they refer to.
    QQuickView::setSource(QUrl());
}

bool ClipboardManagement::load()
{
    if (status() == QQuickView::Ready)
        return true;
    setInitialProperties({{QStringLiteral("controller"), QVariant::fromValue(this)}});
    QQuickView::setSource(QUrl(QStringLiteral("qrc:/copyq/QClip/gui/qml/ManagementWindow.qml")));
    if (status() != QQuickView::Ready) {
        QStringList messages;
        for (const auto &error : errors())
            messages.append(error.toString());
        setError(tr("Unable to load clipboard manager: %1").arg(messages.join('\n')));
        return false;
    }
    return true;
}

void ClipboardManagement::open()
{
    if (!load())
        return;
    if (auto screen = this->screen()) {
        const auto area = screen->availableGeometry();
        if (!isVisible()) {
            const QSize size(qMin(1100, area.width()), qMin(740, area.height()));
            setGeometry(QRect(area.center() - QPoint(size.width() / 2, size.height() / 2), size));
        }
    }
    show();
    raise();
    requestActivate();
    emit opened();
}

void ClipboardManagement::setSource(QAbstractItemModel *source, const QString &tabName,
        const QStringList &tabs, const QVariantMap &properties)
{
    m_tabs = tabs;
    m_tabProperties = properties;
    m_model.setSourceModel(source, tabName);
    emit sourceChanged();
}

void ClipboardManagement::setTheme(const QVariantMap &theme)
{
    m_theme = theme;
    emit themeChanged();
}

void ClipboardManagement::setMonitoring(bool enabled)
{
    m_monitoring = enabled;
    emit stateChanged();
}

void ClipboardManagement::setError(const QString &error)
{
    m_error = error;
    emit stateChanged();
}

void ClipboardManagement::changeSource(const QString &tabName)
{
    if (m_tabs.contains(tabName))
        emit sourceRequested(tabName);
}

void ClipboardManagement::setActions(const MenuItems &items)
{
    m_actions = items;
    emit commandsChanged();
}

QVariantList ClipboardManagement::actions() const
{
    QVariantList result;
    for (int i = 0; i < Actions::Count; ++i) {
        const auto &action = m_actions[size_t(i)];
        result.append(QVariantMap{{QStringLiteral("id"), i}, {QStringLiteral("name"), QString(action.text).remove('&')},
            {QStringLiteral("shortcut"), action.shortcuts.isEmpty() ? QString() : action.shortcuts.first().toString(QKeySequence::NativeText)}});
    }
    return result;
}

void ClipboardManagement::setCommandMenu(QMenu *menu)
{
    m_commands = menu;
    emit commandsChanged();
}

QVariantList ClipboardManagement::commands() const
{
    QVariantList result;
    if (m_commands) {
        for (const auto action : m_commands->actions()) {
            result.append(QVariantMap{{QStringLiteral("name"), action->text().remove('&')},
                {QStringLiteral("enabled"), action->isEnabled()},
                {QStringLiteral("shortcut"), action->shortcut().toString(QKeySequence::NativeText)}});
        }
    }
    return result;
}

void ClipboardManagement::triggerAction(int id)
{
    if (id == Actions::Tabs_NewTab) { emit newTabRequested(); return; }
    if (id == Actions::Tabs_RenameTab) { emit renameTabRequested(); return; }
    if (id == Actions::Tabs_RemoveTab) { emit removeTabRequested(); return; }
    if (id == Actions::Edit_FindItems) { emit searchRequested(); return; }
    if (id == Actions::ItemMenu) { emit itemMenuRequested(); return; }
    if (m_model.filtering() || (activeFocusItem() && activeFocusItem()->property("inputMethodComposing").toBool()))
        return;
    setError(QString());
    emit actionRequested(id, tabName(), m_model.selectedIndexes(), m_model.selectedIndex());
}

void ClipboardManagement::triggerCommand(int index)
{
    if (!m_model.filtering() && m_commands && index >= 0 && index < m_commands->actions().size()) {
        auto action = m_commands->actions().at(index);
        if (action->isEnabled())
            action->trigger();
    }
}

void ClipboardManagement::createTab(const QString &name)
{
    emit tabRequested(QStringLiteral("create"), name, {});
}

void ClipboardManagement::renameTab(const QString &name, const QString &sourceTab)
{
    emit tabRequested(QStringLiteral("rename"), sourceTab.isEmpty() ? tabName() : sourceTab, {{QStringLiteral("name"), name}});
}

void ClipboardManagement::removeTab(const QString &sourceTab)
{
    emit tabRequested(QStringLiteral("remove"), sourceTab.isEmpty() ? tabName() : sourceTab, {});
}

void ClipboardManagement::moveTab(int step)
{
    emit tabRequested(QStringLiteral("move"), tabName(), {{QStringLiteral("step"), step}});
}

void ClipboardManagement::saveTabProperties(const QVariantMap &properties, const QString &sourceTab)
{
    emit tabRequested(QStringLiteral("properties"), sourceTab.isEmpty() ? tabName() : sourceTab, properties);
}

void ClipboardManagement::transferItems(const QString &targetTab, bool move)
{
    if (!m_model.filtering()) {
        bool accepted = false;
        emit transferRequested(tabName(), targetTab, m_model.selectedIndexes(), move, 0, &accepted);
    }
}

void ClipboardManagement::startDrag(int row)
{
    if (!m_model.filtering()) {
        if (!m_model.isSelected(row))
            m_model.selectRow(row);
        m_dragIndexes = m_model.selectedIndexes();
        m_dragTab = tabName();
        emit dragRequested(m_dragTab, m_dragIndexes);
        m_dragIndexes.clear();
        m_dragTab.clear();
    }
}

bool ClipboardManagement::dropItems(const QString &targetTab, int row, bool move)
{
    bool accepted = false;
    if (!m_dragIndexes.isEmpty())
        emit transferRequested(m_dragTab, targetTab, m_dragIndexes, move, row, &accepted);
    else if (!m_dropData.isEmpty())
        emit dataDropped(m_dropData, targetTab, row, &accepted);
    return accepted;
}

bool ClipboardManagement::event(QEvent *event)
{
    if (event->type() == QEvent::DragEnter || event->type() == QEvent::DragMove || event->type() == QEvent::Drop)
        m_dropData = cloneData(static_cast<QDropEvent*>(event)->mimeData());
    if (event->type() == QEvent::DragLeave)
        m_dropData.clear();
    if (event->type() == QEvent::KeyPress) {
        if (rootObject() && rootObject()->property("popupActive").toBool())
            return QQuickView::event(event);
        const auto key = static_cast<QKeyEvent*>(event);
        const auto focus = activeFocusItem();
        if (focus && focus->property("inputMethodComposing").toBool())
            return QQuickView::event(event);
        const bool editing = focus && focus->property("text").isValid()
            && !focus->property("readOnly").toBool();
        if (key->key() == Qt::Key_Escape) {
            if (!m_model.query().isEmpty())
                m_model.setQuery(QString());
            else
                hide();
            return true;
        }
        if (!editing) {
            if (key->matches(QKeySequence::SelectAll)) {
                m_model.selectAll();
                return true;
            }
            if (key->key() == Qt::Key_Up || key->key() == Qt::Key_Down) {
                const int row = qBound(0, m_model.selectedRow() + (key->key() == Qt::Key_Up ? -1 : 1), m_model.count() - 1);
                m_model.select(row, key->modifiers().testFlag(Qt::ShiftModifier));
                return true;
            }
            const auto sequence = QKeySequence(key->keyCombination());
            if (m_commands && !m_model.filtering()) {
                for (auto action : m_commands->actions()) {
                    if (action->shortcuts().contains(sequence)) {
                        if (action->isEnabled())
                            action->trigger();
                        else
                            setError(tr("This command is currently unavailable."));
                        return true;
                    }
                }
            }
            for (int i = 0; i < Actions::Count; ++i) {
                if (m_actions[size_t(i)].shortcuts.contains(sequence)) {
                    triggerAction(i);
                    return true;
                }
            }
            if (key->key() == Qt::Key_Return || key->key() == Qt::Key_Enter) {
                triggerAction(Actions::Item_MoveToClipboard);
                return true;
            }
        }
    }
    return QQuickView::event(event);
}
