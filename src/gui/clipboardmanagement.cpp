// SPDX-License-Identifier: GPL-3.0-or-later
#include "clipboardmanagement.h"
#include "common/common.h"
#include "common/appconfig.h"
#include "common/tabs.h"
#include "gui/iconfont.h"
#include "gui/icon_list.h"
#include "gui/tabicons.h"
#include "gui/geometry.h"
#include "gui/navigation.h"
#include "common/navigationstyle.h"

#include <QAction>
#include <QDropEvent>
#include <QCursor>
#include <QFileDialog>
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
    m_model.setDisplayEnabled(false);
    connect(this, &QWindow::visibleChanged, this, [this] { m_model.setDisplayEnabled(isVisible() && windowState() != Qt::WindowMinimized); });
    connect(this, &QWindow::windowStateChanged, this, [this](Qt::WindowState state) {
        m_model.setDisplayEnabled(isVisible() && state != Qt::WindowMinimized);
        if (state != Qt::WindowMinimized) m_restoreMaximized = state == Qt::WindowMaximized;
    });
    engine()->addImageProvider(QStringLiteral("palette"), new PaletteImageProvider(&m_model));
}

ClipboardManagement::~ClipboardManagement()
{
    if (isVisible())
        saveGeometry();
    disconnect(this, nullptr, this, nullptr);
    // Destroy QML bindings before the model member they refer to.
    QQuickView::setSource(QUrl());
    hide();
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
    AppConfig config;
    setFlag(Qt::WindowStaysOnTopHint, config.option<Config::always_on_top>());
    setFlag(Qt::Tool, config.option<Config::hide_main_window_in_task_bar>());
    setFlag(Qt::FramelessWindowHint, config.option<Config::frameless_window>());
    setOpacity(1.0 - config.option<Config::transparency_focused>() / 100.0);
    if (config.option<Config::open_windows_on_current_screen>()) {
        if (auto screen = QGuiApplication::screenAt(QCursor::pos()))
            setScreen(screen);
    }
    if (auto screen = this->screen(); screen && !m_explicitGeometry && !isVisible()) {
        const auto area = screen->availableGeometry();
        setMinimumSize(QSize(qMin(800, area.width()), qMin(520, area.height())));
        const QSize size(qMin(1100, area.width()), qMin(740, area.height()));
        auto geometry = QRect(area.center() - QPoint(size.width() / 2, size.height() / 2), size);
        const auto saved = geometryOptionValue(geometryKey()).toRect();
        if (config.option<Config::restore_geometry>() && saved.isValid())
            geometry = saved;
        geometry.setSize(geometry.size().boundedTo(area.size()).expandedTo(minimumSize()));
        geometry.moveLeft(qBound(area.left(), geometry.left(), qMax(area.left(), area.right() - geometry.width() + 1)));
        geometry.moveTop(qBound(area.top(), geometry.top(), qMax(area.top(), area.bottom() - geometry.height() + 1)));
        setGeometry(geometry);
        m_normalGeometry = geometry;
        m_restoreMaximized = config.option<Config::restore_geometry>()
            && geometryOptionValue(geometryKey() + QStringLiteral("_maximized")).toBool();
    }
    if (m_restoreMaximized && !m_explicitGeometry) showMaximized();
    else showNormal();
    raise();
    requestActivate();
    emit opened();
}

QString ClipboardManagement::geometryKey() const
{
    AppConfig config;
    const auto suffix = config.option<Config::open_windows_on_current_screen>() && screen()
        ? screen()->name() : QStringLiteral("global");
    return QStringLiteral("Options/clipboard_management_geometry_%1").arg(suffix);
}

void ClipboardManagement::saveGeometry()
{
    if (!m_explicitGeometry && status() == QQuickView::Ready && geometry().isValid()
            && visibility() != QWindow::Minimized && AppConfig().option<Config::restore_geometry>())
    {
        setGeometryOptionValue(geometryKey(), windowState() == Qt::WindowNoState || !m_normalGeometry.isValid()
            ? geometry() : m_normalGeometry);
        setGeometryOptionValue(geometryKey() + QStringLiteral("_maximized"), m_restoreMaximized);
    }
}

void ClipboardManagement::openAt(const QRect &rect)
{
    const auto position = rect.x() == -1 && rect.y() == -1 ? QCursor::pos() : rect.topLeft();
    auto target = QGuiApplication::screenAt(position);
    if (!target) target = screen();
    if (!target) return;
    setScreen(target);
    const auto area = target->availableGeometry();
    auto size = this->size();
    if (rect.width() > 0 && rect.height() > 0) {
        // showAt() uses points, as the original Widgets implementation does.
        const auto scale = target->physicalDotsPerInchX() / 72.0;
        size = QSize(int(rect.width() * scale), int(rect.height() * scale));
    }
    size = size.expandedTo(minimumSize()).boundedTo(area.size());
    const auto x = qBound(area.left(), position.x(), qMax(area.left(), area.right() - size.width() + 1));
    const auto y = qBound(area.top(), position.y(), qMax(area.top(), area.bottom() - size.height() + 1));
    m_explicitGeometry = true;
    m_restoreMaximized = false;
    setWindowState(Qt::WindowNoState);
    setGeometry(QRect(QPoint(x, y), size));
    open();
}

void ClipboardManagement::setSource(QAbstractItemModel *source, const QString &tabName,
        const QStringList &tabs, const QVariantMap &properties)
{
    m_tabs = tabs;
    m_tabProperties = properties;
    m_model.setSourceModel(source, tabName);
    if (m_selectedTabPath.isEmpty() || !selectedTabIsGroup())
        m_selectedTabPath = tabName;
    emit sourceChanged();
}

QStringList ClipboardManagement::collectionsInGroup(const QString &path) const
{
    if (path.isEmpty())
        return m_tabs;
    QStringList result;
    for (const auto &name : m_tabs)
        if (name == path || name.startsWith(path + '/'))
            result.append(name);
    return result;
}

void ClipboardManagement::setTabs(const QStringList &tabs)
{
    m_tabs = tabs;
    emit sourceChanged();
}

void ClipboardManagement::setTabCounts(const QVariantMap &counts)
{
    m_tabCounts = counts;
    emit sourceChanged();
}

void ClipboardManagement::setOptions(const QVariantMap &options)
{
    m_options = options;
    emit stateChanged();
}

void ClipboardManagement::togglePreview()
{
    m_options.insert(QStringLiteral("preview"), !m_options.value(QStringLiteral("preview"), true).toBool());
    emit stateChanged();
}

void ClipboardManagement::clearHistory(int minutes)
{
    emit tabRequested(QStringLiteral("clearHistory"), tabName(), {{QStringLiteral("minutes"), minutes}});
}

void ClipboardManagement::sortGroup(const QString &path)
{
    emit tabRequested(QStringLiteral("sortGroup"), path, {});
}

bool ClipboardManagement::selectedTabIsGroup() const
{
    if (!m_options.value(QStringLiteral("treeMode"), true).toBool()) return false;
    for (const auto &name : m_tabs)
        if (name.startsWith(m_selectedTabPath + '/'))
            return true;
    return false;
}

QVariantList ClipboardManagement::tabTree() const
{
    if (!m_options.value(QStringLiteral("treeMode"), true).toBool()) {
        const Tabs properties;
        QVariantList result;
        for (const auto &name : m_tabs)
            result.append(QVariantMap{{QStringLiteral("path"), name}, {QStringLiteral("name"), name},
                {QStringLiteral("depth"), 0}, {QStringLiteral("group"), false}, {QStringLiteral("collection"), true},
                {QStringLiteral("expanded"), true}, {QStringLiteral("icon"), properties.tabProperties(name).iconName},
                {QStringLiteral("count"), m_tabCounts.value(name, -1)}});
        return result;
    }
    QHash<QString, QStringList> children;
    QSet<QString> paths;
    QSet<QString> groups;
    const QSet<QString> collections(m_tabs.begin(), m_tabs.end());
    const Tabs properties;
    for (const auto &name : m_tabs) {
        const auto parts = name.split('/');
        QString path;
        for (const auto &part : parts) {
            const QString parent = path;
            path = path.isEmpty() ? part : path + '/' + part;
            if (!paths.contains(path)) {
                paths.insert(path);
                children[parent].append(path);
            }
            if (!parent.isEmpty())
                groups.insert(parent);
        }
    }
    QVariantList result;
    // Visit siblings in saved order; descendants stay contiguous in the tree.
    const auto appendChildren = [&](const auto &self, const QString &parent, int depth) -> void {
        for (const auto &path : children.value(parent)) {
            const int slash = path.lastIndexOf('/');
            const bool group = groups.contains(path);
            result.append(QVariantMap{{QStringLiteral("path"), path},
                {QStringLiteral("name"), path.mid(slash + 1)}, {QStringLiteral("depth"), depth},
                {QStringLiteral("group"), group}, {QStringLiteral("collection"), collections.contains(path)},
                {QStringLiteral("expanded"), !m_collapsedGroups.contains(path)},
                {QStringLiteral("count"), m_tabCounts.value(path, -1)},
                {QStringLiteral("icon"), properties.tabProperties(path).iconName}});
            if (group && !m_collapsedGroups.contains(path))
                self(self, path, depth + 1);
        }
    };
    appendChildren(appendChildren, QString(), 0);
    return result;
}

void ClipboardManagement::selectTabPath(const QString &path)
{
    if (collectionsInGroup(path).isEmpty())
        return;
    m_selectedTabPath = path;
    if (m_tabs.contains(path))
        changeSource(path);
    emit sourceChanged();
}

void ClipboardManagement::toggleGroup(const QString &path)
{
    if (m_collapsedGroups.contains(path))
        m_collapsedGroups.remove(path);
    else
        m_collapsedGroups.insert(path);
    emit sourceChanged();
}

void ClipboardManagement::renameGroup(const QString &path, const QString &name, const QStringList &members)
{
    emit tabRequested(QStringLiteral("renameGroup"), path,
        {{QStringLiteral("name"), name}, {QStringLiteral("members"), members}});
}

void ClipboardManagement::removeGroup(const QString &path, const QStringList &members)
{
    emit tabRequested(QStringLiteral("removeGroup"), path, {{QStringLiteral("members"), members}});
}

void ClipboardManagement::moveGroup(const QString &path, int step)
{
    emit tabRequested(QStringLiteral("moveGroup"), path, {{QStringLiteral("step"), step}});
}

QVariant ClipboardManagement::iconFont() const { return ::iconFont(); }

QVariantList ClipboardManagement::icons(const QString &filter) const
{
    QVariantList result{QVariantMap{{QStringLiteral("icon"), QString()}, {QStringLiteral("name"), tr("Default")}}};
    for (const auto &icon : iconList) {
        const auto terms = QString::fromUtf8(icon.searchTerms);
        if (terms.contains(filter, Qt::CaseInsensitive))
            result.append(QVariantMap{{QStringLiteral("icon"), QString(QChar(icon.unicode))},
                {QStringLiteral("name"), terms.section('|', 0, 0)}});
    }
    return result;
}

void ClipboardManagement::saveIcon(const QString &icon, const QString &path)
{
    emit tabRequested(QStringLiteral("icon"), path, {{QStringLiteral("icon"), icon}});
}

QString ClipboardManagement::browseIcon()
{
    return QFileDialog::getOpenFileName(nullptr, tr("Collection icon"), QString(), tr("Images (*.png *.svg *.jpg *.ico);;All files (*)"));
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
    if (m_tabs.contains(tabName)) {
        m_selectedTabPath = tabName;
        emit sourceRequested(tabName);
    }
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
            {QStringLiteral("icon"), QString(QChar(action.iconId))},
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
    if (id == Actions::Tabs_ChangeTabIcon) { emit iconRequested(); return; }
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
    if ((event->type() == QEvent::Move || event->type() == QEvent::Resize)
            && isVisible() && windowState() == Qt::WindowNoState)
        m_normalGeometry = geometry();
    if (event->type() == QEvent::Hide) {
        saveGeometry();
        m_explicitGeometry = false;
    }
    if (event->type() == QEvent::Close) {
        emit hideRequested();
        event->ignore();
        return true;
    }
    if (event->type() == QEvent::DragEnter || event->type() == QEvent::DragMove || event->type() == QEvent::Drop)
        m_dropData = cloneData(static_cast<QDropEvent*>(event)->mimeData());
    if (event->type() == QEvent::DragLeave)
        m_dropData.clear();
    if (event->type() == QEvent::KeyPress) {
        if (rootObject() && rootObject()->property("popupActive").toBool())
            return ClipboardWindow::event(event);
        const auto key = static_cast<QKeyEvent*>(event);
        const auto focus = activeFocusItem();
        if (focus && focus->property("inputMethodComposing").toBool())
            return ClipboardWindow::event(event);
        const bool editing = focus && focus->flags().testFlag(QQuickItem::ItemAcceptsInputMethod)
            && focus->property("text").isValid()
            && !focus->property("readOnly").toBool();
        if (!editing) {
            const auto style = NavigationStyle(m_options.value(QStringLiteral("navigationStyle")).toInt());
            if (style == NavigationStyle::Vi && key->modifiers() == Qt::NoModifier) {
                if (key->key() == Qt::Key_Slash) { emit searchRequested(); return true; }
                if (key->key() == Qt::Key_H) { triggerAction(Actions::Tabs_PreviousTab); return true; }
                if (key->key() == Qt::Key_L) { triggerAction(Actions::Tabs_NextTab); return true; }
            }
            const KeyMods input{key->key(), key->modifiers()};
            const auto translated = style == NavigationStyle::Vi ? translateToVi(input)
                : style == NavigationStyle::Emacs ? translateToEmacs(input) : KeyMods();
            if (translated.key != 0 && (translated.key != input.key || translated.mods != input.mods)) {
                QKeyEvent translatedEvent(QEvent::KeyPress, translated.key, translated.mods);
                return this->event(&translatedEvent);
            }
        }
        if (key->key() == Qt::Key_Escape) {
            if (!m_model.query().isEmpty())
                m_model.setQuery(QString());
            else
                emit hideRequested();
            return true;
        }
        if (editing && key->matches(QKeySequence::Copy) && activeFocusItem()
                && activeFocusItem()->property("selectedText").toString().isEmpty()) {
            triggerAction(Actions::Edit_CopySelectedItems);
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
            if (key->key() == Qt::Key_Home || key->key() == Qt::Key_End
                    || key->key() == Qt::Key_PageUp || key->key() == Qt::Key_PageDown) {
                const int page = rootObject() ? qMax(1, rootObject()->property("pageRows").toInt()) : 1;
                const int target = key->key() == Qt::Key_Home ? 0
                    : key->key() == Qt::Key_End ? m_model.count() - 1
                    : m_model.selectedRow() + (key->key() == Qt::Key_PageUp ? -page : page);
                m_model.select(qBound(0, target, m_model.count() - 1), key->modifiers().testFlag(Qt::ShiftModifier));
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
                // Editor actions share shortcuts such as F2 with collection actions.
                if (i >= Actions::Editor_Save && i <= Actions::Editor_Search)
                    continue;
                if (m_actions[size_t(i)].shortcuts.contains(sequence)) {
                    triggerAction(i);
                    return true;
                }
            }
            if (key->key() == Qt::Key_Return || key->key() == Qt::Key_Enter) {
                triggerAction(Actions::Item_MoveToClipboard);
                return true;
            }
            if (!key->text().isEmpty() && !key->modifiers().testFlag(Qt::ControlModifier)
                    && !key->modifiers().testFlag(Qt::MetaModifier) && !key->modifiers().testFlag(Qt::AltModifier))
                emit searchRequested();
        }
    }
    return ClipboardWindow::event(event);
}
